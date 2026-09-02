#!/usr/bin/env bash
set -euo pipefail

spike_dir="$(cd "$(dirname "$0")" && pwd)"
arrow_source="${HAYBARN_ARROW_SOURCE:-$spike_dir/../../../haybarn-wasm/submodules/arrow/cpp}"
arrow_build="$spike_dir/build/arrow-minimal-build"
arrow_install="$spike_dir/build/arrow-minimal-install"
spike_build="$spike_dir/build/minimal-spike"
artifact_dir="$spike_dir/build/artifacts"

if [[ ! -f "$arrow_source/CMakeLists.txt" ]]; then
  echo "Arrow C++ source was not found at $arrow_source" >&2
  echo "Set HAYBARN_ARROW_SOURCE to an Apache Arrow cpp source directory." >&2
  exit 1
fi

# These intentionally mirror haybarn-wasm/lib/cmake/arrow.cmake. They retain
# Arrow IPC and the C Data Interface bridge while excluding optional services,
# filesystems, formats, compression libraries, and allocators.
cmake -S "$arrow_source" -B "$arrow_build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$arrow_install" \
  -DARROW_BUILD_SHARED=OFF \
  -DARROW_BUILD_STATIC=ON \
  -DARROW_BUILD_UTILITIES=OFF \
  -DARROW_COMPUTE=OFF \
  -DARROW_CSV=OFF \
  -DARROW_DATASET=OFF \
  -DARROW_FILESYSTEM=OFF \
  -DARROW_FLIGHT=OFF \
  -DARROW_GCS=OFF \
  -DARROW_HDFS=OFF \
  -DARROW_IPC=ON \
  -DARROW_JSON=OFF \
  -DARROW_ORC=OFF \
  -DARROW_PARQUET=OFF \
  -DARROW_S3=OFF \
  -DARROW_AZURE=OFF \
  -DARROW_JEMALLOC=OFF \
  -DARROW_MIMALLOC=OFF \
  -DARROW_DEPENDENCY_USE_SHARED=OFF \
  -DARROW_SIMD_LEVEL=NONE \
  -DARROW_RUNTIME_SIMD_LEVEL=NONE \
  -DARROW_USE_GLOG=OFF \
  -DARROW_USE_CCACHE=OFF \
  -DARROW_USE_SCCACHE=OFF \
  -DARROW_WITH_BROTLI=OFF \
  -DARROW_WITH_LZ4=OFF \
  -DARROW_WITH_PROTOBUF=OFF \
  -DARROW_WITH_RAPIDJSON=OFF \
  -DARROW_WITH_RE2=OFF \
  -DARROW_WITH_SNAPPY=OFF \
  -DARROW_WITH_UTF8PROC=OFF \
  -DARROW_WITH_ZLIB=OFF \
  -DARROW_WITH_ZSTD=OFF \
  -DARROW_ALTIVEC=OFF \
  -DARROW_ENABLE_TIMING_TESTS=OFF

cmake --build "$arrow_build" --target install --parallel

cmake -S "$spike_dir" -B "$spike_build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DArrow_DIR="$arrow_install/lib/cmake/Arrow" \
  -DSPIKE_BUILD_NANOARROW=OFF
cmake --build "$spike_build" --target haybarn_ipc_arrow_cpp_static --parallel

mkdir -p "$artifact_dir"
"$spike_build/haybarn_ipc_arrow_cpp_static" \
  "$artifact_dir/arrow-cpp-minimal-all-types.arrow"

if [[ "$(uname -s)" == "Darwin" ]]; then
  # Report the package-relevant size; debug symbols are not shipped.
  strip -x "$spike_build/haybarn_ipc_arrow_cpp_static"
fi

ls -lh \
  "$arrow_install/lib/libarrow.a" \
  "$spike_build/haybarn_ipc_arrow_cpp_static" \
  "$artifact_dir/arrow-cpp-minimal-all-types.arrow"

if command -v otool >/dev/null 2>&1; then
  otool -L "$spike_build/haybarn_ipc_arrow_cpp_static"
fi
