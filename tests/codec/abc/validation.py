#!/usr/bin/env python3
#
# @file validation.py
# @date 2026-10-02
# @license{LicenseRef-AWH-1.0}
# @author Yuriy Lobarev
# @brief Свежая сборка полного набора ABC и матрицы повреждённых кадров
# @copyright Copyright © 2026

"""Проверки ABC в отдельном снимке: штатная сборка, ASan и UBSan."""

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import tempfile
import xml.etree.ElementTree as ET

from memory import MIB, SOURCES, require, run


SCENARIOS = (["valid"] + [f"cut-{index}" for index in range(35)] +
             ["length-max", "origin-max", "origin-zero", "origin-small",
              "checksum", "stream-zero", "stream-half"])


def gtest_location(explicit):
    roots = [explicit] if explicit else [Path("/opt/homebrew"), Path("/usr"), Path("/usr/local")]
    for root in roots:
        if not (root / "include/gtest/gtest.h").is_file():
            continue
        for name in ("lib", "lib64", "lib/x86_64-linux-gnu", "lib/aarch64-linux-gnu"):
            libs = [root / name / (library + ".a") for library in ("libgtest", "libgmock")]
            if all(path.is_file() for path in libs):
                return root / "include", libs
    raise RuntimeError("Не найдены заголовки GoogleTest и libgtest.a/libgmock.a; задайте --gtest-root")


def snapshot(root, dependencies, output, gtest):
    target = output / "snapshot"
    for name in ("src", "include", "tests/codec/abc"):
        shutil.copytree(root / name, target / name)
    for name in ("tests/main.cpp", "tests/main.hpp", "tests/codec/silent.hpp", "tests/codec/temporary.hpp"):
        shutil.copy2(root / name, target / name)
    shutil.copytree(dependencies / "include", target / "third_party/include")
    (target / "third_party/lib").mkdir()
    shutil.copy2(dependencies / "lib/libdependence.a", target / "third_party/lib/libdependence.a")
    includes, libs = gtest_location(gtest)
    for name in ("gtest", "gmock"):
        shutil.copytree(includes / name, target / "gtest/include" / name)
    (target / "gtest/lib").mkdir()
    for library in libs:
        shutil.copy2(library, target / "gtest/lib" / library.name)
    manifest = {str(path.relative_to(target)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in target.rglob("*") if path.is_file()}
    (output / "snapshot-sha256.json").write_text(json.dumps(manifest, indent=2))
    return target


def validate(source, output, mode, compiler, jobs, timeout):
    build = output / mode
    build.mkdir()
    objects = build / "objects"
    objects.mkdir()
    system = platform.system()
    flags = ["-std=c++17", "-pthread", "-O2" if mode == "normal" else "-O1",
             "-I" + str(source / "include"), "-I" + str(source / "third_party/include"),
             "-I" + str(source / "gtest/include")]
    for name in ("lz4", "bz2", "zstd", "lzma", "zlib", "brotli", "snappy", "lizard", "density"):
        flags.append("-I" + str(source / "third_party/include" / name))
    if mode != "normal":
        flags += ["-g", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all",
                  "-fsanitize=" + mode, "-DAWH_ALLOC_DISABLED"]
    common = sorted(set([source / name for name in SOURCES] +
                        list((source / "src/codec/abc").glob("*.cpp")) +
                        list((source / "src/alloc").glob("*.cpp")) +
                        [source / "src/codec/numeric.cpp", source / "src/alloc/capture" /
                         ("mach.cpp" if system == "Darwin" else "elf.cpp")]))
    # Метаданные AppleDouble, перенесённые архивом macOS, исходниками не являются.
    common = [path for path in common if not path.name.startswith(".")]
    tests = [source / "tests/main.cpp"] + sorted(
        path for path in (source / "tests/codec/abc").glob("*.cpp")
        if not path.name.startswith(".") and path.name not in ("memory.cpp", "damage.cpp"))

    def compile_source(path):
        obj = objects / (str(path.relative_to(source)).replace("/", "_") + ".o")
        language = (["-x", "objective-c++", "-fobjc-arc"]
                    if system == "Darwin" and path == source / "src/sys/fs.cpp" else [])
        run([compiler] + flags + language + ["-c", str(path), "-o", str(obj)],
            build, obj.with_suffix(".log"), timeout)
        print(f"{mode}: собрано {path.relative_to(source)}", flush=True)
        return str(obj)

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        compiled = list(pool.map(compile_source, common + tests + [source / "tests/codec/abc/damage.cpp"]))
    support = compiled[:len(common)]
    libraries = [str(source / "third_party/lib/libdependence.a")]
    system_libs = ["-framework", "Foundation"] if system == "Darwin" else ["-ldl"]
    binary = build / "abc-tests"
    run([compiler] + flags + support + compiled[len(common):-1] + libraries +
        [str(source / "gtest/lib/libgmock.a"), str(source / "gtest/lib/libgtest.a")] +
        system_libs + ["-o", str(binary)], build, build / "tests-link.log", timeout)
    run([compiler] + flags + support + [compiled[-1]] + libraries + system_libs +
        ["-o", str(build / "damage")], build, build / "damage-link.log", timeout)
    # Остановка на первом сообщении; ASan на macOS не предоставляет LeakSanitizer.
    os.environ["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=" + ("0" if system == "Darwin" else "1")
    os.environ["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    run([str(binary), "--gtest_output=xml:" + str(build / "tests.xml")],
        build, build / "tests.log", timeout)
    stats = ET.parse(build / "tests.xml").getroot().attrib
    require(int(stats["tests"]) >= 400 and int(stats["failures"]) == 0 and
            int(stats["errors"]) == 0 and int(stats["disabled"]) == 0,
            "Неполный либо неуспешный прогон набора ABC")
    skipped = ET.parse(build / "tests.xml").findall(".//skipped")
    require(not skipped, "Проверки ABC пропущены")
    rows = []
    for method in range(1, 12):
        for encrypted in (0, 1):
            for scenario in SCENARIOS:
                label = f"{method}-{encrypted}-{scenario}"
                text = run([str(build / "damage"), str(method), str(encrypted), scenario],
                           build, build / (label + ".log"), timeout)
                lines = [line for line in text.splitlines() if line.startswith("RESULT ")]
                require(len(lines) == 1, f"Нет единственного результата: {label}")
                row = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", lines[0])}
                require(set(row) == {"valid", "accepted", "error", "expected", "output", "offset", "rss", "delta"},
                        f"Неполный результат: {label}")
                require(row["valid"] == 1 and row["error"] == row["expected"], f"Неверный исход: {label}")
                for key in ("rss", "delta"):
                    row[key] *= 1 if system == "Darwin" else 1024
                require(0 < row["rss"] < (512 if mode == "normal" else 1024) * MIB and
                        0 <= row["delta"] <= row["rss"], f"Недопустимый RSS: {label}")
                row.update(method=method, encrypted=encrypted, scenario=scenario)
                rows.append(row)
                (build / "damage.json").write_text(json.dumps(rows, indent=2))
            print(f"{mode}: алгоритм {method}, шифрование {encrypted}: готово", flush=True)
    require(len(rows) == 11 * 2 * len(SCENARIOS), "Матрица повреждений не завершена")
    return {"passed": True, "unit_tests": stats, "damage_runs": len(rows),
            "maximum_rss": max(row["rss"] for row in rows)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument("--dependencies", type=Path, help="Каталог include и lib/libdependence.a")
    parser.add_argument("--gtest-root", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--modes", nargs="+", choices=("normal", "address", "undefined", "address,undefined"),
                        default=["normal", "address", "undefined"])
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=300)
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout < 1:
        parser.error("--jobs и --timeout должны быть положительными")
    output = Path(tempfile.mkdtemp(prefix="awh-abc-validation-", dir=args.output)).resolve()
    print(f"Стенд: {output}", flush=True)
    results = {}
    try:
        require(platform.system() in ("Darwin", "Linux"), "Поддерживаются macOS и Linux")
        compiler = os.environ.get("CXX", "c++")
        run([compiler, "--version"], output, output / "compiler.log", args.timeout)
        source = snapshot(args.root.resolve(), (args.dependencies or args.root / "third_party").resolve(),
                          output, args.gtest_root)
        for mode in dict.fromkeys(args.modes):
            results[mode] = validate(source, output, mode, compiler, args.jobs, args.timeout)
            (output / "results.json").write_text(json.dumps(results, indent=2))
        result = {"passed": True, "modes": results}
    except (OSError, RuntimeError, ET.ParseError, ValueError, KeyError) as error:
        result = {"passed": False, "error": str(error), "modes": results}
    (output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2))
    print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
