# Vendored nanoarrow

This directory contains the bundled nanoarrow C Data and IPC implementation
generated from Query-farm/arrow-nanoarrow commit
`de091e0a7d2d8906457ce71d4042fc9111d94c00`.

The bundle was generated with nanoarrow's `ci/scripts/bundle.py`:

```sh
python3 ci/scripts/bundle.py \
  --output-dir /path/to/bindings/vendor/nanoarrow \
  --with-ipc \
  --with-flatcc \
  --symbol-namespace HaybarnNanoarrow
```

The namespace prevents the addon symbols from colliding with another copy of
nanoarrow loaded into the same process. Replace this bundle with a released
upstream version once the dictionary writer changes are available there.
