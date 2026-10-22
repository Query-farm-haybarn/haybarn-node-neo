from fetch_libhaybarn import fetch_libhaybarn

# The Haybarn engine release ships one universal (arm64 + x86_64) macOS library.
fetch_libhaybarn("osx-universal", [
  "duckdb.h",
  "libhaybarn.dylib",
])
