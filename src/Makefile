FLAGS_COMMON=-std=c++17 -Wall -Wextra -pedantic -Wshadow -Wfloat-equal -Wswitch-default -Winit-self
FLAGS_DEBUG=$(FLAGS_COMMON) -O0 -g -DDEBUG_PRINTS -coverage -ftrapv -fsanitize=address
FLAGS_RELEASE=$(FLAGS_COMMON) -O3

CC=g++-6

headers=$(wildcard *.h*)
deps = source.cc Makefile $(headers)

default: debug.e

debug.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_DEBUG} -I${cflib}

verbose.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_DEBUG} -DVERBOSE_PRINTS -I${cflib}

release.e: $(deps)
	$(CC) $< -o $@ ${FLAGS_RELEASE} -I${cflib}

all: $(default) verbose.e release.e

coverage: debug.e
	lcov -d . -c > lcov.info
	genhtml lcov.info -o html
	firefox -new-window html/index.html

clean:
	rm -rf html *.e *.gcno *.gcda lcov.info

