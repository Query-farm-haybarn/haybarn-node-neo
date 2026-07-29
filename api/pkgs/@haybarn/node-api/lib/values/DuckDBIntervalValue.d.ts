import { Interval } from '@haybarn/node-bindings';
export declare class DuckDBIntervalValue implements Interval {
    readonly months: number;
    readonly days: number;
    readonly micros: bigint;
    constructor(months: number, days: number, micros: bigint);
    toString(): string;
}
export declare function intervalValue(months: number, days: number, micros: bigint): DuckDBIntervalValue;
