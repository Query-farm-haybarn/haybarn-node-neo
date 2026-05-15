# CLAUDE.md — haybarn-node-neo

Guidance for Claude Code working in this repository.

## What this repo is

**haybarn-node-neo** is the Node.js client for
[Haybarn](https://github.com/Query-farm-haybarn/haybarn) — an independent
derived distribution of DuckDB ("Haybarn, powered by DuckDB"), published by
Query Farm LLC. This repo is a **hard fork** of `duckdb/duckdb-node-neo`,
currently based on upstream main at SHA `23c7c4891f73769d16f2c3f43c1be6253a64431a`
(the v1.5.2-r.2 release).

All Haybarn-specific changes are a small, curated **commit stack** on top of
the upstream pin — not scattered edits. Keep it that way: the stack must stay
easy to rebase onto future upstream commits.

## Hard rules

- **Never rename the C API header (`duckdb.h`).** That's the upstream C surface
  and Haybarn is intentionally ABI-compatible with it. The bindings link a
  Haybarn-renamed library (`libhaybarn.so`/`libhaybarn.dylib`/`haybarn.dll`)
  that exports the same `duckdb_*` C symbols.
- **Trademark compliance is mandatory.** Product name is always "Haybarn",
  never "DuckDB Haybarn". DuckDB only appears descriptively
  ("powered by DuckDB", "derived distribution of DuckDB"). Keep the MIT
  `LICENSE` verbatim; keep `NOTICE` accurate. Full rules:
  https://duckdb.org/trademark_guidelines.
- **One commit = one concern.** Prefer additive files over edits to upstream
  files. When you must edit upstream, keep it surgical.

## The Haybarn commit stack (on top of duckdb-node-neo @ 23c7c48)

| Concern | Key files |
|---|---|
| branding: rename @duckdb/* → @haybarn/* | `api/pkgs/@haybarn/`, `bindings/pkgs/@haybarn/`, `pnpm-workspace.yaml`, every `package.json`, all `.ts/.gyp/.md` imports |
| branding: fetch libhaybarn release zips | `bindings/scripts/fetch_libhaybarn*.py`, `bindings/binding.gyp`, `bindings/pkgs/@haybarn/node-bindings/haybarn.{js,d.ts}`, `bindings/.gitignore` |
| docs | `README.md`, `NOTICE`, `CLAUDE.md`, package READMEs |
| ci: haybarn publish | `.github/workflows/haybarn-node-neo.yml` (new — see below) |

## Releasing

Tag `haybarn-v<version>` triggers `.github/workflows/haybarn-node-neo.yml`:

1. Each platform builds against the matching `libhaybarn-<plat>.zip` from
   `Query-farm-haybarn/haybarn` release `haybarn-v<version>`.
2. The native `haybarn.node` is built per platform and copied into the
   corresponding `@haybarn/node-bindings-<plat>` package.
3. All 10 `@haybarn/*` packages are published to npm with `pnpm publish`.

A Haybarn engine release (`haybarn-v<version>` on `Query-farm-haybarn/haybarn`)
must exist first so the fetch scripts can find `libhaybarn-*.zip`.

## Layout

```
api/
├── pkgs/@haybarn/node-api/       (publishable npm package — high-level API)
└── src/                          (TS source, builds to lib/)

bindings/
├── binding.gyp                   (gyp build of native addon)
├── src/                          (C++ source — node-addon-api wrappers)
├── scripts/fetch_libhaybarn*.py  (downloads libhaybarn-*.zip at build time)
├── libhaybarn/                   (gitignored — extracted .so/.dylib/.dll + duckdb.h)
└── pkgs/@haybarn/
    ├── node-bindings/                            (dispatcher, picks per-platform addon)
    ├── node-bindings-darwin-arm64/
    ├── node-bindings-darwin-x64/
    ├── node-bindings-linux-arm64/
    ├── node-bindings-linux-arm64-musl/
    ├── node-bindings-linux-x64/
    ├── node-bindings-linux-x64-musl/
    ├── node-bindings-win32-arm64/
    └── node-bindings-win32-x64/

pnpm-workspace.yaml               (workspace globs include @haybarn/* subdirs)
README.md, NOTICE, CLAUDE.md, LICENSE
```

## Related Haybarn repos

- `Query-farm-haybarn/haybarn` — core engine fork (produces `libhaybarn-*.zip`)
- `Query-farm-haybarn/haybarn-python` — Python client
- `Query-farm-haybarn/haybarn-jdbc` — JDBC driver
- See the core fork's `HAYBARN/` directory for the multi-repo overview.
