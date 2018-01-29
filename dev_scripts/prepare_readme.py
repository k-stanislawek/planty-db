#!/usr/bin/env python3.5
import argparse
from pathlib import Path

import sys
from string import Template

from ruamel.yaml import YAML

header = ["plantydb", "sqlmem", "sqldisk", "desc"]
def prepare_presets_table(conf):
    lines = []
    for name in sorted(conf["presets"]):
        info = conf["presets"][name]["info"]
        lines.append(" | ".join((name, *(str(info[c]) for c in header))))
    return "\n".join(lines)

if __name__ == "__main__":
    def _run():
        with (Path(sys.argv[0]).parent / "perf_presets.yaml").open("r") as f:
            presets_config = YAML(typ="safe").load(f.read())

        inp = sys.stdin.read()
        outp = Template(inp).substitute(benchmarks=prepare_presets_table(presets_config))
        sys.stdout.write(outp)
    _run()