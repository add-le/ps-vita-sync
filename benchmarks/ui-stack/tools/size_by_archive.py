#!/usr/bin/env python3
"""Sum code+data bytes per archive (or object) from GNU ld map files.

usage: size_by_archive.py build-vita/bench_empty.map build-vita/bench_a_rmlui.map ...
Prints one column per map: bytes of .text/.rodata/.data (in the binary) and .bss (RAM only).
"""
import collections
import os
import re
import sys

LINE = re.compile(r"^\s*(\.[\w.]+)?\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$")


def group(path):
    m = re.match(r"(?:.*/)?lib([^/]+)\.a\(.*\)$", path)
    if m:
        return "lib" + m.group(1)
    if path.endswith(".o") or path.endswith(".obj"):
        parts = path.split("/")
        return "app:" + (parts[-3] if "CMakeFiles" in parts[-3:] else parts[-2])
    return os.path.basename(path)


def parse(path):
    sizes = collections.defaultdict(lambda: [0, 0])
    in_map = False
    pending = None
    for raw in open(path, errors="replace"):
        line = raw.rstrip("\n")
        if line.startswith("Linker script and memory map"):
            in_map = True
            continue
        if not in_map:
            continue
        # Long section names wrap: the name is alone on one line, numbers on the next.
        if re.match(r"^ \.[\w.]+$", line):
            pending = line.strip()
            continue
        m = LINE.match(line)
        if not m:
            pending = None
            continue
        section = m.group(1) or pending
        pending = None
        if not section:
            continue
        size = int(m.group(3), 16)
        kind = 1 if section.startswith((".bss", ".sbss")) or "COMMON" in section else 0
        if section.startswith((".text", ".rodata", ".data", ".bss", ".sbss", ".init_array", ".ARM")):
            sizes[group(m.group(4))][kind] += size
    return sizes


def main():
    maps = sys.argv[1:]
    tables = [parse(p) for p in maps]
    keys = sorted(set().union(*tables), key=lambda k: -max(t[k][0] for t in tables))
    names = [os.path.basename(p).replace(".map", "") for p in maps]
    print("%-28s" % "archive" + "".join("%18s" % n for n in names))
    for k in keys:
        if max(t[k][0] for t in tables) < 4096:
            continue
        print("%-28s" % k[:28] + "".join("%18d" % t[k][0] for t in tables))
    print("%-28s" % "TOTAL (binary)" + "".join("%18d" % sum(v[0] for v in t.values()) for t in tables))
    print("%-28s" % "TOTAL .bss (RAM only)" + "".join("%18d" % sum(v[1] for v in t.values()) for t in tables))


main()