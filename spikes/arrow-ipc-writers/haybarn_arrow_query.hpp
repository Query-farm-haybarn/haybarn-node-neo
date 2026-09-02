#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include "duckdb.h"

// duckdb.h intentionally only forward-declares these Arrow ABI structs. Each
// writer includes its library's canonical definitions before this header.

class HayBarnArrowQuery {
 public:
  explicit HayBarnArrowQuery(const char* sql) {
    try {
      if (duckdb_open(nullptr, &database_) == DuckDBError) {
        throw std::runtime_error("duckdb_open failed");
      }
      if (duckdb_connect(database_, &connection_) == DuckDBError) {
        throw std::runtime_error("duckdb_connect failed");
      }

      Execute("SET arrow_lossless_conversion = true");
      // Match the current HayBarn-WASM exporter. Avoiding view types also lets
      // older Arrow JS consumers decode the comparison artifacts.
      Execute("SET arrow_output_version = '1.0'");

      if (duckdb_query_arrow(connection_, sql, &result_) == DuckDBError) {
        const char* detail = result_ ? duckdb_query_arrow_error(result_) : nullptr;
        throw std::runtime_error(detail ? detail : "duckdb_query_arrow failed");
      }
    } catch (...) {
      Close();
      throw;
    }
  }

  HayBarnArrowQuery(const HayBarnArrowQuery&) = delete;
  HayBarnArrowQuery& operator=(const HayBarnArrowQuery&) = delete;

  ~HayBarnArrowQuery() { Close(); }

  void ExportSchema(struct ArrowSchema* schema) {
    std::memset(schema, 0, sizeof(*schema));
    auto opaque = reinterpret_cast<duckdb_arrow_schema>(schema);
    if (duckdb_query_arrow_schema(result_, &opaque) == DuckDBError) {
      throw std::runtime_error("duckdb_query_arrow_schema failed");
    }
  }

  bool Next(struct ArrowArray* array) {
    std::memset(array, 0, sizeof(*array));
    auto opaque = reinterpret_cast<duckdb_arrow_array>(array);
    if (duckdb_query_arrow_array(result_, &opaque) == DuckDBError) {
      throw std::runtime_error("duckdb_query_arrow_array failed");
    }
    return array->release != nullptr;
  }

  std::int64_t row_count() const {
    return static_cast<std::int64_t>(duckdb_arrow_row_count(result_));
  }

 private:
  void Close() noexcept {
    if (result_) {
      duckdb_destroy_arrow(&result_);
    }
    if (connection_) {
      duckdb_disconnect(&connection_);
    }
    if (database_) {
      duckdb_close(&database_);
    }
  }
  void Execute(const char* sql) {
    duckdb_result result{};
    if (duckdb_query(connection_, sql, &result) == DuckDBError) {
      const char* detail = duckdb_result_error(&result);
      const std::string message = detail ? detail : "HayBarn configuration query failed";
      duckdb_destroy_result(&result);
      throw std::runtime_error(message);
    }
    duckdb_destroy_result(&result);
  }

  duckdb_database database_ = nullptr;
  duckdb_connection connection_ = nullptr;
  duckdb_arrow result_ = nullptr;
};

// Adapt HayBarn's pull-based Arrow C Data Interface result to the Arrow C Stream
// Interface. The adapter does not own the query; callers must keep both alive until
// the consumer has finished with the exported stream.
class HayBarnArrowArrayStream {
 public:
  explicit HayBarnArrowArrayStream(HayBarnArrowQuery& query) : query_(query) {}

  void Export(struct ArrowArrayStream* stream) {
    std::memset(stream, 0, sizeof(*stream));
    stream->get_schema = &GetSchema;
    stream->get_next = &GetNext;
    stream->get_last_error = &GetLastError;
    stream->release = &Release;
    stream->private_data = this;
  }

 private:
  static HayBarnArrowArrayStream* Private(struct ArrowArrayStream* stream) {
    return static_cast<HayBarnArrowArrayStream*>(stream->private_data);
  }

  static int GetSchema(struct ArrowArrayStream* stream, struct ArrowSchema* schema) {
    auto* self = Private(stream);
    try {
      self->last_error_.clear();
      self->query_.ExportSchema(schema);
      return 0;
    } catch (const std::exception& error) {
      self->last_error_ = error.what();
      return EIO;
    }
  }

  static int GetNext(struct ArrowArrayStream* stream, struct ArrowArray* array) {
    auto* self = Private(stream);
    try {
      self->last_error_.clear();
      self->query_.Next(array);
      return 0;
    } catch (const std::exception& error) {
      self->last_error_ = error.what();
      return EIO;
    }
  }

  static const char* GetLastError(struct ArrowArrayStream* stream) {
    return Private(stream)->last_error_.empty() ? nullptr
                                                : Private(stream)->last_error_.c_str();
  }

  static void Release(struct ArrowArrayStream* stream) {
    stream->release = nullptr;
    stream->private_data = nullptr;
  }

  HayBarnArrowQuery& query_;
  std::string last_error_;
};

inline const char* SpikeSql(int argc, char** argv) {
  return argc >= 3 ? argv[2] : "FROM test_all_types()";
}

inline const char* SpikeOutputPath(int argc, char** argv, const char* fallback) {
  return argc >= 2 ? argv[1] : fallback;
}
