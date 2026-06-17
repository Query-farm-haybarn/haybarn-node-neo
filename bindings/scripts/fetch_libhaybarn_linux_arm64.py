import os
from fetch_libhaybarn import fetch_libhaybarn

zip_url = "https://github.com/Query-farm-haybarn/haybarn/releases/download/haybarn-v1.5.4-rc1/libhaybarn-linux-arm64.zip"
output_dir = os.path.join(os.path.dirname(__file__), "..", "libhaybarn")
files = [
  "duckdb.h",
  "libhaybarn.so",
]

fetch_libhaybarn(zip_url, output_dir, files)
