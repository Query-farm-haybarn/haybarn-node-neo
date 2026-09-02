#include "arrow_ipc.hpp"

#include <cerrno>
#include <cstring>
#include <exception>
#include <new>
#include <string>
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

  struct ArrowIpcWriter writer;
  std::memset(&writer, 0, sizeof(writer));
  result_code = ArrowIpcWriterInit(&writer, &output_stream);
  if (result_code != NANOARROW_OK) {
    output_stream.release(&output_stream);
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
