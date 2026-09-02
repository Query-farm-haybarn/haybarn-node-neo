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
        const bytes = await duckdb.result_to_arrow_ipc_stream(result);
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
      const bytes = await duckdb.result_to_arrow_ipc_stream(result);
      const table = tableFromIPC(bytes);

      expect(table.numRows).toBe(0);
      expect(table.schema.fields[0].name).toBe('answer');
      expect(table.schema.fields[0].type.toString()).toBe('Int32');
    });
  });
});
