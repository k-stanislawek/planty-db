from pathlib import Path
from plantydb.shcall import shcall


def sh_copy(src, tgt):
    shcall("cp {} {}".format(src, tgt))


def git_record(msg="bump", cwd=Path(".")):
    assert (cwd / ".git").is_dir()
    shcall("git add .", cwd=cwd)
    shcall("git commit -a -m \"{}\" -q".format(msg), cwd=cwd)


def git_tag(tagname, cwd=Path(".")):
    assert (cwd / ".git").is_dir()
    shcall("git tag {}".format(tagname))


def git_get_commit(cwd=Path(".")):
    assert (cwd / ".git").is_dir()
    return shcall("git rev-parse --short HEAD", cwd=cwd).stdout.decode().rstrip()


def sh_make(make_target_name, source_dir, final_target_path, compiler_output_log: Path):
    completed = shcall("make {} 2>&1 | tee {}".format(make_target_name,
                                                      compiler_output_log.absolute()),
                       check=False, cwd=source_dir, info_lines=10)
    if completed.returncode == 0:
        sh_copy(source_dir / make_target_name, final_target_path)


def sh_grep_pref(sign, inp, outp, append=False):
    shcall("grep ^{0}: {1} {3} {2}".format(sign, inp, outp, ">>" if append else ">"),
           check=False, log_rc=False, info_lines=0)


def sh_diff(reference_file, user_file, outp, append=False):
    return shcall("diff {0} {1} | tee{3} {2}"
                  .format(reference_file, user_file, outp, " -a" if append else ""),
                  check=False, log_rc=False, info_lines=10) \
               .returncode == 0
