import inspect
import logging
from pathlib import Path
from typing import List

from plantydb.shops import git_get_commit, git_record
import sys
import os


def dev_scripts_root() -> Path:
    return Path(sys.argv[0]).parent

def plantydb_root() -> Path:
    return dev_scripts_root().parent

def auto_tests() -> List[Path]:
    with (dev_scripts_root() / "auto_tests").open("r") as f:
        return [Path(p.rstrip()) for p in f]

def get_argument_names_in_namespace(namespace, func):
    for k in inspect.signature(func).parameters.keys():
        if k in namespace.__dict__:
            yield k

def call(fun, in_argnames, args):
    argnames = list(get_argument_names_in_namespace(args, fun))
    if in_argnames is not None:
        assert argnames == in_argnames, (argnames, in_argnames)
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

def configure_logging(logger):
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
