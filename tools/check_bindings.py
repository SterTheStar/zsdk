#!/usr/bin/env python3
"""
ZSDK Bindings & C ABI Self-Verification Tool.

Checks:
  1. Every ZSDK C++ subsystem directory (include/zsdk/<subsys>) has a matching
     C ABI header (include/zsdk/c/zsdk_c_<subsys>.h), C++ bridge (src/c/zsdk_c_<subsys>.cpp),
     inclusion in zsdk_c.h, build entry in CMakeLists.txt, and Lua binding module.
  2. Every ZSDK_C_API function declared in include/zsdk/c/*.h has a definition in src/c/*.cpp.
  3. Enum parity: zsdk::Result vs zsdk_result_t (count and numeric values).
  4. Lua coverage report: warns/fails if a C ABI module is missing from bindings/lua/.
"""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Subsystems in include/zsdk/<name> that map to C ABI & Lua modules.
# 'debug' maps to 'log' (plus core's DebugOverlay/MemoryTracker).
SUBSYSTEM_MAP = {
    "core":     "core",
    "debug":    "log",
    "input":    "input",
    "audio":    "audio",
    "graphics": "graphics",
    "fs":       "fs",
    "net":      "net",
    "security": "security",
}


def check_subsystems(errors: list[str]) -> None:
    inc_zsdk = ROOT / "include" / "zsdk"
    inc_c = ROOT / "include" / "zsdk" / "c"
    src_c = ROOT / "src" / "c"
    lua_src = ROOT / "bindings" / "lua" / "src"

    umbrella = (inc_c / "zsdk_c.h").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    lua_loader = (lua_src / "zsdk_lua.c").read_text(encoding="utf-8")
    lua_cmake = (ROOT / "bindings" / "lua" / "CMakeLists.txt").read_text(encoding="utf-8")

    # Detect any new C++ subsystem folder not listed in SUBSYSTEM_MAP
    for entry in sorted(inc_zsdk.iterdir()):
        if not entry.is_dir() or entry.name == "c":
            continue
        if entry.name not in SUBSYSTEM_MAP:
            errors.append(
                f"New C++ subsystem 'include/zsdk/{entry.name}' has no mapping in "
                "tools/check_bindings.py (add C ABI + Lua binding)."
            )

    for cpp_sub, c_sub in SUBSYSTEM_MAP.items():
        if not (inc_zsdk / cpp_sub).is_dir():
            continue
        c_hdr = inc_c / f"zsdk_c_{c_sub}.h"
        c_src = src_c / f"zsdk_c_{c_sub}.cpp"
        lua_c = lua_src / f"zsdk_lua_{c_sub}.c"

        if not c_hdr.is_file():
            errors.append(f"Missing C ABI header: {c_hdr.relative_to(ROOT)}")
        elif f"zsdk_c_{c_sub}.h" not in umbrella:
            errors.append(f"C ABI header zsdk_c_{c_sub}.h not included in include/zsdk/c/zsdk_c.h")

        if not c_src.is_file():
            errors.append(f"Missing C ABI implementation: {c_src.relative_to(ROOT)}")
        elif f"src/c/zsdk_c_{c_sub}.cpp" not in cmake:
            errors.append(f"C ABI source src/c/zsdk_c_{c_sub}.cpp not listed in CMakeLists.txt")

        if not lua_c.is_file():
            errors.append(f"Missing Lua binding file: {lua_c.relative_to(ROOT)}")
        else:
            if f"luaopen_zsdk_{c_sub}" not in lua_loader:
                errors.append(f"luaopen_zsdk_{c_sub} not registered in bindings/lua/src/zsdk_lua.c")
            if f"src/zsdk_lua_{c_sub}.c" not in lua_cmake:
                errors.append(f"src/zsdk_lua_{c_sub}.c not listed in bindings/lua/CMakeLists.txt")


def check_c_abi_definitions(errors: list[str]) -> list[str]:
    inc_c = ROOT / "include" / "zsdk" / "c"
    src_c = ROOT / "src" / "c"

    decl_re = re.compile(r"ZSDK_C_API\s+[^;()]+?\b(zsdk_[a-z0-9_]+)\s*\(")
    declared: list[tuple[str, str]] = []
    for hdr in sorted(inc_c.glob("*.h")):
        text = hdr.read_text(encoding="utf-8")
        for m in decl_re.finditer(text):
            declared.append((m.group(1), hdr.name))

    all_cpp_text = "\n".join(p.read_text(encoding="utf-8") for p in sorted(src_c.glob("*.cpp")))
    for fn, hdr_name in declared:
        if not re.search(rf"\b{re.escape(fn)}\s*\(", all_cpp_text):
            errors.append(f"C ABI function '{fn}' declared in {hdr_name} has no definition in src/c/*.cpp")

    return [fn for fn, _ in declared]


def parse_enum_values(text: str, enum_pattern: str) -> list[int]:
    m = re.search(enum_pattern, text, re.DOTALL)
    if not m:
        return []
    body = m.group(1)
    # Strip C/C++ comments
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.DOTALL)
    body = re.sub(r"//.*", "", body)
    values: list[int] = []
    next_val = 0
    for raw_item in body.split(","):
        item = raw_item.strip()
        if not item:
            continue
        if "=" in item:
            _, rhs = item.split("=", 1)
            next_val = int(rhs.strip(), 0)
        values.append(next_val)
        next_val += 1
    return values


def check_enum_parity(errors: list[str]) -> None:
    types_hpp = (ROOT / "include" / "zsdk" / "core" / "types.hpp").read_text(encoding="utf-8")
    c_types_h = (ROOT / "include" / "zsdk" / "c" / "zsdk_c_types.h").read_text(encoding="utf-8")

    cpp_vals = parse_enum_values(types_hpp, r"enum\s+class\s+Result\s*:\s*int32_t\s*\{(.*?)\};")
    c_vals = parse_enum_values(c_types_h, r"typedef\s+enum\s+zsdk_result_t\s*\{(.*?)\}\s*zsdk_result_t;")

    if not cpp_vals or not c_vals:
        errors.append("Failed to parse Result or zsdk_result_t enum values.")
    elif cpp_vals != c_vals:
        errors.append(
            f"Enum mismatch: zsdk::Result ({cpp_vals}) != zsdk_result_t ({c_vals}) "
            "in types.hpp vs zsdk_c_types.h"
        )


def main() -> int:
    errors: list[str] = []
    check_subsystems(errors)
    declared_fns = check_c_abi_definitions(errors)
    check_enum_parity(errors)

    lua_text = "\n".join(
        p.read_text(encoding="utf-8")
        for p in sorted((ROOT / "bindings" / "lua" / "src").glob("*.c"))
    )
    # C-only vararg/callback primitives (covered in Lua via zsdk_log_msg / async wrappers)
    c_only_fns = {"zsdk_log", "zsdk_log_v", "zsdk_http_get"}
    missing_in_lua = [
        fn for fn in declared_fns
        if fn not in c_only_fns and not re.search(rf"\b{re.escape(fn)}\b", lua_text)
    ]
    for fn in missing_in_lua:
        errors.append(f"C ABI function '{fn}' is not bound in bindings/lua/src/*.c")

    wrapped_in_lua = len(declared_fns) - len(missing_in_lua) - len(c_only_fns & set(declared_fns))

    if errors:
        print("[ZSDK BINDINGS CHECK FAILED]")
        for err in errors:
            print(f"  - {err}")
        return 1

    print(
        f"[ZSDK BINDINGS CHECK OK] {len(SUBSYSTEM_MAP)} subsystems, "
        f"{len(declared_fns)} C ABI functions verified, "
        f"{wrapped_in_lua}/{len(declared_fns)} bound in Lua, Result enum 1:1 parity."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
