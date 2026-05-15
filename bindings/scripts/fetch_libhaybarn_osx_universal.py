import os
from fetch_libhaybarn import fetch_libhaybarn

zip_url = "https://github.com/Query-farm-haybarn/haybarn/releases/download/haybarn-v1.5.2/libhaybarn-osx-universal.zip"
output_dir = os.path.join(os.path.dirname(__file__), "..", "libhaybarn")
files = [
  "duckdb.h",
  "libhaybarn.dylib",
]

fetch_libhaybarn(zip_url, output_dir, files)
