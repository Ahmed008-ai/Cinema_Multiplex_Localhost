#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p tests/test_run
rm -f tests/test_run/*.txt
gcc -std=c99 -Wall -Wextra -Wpedantic -DCINEMA_TEST -Ibackend tests/test_core.c backend/main.c -o tests/test_core
(cd tests/test_run && ../test_core)
gcc -std=c99 -Wall -Wextra -Wpedantic -o cinema backend/main.c
python3 tests/test_cli.py
