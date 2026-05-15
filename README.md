# DuckDB Node Bindings & API

[Node](https://nodejs.org/) bindings to the [DuckDB C API](https://duckdb.org/docs/api/c/overview), plus a friendly API for using DuckDB in Node applications.

## Packages

### Documentation

- [@haybarn/node-api](api/pkgs/@haybarn/node-api/README.md)
- [@haybarn/node-bindings](bindings/pkgs/@haybarn/node-bindings/README.md)
- [@haybarn/node-bindings-darwin-arm64](bindings/pkgs/@haybarn/node-bindings-darwin-arm64/README.md)
- [@haybarn/node-bindings-darwin-x64](bindings/pkgs/@haybarn/node-bindings-darwin-x64/README.md)
- [@haybarn/node-bindings-linux-arm64](bindings/pkgs/@haybarn/node-bindings-linux-arm64/README.md)
- [@haybarn/node-bindings-linux-x64](bindings/pkgs/@haybarn/node-bindings-linux-x64/README.md)
- [@haybarn/node-bindings-win32-arm64](bindings/pkgs/@haybarn/node-bindings-win32-arm64/README.md)
- [@haybarn/node-bindings-win32-x64](bindings/pkgs/@haybarn/node-bindings-win32-x64/README.md)

### Published

- [@haybarn/node-api](https://www.npmjs.com/package/@haybarn/node-api)
- [@haybarn/node-bindings](https://www.npmjs.com/package/@haybarn/node-bindings)
- [@haybarn/node-bindings-darwin-arm64](https://www.npmjs.com/package/@haybarn/node-bindings-darwin-arm64)
- [@haybarn/node-bindings-darwin-x64](https://www.npmjs.com/package/@haybarn/node-bindings-darwin-x64)
- [@haybarn/node-bindings-linux-arm64](https://www.npmjs.com/package/@haybarn/node-bindings-linux-arm64)
- [@haybarn/node-bindings-linux-x64](https://www.npmjs.com/package/@haybarn/node-bindings-linux-x64)
- [@haybarn/node-bindings-win32-arm64](https://www.npmjs.com/package/@haybarn/node-bindings-win32-arm64)
- [@haybarn/node-bindings-win32-x64](https://www.npmjs.com/package/@haybarn/node-bindings-win32-x64)

## Development

### Setup
- [Install pnpm](https://pnpm.io/installation)
- `pnpm install`

### Build & Test Bindings
- `cd bindings`
- `pnpm run build`
- `pnpm test`

### Build & Test API
- `cd api`
- `pnpm run build`
- `pnpm test`

### Run API Benchmarks
- `cd api`
- `pnpm bench`

### Update Package Versions

Change version in:
- `api/pkgs/@haybarn/node-api/package.json`
- `bindings/pkgs/@haybarn/node-bindings/package.json`
- `bindings/pkgs/@haybarn/node-bindings-darwin-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-darwin-x64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-x64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-win32-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-win32-x64/package.json`

### Upgrade DuckDB Version

Change version in:
- `bindings/scripts/fetch_libduckdb_linux_amd64_musl.py`
- `bindings/scripts/fetch_libduckdb_linux_amd64.py`
- `bindings/scripts/fetch_libduckdb_linux_arm64_musl.py`
- `bindings/scripts/fetch_libduckdb_linux_arm64.py`
- `bindings/scripts/fetch_libduckdb_osx_universal.py`
- `bindings/scripts/fetch_libduckdb_windows_amd64.py`
- `bindings/scripts/fetch_libduckdb_windows_arm64.py`
- `bindings/test/constants.test.ts`

Also change DuckDB version in package versions.

### Check Function Signatures

- `node scripts/checkFunctionSignatures.mjs [writeFiles]`

Checks for differences between the function signatures in `duckdb.h` and those declared in `duckdb.d.ts` and implemented in `duckdb_node_bindings.cpp`.

Optionally outputs JSON files that can be diff'd.

Useful when upgrading the DuckDB version to detect changes to the C API.

### Publish Packages

- Update package versions (as above).
- Use the workflow dispatch for the [DuckDB Node Bindings & API GitHub action](https://github.com/duckdb/duckdb-node-neo/actions/workflows/DuckDBNodeBindingsAndAPI.yml).
- Select all initially-unchecked checkboxes to build on all platforms and publish all packages.
- Uncheck "Publish Dry Run" to actually publish.
