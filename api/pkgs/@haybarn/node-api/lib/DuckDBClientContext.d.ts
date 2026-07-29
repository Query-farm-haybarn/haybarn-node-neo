import duckdb from '@haybarn/node-bindings';
export declare class DuckDBClientContext {
    private readonly client_context;
    constructor(client_context: duckdb.ClientContext);
    get connectionId(): number;
}
