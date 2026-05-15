# Haybarn Node Bindings & API

[Node](https://nodejs.org/) bindings to the
[DuckDB C API](https://duckdb.org/docs/api/c/overview), plus a friendly API
for using Haybarn — *a derived distribution of DuckDB, powered by DuckDB* —
in Node applications.

This is a hard fork of
[duckdb/duckdb-node-neo](https://github.com/duckdb/duckdb-node-neo). It links
against [Haybarn](https://github.com/Query-farm-haybarn/haybarn) (`libhaybarn`)
rather than upstream `libduckdb`, and its npm packages are published under the
`@haybarn` scope. See [`NOTICE`](NOTICE) for what's been modified and the
trademark attribution.

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

## Use

```ts
import { DuckDBInstance } from '@haybarn/node-api';

const instance = await DuckDBInstance.create(':memory:');
const connection = await instance.connect();
const reader = await connection.runAndReadAll('select version()');
console.log(reader.getRows()); // [ [ 'v1.5.2' ] ]
```

The JavaScript API surface is unchanged from upstream — class names, method
signatures, and return shapes are kept verbatim so migration is essentially
a single import-source swap:

```ts
- import { DuckDBInstance } from '@duckdb/node-api';
+ import { DuckDBInstance } from '@haybarn/node-api';
```

See [`api/pkgs/@haybarn/node-api/README.md`](api/pkgs/@haybarn/node-api/README.md)
for the full API.

## Development

### Setup
- [Install pnpm](https://pnpm.io/installation)
- `pnpm install`

### Build & Test Bindings
- `cd bindings`
- `pnpm run build`     *(downloads `libhaybarn-*.zip` from Haybarn releases)*
- `pnpm test`

### Build & Test API
- `cd api`
- `pnpm run build`
- `pnpm test`

### Update Package Versions

Bump version in:
- `api/pkgs/@haybarn/node-api/package.json`
- `bindings/pkgs/@haybarn/node-bindings/package.json`
- `bindings/pkgs/@haybarn/node-bindings-darwin-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-darwin-x64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-arm64-musl/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-x64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-linux-x64-musl/package.json`
- `bindings/pkgs/@haybarn/node-bindings-win32-arm64/package.json`
- `bindings/pkgs/@haybarn/node-bindings-win32-x64/package.json`

### Upgrade Haybarn / DuckDB Version

Change the release tag (`haybarn-v<version>`) and the inside-zip file names
in each fetch script:

- `bindings/scripts/fetch_libhaybarn_linux_amd64.py`
- `bindings/scripts/fetch_libhaybarn_linux_amd64_musl.py`
- `bindings/scripts/fetch_libhaybarn_linux_arm64.py`
- `bindings/scripts/fetch_libhaybarn_linux_arm64_musl.py`
- `bindings/scripts/fetch_libhaybarn_osx_universal.py`
- `bindings/scripts/fetch_libhaybarn_windows_amd64.py`
- `bindings/scripts/fetch_libhaybarn_windows_arm64.py`
- `bindings/test/constants.test.ts`

Then bump the package versions as above.

### Check Function Signatures

- `node scripts/checkFunctionSignatures.mjs [writeFiles]`

Checks for differences between the function signatures in `duckdb.h` (kept
upstream-named since it's the C API surface) and those declared in
`haybarn.d.ts` and implemented in `duckdb_node_bindings.cpp`. Useful when
rolling forward the Haybarn version to detect changes to the C API.

### Publish Packages

Push a `haybarn-v<version>` tag (or use the
[Haybarn Node workflow_dispatch](https://github.com/Query-farm-haybarn/haybarn-node-neo/actions/workflows/haybarn-node-neo.yml))
to build on all platforms and publish all 10 `@haybarn/*` packages to npm.

## Attribution

Haybarn is independent of and not endorsed by the DuckDB Foundation. DuckDB is
a trademark of the DuckDB Foundation. See [`NOTICE`](NOTICE) for the modifications
this fork makes to the upstream `duckdb/duckdb-node-neo` repository.
