#!/usr/bin/env python3

import sys
import os
import argparse
import logging
import subprocess
import shutil
from collections import defaultdict
from pathlib import Path


# shcall, shcopy {{{
INFO_LINES_COUNT = 10

def split(s, c):
    return s.split(c) if s else []

class CommandLogger:
    def __init__(self, cmd, info_lines_count):
        self._cmd = cmd
        self._info_lines_count = info_lines_count
        self._command_logged = False

    def log_output(self, outp, sign):
        for x, line in enumerate(split(outp.rstrip(), "\n")):
            method = logging.info if x < self._info_lines_count else logging.debug
            if not self._command_logged:
                self._command_logged = True
                self._log_cmd(method)
            method("  {}: {}".format(sign, line))

    def log_returncode(self, returncode):
        if returncode != 0:
            self.log_output(str(returncode), "returncode")

    def _log_cmd(self, method):
        method("{}: {}".format("command", self._cmd))

    def finalize(self):
        if not self._command_logged:
            self._log_cmd(logging.debug)

def shcall(cmd, cwd=None, check=True, quiet=False, log_returncode=True):
    if cwd:
        cmd = "cd {} && {}".format(cwd, cmd)
    sub = subprocess.run(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    logger = CommandLogger(cmd, INFO_LINES_COUNT if not quiet else 0)
    logger.log_output(sub.stdout.decode(), "stdout")
    logger.log_output(sub.stderr.decode(), "stderr")
    if log_returncode:
        logger.log_returncode(sub.returncode)
    if check:
        sub.check_returncode()
    logger.finalize()
    return sub

def shcopy(src, tgt):
    shcall("cp {} {}".format(src, tgt))
# }}}
# {{{ paths
def dev_scripts_root() -> Path:
    return Path(sys.argv[0]).parent

def plantydb_root() -> Path:
    return dev_scripts_root().parent
# }}}
# {{{ git
def git_record(msg="bump", cwd=None):
    shcall("git add .", cwd=cwd)
    shcall("git commit -a -m \"{}\" -q".format(msg), cwd=cwd)

def git_tag(savename):
    shcall("git tag {}".format(savename))

def git_get_commit(repo):
    return shcall("git rev-parse --short HEAD", cwd=repo, quiet=True).stdout.decode().rstrip()
# }}}
def call(fun, argnames, args): # {{{
    s = fun.__name__ + "("
    l = []
    kw = {}
    for argname in argnames:
        kw[argname] = getattr(args, argname)
        l.append("{}={}".format(argname, kw[argname]))
    s += ",".join(l)
    s += ")"
    logging.info("call: %s, plantydb commit: %s", s, git_get_commit(plantydb_root()))
    fun(**kw)
    git_record(s)
# }}}
def gentest(testname, lines, queries, times=1, key_len=0): # {{{
    if not testname:
        testname = "gentest.{}.{}.{}.{}".format(lines, queries, times, key_len)
        if not os.path.isdir(testname):
            os.makedirs(testname)
    csvname = "%s/csv" % testname
    inname = "%s/in" % testname
    logging.info("  testname: %s", testname)
    lines = 10**int(lines)
    queries = 10**int(queries)
    times = int(times)
    with open(csvname, "w") as f:
        print("a; %d" % key_len, file=f)
        for x in range(lines):
            print(x, file=f)
    with open(inname, "w") as f:
        for _ in range(times):
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
# {{{ perf
def read_perf_data(p):
    m = {}
    with open(p, "r") as f:
        for line in f:
            k, v = line.split(" ")
            m[k] = int(v)
    return m

def diff_perf_mins(fnew, fprev):
    new = read_perf_data(fnew)
    prev = read_perf_data(fprev)
    for k in sorted(set(new).union(prev)):
        print("{}: {} -> {}".format(k, prev.get(k), new.get(k)))
# }}}

def copy_test(test_dir):
    test_dir = Path(test_dir)
    shcopy(test_dir / "0.in", ".")
    shcopy(test_dir / "0.csv", ".")

def read_outp(p):
    with open(p, "r") as f:
        return list(f)

def build(target):
    sdir = plantydb_root() / "src"
    completed = shcall("make {}".format(target), check=False, cwd=sdir)
    if completed.returncode == 0:
        shcopy(sdir / target, "executable.e")

def sh_grep_pref(sign, inp, outp, append=False):
    shcall("grep ^{0}: {1} {3} {2}".format(sign, inp, outp, ">>" if append else ">"),
           check=False, log_returncode=False)

def sh_diff(reference_file, user_file, outp, append=False):
    return shcall("diff {0} {1} | tee{3} {2}"
                  .format(reference_file, user_file, outp, " -a" if append else ""),
                  check=False, log_returncode=False).returncode == 0

executable_cmd = "./executable.e {testname}/csv < {testname}/in 1> {testname}/user.out " +\
        "2> {testname}/user.err"
full_cg_args = "valgrind --dump-instr=yes --collect-jumps=yes --tool=callgrind {cg_args} " +\
    "--callgrind-out-file={testname}/callgrind.log"

def run_perf(testname, ntimes):
    open(testname + "/user.perf", "w").close()
    for i in range(1, ntimes + 1):
        print("run number {}/{}".format(i, ntimes))
        shcall(executable_cmd.format(testname=testname))
        sh_grep_pref("perf", "%s/user.err" % testname, "%s/user.perf" % testname)
    perfmin = "{testname}/user.perf.min".format(testname=testname)
    if os.path.isfile(perfmin):
        shcopy("{testname}/user.perf.min".format(testname=testname),
               "{testname}/user.perf.min.prev".format(testname=testname))
    accumulate_performance_data("{testname}/user.perf".format(testname=testname),
                                "{testname}/user.perf.min".format(testname=testname))
    prev = "{testname}/user.perf.min.prev".format(testname=testname)
    if os.path.isfile(prev):
        diff_perf_mins("{testname}/user.perf.min".format(testname=testname), prev)

def run_cg(testname, cg_args):
    shcall(" ".join([full_cg_args, executable_cmd]).format(testname=testname, cg_args=cg_args))

families = ["info", "plan", "debug", "perf", "query"]

def run_test(testname, apply=False):
    shcall(executable_cmd.format(testname=testname))
    for family in families:
        sh_grep_pref(family, inp="%s/user.err" % testname, outp="%s/user.%s" % (testname, family))
    with open("{testname}/user.err".format(testname=testname), "r") as f:
        with open("{testname}/user.anon".format(testname=testname), "w") as o:
            for l in f:
                if any(l.startswith(f + ":") for f in families):
                    continue
                if not l.strip():
                    continue
                print(l.rstrip(), file=o)
    checked = ["plan", "out"]
    for f in families + ["anon", "out"]:
        userpath = "{testname}/user.{f}".format(testname=testname, f=f)
        testpath = "{testname}/{f}".format(testname=testname, f=f)
        if not os.path.exists(testpath):
            open(testpath, "w").close()
        diffpath = "{testname}/diff.{f}".format(testname=testname, f=f)
        if f in checked:
            sh_diff(testpath, userpath, diffpath)
        if apply:
            shcopy(userpath, testpath)

# todo:
# w plantydb:
# - dodac log_input() ktore wypisuje na stdout i na stderr z prefiksem "query:"
# - dodac prefiks debug: do dprintln

def run_tests(testnames, apply=False):
    for t in testnames:
        if not t.is_dir():
            logging.error("No dir %s", t)
            continue
        run_test(t, apply=apply)

def _configure_logging(logger):
    file_format = logging.Formatter("%(asctime)s %(message)s", datefmt="%Y-%m-%d %H:%M:%S")
    terminal_format = logging.Formatter("%(message)s")
    handler_devlog = logging.FileHandler("diary.log")
    handler_devlog.setLevel(logging.INFO)
    handler_devlog.setFormatter(file_format)
    handler_debug = logging.FileHandler("diagnostic.log")
    handler_debug.setLevel(logging.DEBUG)
    handler_debug.setFormatter(file_format)
    handler_terminal = logging.StreamHandler(sys.stdout)
    handler_terminal.setLevel(logging.INFO)
    handler_terminal.setFormatter(terminal_format)
    logger.addHandler(handler_devlog)
    logger.addHandler(handler_debug)
    logger.addHandler(handler_terminal)
    logger.setLevel(logging.DEBUG)
    logger.debug("pwd: %s" % os.getcwd())

def _new(path: Path):
    if not path.is_dir():
        os.mkdir(str(path))
    shutil.copy(str(dev_scripts_root() / "workspace_gitignore"), str(path / ".gitignore"))
    shcall("git init", quiet=True, cwd=path)
    git_record("init", cwd=path)

if __name__ == "__main__":
    def _run():
        parser_master = argparse.ArgumentParser()
        subparsers = parser_master.add_subparsers(dest="subcommand")
        subparsers.required = True
        parser = subparsers.add_parser("new")
        parser.add_argument("path", type=Path)
        parser = subparsers.add_parser("copytest")
        parser.add_argument("-t", "--test-dir", required=True, help="directory with 0.in, 0.csv")
        parser = subparsers.add_parser("gentest")
        parser.add_argument("--testname", required=False)
        parser.add_argument("-l", "--lines", type=int, required=True, help="10**x will be used")
        parser.add_argument("-q", "--queries", type=int, required=True, help="10**x will be used")
        parser.add_argument("-t", "--times", type=int, required=True, help="x will be used")
        parser.add_argument("-k", "--key-len", type=int, default=0,
                            help="length of the key; 0 or 1")
        parser = subparsers.add_parser("save")
        parser.add_argument("savename", help="name for a save (git tag)")
        parser = subparsers.add_parser("build")
        parser.add_argument("-t", "--target", default="release.e", help="what target to make")
        parser = subparsers.add_parser("run_perf")
        parser.add_argument("-t", "--testname", default="0",
                            help="default 0, pass to change test variant")
        parser.add_argument("-n", "--ntimes", type=int, default=1,
                            help="number of times test will be run")
        parser = subparsers.add_parser("run_tests")
        parser.add_argument("testnames", default=[Path("0")], nargs="*", type=Path,
                            help="default 0, pass to change test variant")
        parser.add_argument("-a", "--apply", action="store_true")
        parser = subparsers.add_parser("run_cg")
        parser.add_argument("-t", "--testname", default="0",
                            help="default 0, pass to change test variant")
        parser.add_argument("-a", "--cg_args", default="", help="additional args to callgrind")
        args = parser_master.parse_args()
        if args.subcommand == "new":
            _new(args.path)
            exit(0)
        _configure_logging(logging.getLogger(""))
        if args.subcommand == "gentest":
            call(gentest, ["lines", "queries", "times", "testname", "key_len"], args)
        elif args.subcommand == "save":
            call(git_tag, ["savename"], args)
        elif args.subcommand == "copytest":
            call(copy_test, ["test_dir"], args)
        elif args.subcommand == "build":
            call(build, ["target"], args)
        elif args.subcommand == "run_perf":
            call(run_perf, ["testname", "ntimes"], args)
        elif args.subcommand == "run_cg":
            call(run_cg, ["testname", "cg_args"], args)
        elif args.subcommand == "run_tests":
            call(run_tests, ["testnames", "apply"], args)
        else:
            logging.error("unknown command")
    _run()
