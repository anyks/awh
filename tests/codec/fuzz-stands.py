#!/usr/bin/env python3
"""Проверки раскладки fuzz без сети: исполняется настоящий удалённый shell-сценарий."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = Path(os.environ.get("AWH_FUZZ_STANDS_SCRIPT", ROOT / "tools/fuzz/stands.sh"))

# Подменяются только транспорт, упаковка и сборка. Команда SSH исполняется целиком
# в отдельном каталоге, поэтому проверяется сохранение исхода удалённой программы.
MOCK = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import shutil
import subprocess
import sys

name = Path(sys.argv[0]).name
args = sys.argv[1:]
case = os.environ['AWH_MOCK_CASE']
base = Path(os.environ['AWH_MOCK_ROOT'])
remote = base / 'remote-tmp'
remote.mkdir(exist_ok=True)
if name == 'scp':
    if case == 'transfer' or (case == 'partial' and 'second' in args[-1]):
        sys.exit(23)
    shutil.copyfile(args[-2], remote / Path(args[-2]).name)
elif name == 'ssh':
    if case == 'ssh':
        sys.exit(37)
    host = next(arg for arg in args if arg.startswith('forman@')).split('@')[1]
    env = os.environ.copy()
    home = base / host
    home.mkdir(exist_ok=True)
    env.update(HOME=str(home), AWH_MOCK_HOST=host)
    command = args[-1].replace('/tmp/', str(remote) + '/')
    sys.exit(subprocess.run(['/bin/sh', '-c', command], env=env).returncode)
elif name == 'gzip':
    pass
elif name == 'tar':
    if '-czf' in args:
        Path(args[args.index('-czf') + 1]).touch()
    else:
        # Вместо исходников создаётся минимальный сборщик.
        build = Path('tools/fuzz/build.sh')
        build.parent.mkdir(parents=True)
        build.write_text('''#!/bin/sh
if [ "$AWH_MOCK_CASE" = build ]; then exit 29; fi
mkdir -p "$3"
cp "$AWH_MOCK_ROOT/fuzzer" "$3/$1-fuzz"
chmod +x "$3/$1-fuzz"
''')
"""

FUZZER = r"""#!/bin/sh
if [ "$AWH_MOCK_CASE" = fuzz ]; then
    echo "simulated fuzz failure" >&2
    exit 31
fi
if [ "$AWH_MOCK_CASE" = missing ]; then exit 0; fi
REWRITES=17
MARKER=""
if [ "$AWH_MOCK_CASE" = mismatch ] && [ "$AWH_MOCK_HOST" = second ]; then REWRITES=18; fi
if [ "$AWH_MOCK_CASE" = marker ] && [ "$AWH_MOCK_HOST" = second ]; then MARKER=" [БЕЗ НАДЗИРАТЕЛЕЙ]"; fi
echo "cef fuzz: 30 records, $REWRITES rewrites$MARKER"
if [ "$AWH_MOCK_CASE" = duplicate ]; then echo "cef fuzz: 30 records, 17 rewrites"; fi
echo "cef fuzz: diagnostic one"
echo "cef fuzz: diagnostic two"
echo "cef fuzz: diagnostic three"
exit 0
"""


class FuzzStands(unittest.TestCase):
    def run_case(self, case, expected):
        with tempfile.TemporaryDirectory(prefix="awh-stands-test-") as directory:
            base = Path(directory)
            binary = base / "bin"
            binary.mkdir()
            for name in ("scp", "ssh", "gzip", "tar"):
                target = binary / name
                target.write_text(MOCK)
                target.chmod(0o755)
            (base / "fuzzer").write_text(FUZZER)
            env = os.environ.copy()
            env.update(
                PATH=str(binary) + os.pathsep + env["PATH"],
                TMPDIR=str(base),
                AWH_MOCK_ROOT=str(base),
                AWH_MOCK_CASE=case,
                AWH_STANDS="first|first\nsecond|second",
            )
            result = subprocess.run(
                ["/bin/sh", str(RUNNER), "cef", "30"],
                env=env, text=True, capture_output=True, timeout=30,
            )
            self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
            if expected == 0:
                self.assertIn("машин 2, без отказа 2", result.stdout)
                self.assertIn("cef fuzz: 30 records", result.stdout)
            if case == "fuzz":
                self.assertIn("исход: 31", result.stdout)
            if case == "build":
                self.assertIn("исход: 29", result.stdout)

    def test_trailing_diagnostics(self):
        self.run_case("success", 0)

    def test_sanitizer_marker(self):
        self.run_case("marker", 0)

    def test_counter_mismatch(self):
        self.run_case("mismatch", 1)

    def test_transfer_failure(self):
        self.run_case("transfer", 1)

    def test_partial_transfer_failure(self):
        self.run_case("partial", 1)

    def test_ssh_failure(self):
        self.run_case("ssh", 1)

    def test_build_failure(self):
        self.run_case("build", 1)

    def test_fuzz_failure(self):
        self.run_case("fuzz", 1)

    def test_missing_counters(self):
        self.run_case("missing", 1)

    def test_duplicate_counters(self):
        self.run_case("duplicate", 1)


if __name__ == "__main__":
    unittest.main()
