# Third-party licenses

AWH (ANYKS Web Hub) itself is distributed under the **AWH License 1.0**
(`LicenseRef-AWH-1.0`) — see [LICENSE](LICENSE).

The components listed on this page are **not** part of AWH and are **not**
covered by the AWH License. Each of them is the work of its own authors and is
distributed under its own terms. Nothing in the AWH License restricts your
rights in any of them: you may obtain, use, modify, extract and redistribute
any of these components separately, on the terms of its own license, whether or
not you use AWH.

If you redistribute a build of AWH, you are responsible for complying with the
licenses below — in practice this means shipping the copyright notices and
license texts of the components you actually linked in.

## Summary

| Component | Upstream | License (SPDX) | Notes |
|---|---|---|---|
| BoringSSL | [boringssl.googlesource.com](https://boringssl.googlesource.com/boringssl) | `Apache-2.0` | TLS/QUIC crypto. The Go-licensed part of the TLS test suite is not compiled into `libcrypto`/`libssl` and is not distributed with AWH builds. |
| Brotli | [github.com/google/brotli](https://github.com/google/brotli) | `MIT` | Compression. |
| bzip2 | [sourceware.org/bzip2](https://sourceware.org/bzip2/) | `bzip2-1.0.6` | Compression. BSD-style, four clauses. |
| Density | [github.com/k0dai/density](https://github.com/k0dai/density) | `BSD-3-Clause` | Compression. |
| Lizard | [github.com/inikep/lizard](https://github.com/inikep/lizard) | `BSD-2-Clause` | Compression. Only the `lib` directory is used; the `programs`/`examples` directories are GPL-2.0 and are neither built nor shipped. |
| LZ4 | [lz4.org](https://lz4.org) | `BSD-2-Clause` | Compression. Only the `lib` directory is used; the `programs`/`tests`/`examples` directories are GPL-2.0-or-later and are neither built nor shipped. |
| liblzma (XZ Utils) | [tukaani.org/xz](https://tukaani.org/xz/) | public domain | Compression. `liblzma` itself is in the public domain. The GPL-licensed parts of XZ Utils are build-system and command-line-tool files that do not end up in the binaries. |
| Snappy | [github.com/google/snappy](https://github.com/google/snappy) | `BSD-3-Clause` | Compression. |
| zlib | [zlib.net](http://www.zlib.net) | `Zlib` | Compression (DEFLATE / GZIP). |
| Zstandard | [github.com/facebook/zstd](https://github.com/facebook/zstd) | `BSD-3-Clause OR GPL-2.0-only` | Compression. Dual-licensed; AWH relies on the **BSD-3-Clause** branch. |

All of the above are wired in as git submodules under `submodules/` (see
`.gitmodules`).

AWH no longer links **libiconv** or **libidn2**. Charset conversion is provided
by the built-in `awh::charset` module and internationalised domain names by the
built-in `awh::idna` module. Their tables are generated from the reference data
those projects ship, but no code of theirs is compiled into AWH, and no LGPL
component is part of a distributed build. Their sources may still be present
under `submodules/` as reference material for the verification stands under
`tools/verify/`; that material is not part of any AWH build.

Other software may be present in a working copy of the repository without being
part of AWH or of any AWH build — development and interoperability tooling, for
example, is neither linked into the library nor shipped with it. Only the
components listed above can end up in a distributed build.

## Locating the license texts

The full text of each license ships with the corresponding component:

```
submodules/boringssl/LICENSE       submodules/lz4/LICENSE
submodules/brotli/LICENSE          submodules/lzma/COPYING
submodules/bz2/COPYING             submodules/snappy/COPYING
submodules/density/LICENSE.md      submodules/zlib/LICENSE
submodules/lizard/lib/LICENSE      submodules/zstd/LICENSE
```

Note that for **Lizard** and **LZ4** the applicable file is the one inside
`lib/`, not the one in the repository root.

## Reporting a mistake

If you believe a component is attributed incorrectly here, or a component is
missing, please open an issue at <https://github.com/anyks/awh/issues> — it will
be corrected.
