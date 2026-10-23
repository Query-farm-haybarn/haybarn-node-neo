from fetch_libhaybarn import fetch_libhaybarn

# Haybarn 2.0 ships macOS arm64 only (libhaybarn-osx-arm64.zip). Engine
# releases up to haybarn-v2.0.0-rc5 shipped only the universal library.
fetch_libhaybarn("osx-arm64", [
  "duckdb.h",
  "libhaybarn.dylib",
], fallback_suffix="osx-universal")
