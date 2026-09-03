#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nanoarrow/nanoarrow.h"

#include "duckdb.h"

// Consume the remaining chunks of result and encode them as an Arrow IPC stream.
// output must be initialized with ArrowBufferInit() before calling.
ArrowErrorCode HayBarnResultToArrowIpcStream(duckdb_result* result,
                                             struct ArrowBuffer* output,
                                             struct ArrowError* error);

// A blocking Arrow IPC producer backed by a bounded byte queue. The producer
// runs on its own thread; Next() may block and must not be called on the Node.js
// main thread.
struct HayBarnArrowIpcStream;

HayBarnArrowIpcStream* HayBarnArrowIpcStreamCreate(duckdb_result* result,
                                                   int64_t max_queue_bytes,
                                                   int64_t max_chunk_bytes);
ArrowErrorCode HayBarnArrowIpcStreamStart(HayBarnArrowIpcStream* stream);
bool HayBarnArrowIpcStreamBeginNext(HayBarnArrowIpcStream* stream);
void HayBarnArrowIpcStreamEndNext(HayBarnArrowIpcStream* stream);
ArrowErrorCode HayBarnArrowIpcStreamNext(HayBarnArrowIpcStream* stream,
                                         std::vector<uint8_t>* chunk,
                                         bool* done,
                                         std::string* error);
void HayBarnArrowIpcStreamCancel(HayBarnArrowIpcStream* stream);
void HayBarnArrowIpcStreamDestroy(HayBarnArrowIpcStream* stream);
