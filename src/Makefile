FLAGS_COMMON=-std=c++17 -Wall -Wextra -pedantic -Wshadow -Wfloat-equal -Winit-self
FLAGS_DEBUG=$(FLAGS_COMMON) -O0 -g -DDEBUG_PRINTS -coverage -ftrapv -fsanitize=address
FLAGS_RELEASE=$(FLAGS_COMMON) -O3 -DNDEBUG
FLAGS_PROFILING=$(FLAGS_RELEASE) -g

CC=g++-7

headers=$(wildcard *.h*)
deps = source.cc Makefile $(headers)

default: debug.e

debug.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_DEBUG}

profiling_clang.e: $(deps)
	clang++-5.0 $< -o $@ ${FLAGS_PROFILING}

profiling.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_PROFILING}

clang.e: $(deps)
	clang++-5.0 $< -o $@ ${FLAGS_DEBUG}

release.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_RELEASE}

all: $(default) verbose.e release.e

coverage: debug.e
	lcov -d . -c > lcov.info
	genhtml lcov.info -o html
	firefox -new-window html/index.html

clean:
	rm -rf html *.e *.gcno *.gcda lcov.info a.out

test: debug.e
	./test.sh

