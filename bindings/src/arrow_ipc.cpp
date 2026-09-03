#include "arrow_ipc.hpp"

#include <algorithm>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <exception>
#include <mutex>
#include <new>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "nanoarrow/nanoarrow_ipc.h"

namespace {

class DuckDBResultArrayStream {
 public:
  explicit DuckDBResultArrayStream(duckdb_result* result)
      : result_(result), arrow_options_(duckdb_result_get_arrow_options(result)) {}

  ~DuckDBResultArrayStream() {
    if (arrow_options_ != nullptr) {
      duckdb_destroy_arrow_options(&arrow_options_);
    }
  }

  void Export(struct ArrowArrayStream* stream) {
    std::memset(stream, 0, sizeof(*stream));
    stream->get_schema = &GetSchema;
    stream->get_next = &GetNext;
    stream->get_last_error = &GetLastError;
    stream->release = &Release;
    stream->private_data = this;
  }

 private:
  static DuckDBResultArrayStream* Private(struct ArrowArrayStream* stream) {
    return static_cast<DuckDBResultArrayStream*>(stream->private_data);
  }

  static int GetSchema(struct ArrowArrayStream* stream, struct ArrowSchema* schema) {
    auto* self = Private(stream);
    try {
      return self->GetSchema(schema);
    } catch (const std::bad_alloc&) {
      self->last_error_ = "Out of memory while exporting the Arrow schema";
      return ENOMEM;
    } catch (const std::exception& exception) {
      self->last_error_ = exception.what();
      return EIO;
    } catch (...) {
      self->last_error_ = "Unknown error while exporting the Arrow schema";
      return EIO;
    }
  }

  static int GetNext(struct ArrowArrayStream* stream, struct ArrowArray* array) {
    auto* self = Private(stream);
    try {
      return self->GetNext(array);
    } catch (const std::bad_alloc&) {
      self->last_error_ = "Out of memory while exporting an Arrow array";
      return ENOMEM;
    } catch (const std::exception& exception) {
      self->last_error_ = exception.what();
      return EIO;
    } catch (...) {
      self->last_error_ = "Unknown error while exporting an Arrow array";
      return EIO;
    }
  }

  static const char* GetLastError(struct ArrowArrayStream* stream) {
    const auto& last_error = Private(stream)->last_error_;
    return last_error.empty() ? nullptr : last_error.c_str();
  }

  static void Release(struct ArrowArrayStream* stream) {
    stream->release = nullptr;
    stream->private_data = nullptr;
  }

  int GetSchema(struct ArrowSchema* schema) {
    std::memset(schema, 0, sizeof(*schema));
    last_error_.clear();

    if (arrow_options_ == nullptr) {
      last_error_ = "Failed to get Arrow conversion options for result";
      return EIO;
    }

    const idx_t column_count = duckdb_column_count(result_);
    std::vector<duckdb_logical_type> types(column_count, nullptr);
    std::vector<const char*> names(column_count, nullptr);

    for (idx_t i = 0; i < column_count; ++i) {
      types[i] = duckdb_column_logical_type(result_, i);
      names[i] = duckdb_column_name(result_, i);
      if (types[i] == nullptr || names[i] == nullptr) {
        for (auto& type : types) {
          if (type != nullptr) {
            duckdb_destroy_logical_type(&type);
          }
        }
        last_error_ = "Failed to get result column metadata for Arrow conversion";
        return EIO;
      }
    }

    duckdb_error_data conversion_error = duckdb_to_arrow_schema(
        arrow_options_, types.data(), names.data(), column_count, schema);
    for (auto& type : types) {
      duckdb_destroy_logical_type(&type);
    }

    return ConsumeDuckDBError(&conversion_error, "Failed to convert result schema");
  }

  int GetNext(struct ArrowArray* array) {
    std::memset(array, 0, sizeof(*array));
    last_error_.clear();

    duckdb_data_chunk chunk = duckdb_fetch_chunk(*result_);
    if (chunk == nullptr) {
      return 0;
    }

    duckdb_error_data conversion_error =
        duckdb_data_chunk_to_arrow(arrow_options_, chunk, array);
    duckdb_destroy_data_chunk(&chunk);

    const int result = ConsumeDuckDBError(&conversion_error,
                                          "Failed to convert result data chunk");
    if (result != 0 && array->release != nullptr) {
      array->release(array);
    }
    return result;
  }

  int ConsumeDuckDBError(duckdb_error_data* error, const char* fallback) {
    const bool has_error = duckdb_error_data_has_error(*error);
    if (has_error) {
      const char* message = duckdb_error_data_message(*error);
      last_error_ = message == nullptr ? fallback : message;
    }
    duckdb_destroy_error_data(error);
    return has_error ? EIO : 0;
  }

  duckdb_result* result_;
  duckdb_arrow_options arrow_options_;
  std::string last_error_;
};

ArrowErrorCode WriteResultToArrowIpcStream(
    duckdb_result* result, struct ArrowIpcOutputStream* output_stream,
    struct ArrowError* error) {
  struct ArrowIpcWriter writer;
  std::memset(&writer, 0, sizeof(writer));
  ArrowErrorCode result_code = ArrowIpcWriterInit(&writer, output_stream);
  if (result_code != NANOARROW_OK) {
    if (output_stream->release != nullptr) {
      output_stream->release(output_stream);
    }
    return result_code;
  }

  DuckDBResultArrayStream result_stream(result);
  struct ArrowArrayStream input_stream;
  result_stream.Export(&input_stream);
  result_code = ArrowIpcWriterWriteArrayStream(&writer, &input_stream, error);

  ArrowArrayStreamRelease(&input_stream);
  ArrowIpcWriterReset(&writer);
  return result_code;
}

}  // namespace

ArrowErrorCode HayBarnResultToArrowIpcStream(duckdb_result* result,
                                             struct ArrowBuffer* output,
                                             struct ArrowError* error) {
  if (result == nullptr) {
    ArrowErrorSet(error, "Cannot encode a destroyed result");
    return EINVAL;
  }

  struct ArrowIpcOutputStream output_stream;
  ArrowErrorCode result_code =
      ArrowIpcOutputStreamInitBuffer(&output_stream, output);
  if (result_code != NANOARROW_OK) {
    return result_code;
  }

  return WriteResultToArrowIpcStream(result, &output_stream, error);
}

struct HayBarnArrowIpcStream {
  HayBarnArrowIpcStream(duckdb_result* result, int64_t max_queue_bytes,
                        int64_t max_chunk_bytes)
      : result(result),
        max_queue_bytes(max_queue_bytes),
        max_chunk_bytes(std::min(max_chunk_bytes, max_queue_bytes)) {}

  ~HayBarnArrowIpcStream() {
    Cancel();
    if (producer.joinable()) {
      producer.join();
    }
  }

  ArrowErrorCode Start() {
    try {
      producer = std::thread([this]() { Produce(); });
      return NANOARROW_OK;
    } catch (const std::system_error&) {
      return EAGAIN;
    }
  }

  bool BeginNext() {
    std::lock_guard<std::mutex> lock(mutex);
    if (next_pending) {
      return false;
    }
    next_pending = true;
    return true;
  }

  void EndNext() {
    std::lock_guard<std::mutex> lock(mutex);
    next_pending = false;
  }

  ArrowErrorCode Next(std::vector<uint8_t>* chunk, bool* out_done,
                      std::string* out_error) {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [this]() {
      return cancelled || !queue.empty() || done;
    });

    ArrowErrorCode result_code = NANOARROW_OK;
    if (cancelled) {
      *out_done = true;
    } else if (!queue.empty()) {
      *chunk = std::move(queue.front());
      queued_bytes -= static_cast<int64_t>(chunk->size());
      queue.pop_front();
      *out_done = false;
    } else if (!error.empty()) {
      *out_error = error;
      result_code = EIO;
      *out_done = true;
    } else {
      *out_done = true;
    }

    lock.unlock();
    condition.notify_all();
    return result_code;
  }

  void Cancel() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      cancelled = true;
      queue.clear();
      queued_bytes = 0;
    }
    condition.notify_all();
  }

  static ArrowErrorCode Write(struct ArrowIpcOutputStream* output_stream,
                              const void* buffer, int64_t buffer_size,
                              int64_t* size_written, struct ArrowError* error) {
    auto* self = static_cast<HayBarnArrowIpcStream*>(output_stream->private_data);
    return self->Enqueue(buffer, buffer_size, size_written, error);
  }

  static void Release(struct ArrowIpcOutputStream* output_stream) {
    output_stream->release = nullptr;
    output_stream->private_data = nullptr;
  }

  ArrowErrorCode Enqueue(const void* buffer, int64_t buffer_size,
                         int64_t* size_written, struct ArrowError* output_error) {
    *size_written = 0;
    if (buffer_size <= 0) {
      return NANOARROW_OK;
    }

    const int64_t write_size = std::min(buffer_size, max_chunk_bytes);
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [this, write_size]() {
      return cancelled || queued_bytes + write_size <= max_queue_bytes;
    });

    if (cancelled) {
      ArrowErrorSet(output_error, "Arrow IPC stream was cancelled");
      return ECANCELED;
    }

    const auto* bytes = static_cast<const uint8_t*>(buffer);
    try {
      queue.emplace_back(bytes, bytes + write_size);
    } catch (const std::bad_alloc&) {
      ArrowErrorSet(output_error,
                    "Out of memory while queuing Arrow IPC stream bytes");
      return ENOMEM;
    }
    queued_bytes += write_size;
    *size_written = write_size;
    lock.unlock();
    condition.notify_all();
    return NANOARROW_OK;
  }

  void Produce() {
    struct ArrowIpcOutputStream output_stream{
        &HayBarnArrowIpcStream::Write,
        &HayBarnArrowIpcStream::Release,
        this,
    };
    struct ArrowError output_error{};
    ArrowErrorCode result_code =
        WriteResultToArrowIpcStream(result, &output_stream, &output_error);

    {
      std::lock_guard<std::mutex> lock(mutex);
      if (!cancelled && result_code != NANOARROW_OK) {
        error = output_error.message[0] == '\0'
                    ? "Failed to encode Arrow IPC stream"
                    : output_error.message;
      }
      done = true;
    }
    condition.notify_all();
  }

  duckdb_result* result;
  const int64_t max_queue_bytes;
  const int64_t max_chunk_bytes;
  std::mutex mutex;
  std::condition_variable condition;
  std::deque<std::vector<uint8_t>> queue;
  int64_t queued_bytes = 0;
  bool cancelled = false;
  bool done = false;
  bool next_pending = false;
  std::string error;
  std::thread producer;
};

HayBarnArrowIpcStream* HayBarnArrowIpcStreamCreate(duckdb_result* result,
                                                   int64_t max_queue_bytes,
                                                   int64_t max_chunk_bytes) {
  if (result == nullptr || max_queue_bytes <= 0 || max_chunk_bytes <= 0) {
    return nullptr;
  }
  return new (std::nothrow)
      HayBarnArrowIpcStream(result, max_queue_bytes, max_chunk_bytes);
}

ArrowErrorCode HayBarnArrowIpcStreamStart(HayBarnArrowIpcStream* stream) {
  return stream->Start();
}

bool HayBarnArrowIpcStreamBeginNext(HayBarnArrowIpcStream* stream) {
  return stream->BeginNext();
}

void HayBarnArrowIpcStreamEndNext(HayBarnArrowIpcStream* stream) {
  stream->EndNext();
}

ArrowErrorCode HayBarnArrowIpcStreamNext(HayBarnArrowIpcStream* stream,
                                         std::vector<uint8_t>* chunk,
                                         bool* done,
                                         std::string* error) {
  return stream->Next(chunk, done, error);
}

void HayBarnArrowIpcStreamCancel(HayBarnArrowIpcStream* stream) {
  if (stream != nullptr) {
    stream->Cancel();
  }
}

void HayBarnArrowIpcStreamDestroy(HayBarnArrowIpcStream* stream) {
  delete stream;
}
