#!/usr/bin/env python3
"""
ZSDK Bindings & C ABI Self-Verification Tool.

Checks:
  1. Every ZSDK C++ subsystem directory (include/zsdk/<subsys>) has a matching
     C ABI header, C++ bridge, inclusion in zsdk_c.h, build entry in CMakeLists.txt,
     Lua binding module, Rust zsdk-sys FFI module, and Rust safe zsdk module.
  2. Every ZSDK_C_API function declared in include/zsdk/c/*.h has a definition in src/c/*.cpp.
  3. 1:1 numeric enum parity across C++ (.hpp), C ABI (zsdk_c_*.h), and Rust (zsdk-sys)
     for all 13 shared enums.
  4. Lua coverage: every non-excluded C ABI function is called in bindings/lua/src/*.c,
     and every luaL_Reg entry resolves to a defined function.
  5. Rust coverage: every non-excluded C ABI function is declared in zsdk-sys/src/*.rs
     AND wrapped in zsdk/src/*.rs.
  6. C++ public API vs C ABI coverage summary (use --cpp-gaps to list all unbridged C++ methods).
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Subsystems in include/zsdk/<name> that map to C ABI, Lua, and Rust modules.
# Tuple: (c_and_lua_name, rust_safe_module_name)
SUBSYSTEM_MAP = {
    "core":     ("core",     "core_sys"),
    "debug":    ("log",      "log"),
    "input":    ("input",    "input"),
    "audio":    ("audio",    "audio"),
    "graphics": ("graphics", "graphics"),
    "fs":       ("fs",       "fs"),
    "net":      ("net",      "net"),
    "security": ("security", "security"),
}

# Host-side subsystems with nothing to expose through the C ABI / Lua / Rust bindings.
HOST_ONLY_SUBSYSTEMS = {"script"}

# Shared enums verified 1:1 across C++, C ABI, and Rust zsdk-sys:
# (cpp_rel_path, cpp_enum, c_rel_path, c_enum, rust_rel_path, rust_enum)
SHARED_ENUMS = [
    ("core/types.hpp",           "Result",         "zsdk_c_types.h",    "zsdk_result_t",         "types.rs",    "ZsdkResult"),
    ("debug/logger.hpp",         "LogLevel",       "zsdk_c_types.h",    "zsdk_log_level_t",      "types.rs",    "ZsdkLogLevel"),
    ("input/input_types.hpp",    "GamepadButton",  "zsdk_c_input.h",    "zsdk_button_t",         "input.rs",    "ZsdkButton"),
    ("graphics/renderer_types.hpp", "BlendMode",   "zsdk_c_graphics.h", "zsdk_blend_mode_t",     "graphics.rs", "ZsdkBlendMode"),
    ("graphics/renderer_types.hpp", "PrimitiveType","zsdk_c_graphics.h","zsdk_primitive_type_t", "graphics.rs", "ZsdkPrimitiveType"),
    ("graphics/texture.hpp",     "TextureFormat",  "zsdk_c_graphics.h", "zsdk_tex_format_t",     "graphics.rs", "ZsdkTexFormat"),
    ("graphics/texture.hpp",     "TextureFilter",  "zsdk_c_graphics.h", "zsdk_tex_filter_t",     "graphics.rs", "ZsdkTexFilter"),
    ("graphics/renderer_types.hpp", "TextureWrap", "zsdk_c_graphics.h", "zsdk_tex_wrap_t",       "graphics.rs", "ZsdkTexWrap"),
    ("fs/filesystem.hpp",        "FileMode",       "zsdk_c_fs.h",       "zsdk_file_mode_t",      "fs.rs",       "ZsdkFileMode"),
    ("fs/filesystem.hpp",        "SeekOrigin",     "zsdk_c_fs.h",       "zsdk_seek_origin_t",    "fs.rs",       "ZsdkSeekOrigin"),
    ("net/network_mgr.hpp",      "NetworkState",   "zsdk_c_net.h",      "zsdk_net_state_t",      "net.rs",      "ZsdkNetState"),
    ("net/socket.hpp",           "SocketProtocol", "zsdk_c_net.h",      "zsdk_socket_proto_t",   "net.rs",      "ZsdkSocketProto"),
    ("security/crypto.hpp",      "HashAlgorithm",  "zsdk_c_security.h", "zsdk_hash_algo_t",      "security.rs", "ZsdkHashAlgo"),
]

# C++ headers whose public methods are tracked against src/c/*.cpp
TRACKED_CPP_HEADERS = [
    ("core/clock.hpp",              "FrameTimer"),
    ("core/system_info.hpp",        None),
    ("debug/debug_overlay.hpp",     None),
    ("input/input.hpp",             "InputManager"),
    ("input/keyboard_mouse.hpp",    "KeyboardMouseManager"),
    ("input/virtual_keyboard.hpp",  "VirtualKeyboard"),
    ("audio/audio.hpp",             "AudioEngine"),
    ("audio/pcm_mixer.hpp",         "PcmMixer"),
    ("graphics/gl_context.hpp",     "GLContext"),
    ("graphics/texture.hpp",        "Texture"),
    ("graphics/mesh_buffer.hpp",    "MeshBuffer"),
    ("graphics/renderer.hpp",       "Renderer"),
    ("graphics/sprite_batch.hpp",   "SpriteBatch2D"),
    ("graphics/frustum.hpp",        "Frustum"),
    ("fs/filesystem.hpp",           "FileSystem"),
    ("fs/local_storage.hpp",        "LocalStorage"),
    ("fs/savegame.hpp",             "SaveGameManager"),
    ("fs/ram_cache.hpp",            "RamCache"),
    ("net/network_mgr.hpp",         "NetworkManager"),
    ("net/socket.hpp",              "Socket"),
    ("net/http_client.hpp",         "HttpClient"),
    ("security/crypto.hpp",         None),
    ("security/license.hpp",        None),
]

CPP_CONSTRUCTORS_AND_INTERNAL = {
    "FrameTimer", "InputManager", "GamepadState", "KeyboardMouseManager", "VirtualKeyboard",
    "AudioEngine", "MediaStream", "SoundEffect", "PcmMixer", "GLContext", "Texture",
    "MeshBuffer", "SpriteBatch2D", "File", "FileSystem", "LocalStorage", "SaveGameManager",
    "RamCache", "NetworkManager", "Socket", "HttpClient", "Cipher",
    "BeginFrame", "HandlePlatformEvent", "SetAnalogConfig", "SetButtonState", "SetRawSticks",
    "GetGamepadMutable", "RecomputeStep", "GetSharedFontTextureId", "ReleaseSharedFontTexture",
    "PushLog", "BuildUdataPath", "Commit", "HasRoomFor", "LoadPath",
    "GetDeltaMs", "GetDeltaSeconds", "GetFps", "Reset", "Tick",
    "IsPowerOfTwo", "NextPowerOfTwo", "RotateFx", "ScaleFx", "TranslateFx",
    "GenerateMipmaps", "SetBlendEquation", "LoadPaletteMatrixFromModelView", "Hmac",
}


def check_subsystems(errors: list[str]) -> None:
    inc_zsdk = ROOT / "include" / "zsdk"
    inc_c = ROOT / "include" / "zsdk" / "c"
    src_c = ROOT / "src" / "c"
    lua_src = ROOT / "bindings" / "lua" / "src"
    rust_sys_lib = (ROOT / "bindings" / "rust" / "zsdk-sys" / "src" / "lib.rs").read_text(encoding="utf-8")
    rust_safe_lib = (ROOT / "bindings" / "rust" / "zsdk" / "src" / "lib.rs").read_text(encoding="utf-8")

    umbrella = (inc_c / "zsdk_c.h").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    lua_loader = (lua_src / "zsdk_lua.c").read_text(encoding="utf-8")
    lua_cmake = (ROOT / "bindings" / "lua" / "CMakeLists.txt").read_text(encoding="utf-8")

    for entry in sorted(inc_zsdk.iterdir()):
        if not entry.is_dir() or entry.name == "c":
            continue
        if entry.name not in SUBSYSTEM_MAP and entry.name not in HOST_ONLY_SUBSYSTEMS:
            errors.append(
                f"New C++ subsystem 'include/zsdk/{entry.name}' has no mapping in "
                "tools/check_bindings.py (add C ABI + Lua + Rust binding)."
            )

    for cpp_sub, (c_sub, rust_safe_sub) in SUBSYSTEM_MAP.items():
        if not (inc_zsdk / cpp_sub).is_dir():
            continue
        c_hdr = inc_c / f"zsdk_c_{c_sub}.h"
        c_src = src_c / f"zsdk_c_{c_sub}.cpp"
        lua_c = lua_src / f"zsdk_lua_{c_sub}.c"
        rust_sys_mod = ROOT / "bindings" / "rust" / "zsdk-sys" / "src" / f"{c_sub}.rs"
        rust_safe_mod = ROOT / "bindings" / "rust" / "zsdk" / "src" / f"{rust_safe_sub}.rs"

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

        if not rust_sys_mod.is_file():
            errors.append(f"Missing Rust FFI module: {rust_sys_mod.relative_to(ROOT)}")
        elif f"pub mod {c_sub};" not in rust_sys_lib:
            errors.append(f"pub mod {c_sub}; not exported in bindings/rust/zsdk-sys/src/lib.rs")

        if not rust_safe_mod.is_file():
            errors.append(f"Missing Rust safe module: {rust_safe_mod.relative_to(ROOT)}")
        elif f"pub mod {rust_safe_sub};" not in rust_safe_lib:
            errors.append(f"pub mod {rust_safe_sub}; not exported in bindings/rust/zsdk/src/lib.rs")


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


def check_enum_parity(errors: list[str]) -> int:
    checked = 0
    for cpp_rel, cpp_enum, c_rel, c_enum, rust_rel, rust_enum in SHARED_ENUMS:
        cpp_text = (ROOT / "include" / "zsdk" / cpp_rel).read_text(encoding="utf-8")
        c_text = (ROOT / "include" / "zsdk" / "c" / c_rel).read_text(encoding="utf-8")
        rust_text = (ROOT / "bindings" / "rust" / "zsdk-sys" / "src" / rust_rel).read_text(encoding="utf-8")

        cpp_vals = parse_enum_values(cpp_text, rf"enum\s+class\s+{cpp_enum}\b[^{{]*\{{(.*?)\}};")
        c_vals = parse_enum_values(c_text, rf"typedef\s+enum\s+{c_enum}\s*\{{(.*?)\}}\s*{c_enum};")
        rust_vals = parse_enum_values(rust_text, rf"pub\s+enum\s+{rust_enum}\s*\{{(.*?)\}}")

        # C++ GamepadButton omits 'Count' sentinel; strip trailing Count if C/Rust have one extra sentinel
        if cpp_enum == "GamepadButton" and len(c_vals) == len(cpp_vals) + 1:
            c_vals_cmp = c_vals[:-1]
            rust_vals_cmp = rust_vals[:-1]
        else:
            c_vals_cmp = c_vals
            rust_vals_cmp = rust_vals

        if not cpp_vals or not c_vals or not rust_vals:
            errors.append(f"Failed to parse enum {cpp_enum} / {c_enum} / {rust_enum}.")
            continue

        if cpp_vals != c_vals_cmp:
            errors.append(f"Enum mismatch: zsdk::{cpp_enum} != {c_enum} ({cpp_rel} vs {c_rel})")
        if cpp_vals != rust_vals_cmp:
            errors.append(f"Enum mismatch: zsdk::{cpp_enum} != {rust_enum} ({cpp_rel} vs {rust_rel})")
        checked += 1
    return checked


def check_lua_registrations(errors: list[str]) -> None:
    lua_files = sorted((ROOT / "bindings" / "lua" / "src").glob("zsdk_lua_*.c"))
    for p in lua_files:
        text = p.read_text(encoding="utf-8")
        for m in re.finditer(r'\{\s*"[^"]+"\s*,\s*(l_[a-zA-Z0-9_]+)\s*\}', text):
            fn_name = m.group(1)
            if not re.search(rf"\bstatic\s+int\s+{re.escape(fn_name)}\s*\(", text):
                errors.append(f"Lua table in {p.name} references undefined handler '{fn_name}'")


def compute_cpp_gaps() -> tuple[int, int, dict[str, list[str]]]:
    src_c = "\n".join(p.read_text(encoding="utf-8") for p in sorted((ROOT / "src" / "c").glob("*.cpp")))
    method_re = re.compile(
        r"^\s*(?:static\s+|virtual\s+|inline\s+|ZSDK_FORCE_INLINE\s+)*"
        r"[A-Za-z_][\w:<>*&\s]+?\s+([A-Z][A-Za-z0-9_]*)\s*\(",
        re.M,
    )
    total = 0
    bridged = 0
    gaps: dict[str, list[str]] = {}
    for rel, _ in TRACKED_CPP_HEADERS:
        text = (ROOT / "include" / "zsdk" / rel).read_text(encoding="utf-8")
        methods = sorted(m for m in set(method_re.findall(text)) if m not in CPP_CONSTRUCTORS_AND_INTERNAL)
        missing = []
        for m in methods:
            total += 1
            if re.search(rf"(?:\.|->|::)\b{re.escape(m)}\s*\(", src_c):
                bridged += 1
            else:
                missing.append(m)
        if missing:
            gaps[rel] = missing
    return bridged, total, gaps


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify ZSDK C ABI, Lua, and Rust bindings parity.")
    parser.add_argument("--cpp-gaps", action="store_true", help="Print unbridged C++ methods per header.")
    args = parser.parse_args()

    errors: list[str] = []
    check_subsystems(errors)
    declared_fns = check_c_abi_definitions(errors)
    enum_count = check_enum_parity(errors)
    check_lua_registrations(errors)

    lua_text = "\n".join(
        p.read_text(encoding="utf-8")
        for p in sorted((ROOT / "bindings" / "lua" / "src").glob("*.c"))
    )
    c_only_fns = {"zsdk_log", "zsdk_log_v", "zsdk_http_get", "zsdk_http_post", "zsdk_net_resolve_host"}
    missing_in_lua = [
        fn for fn in declared_fns
        if fn not in c_only_fns and not re.search(rf"\b{re.escape(fn)}\s*\(", lua_text)
    ]
    for fn in missing_in_lua:
        errors.append(f"C ABI function '{fn}' is not called in bindings/lua/src/*.c")
    wrapped_in_lua = len(declared_fns) - len(missing_in_lua) - len(c_only_fns & set(declared_fns))

    rust_sys_text = "\n".join(
        p.read_text(encoding="utf-8")
        for p in sorted((ROOT / "bindings" / "rust" / "zsdk-sys" / "src").glob("*.rs"))
    )
    rust_sys_excluded = {"zsdk_log_v"}
    missing_in_rust_sys = [
        fn for fn in declared_fns
        if fn not in rust_sys_excluded and not re.search(rf"\b{re.escape(fn)}\s*\(", rust_sys_text)
    ]
    for fn in missing_in_rust_sys:
        errors.append(f"C ABI function '{fn}' is not declared in bindings/rust/zsdk-sys/src/*.rs")
    wrapped_in_rust_sys = len(declared_fns) - len(missing_in_rust_sys) - len(rust_sys_excluded & set(declared_fns))

    rust_safe_text = "\n".join(
        p.read_text(encoding="utf-8")
        for p in sorted((ROOT / "bindings" / "rust" / "zsdk" / "src").glob("*.rs"))
    )
    rust_safe_excluded = {"zsdk_log", "zsdk_log_v"}
    missing_in_rust_safe = [
        fn for fn in declared_fns
        if fn not in rust_safe_excluded and not re.search(rf"\b{re.escape(fn)}\s*\(", rust_safe_text)
    ]
    for fn in missing_in_rust_safe:
        errors.append(f"C ABI function '{fn}' is not wrapped in bindings/rust/zsdk/src/*.rs")
    wrapped_in_rust_safe = len(declared_fns) - len(missing_in_rust_safe) - len(rust_safe_excluded & set(declared_fns))

    bridged_cpp, total_cpp, cpp_gaps = compute_cpp_gaps()

    if errors:
        print("[ZSDK BINDINGS CHECK FAILED]")
        for err in errors:
            print(f"  - {err}")
        return 1

    print(
        f"[ZSDK BINDINGS CHECK OK] {len(SUBSYSTEM_MAP)} subsystems, {enum_count} enums 1:1 parity, "
        f"{len(declared_fns)} C ABI functions, "
        f"{wrapped_in_lua}/{len(declared_fns)} Lua, "
        f"{wrapped_in_rust_sys}/{len(declared_fns)} Rust-sys, "
        f"{wrapped_in_rust_safe}/{len(declared_fns)} Rust-safe, "
        f"{bridged_cpp}/{total_cpp} C++ methods bridged."
    )
    if args.cpp_gaps and cpp_gaps:
        print("\n[C++ methods not exposed in C ABI]")
        for hdr, methods in cpp_gaps.items():
            print(f"  {hdr}: {', '.join(methods)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
