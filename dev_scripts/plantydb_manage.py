#!/usr/bin/env python3

import argparse
import logging
from pathlib import Path

from plantydb.commands import run_perf, run_cg, run_tests, build, new
from plantydb.common import configure_logging, call
from plantydb.generate import generate_fullscan_case
from plantydb.perf_commands import gentest
from plantydb.shops import git_tag


if __name__ == "__main__":
    # noinspection PyTypeChecker
    def _run():
        parser_master = argparse.ArgumentParser()
        subparsers = parser_master.add_subparsers(dest="subcommand")
        subparsers.required = True

        parser = subparsers.add_parser("new")
        parser.add_argument("path", type=Path)

        parser = subparsers.add_parser("genperf")
        parser.add_argument("--testname", type=Path, required=False)
        parser.add_argument("-l", "--lines", type=int, required=True, help="10**x will be used")
        parser.add_argument("-q", "--queries", type=int, required=True, help="10**x will be used")
        parser.add_argument("-t", "--times", type=int, required=True, help="x will be used")
        parser.add_argument("-k", "--key-len", type=int, default=0,
                            help="length of the key; 0 or 1")

        parser = subparsers.add_parser("save")
        parser.add_argument("tagname", help="name for a save (git tag)")

        parser = subparsers.add_parser("build")
        parser.add_argument("target", default="release.e", help="what target to make")

        parser = subparsers.add_parser("run_perf")
        parser.add_argument("testname", type=Path)
        parser.add_argument("-n", "--ntimes", type=int, default=1,
                            help="number of times test will be run")

        parser = subparsers.add_parser("run_cg")
        parser.add_argument("testname", type=Path)
        parser.add_argument("-a", "--cg_args", default="", help="additional args to callgrind")

        parser = subparsers.add_parser("run_tests")
        parser.add_argument("testnames", nargs="*", type=Path)
        parser.add_argument("-a", "--apply", action="store_true")

        parser = subparsers.add_parser("gen_fullscan")
        parser.add_argument("--testname", type=Path)
        parser.add_argument("-c", "--n-columns", type=int, required=True)
        parser.add_argument("-v", "--max-value", type=int, required=True)
        parser.add_argument("-k", "--key-len", type=int, required=True)

        args = parser_master.parse_args()
        if args.subcommand == "new":
            new(args.path)
            exit(0)
        configure_logging(logging.getLogger(""))

        if args.subcommand == "genperf":
            call(gentest, None, args)
        elif args.subcommand == "save":
            call(git_tag, None, args)
        elif args.subcommand == "build":
            call(build, None, args)
        elif args.subcommand == "run_perf":
            call(run_perf, None, args)
        elif args.subcommand == "run_cg":
            call(run_cg, None, args)
        elif args.subcommand == "run_tests":
            call(run_tests, None, args)
        elif args.subcommand == "gen_fullscan":
            call(generate_fullscan_case, None, args)
        else:
            logging.error("unknown command")
    _run()
