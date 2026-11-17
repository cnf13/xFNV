#!/usr/bin/env python3
"""Copy a Bethesda ini and apply overrides: ini.py <src> <dst> 'Section:key=value' ...

Keys are matched case-insensitively within their section (the engine reads them that way); a missing key is added at
the end of its section, a missing section at the end of the file. Output keeps CRLF line ends like the game's files.
"""
import sys


def main() -> int:
    src, dst, *overrides = sys.argv[1:]
    lines = open(src, encoding="latin-1").read().splitlines()
    for spec in overrides:
        section, _, assignment = spec.partition(":")
        key, sep, value = assignment.partition("=")
        if not section or not sep:
            print(f"ini: bad override {spec!r} (want Section:key=value)", file=sys.stderr)
            return 2
        apply(lines, section, key, value)
    with open(dst, "w", encoding="latin-1", newline="\r\n") as f:
        f.write("\n".join(lines) + "\n")
    return 0


def apply(lines: list[str], section: str, key: str, value: str) -> None:
    current = None
    section_end = None
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            if current is not None and current.lower() == section.lower():
                section_end = i
            current = stripped[1:-1]
            continue
        if current is not None and current.lower() == section.lower():
            name = stripped.partition("=")[0].strip()
            if "=" in stripped and name.lower() == key.lower():
                lines[i] = f"{name}={value}"
                return
    if current is not None and current.lower() == section.lower():
        section_end = len(lines)
    if section_end is None:
        lines.extend(["", f"[{section}]", f"{key}={value}"])
        return
    insert = section_end
    while insert > 0 and not lines[insert - 1].strip():
        insert -= 1
    lines.insert(insert, f"{key}={value}")


if __name__ == "__main__":
    sys.exit(main())
