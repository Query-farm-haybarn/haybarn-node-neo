import { tableFromIPC } from 'apache-arrow';
import duckdb from '@haybarn/node-bindings';
import { expect, suite, test } from 'vitest';
import { withConnection } from './utils/withConnection';

suite('Arrow IPC', () => {
  test('encodes a multi-batch dictionary result as an IPC stream', async () => {
    await withConnection(async (connection) => {
      await duckdb.query(connection, 'SET arrow_lossless_conversion = true');
      await duckdb.query(connection, "SET arrow_output_version = '1.0'");
      await duckdb.query(
        connection,
        "CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy')"
      );

      const prepared = await duckdb.prepare(
        connection,
        `SELECT i,
          CASE i % 3
            WHEN 0 THEN 'sad'::mood
            WHEN 1 THEN 'ok'::mood
            ELSE 'happy'::mood
          END AS mood
        FROM range(5000) AS rows(i)`
      );
      try {
        const result = await duckdb.execute_prepared_streaming(prepared);
        const bytes = await duckdb.result_to_arrow_ipc(result);
        const table = tableFromIPC(bytes);

        expect(duckdb.result_is_streaming(result)).toBe(true);
        expect(Buffer.isBuffer(bytes)).toBe(true);
        expect(table.numRows).toBe(5000);
        expect(table.batches).toHaveLength(3);
        expect(table.schema.fields.map((field) => field.name)).toEqual([
          'i',
          'mood',
        ]);
        expect(table.schema.fields[1].type.toString()).toBe(
          'Dictionary<Uint8, Utf8>'
        );
        expect([...table.getChild('mood')!.slice(0, 6)]).toEqual([
          'sad',
          'ok',
          'happy',
          'sad',
          'ok',
          'happy',
        ]);
      } finally {
        duckdb.destroy_prepare_sync(prepared);
      }
    });
  });

  test('encodes an empty result', async () => {
    await withConnection(async (connection) => {
      const result = await duckdb.query(
        connection,
        'SELECT 42::INTEGER AS answer WHERE false'
      );
      const bytes = await duckdb.result_to_arrow_ipc(result);
      const table = tableFromIPC(bytes);

      expect(table.numRows).toBe(0);
      expect(table.schema.fields[0].name).toBe('answer');
      expect(table.schema.fields[0].type.toString()).toBe('Int32');
    });
  });

  test('preserves lossless Arrow extension metadata', async () => {
    await withConnection(async (connection) => {
      await duckdb.query(connection, 'SET arrow_lossless_conversion = true');
      await duckdb.query(connection, "SET arrow_output_version = '1.0'");

      const result = await duckdb.query(
        connection,
        `SELECT bool, hugeint, uhugeint, bignum, time_tz, uuid, bit, geometry
        FROM test_all_types()`
      );
      const table = tableFromIPC(
        await duckdb.result_to_arrow_ipc(result)
      );
      const extensions = Object.fromEntries(
        table.schema.fields.map((field) => [
          field.name,
          {
            name: field.metadata.get('ARROW:extension:name'),
            metadata: field.metadata.get('ARROW:extension:metadata'),
          },
        ])
      );

      expect(extensions).toEqual({
        bool: { name: 'arrow.bool8', metadata: '' },
        hugeint: {
          name: 'arrow.opaque',
          metadata: '{"type_name":"hugeint","vendor_name":"DuckDB"}',
        },
        uhugeint: {
          name: 'arrow.opaque',
          metadata: '{"type_name":"uhugeint","vendor_name":"DuckDB"}',
        },
        bignum: {
          name: 'arrow.opaque',
          metadata: '{"type_name":"bignum","vendor_name":"DuckDB"}',
        },
        time_tz: {
          name: 'arrow.opaque',
          metadata: '{"type_name":"time_tz","vendor_name":"DuckDB"}',
        },
        uuid: { name: 'arrow.uuid', metadata: '' },
        bit: {
          name: 'arrow.opaque',
          metadata: '{"type_name":"bit","vendor_name":"DuckDB"}',
        },
        geometry: { name: 'geoarrow.wkb', metadata: '{}' },
      });
    });
  });

  test('pulls bounded IPC chunks after native backpressure', async () => {
    await withConnection(async (connection) => {
      await duckdb.query(connection, 'SET arrow_lossless_conversion = true');
      await duckdb.query(
        connection,
        "CREATE TYPE stream_mood AS ENUM ('sad', 'ok', 'happy')",
      );
      const result = await duckdb.query(
        connection,
        `SELECT i, ['sad', 'ok', 'happy'][1 + (i % 3)]::stream_mood AS mood
        FROM range(5000) AS rows(i)`,
      );
      const stream = duckdb.result_arrow_ipc_stream(result, 1024, 257);

      // Let the bounded queue fill before consuming it.
      await new Promise((resolve) => setTimeout(resolve, 10));

      const chunks: Uint8Array[] = [];
      while (true) {
        const chunk = await duckdb.arrow_ipc_stream_next(stream);
        if (chunk === null) {
          break;
        }
        expect(chunk.byteLength).toBeLessThanOrEqual(257);
        chunks.push(chunk);
      }

      expect(chunks.length).toBeGreaterThan(1);
      const table = tableFromIPC(Buffer.concat(chunks));
      expect(table.numRows).toBe(5000);
      expect(table.batches).toHaveLength(3);
      expect(table.schema.fields[1].type.toString()).toBe(
        'Dictionary<Uint8, Utf8>',
      );
    });
  });

  test('cancels a producer blocked by a full queue', async () => {
    await withConnection(async (connection) => {
      const result = await duckdb.query(
        connection,
        "SELECT repeat('x', 1000) FROM range(10000)",
      );
      const stream = duckdb.result_arrow_ipc_stream(result, 64, 64);

      await new Promise((resolve) => setTimeout(resolve, 10));
      duckdb.arrow_ipc_stream_cancel(stream);

      await expect(duckdb.arrow_ipc_stream_next(stream)).resolves.toBeNull();
    });
  });

  test('rejects concurrent reads from one IPC stream', async () => {
    await withConnection(async (connection) => {
      const result = await duckdb.query(connection, 'SELECT * FROM range(10)');
      const stream = duckdb.result_arrow_ipc_stream(result);
      const firstRead = duckdb.arrow_ipc_stream_next(stream);

      expect(() => duckdb.arrow_ipc_stream_next(stream)).toThrow(
        'An Arrow IPC stream read is already in progress',
      );
      expect(await firstRead).not.toBeNull();
      duckdb.arrow_ipc_stream_cancel(stream);
    });
  });
});
