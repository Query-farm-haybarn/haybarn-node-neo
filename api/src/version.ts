import duckdb from '@haybarn/node-bindings';

export function version(): string {
  return duckdb.library_version();
}
