#!/usr/bin/env python3
#
# @file density.py
# @date 2026-10-02
# @license{LicenseRef-AWH-1.0}
# @author Yuriy Lobarev
# @brief Изолированная проверка патча Density с санитайзерами и обратной мутацией
# @copyright Copyright © 2026

"""Проверка трёх алгоритмов Density, смещений 0..7 и совместимости потока."""

import argparse
import concurrent.futures
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--compiler", default=os.environ.get("CC", "clang"))
    parser.add_argument("--sanitizers", default="address,undefined")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs должен быть положительным")
    root = args.root.resolve()
    output = Path(tempfile.mkdtemp(prefix="awh-density-", dir=args.output)).resolve()
    print(f"Стенд: {output}", flush=True)
    spec = importlib.util.spec_from_file_location("memory", root / "tests/codec/abc/memory.py")
    memory = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(memory)
    run, require = memory.run, memory.require
    result = {"passed": False}
    try:
        patch = output / "density.patch"
        shutil.copy2(root / "sh/patches/density/density.patch", patch)
        probe = output / "density.c"
        shutil.copy2(root / "tests/sys/compressor/density.c", probe)
        source = output / "source"
        shutil.copytree(root / "submodules/density/src", source / "src")
        # Оба состояния рабочего сабмодуля допустимы; неизвестный исходник — отказ.
        import subprocess
        applied = subprocess.run(["git", "apply", "--reverse", "--check", str(patch)],
                                 cwd=source, capture_output=True).returncode == 0
        if applied:
            run(["git", "apply", "--reverse", str(patch)], source, output / "reverse.log", 30)
        baseline = output / "baseline"
        shutil.copytree(source, baseline)
        run(["git", "apply", "--check", str(patch)], source, output / "patch-check.log", 30)
        run(["git", "apply", str(patch)], source, output / "patch.log", 30)
        (output / "source-sha256.json").write_text(json.dumps({
            str(p.relative_to(output)): hashlib.sha256(p.read_bytes()).hexdigest()
            for directory in (source, baseline) for p in directory.rglob("*") if p.is_file()
        }, indent=2))
        run([args.compiler, "--version"], output, output / "compiler.log", 30)
        os.environ["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=0"
        os.environ["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        flags = ["-std=c99", "-O1", "-g", "-fno-omit-frame-pointer",
                 "-fno-sanitize-recover=all", "-fsanitize=" + args.sanitizers]

        def build(directory, name, options):
            target = output / name
            target.mkdir()

            def compile_one(path):
                obj = target / (str(path.relative_to(directory)).replace("/", "_") + ".o")
                run([args.compiler] + options + ["-I" + str(directory / "src"),
                    "-c", str(path), "-o", str(obj)], target, obj.with_suffix(".log"), 180)
                return str(obj)

            with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
                objects = list(pool.map(compile_one, sorted((directory / "src").rglob("*.c"))))
            binary = target / "probe"
            run([args.compiler] + options + ["-I" + str(directory / "src"), str(probe)] +
                objects + ["-o", str(binary)], target, target / "link.log", 180)
            return binary

        legacy = build(baseline, "legacy", ["-std=c99", "-O2"])
        fixed = build(source, "fixed", flags)
        mutant = build(baseline, "mutant", flags)
        for binary in (legacy, fixed):
            text = run([str(binary), str(binary.parent / "wire.bin"), "0"],
                       binary.parent, binary.parent / "run.log", 120)
            require("RESULT runs=1008" in text, "Неполная матрица Density")
        require((legacy.parent / "wire.bin").read_bytes() == (fixed.parent / "wire.bin").read_bytes(),
                "Изменился байтовый формат Density")
        mutations = []
        for level, name in enumerate(("chameleon", "cheetah", "lion"), 1):
            log = mutant.parent / (name + ".log")
            detected = False
            try:
                run([str(mutant), str(mutant.parent / (name + ".bin")), str(level)],
                    mutant.parent, log, 120)
            except RuntimeError:
                text = log.read_text()
                detected = ("runtime error:" in text and "misaligned address" in text and
                            name + "_encode.c" in text)
            require(detected, "Мутация не обнаружена: " + name)
            mutations.append(name)
        result = {"passed": True, "roundtrips": 1008, "wire_identical": True,
                  "mutations_detected": mutations, "sanitizers": args.sanitizers,
                  "wire_sha256": hashlib.sha256((fixed.parent / "wire.bin").read_bytes()).hexdigest()}
    except (OSError, RuntimeError, ValueError) as error:
        result["error"] = str(error)
    (output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2))
    print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
