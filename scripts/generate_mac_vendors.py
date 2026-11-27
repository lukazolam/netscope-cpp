
#!/usr/bin/env python3

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

DATA_DIR = ROOT / "data" / "ieee"
OUTPUT = ROOT / "include" / "macvendor_generated.h"

REGISTRIES = [
    ("oui.csv", "maL", 24),
    ("mam.csv", "maM", 28),
    ("oui36.csv", "maS", 36),
]


def read_registry(filename, bits):
    path = DATA_DIR / filename
    entries = {}

    with path.open("r", encoding="utf-8-sig", newline="") as file:
        reader = csv.DictReader(file)

        required = {"Assignment", "Organization Name"}

        if not required.issubset(reader.fieldnames or []):
            raise ValueError(f"Unexpected CSV format: {path}")

        for row in reader:
            assignment = row["Assignment"].strip()
            assignment = assignment.replace("-", "").replace(":", "")

            vendor = row["Organization Name"].strip()

            # An assignment is represented by 24, 28, or 36 bits.
            if len(assignment) != bits // 4:
                raise ValueError(
                    f"Invalid {bits}-bit assignment: {assignment}"
                )

            prefix = int(assignment, 16)

            if not vendor:
                vendor = "Private registration"

            if prefix in entries:
                if entries[prefix] != vendor:
                    # Multiple organizations are associated
                    # with this assignment. Avoid attributing
                    # it to an arbitrary manufacturer.
                    entries[prefix] = "Shared / multiple registrants"
            else:
                entries[prefix] = vendor

    if not entries:
        raise ValueError(f"No assignments found in {path}")

    return sorted(entries.items())


def generate():
    lines = [
        "// Generated from IEEE Registration Authority CSV files.",
        "// Do not edit manually.",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace netscope_vendor_db {",
        "",
        "struct Entry {",
        "    std::uint64_t prefix;",
        "    const char* vendor;",
        "};",
        "",
    ]

    for filename, variable, bits in REGISTRIES:
        entries = read_registry(filename, bits)

        lines.append(
            f"inline constexpr Entry {variable}[] = {{"
        )

        for prefix, vendor in entries:
            # JSON quoting also escapes quotes and backslashes
            # in C++-compatible string literals.
            name = json.dumps(vendor, ensure_ascii=True)

            lines.append(
                f"    {{0x{prefix:X}ULL, {name}}},"
            )

        lines.append("};")
        lines.append("")

        print(f"{filename}: {len(entries)} assignments")

    lines.append("} // namespace netscope_vendor_db")
    lines.append("")

    OUTPUT.write_text(
        "\n".join(lines),
        encoding="utf-8"
    )

    print(f"Generated {OUTPUT}")


if __name__ == "__main__":
    generate()
