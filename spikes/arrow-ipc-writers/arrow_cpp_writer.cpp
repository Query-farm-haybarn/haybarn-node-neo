#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <arrow/buffer.h>
#include <arrow/c/abi.h>
#include <arrow/c/bridge.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/writer.h>

#include "haybarn_arrow_query.hpp"

template <typename T>
T Unwrap(arrow::Result<T> result, const char* operation) {
  if (!result.ok()) {
    throw std::runtime_error(std::string(operation) + ": " + result.status().ToString());
  }
  return std::move(result).ValueUnsafe();
}

void Check(const arrow::Status& status, const char* operation) {
  if (!status.ok()) {
    throw std::runtime_error(std::string(operation) + ": " + status.ToString());
  }
}

int main(int argc, char** argv) {
  try {
    const auto started = std::chrono::steady_clock::now();
    HayBarnArrowQuery query(SpikeSql(argc, argv));

    ArrowSchema raw_schema{};
    query.ExportSchema(&raw_schema);
    auto schema = Unwrap(arrow::ImportSchema(&raw_schema), "arrow::ImportSchema");
    auto output = Unwrap(arrow::io::BufferOutputStream::Create(),
                         "arrow::io::BufferOutputStream::Create");
    auto writer = Unwrap(arrow::ipc::MakeStreamWriter(output, schema),
                         "arrow::ipc::MakeStreamWriter");

    std::int64_t batches = 0;
    while (true) {
      ArrowArray raw_array{};
      if (!query.Next(&raw_array)) {
        break;
      }
      auto batch = Unwrap(arrow::ImportRecordBatch(&raw_array, schema),
                          "arrow::ImportRecordBatch");
      Check(writer->WriteRecordBatch(*batch), "RecordBatchWriter::WriteRecordBatch");
      ++batches;
    }

    Check(writer->Close(), "RecordBatchWriter::Close");
    auto buffer = Unwrap(output->Finish(), "BufferOutputStream::Finish");

    const char* path = SpikeOutputPath(argc, argv, "arrow-cpp.arrow");
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer->data()), buffer->size());
    file.close();

    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started);
    std::cout << "writer=arrow-cpp format=stream rows=" << query.row_count()
              << " batches=" << batches << " bytes=" << buffer->size()
              << " elapsed_ms=" << elapsed.count() << " output=" << path << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Arrow C++ spike failed: " << error.what() << '\n';
    return 1;
  }
}
