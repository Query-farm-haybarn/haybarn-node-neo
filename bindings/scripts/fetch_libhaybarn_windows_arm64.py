import os
from fetch_libhaybarn import fetch_libhaybarn

zip_url = "https://github.com/Query-farm-haybarn/haybarn/releases/download/haybarn-v1.5.5-rc1/libhaybarn-windows-arm64.zip"
output_dir = os.path.join(os.path.dirname(__file__), "..", "libhaybarn")
files = [
  "duckdb.h",
  "haybarn.lib",
  "haybarn.dll",
]

fetch_libhaybarn(zip_url, output_dir, files)
