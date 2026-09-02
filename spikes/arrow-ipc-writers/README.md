# Arrow IPC writer comparison spike

This isolates two native IPC writers behind the same HayBarn Arrow C Data
Interface result:

- Apache nanoarrow IPC, statically linked into the executable
- Apache Arrow C++ IPC, built in both dynamically and statically linked forms

Both execute the same broad type fixture with lossless Arrow conversion
enabled, write an Arrow IPC stream, and compare schemas, extension metadata,
and values with PyArrow. The nanoarrow writer is pinned to the Query-farm
integration commit containing dictionary batch writing and delta support.
HayBarn exports `ENUM` columns using Arrow dictionary encoding, so the
unmodified `test_all_types()` fixture exercises dictionary IPC messages.

The harness also repeats that fixture to produce three record batches. This
checks that nanoarrow emits each unchanged dictionary once and that subsequent
record batches continue to resolve its dictionary IDs.

## Run

Prerequisites on macOS:

```sh
brew install apache-arrow pyarrow
```

Then:

```sh
./run.sh
```

The observed comparison and recommendation are in [RESULTS.md](RESULTS.md).
To reproduce the self-contained, minimal Arrow C++ build used for the footprint
measurement:

```sh
./build-minimal-arrow.sh
```

That script defaults to the Arrow 21 source already pinned under
`haybarn-wasm/submodules/arrow`. Set `HAYBARN_ARROW_SOURCE` to test a different
Arrow C++ source tree.

The build fetches the exact nanoarrow fork commit pinned in `CMakeLists.txt`.
That commit is intended for integration testing while its constituent changes
are reviewed upstream. The harness uses the HayBarn distribution already
downloaded by the Node binding in `../../bindings/libhaybarn`.

This is deliberately not wired into `binding.gyp` yet. The purpose is to
measure compatibility, linkage, artifact size, and encoding cost before one
implementation becomes part of every published platform package.
