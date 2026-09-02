#!/usr/bin/env python3
"""Compare both IPC artifacts with PyArrow at the schema and value levels."""

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path

import pyarrow.ipc as ipc


def load(path: Path):
    with path.open("rb") as source:
        table = ipc.open_stream(source).read_all()
    with path.open("rb") as source:
        messages = Counter(
            str(message.type) for message in ipc.MessageReader.open_stream(source)
        )
    return table, dict(messages)


def schema_signature(table) -> list[dict[str, object]]:
    return [
        {
            "name": field.name,
            "type": str(field.type),
            "nullable": field.nullable,
            "metadata": {
                key.decode("utf-8"): value.decode("utf-8")
                for key, value in (field.metadata or {}).items()
            },
        }
        for field in table.schema
    ]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("nanoarrow", type=Path)
    parser.add_argument("arrow_cpp", type=Path)
    args = parser.parse_args()

    nano, nano_messages = load(args.nanoarrow)
    cpp, cpp_messages = load(args.arrow_cpp)
    nano_schema = schema_signature(nano)
    cpp_schema = schema_signature(cpp)

    differing_columns = []
    for index, field in enumerate(nano.schema):
        nano_column = nano.column(index)
        cpp_column = cpp.column(index)
        # Arrow considers NaN unequal to itself, and test_all_types() includes
        # NaN inside a nested DOUBLE[]. Its stable representation also handles
        # DuckDB's out-of-range +/-infinity date sentinels, which to_pylist()
        # cannot convert to Python datetime values.
        if not nano_column.equals(cpp_column) and str(nano_column) != str(cpp_column):
            differing_columns.append(field.name)

    report = {
        "rows": {"nanoarrow": nano.num_rows, "arrow_cpp": cpp.num_rows},
        "columns": {"nanoarrow": nano.num_columns, "arrow_cpp": cpp.num_columns},
        "messages": {"nanoarrow": nano_messages, "arrow_cpp": cpp_messages},
        "schemas_equal": nano_schema == cpp_schema,
        "values_equal": not differing_columns,
        "differing_columns": differing_columns,
    }
    print(json.dumps(report, indent=2))

    if not report["schemas_equal"] or not report["values_equal"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
