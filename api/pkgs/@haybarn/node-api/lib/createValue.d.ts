import { Value } from '@haybarn/node-bindings';
import { DuckDBType } from './DuckDBType';
import { DuckDBValue } from './values';
export declare function createValue(type: DuckDBType, input: DuckDBValue): Value;
