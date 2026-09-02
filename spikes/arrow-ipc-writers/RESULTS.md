# Arrow IPC writer spike results

## Recommendation

The dictionary-enabled nanoarrow branch is now a viable candidate for the Neo
binding's Arrow IPC stream writer. It handled every type in HayBarn's
`test_all_types()` result, including all three dictionary-encoded `ENUM`
widths, and produced values and schemas identical to Arrow C++ when decoded by
PyArrow.

Keep the dependency pinned to the immutable integration commit during
development. The functionality is being split into independently reviewed
upstream changes, so the fork commit should not become the long-term release
dependency.

## Results

Measurements were taken on macOS arm64. Both writers consumed the same HayBarn
Arrow C Data Interface schema and record batches and wrote Arrow IPC streams.

| Test | nanoarrow | Arrow C++ |
| --- | ---: | ---: |
| `test_all_types()` | 56 columns, exact match | 56 columns, reference |
| Repeated fixture | 6,000 rows / 3 batches, exact match | 6,000 rows / 3 batches, reference |
| Dictionary messages in repeated fixture | 3 total | 3 total |

The supported-type fixture includes extension metadata, 128-bit integers,
bignum, temporal and timezone types, decimal, UUID, interval, BLOB, BIT,
lists, fixed arrays, structs, maps, sparse unions, and geometry. PyArrow found
the schemas and values identical between the two artifacts.

PyArrow 24 read both nanoarrow streams successfully. Message inspection found
one schema, three dictionary messages, and three record batches in the
multi-batch nanoarrow artifact: the dictionaries were emitted once and reused
by all batches.

## Packaging observations

- The nanoarrow executable remains much smaller because its IPC writer and C
  Data implementation are narrowly scoped.
- nanoarrow can be compiled into the addon without introducing an Arrow C++
  runtime dependency.
- The integration commit is larger than the eventual writer-only dependency
  because it also contains delta decoding and the array-view appender.

## Integration shape

The binding should keep HayBarn/DuckDB as the owner of query execution and
batch production. A small `ArrowArrayStream` adapter can expose HayBarn's
schema and batches to `ArrowIpcWriterWriteArrayStream()`, which discovers and
writes dictionaries before their record batches. JavaScript can initially
receive the completed bytes through the existing native async-worker pattern.

The standalone harness uses the deprecated `duckdb_query_arrow_*` entry points
to keep the comparison small. Production code should use HayBarn's current
`duckdb_to_arrow_schema()` and `duckdb_data_chunk_to_arrow()` APIs on the
binding's existing result chunks and preserve the binding's Arrow conversion
options.

## Remaining validation

- Decode the generated stream with Apache Arrow JS in a binding-level test.
- Add the nanoarrow C sources and headers to the cross-platform node-gyp build.
- Decide whether a later API also exposes incremental chunks or writes to a
  file. Large exports should not permanently require one giant JavaScript
  allocation.
- Add binding-level tests for cancellation, write failures, empty results,
  dictionary columns, and Electron packaging/signing.
