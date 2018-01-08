import logging
import subprocess

def split(s, c):
    return s.split(c) if s else []

class CommandLogger:
    def __init__(self, cmd, info_lines):
        self._cmd = cmd
        self._info_lines_count = info_lines
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

def shcall(cmd, cwd=None, check=True, info_lines=0, log_rc=True):
    if cwd:
        cmd = "cd {} && {}".format(cwd, cmd)
    sub = subprocess.run(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    logger = CommandLogger(cmd, info_lines)
    logger.log_output(sub.stdout.decode(), "stdout")
    logger.log_output(sub.stderr.decode(), "stderr")
    if log_rc:
        logger.log_returncode(sub.returncode)
    if check:
        sub.check_returncode()
    logger.finalize()
    return sub
