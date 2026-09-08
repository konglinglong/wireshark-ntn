# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this tree is

A Wireshark fork (`wireshark-ntn`, remote `github.com/konglinglong/wireshark-ntn`) used for Non-Terrestrial-Network (NTN) RRC development. Day-to-day work happens on branch `nr_rrc_ntn` and centres on the NR RRC dissector (`epan/dissectors/asn1/nr-rrc/` → generated `epan/dissectors/packet-nr-rrc.c`), which now carries this fork's NTN customizations directly in its `nr-rrc` protocol namespace. The earlier duplicated variant `nr-rrc-ntn` was removed on 2026-09-08: the standard→custom delta (canonical source `custom/nr-rrc-ntn/38331-j20-custom_0831.asn`) was folded into `epan/dissectors/asn1/nr-rrc/NR-RRC-Definitions.asn` and `packet-nr-rrc.c` regenerated. Everything else is upstream Wireshark; treat upstream code as read-only unless a task says otherwise.

- Local build root: `D:\workspace\wireshark-ntn`; generated build dir used here is `build-min/`.
- Toolchain lives in `D:\DevTools\msys64` (MSYS2/MinGW-w64). Use the **mingw64** python, not the WindowsApps stub:
  `export PATH="/d/DevTools/msys64/mingw64/bin:/d/DevTools/msys64/usr/bin:$PATH"`

## Build

Configure + build (confirmed working, Release, Qt GUI + plugins + tools):

```bash
export PATH="/d/DevTools/msys64/mingw64/bin:/d/DevTools/msys64/usr/bin:$PATH"
cmake -S /d/workspace/wireshark-ntn -B /d/workspace/wireshark-ntn/build-min -G Ninja \
  -DENABLE_WERROR=OFF -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_wireshark=ON -DBUILD_tshark=ON -DBUILD_dumpcap=ON -DBUILD_capinfos=ON \
  -DBUILD_rawshark=ON -DBUILD_randpkt=ON -DENABLE_PLUGINS=ON
cd /d/workspace/wireshark-ntn/build-min && ninja        # full build
ninja tshark                                            # or single targets
```

- **Always use explicit `-S <src> -B <build>`.** Running `cmake ... .` from the repo root creates an in-source build: it writes `CMakeCache.txt`/`CMakeFiles`/`build.ninja` plus generated files (`epan/dissectors/dissectors.c`, `ui/qt/json_resource.qrc`, `config.h`, …) into the source tree. `.gitignore` hides them (so `git status` stays clean) but they must be removed by hand (delete `CMakeFiles` dirs everywhere + the generated files; `git clean -nfdX` lists them).
- Built binaries/plugins land in `build-min/run/` (e.g. `wireshark.exe`, `tshark.exe`, `libwireshark.dll`, `plugins/`).
- Runtime protocol/field checks without a capture: `build-min/run/tshark.exe -G protocols | grep nr-rrc` and `tshark -G fields | grep nr-rrc-ntn`.

### Tests
The test suite is pytest-based and drives a built `tshark`; it runs from inside the build dir (see `test/README.test` for authoritative steps):
```bash
export PATH="/d/DevTools/msys64/mingw64/bin:/d/DevTools/msys64/usr/bin:$PATH"
cd /d/workspace/wireshark-ntn/build-min
ninja test-programs          # extra binaries the "unittests" suite needs
pytest                       # whole suite
pytest ../test/suite_dissection.py -k nr_rrc   # one suite / filtered cases
```
Needs `pip install pytest pytest-xdist`.

## ASN.1 dissector workflow (core activity in this fork)

Wireshark builds its ASN.1 dissectors from `epan/dissectors/asn1/<proto>/` into **committed, in-tree** generated files `epan/dissectors/packet-<proto>.c`/`.h`. Regenerating is how NR RRC edits take effect.

Per-proto source dir layout: `CMakeLists.txt` (defines `PROTOCOL_NAME`, lists the `.asn` files via `ASN_FILE_LIST`), `packet-<proto>-template.c/.h`, `<proto>.cnf`, and the `.asn` module files. The `ASN2WRS()` CMake macro (`cmake/modules/UseAsn2Wrs.cmake`) runs `tools/asn2wrs.py`; the exact command line is printed in the header banner of the generated `.c`/`.h`.

To regenerate one protocol from its source dir (paths are relative to that dir; replicate the banner):

```bash
cd epan/dissectors/asn1/<proto>
PYTHONUTF8=1 /d/DevTools/msys64/mingw64/bin/python.exe \
  /d/workspace/wireshark-ntn/tools/asn2wrs.py \
  -q -L -p <PROTOCOL_NAME> -c ./<proto>.cnf -s ./packet-<proto>-template \
  -D . -O ../.. <ASN1>.asn ...     # <ASN1> = the ASN_FILE_LIST files
```

Key mechanics:
- The `-p <proto>` value is the **single source of names**: it drives the generated C identifier prefix (`nr-rrc` → `nr_rrc`), all field/filter strings (`nr-rrc.*`), `register_dissector` names, and the output filenames. Two dissectors sharing nothing at the symbol level just need different `-p` values; ASN module names do **not** leak into C symbols when `-p` is given.
- `.cnf` `PROTO_ROOT_NAME` (e.g. `proto_nr_rrc_ntn`) and identifiers hard-coded in the template `.c/.h` are copied verbatim into generated code — rename them in the source, don't rely on `-p`.
- Generated code is fully self-contained: template `#include "packet-<proto>-hf.c"`-style lines are inlined into the single committed `packet-<proto>.c`.
- Registration is automatic: building runs `tools/make-regs.py`, which scans each dissector `.c` for `proto_register_*`/`proto_reg_handoff_*` and emits `dissectors.c`. Adding a `.c` to the source list is enough.
- Wireshark-internal docs for the generator: `doc/wsdg_src/wsdg_asn2wrs.adoc`.

### Registering a new / duplicated ASN.1 dissector
1. Create `epan/dissectors/asn1/<proto>/` (copy an existing dir, rename files).
2. Add `<proto>` to `ASN1_SRC_DIRS` in `epan/dissectors/asn1/CMakeLists.txt`.
3. Add the generated `packet-<proto>.c` to `CLEAN_ASN1_DISSECTOR_SRC` in `epan/dissectors/CMakeLists.txt`. Prefer `python tools/generate-dissector-lists.py` (regenerates the block authoritatively; it inserts alphabetically and only adds lines). `tools/make-regs.py` then auto-discovers `proto_register_<proto>`.
4. Generated `packet-<proto>.c/.h` are committed to git alongside the source dir.

Note: this fork previously used a duplicated, dormant `nr-rrc-ntn` dissector created this way; it was removed on 2026-09-08 and NTN work now edits `asn1/nr-rrc/` in place instead. Don't re-create the variant.

### NTN-specific naming constraints (learned the hard way)
- `nr-rrc` is the single NR RRC dissector and already carries the NTN customization. External dissectors (mac-nr, rlc-nr, pdcp-nr, f1ap, ngap, xnap, x2ap, e1ap, lte-rrc, catapult…) bind to the `nr-rrc.*` field names / `dissect_nr_rrc_*_PDU` / `packet-nr-rrc.h`; never rename those entry points or existing field abbreviations. Custom NTN content is added as new optional fields/types (e.g. `NTN-Handover-PrivateIEs`, `SUPI-Report`/`MSIN`, `DVB-Config`, `UCI-Config*`, `RACH-ConfigCommonNB`, SUL `SEQUENCE OF`); these register under `nr-rrc.*`.
- Regenerating after an `.asn` edit can re-number asn2wrs' internal generic-field counters (`nonCriticalExtension_NNN`, `setup_NNN`, …) throughout the generated `.c`. That churn is expected and harmless — the public filter abbreviations (`nr-rrc.nonCriticalExtension_element`, etc.) do not change.
- `proto_register_protocol(long, short, abbrev)` long name must be globally unique at runtime or Wireshark aborts with `Duplicate protocol name`. The base dissector uses long `"NR Radio Resource Control (RRC) protocol"`, short `"NR RRC"`, abbrev `"nr-rrc"`.
- Some symbols containing `nr_rrc` belong to **other** dissectors and must NOT be renamed when forking a copy: notably `set_pdcp_nr_rrc_reestablishment_request` (owned by `packet-pdcp-nr`; declared in `packet-pdcp-nr.h`).

### ASN.1 source-format quirk of this fork
The committed `.asn` in `epan/dissectors/asn1/nr-rrc/` is an **expanded form**: every 3GPP `SetupRelease {X}` is already inlined as `CHOICE {release NULL, setup X}` (~355 in NR-RRC-Definitions). The tree's `asn2wrs.py` cannot parse the canonical compact form or trailing commas. Consequences:
- Keep committed `.asn` files in expanded form; write any new SetupRelease-style field as the inline `CHOICE {release NULL, setup X}`.
- The "authoritative" canonical ASN reference lives in `custom/nr-rrc-ntn/` (`38331-j20-standard.asn` vs `...-custom_0831.asn` — the diff between them is the customization). Only `NR-RRC-Definitions` module content differs there; `PC5-RRC-Definitions` and `NR-InterNodeDefinitions` match the committed files byte-for-byte (ignoring CRLF). Since 2026-09-08 the full standard→custom delta has been folded into `epan/dissectors/asn1/nr-rrc/NR-RRC-Definitions.asn` (the customized committed expanded module), so the fork's NTN edits are now made **in that module in place** and regenerated — do not port into a fresh copy again. When the canonical files change, translate the standard→custom delta into the committed **expanded** module by hand — not wholesale replacing it (baselines/notation differ, and asn2wrs rejects the canonical text).
- The generated `.asn` files and `packet-*.c` use CRLF line endings; tools handling them should tolerate that.

## Architecture pointers

This is upstream Wireshark. Its own docs are in-tree and authoritative:
- Dissector development: `doc/README.dissector`, `doc/README.heuristic`, `doc/wsdg_src/wsdg_dissector.adoc`.
- Overall structure (libs `wsutil` → `wiretap` → `epan` [dissectors] → apps / Qt GUI): see `doc/README.developer`, top-level `CMakeLists.txt`.
- `epan/dissectors/CMakeLists.txt` is the central place wiring dissector sources: hand-written `DISSECTOR_SRC`, generated `CLEAN_ASN1_DISSECTOR_SRC`, registration inputs (`ALL_DISSECTOR_SRC` → `tools/make-regs.py`).
- Test suite: `test/` (pytest; see `test/README.test` and its "Tests" section above).

## Git
- Remote `origin` is HTTPS; **SSH** is the working push path for this machine: `git push git@github.com:konglinglong/wireshark-ntn.git <branch>` (SSH key already added to the account).
- Global git identity already set: `user.name "上班不摸鱼"`, `user.email waterairsky@qq.com`.
- Branch `nr_rrc_ntn` has its upstream set to the SSH URL, so `git push` on that branch works directly.
- The untracked `git` file at the repo root is a pre-existing stray; ignore it.
