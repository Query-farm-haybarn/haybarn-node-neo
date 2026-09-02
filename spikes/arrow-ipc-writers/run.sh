#!/usr/bin/env bash
set -euo pipefail

spike_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="${1:-$spike_dir/build}"
artifact_dir="$build_dir/artifacts"
supported_sql="SELECT * EXCLUDE (small_enum, medium_enum, large_enum) FROM test_all_types()"
multibatch_sql="SELECT t.* FROM test_all_types() AS t CROSS JOIN range(2000)"

cmake -S "$spike_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel
mkdir -p "$artifact_dir"

"$build_dir/haybarn_ipc_nanoarrow" "$artifact_dir/nanoarrow.arrow" "$supported_sql"
"$build_dir/haybarn_ipc_arrow_cpp" "$artifact_dir/arrow-cpp.arrow" "$supported_sql"
python3 "$spike_dir/validate.py" \
  "$artifact_dir/nanoarrow.arrow" \
  "$artifact_dir/arrow-cpp.arrow"

"$build_dir/haybarn_ipc_arrow_cpp" "$artifact_dir/arrow-cpp-all-types.arrow"
"$build_dir/haybarn_ipc_nanoarrow" "$artifact_dir/nanoarrow-all-types.arrow"
python3 "$spike_dir/validate.py" \
  "$artifact_dir/nanoarrow-all-types.arrow" \
  "$artifact_dir/arrow-cpp-all-types.arrow"

"$build_dir/haybarn_ipc_arrow_cpp" \
  "$artifact_dir/arrow-cpp-multibatch.arrow" "$multibatch_sql"
"$build_dir/haybarn_ipc_nanoarrow" \
  "$artifact_dir/nanoarrow-multibatch.arrow" "$multibatch_sql"
python3 "$spike_dir/validate.py" \
  "$artifact_dir/nanoarrow-multibatch.arrow" \
  "$artifact_dir/arrow-cpp-multibatch.arrow"

ls -lh \
  "$build_dir/haybarn_ipc_nanoarrow" \
  "$build_dir/haybarn_ipc_arrow_cpp" \
  "$build_dir/haybarn_ipc_arrow_cpp_static" \
  "$artifact_dir/nanoarrow.arrow" \
  "$artifact_dir/arrow-cpp.arrow" \
  "$artifact_dir/nanoarrow-all-types.arrow" \
  "$artifact_dir/arrow-cpp-all-types.arrow" \
  "$artifact_dir/nanoarrow-multibatch.arrow" \
  "$artifact_dir/arrow-cpp-multibatch.arrow"
