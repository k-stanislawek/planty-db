

import pytest
from pathlib import Path

import sys


def pytest_addoption(parser):
    parser.addoption("--plantydb", action="store", type=Path,
                     default=Path(sys.argv[0]).parent.parent / "src" / "debug.e")

@pytest.fixture
def plantydb(request):
    return request.config.getoption("--plantydb")