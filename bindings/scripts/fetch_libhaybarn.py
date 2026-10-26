import os
import urllib.error
import urllib.request
import zipfile

# Where the libhaybarn-<suffix>.zip archives are fetched from: the Haybarn
# engine release on Query-farm-haybarn/haybarn (never DuckDB's own binaries).
base_url = "https://github.com/Query-farm-haybarn/haybarn/releases/download/haybarn-v2.0.0-rc8"

def fetch_libhaybarn(suffix, files, fallback_suffix=None):
  output_dir = os.path.join(os.path.dirname(__file__), "..", "libhaybarn")
  if not os.path.exists(output_dir):
    os.makedirs(output_dir)

  local_archive_path = os.path.join(output_dir, "libhaybarn.zip")
  url = f"{base_url}/libhaybarn-{suffix}.zip"
  print("fetching: " + url)
  try:
    response = urllib.request.urlopen(url)
  except urllib.error.HTTPError as e:
    if e.code != 404 or fallback_suffix is None:
      raise
    url = f"{base_url}/libhaybarn-{fallback_suffix}.zip"
    print("not found; fetching: " + url)
    response = urllib.request.urlopen(url)
  with response, open(local_archive_path, "wb") as f:
    while chunk := response.read(1 << 20):
      f.write(chunk)

  with zipfile.ZipFile(local_archive_path) as zip:
    for file in files:
      print("extracting: " + file)
      zip.extract(file, output_dir)
