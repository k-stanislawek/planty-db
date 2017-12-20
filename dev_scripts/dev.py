#!/usr/bin/env python3

import sys
import os
import argparse
import logging
import subprocess
import shutil
from collections import defaultdict
from pathlib import Path

def shcall(cmd, cwd=None, check=True):
    if cwd:
        cmd = "cd {} && {}".format(cwd, cmd)
    sub = subprocess.run(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    if sub.returncode != 0 or sub.stdout or sub.stderr:
        logging.info(cmd)
        if sub.stdout:
            logging.info("    stdout: {}".format(sub.stdout.decode().rstrip()))
        if sub.stderr:
            logging.info("    stderr: {}".format(sub.stderr.decode().rstrip()))
        if sub.returncode != 0:
            logging.info("    returncode: {}".format(sub.returncode))
            if check:
                sub.check_returncode()
    return sub

def shcopy(src, tgt):
    shutil.copy(str(src), str(tgt))

def dev_scripts_root() -> Path:
    return Path(sys.argv[0]).parent

def git_record(msg="bump", cwd=None):
    shcall("git add .", cwd=cwd)
    shcall("git commit -a -m \"{}\" -q".format(msg), cwd=cwd)

def call(fun, argnames, args): # {{{
    s = fun.__name__ + "("
    l = []
    kw = {}
    for argname in argnames:
        kw[argname] = getattr(args, argname)
        l.append("{}={}".format(argname, kw[argname]))
    s += ",".join(l)
    s += ")"
    logging.info(s)
    fun(**kw)
    git_record(s)
# }}}

def gentest(lines, queries, times=1): # {{{
    csvname = "0.csv"
    inname = "0.in"
    lines = 10**int(lines)
    queries = 10**int(queries)
    times = int(times)
    with open(csvname, "w") as f:
        print("a; 0", file=f)
        for x in range(lines):
            print(x, file=f)
    with open(inname, "w") as f:
        for t in range(times):
            for i in range(0, lines, lines // queries):
                print("select a where a=%d" % i, file=f)
# }}}
def accumulate_performance_data(inp_perf_file, outp_summary_file): # {{{
    vals = defaultdict(list)
    with open(inp_perf_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            row = line.split(" ")
            assert row[0] == "perf:"
            vals[int(row[1])].append(int(row[2]))
    with open(outp_summary_file, "w") as f:
        if vals:
            k = sorted(vals)
            print("first", min(vals[k[0]]), file=f)
            print("min", min(map(min, vals.values())), file=f)
# }}}

def read_map(p):
    m = {}
    with open(p, "r") as f:
        for line in f:
            t = line.split(" ")
            m[t[0]] = int(t[1])
    return m

def diff_perf_mins(fnew, fprev):
    new = read_map(fnew)
    prev = read_map(fprev)
    for k in sorted(set(new).union(prev)):
        print("{}: {} -> {}".format(k, prev.get(k), new.get(k)))

def save(savename):
    shcall("git tag {}".format(savename))

def copytest(test_dir):
    test_dir = Path(test_dir)
    shcopy(test_dir / "0.in", ".")
    shcopy(test_dir / "0.csv", ".")

def build(target):
    sdir = dev_scripts_root().parent / "src"
    completed = shcall("make {}".format(target), check=False, cwd=sdir)
    if completed.returncode == 0:
        shcopy(sdir / target, "executable.e")    

executable_cmd = "./executable.e {testname}.csv < {testname}.in 1> {testname}.out 2> {testname}.err"
full_cg_args = "valgrind --dump-instr=yes --collect-jumps=yes --tool=callgrind {cg_args} --callgrind-out-file={testname}.cg.log"

def run_perf(testname, ntimes):
    open(testname + ".perf", "w").close()
    for i in range(1, ntimes + 1):
        print("run number {}/{}".format(i, ntimes))
        shcall(executable_cmd.format(testname=testname))
        shcall("grep ^perf: {testname}.err >> {testname}.perf".format(testname=testname))
    perfmin = "{testname}.perf.min".format(testname=testname)
    if os.path.isfile(perfmin):
        shcopy("{testname}.perf.min".format(testname=testname),
                "{testname}.perf.min.prev".format(testname=testname))
    accumulate_performance_data("{testname}.perf".format(testname=testname),
            "{testname}.perf.min".format(testname=testname))
    prev = "{testname}.perf.min.prev".format(testname=testname)
    if os.path.isfile(prev):
        diff_perf_mins("{testname}.perf.min".format(testname=testname), prev)

def run_cg(testname, cg_args):
    shcall(" ".join([full_cg_args, executable_cmd]).format(testname=testname, cg_args=cg_args))

if __name__ == "__main__":
    parser_master = argparse.ArgumentParser()
    subparsers = parser_master.add_subparsers(dest="subcommand")
    subparsers.required = True
    def _add(name):
        p = subparsers.add_parser(name)
#        p.set_defaults(subcommand=name)
        return p
    
    parser = _add("new")
    parser.add_argument("-p", "--path", default=".")
    parser = _add("copytest")
    parser.add_argument("-t", "--test-dir", required=True, help="directory with 0.in, 0.csv")
    parser = _add("gentest")
    parser.add_argument("-l", "--lines", type=int, required=True, help="10**x will be used")
    parser.add_argument("-q", "--queries", type=int, required=True, help="10**x will be used")
    parser.add_argument("-t", "--times", type=int, required=True, help="x will be used")
    parser = _add("save")
    parser.add_argument("savename", help="name for a save (git tag)")
    parser = _add("build")
    parser.add_argument("-t", "--target", default="release.e", help="what target to make")
    parser = _add("run_perf")
    parser.add_argument("-t", "--testname", default="0", help="default 0, pass to change test variant")
    parser.add_argument("-n", "--ntimes", type=int, default=1, help="number of times test will be run")
    parser = _add("run_cg")
    parser.add_argument("-t", "--testname", default="0", help="default 0, pass to change test variant")
    parser.add_argument("-a", "--cg_args", default="", help="additional args to callgrind")
    args = parser_master.parse_args()
    logging.basicConfig(filename="dev.log", filemode="a", datefmt="%Y-%m-%d %H:%M:%S",
                        format="%(asctime)s %(message)s", level=logging.INFO)
    if args.subcommand == "new":
        if not os.path.isdir(args.path):
            os.mkdir(args.path)
        shcopy(dev_scripts_root() / "workspace_gitignore", Path(args.path) / ".gitignore")
        shcall("git init", cwd=args.path)
        git_record("init", cwd=args.path)
    elif args.subcommand == "gentest":
        call(gentest, ["lines", "queries", "times"], args)
    elif args.subcommand == "save":
        call(save, ["savename"], args)
    elif args.subcommand == "copytest":
        call(copytest, ["test_dir"], args)
    elif args.subcommand == "build":
        call(build, ["target"], args)
    elif args.subcommand == "run_perf":
        call(run_perf, ["testname", "ntimes"], args)
    elif args.subcommand == "run_cg":
        call(run_cg, ["testname", "cg_args"], args)
    else:
        logging.error("unknown command")

