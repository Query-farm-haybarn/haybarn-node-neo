#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "nanoarrow/nanoarrow.h"
#include "nanoarrow/nanoarrow_ipc.h"

#include "haybarn_arrow_query.hpp"

namespace {

void Check(ArrowErrorCode code, const ArrowError& error, const char* operation) {
  if (code != NANOARROW_OK) {
    throw std::runtime_error(std::string(operation) + ": " + error.message);
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const auto started = std::chrono::steady_clock::now();
    HayBarnArrowQuery query(SpikeSql(argc, argv));
    HayBarnArrowArrayStream query_stream(query);
    ArrowArrayStream input_stream{};
    query_stream.Export(&input_stream);

    ArrowError error{};
    ArrowBuffer output;
    ArrowBufferInit(&output);

    ArrowIpcOutputStream output_stream{};
    Check(ArrowIpcOutputStreamInitBuffer(&output_stream, &output), error,
          "ArrowIpcOutputStreamInitBuffer");

    ArrowIpcWriter writer{};
    Check(ArrowIpcWriterInit(&writer, &output_stream), error, "ArrowIpcWriterInit");
    Check(ArrowIpcWriterWriteArrayStream(&writer, &input_stream, &error), error,
          "ArrowIpcWriterWriteArrayStream");
    ArrowArrayStreamRelease(&input_stream);

    const char* path = SpikeOutputPath(argc, argv, "nanoarrow.arrow");
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(output.data), output.size_bytes);
    file.close();

    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started);
    std::cout << "writer=nanoarrow format=stream rows=" << query.row_count()
              << " bytes=" << output.size_bytes << " elapsed_ms=" << elapsed.count()
              << " output=" << path << '\n';

    ArrowIpcWriterReset(&writer);
    ArrowBufferReset(&output);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "nanoarrow spike failed: " << error.what() << '\n';
    return 1;
  }
}
