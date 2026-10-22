import os
import urllib.request
import zipfile

# Where the libhaybarn-<suffix>.zip archives are fetched from: the Haybarn
# engine release on Query-farm-haybarn/haybarn (never DuckDB's own binaries).
base_url = "https://github.com/Query-farm-haybarn/haybarn/releases/download/haybarn-v2.0.0-rc3"

def fetch_libhaybarn(suffix, files):
  output_dir = os.path.join(os.path.dirname(__file__), "..", "libhaybarn")
  if not os.path.exists(output_dir):
    os.makedirs(output_dir)

  url = f"{base_url}/libhaybarn-{suffix}.zip"
  local_archive_path = os.path.join(output_dir, "libhaybarn.zip")
  print("fetching: " + url)
  with urllib.request.urlopen(url) as response, open(local_archive_path, "wb") as f:
    while chunk := response.read(1 << 20):
      f.write(chunk)

  with zipfile.ZipFile(local_archive_path) as zip:
    for file in files:
      print("extracting: " + file)
      zip.extract(file, output_dir)
