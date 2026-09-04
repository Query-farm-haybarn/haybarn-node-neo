# Vendored nanoarrow

This directory contains the bundled nanoarrow C Data and IPC implementation
generated from Query-farm/arrow-nanoarrow commit
`9367abbdfa5a4b8082b5d7abec64113c57eab27e` on the
`integration/haybarn-arrow-ipc` branch. This revision combines the dictionary
writer from apache/arrow-nanoarrow#928 with the finalized array-view appender
from apache/arrow-nanoarrow#930 on top of upstream `main`.

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
