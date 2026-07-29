import duckdb from '@haybarn/node-bindings';
import { DuckDBResult } from './DuckDBResult';
export declare function createResult(result: duckdb.Result): DuckDBResult;
