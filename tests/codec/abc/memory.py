#!/usr/bin/env python3
#
# @file memory.py
# @date 2026-10-02
# @license{LicenseRef-AWH-1.0}
# @author Yuriy Lobarev
# @brief Регрессионный щуп предела распаковки ABC с самопроверкой мутациями
# @copyright Copyright © 2026

"""Свежая сборка в снимке, отдельный процесс на замер, отказ при незрячей мутации."""

import argparse
import concurrent.futures
import difflib
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import signal
import statistics
import subprocess
import tempfile


MIB = 1024 * 1024
LIMIT = 8 * MIB
METHODS = {name: index for index, name in enumerate((
    "LZ4", "LZMA", "ZSTD", "GZIP", "ZLIB", "BZIP2", "BROTLI",
    "LIZARD", "SNAPPY", "DEFLATE", "DENSITY"), 1)}
SOURCES = """
src/sys/log.cpp src/sys/chrono.cpp src/sys/fmk.cpp src/net/nwt.cpp
src/sys/fs.cpp src/sys/os.cpp src/num/lexical/table.cpp src/num/bignum.cpp
src/encoding/unicode/normalize.cpp src/encoding/unicode/table.cpp
src/encoding/unicode/unicode.cpp src/encoding/unicode/utf8.cpp
src/encoding/charset/charset.cpp src/encoding/charset/table.cpp
src/compressor/block.cpp src/compressor/stream.cpp src/compressor/types.cpp
src/cryptography/crypto.cpp src/cryptography/hash.cpp src/cryptography/vault.cpp
src/codec/abc/common.cpp src/codec/abc/encoding.cpp
""".split()


def require(condition, message):
    # Проверки действуют и при python3 -O.
    if not condition:
        raise RuntimeError(message)


def run(command, directory, log, timeout):
    """Таймаут завершает всю группу, включая потомков драйвера компилятора."""
    with log.open("w") as stream:
        stream.write(json.dumps([str(arg) for arg in command]) + "\n")
        stream.flush()
        process = subprocess.Popen(command, cwd=directory, stdout=stream,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            status = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            raise RuntimeError(f"Таймаут: {log}") from None
    require(status == 0, f"Код завершения {status}: {log}")
    return log.read_text()


def measure(executable, frame, declared, method, directory, log, timeout):
    output = run([str(executable), "read", str(frame), str(declared), str(method)],
                 directory, log, timeout)
    lines = [line for line in output.splitlines() if line.startswith("RESULT ")]
    require(len(lines) == 1, f"Нет единственного результата: {log}")
    result = dict((key, int(value)) for key, value in
                  re.findall(r"(\w+)=(-?\d+)", lines[0]))
    fields = {"original", "declared", "packed", "accepted", "error", "offset",
              "output", "valid", "invalid_chunk", "compression_failed",
              "rss_before", "rss_after", "rss_delta"}
    require(set(result) == fields, f"Неполный результат: {log}")
    # ru_maxrss: байты на macOS, КиБ на Linux. Другие системы не угадываем.
    for key in ("rss_before", "rss_after", "rss_delta"):
        result[key] *= (1 if platform.system() == "Darwin" else 1024)
    require(result["rss_before"] > 0 and result["rss_delta"] >= 0,
            f"Недостоверный счётчик памяти: {log}")
    require(result["rss_after"] - result["rss_before"] == result["rss_delta"],
            f"Несогласованные счётчики памяти: {log}")
    return result


def check(result, size, variant, declared):
    """Проверяем содержимое, причину отказа и расход при малом origin."""
    require(result["original"] == size and result["declared"] == declared,
            "Кадр не соответствует заданному случаю")
    require(0 < result["packed"] < size, "Нет сжатого контрольного входа")
    if declared == size:
        require(result["accepted"] == 1 and result["valid"] == 1 and
                result["error"] == 0 and result["output"] == size and
                result["offset"] == result["packed"],
                "Не прошёл положительный контроль содержимого")
        return
    require(result["accepted"] == 0 and result["output"] == 0 and
            result["offset"] == 0, "Испорченный кадр принят либо изменил выход")
    limited = declared in (64, size - 1) and variant != "no-limit"
    require(result["compression_failed"] == int(limited) and
            result["invalid_chunk"] == int(not limited), "Неверная причина отказа")
    if variant == "baseline" and declared in (0, 64):
        require(result["rss_delta"] < LIMIT,
                f"Регрессия: {variant}, origin={declared}, прирост RSS >= 8 МиБ")


def check_mutations(results, methods, sizes):
    differences = []
    for method in methods:
        for size in sizes:
            for variant, declared in (("no-zero", 0), ("no-limit", 64)):
                samples = {}
                for name in ("baseline", variant):
                    samples[name] = [r["rss_delta"] for r in results
                                     if r["method"] == method and r["original"] == size and
                                     r["variant"] == name and r["declared"] == declared]
                    require(len(samples[name]) == 3, "Неполное доказательство расхода памяти")
                difference = statistics.median(samples[variant]) - statistics.median(samples["baseline"])
                require(difference > size // 2,
                        f"Мутация {variant}, {method}, {size} не обнаружена по памяти")
                differences.append(dict(method=method, size=size, variant=variant,
                                        median_difference=difference, samples=samples))
    return differences


def verify(root, output, compiler, jobs, timeout, methods, sizes):
    system = platform.system()
    require(system in ("Darwin", "Linux"), "Поддерживаются только macOS и Linux")
    shutil.copy2(Path(__file__).resolve(), output / "runner.py")
    run([compiler, "--version"], output, output / "compiler.log", timeout)
    snapshot = output / "snapshot"
    for name in ("src", "include", "third_party/include"):
        shutil.copytree(root / name, snapshot / name)
    library = snapshot / "third_party/lib/libdependence.a"
    library.parent.mkdir(parents=True)
    shutil.copy2(root / "third_party/lib/libdependence.a", library)
    probe = snapshot / "memory.cpp"
    shutil.copy2(root / "tests/codec/abc/memory.cpp", probe)
    manifest = {str(path.relative_to(snapshot)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in snapshot.rglob("*") if path.is_file()}
    (output / "snapshot-sha256.json").write_text(json.dumps(manifest, indent=2))
    # Флаги фиксированы: санитайзеры меняют RSS, готовая libawh может быть устаревшей.
    flags = ["-std=c++17", "-O2", "-pthread", "-I" + str(snapshot / "include"),
             "-I" + str(snapshot / "third_party/include")]
    for name in ("lz4", "bz2", "zstd", "lzma", "zlib", "brotli", "snappy", "lizard", "density"):
        flags += ["-I" + str(snapshot / "third_party/include" / name)]
    sources = [snapshot / name for name in SOURCES]
    sources += sorted((snapshot / "src/alloc").glob("*.cpp"))
    sources += [snapshot / "src/alloc/capture" / ("mach.cpp" if system == "Darwin" else "elf.cpp")]
    sources += [probe]
    objects = output / "objects"
    objects.mkdir()

    def compile_source(source):
        obj = objects / (str(source.relative_to(snapshot)).replace("/", "_") + ".o")
        # На macOS файловая система вызывает Foundation из Objective-C++.
        language = (["-x", "objective-c++", "-fobjc-arc"]
                    if system == "Darwin" and source == snapshot / "src/sys/fs.cpp" else [])
        run([compiler] + flags + language + ["-c", str(source), "-o", str(obj)], output,
            obj.with_suffix(".log"), timeout)
        print(f"Собрано: {source.relative_to(snapshot)}", flush=True)
        return str(obj)

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        common = list(pool.map(compile_source, sources))
    chunk = (snapshot / "src/codec/abc/chunk.cpp").read_text()
    zero = "if(chunk.origin == 0)"
    limit = "opened, static_cast <size_t> (chunk.origin));"
    require(chunk.count(zero) == 1 and chunk.count(limit) == 1,
            "Точки мутаций изменились: требуется пересмотр щупа")
    variants = {"baseline": chunk, "no-zero": chunk.replace(zero, "if(false)"),
                "no-limit": chunk.replace(limit, "opened);")}
    for name, body in variants.items():
        source = snapshot / (name + ".cpp")
        source.write_text(body)
        (output / (name + ".diff")).write_text("".join(difflib.unified_diff(
            chunk.splitlines(True), body.splitlines(True), fromfile="chunk.cpp", tofile=name)))
        obj = compile_source(source)
        libs = ["-framework", "Foundation"] if system == "Darwin" else ["-ldl"]
        # Исходный chunk.o здесь отсутствует: каждая версия связана ровно один раз.
        run([compiler] + flags + common + [obj, str(library)] + libs +
            ["-o", str(output / name)], output, output / (name + "-link.log"), timeout)
    results = []
    # Удерживаем память до конца серии: Linux-щуп обязан пережить грязный пик родителя.
    launching_memory = bytearray(128 * MIB)
    for method in methods:
        for size in sizes:
            frame = output / f"{method}-{size}.abc"
            run([str(output / "baseline"), "generate", str(frame), str(size),
                 str(METHODS[method])], output, frame.with_suffix(".log"), timeout)
            for name in variants:
                for declared in (0, 64, size - 1, size, size + 1):
                    for repeat in range(3):
                        log = output / f"{method}-{name}-{size}-{declared}-{repeat}.log"
                        result = measure(output / name, frame, declared, METHODS[method],
                                         output, log, timeout)
                        result.update(method=method, variant=name, repeat=repeat)
                        results.append(result)
                        (output / "measurements.json").write_text(json.dumps(results, indent=2))
                        check(result, size, name, declared)
                        print(f"{method}, {name}: размер={size // MIB} МиБ, origin={declared}, "
                              f"прирост RSS={result['rss_delta'] / MIB:.2f} МиБ", flush=True)
    differences = check_mutations(results, methods, sizes)
    expected = len(methods) * len(sizes) * len(variants) * 5 * 3
    require(len(results) == expected, "Матрица измерений не завершена")
    return {"passed": True, "runs": len(results), "expected": expected, "system": system,
            "machine": platform.machine(), "methods": methods, "sizes": sizes,
            "limit_bytes": LIMIT, "parent_memory": len(launching_memory),
            "memory_differences": differences}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument("--output", type=Path, help="Родитель нового временного каталога")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=180, help="Таймаут каждой команды, секунды")
    parser.add_argument("--methods", nargs="+", choices=METHODS, default=list(METHODS),
                        help="Алгоритмы сжатия, по умолчанию все 11")
    parser.add_argument("--sizes", nargs="+", type=int, choices=(32, 128), default=[32],
                        help="Размеры исходного содержимого в МиБ, по умолчанию 32")
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout < 1:
        parser.error("--jobs и --timeout должны быть положительными")
    output = Path(tempfile.mkdtemp(prefix="awh-abc-memory-", dir=args.output)).resolve()
    print(f"Стенд: {output}", flush=True)
    try:
        result = verify(args.root.resolve(), output, os.environ.get("CXX", "c++"),
                        args.jobs, args.timeout, list(dict.fromkeys(args.methods)),
                        [size * MIB for size in dict.fromkeys(args.sizes)])
    except (OSError, RuntimeError) as error:
        result = {"passed": False, "error": str(error)}
    (output / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2))
    print(json.dumps(result, ensure_ascii=False), flush=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
