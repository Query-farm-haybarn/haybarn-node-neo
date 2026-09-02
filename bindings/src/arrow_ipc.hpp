#pragma once

#include "nanoarrow/nanoarrow.h"

#include "duckdb.h"

// Consume the remaining chunks of result and encode them as an Arrow IPC stream.
// output must be initialized with ArrowBufferInit() before calling.
ArrowErrorCode HayBarnResultToArrowIpcStream(duckdb_result* result,
                                             struct ArrowBuffer* output,
                                             struct ArrowError* error);
