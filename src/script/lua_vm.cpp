/**
 * @file lua_vm.cpp
 * @brief Embedded Lua 5.4 VM + ZSDK bindings (gfx / input / save / app).
 */

#include "zsdk/script/lua_vm.hpp"
#include "zsdk/core/app.hpp"
#include "zsdk/debug/logger.hpp"
#include "zsdk/fs/savegame.hpp"
#include "zsdk/fs/vfs.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/sprite_batch.hpp"
#include "zsdk/input/input.hpp"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <stdlib.h>
#include <string.h>

namespace zsdk {
namespace script {

struct LuaVM::Impl {
    lua_State* L;
    Impl() : L(nullptr) {}
};

namespace {

const uint32_t kSaveScriptVersion = 0x4C554131u; /* 'LUA1' */

/* Allocator with a hard memory cap; every byte is accounted in LuaVM::m_used. */
void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    LuaVM* vm = static_cast<LuaVM*>(ud);
    if (ptr == nullptr) {
        osize = 0;
    }
    if (nsize == 0) {
        if (ptr != nullptr) {
            vm->m_used -= static_cast<uint32_t>(osize);
            ::free(ptr);
        }
        return nullptr;
    }
    const uint32_t base = vm->m_used - static_cast<uint32_t>(osize);
    if (static_cast<uint64_t>(base) + nsize > vm->m_limit) {
        return nullptr;
    }
    void* np = ::realloc(ptr, nsize);
    if (np != nullptr) {
        vm->m_used = base + static_cast<uint32_t>(nsize);
    }
    return np;
}

int LuaPanic(lua_State* L) {
    ZSDK_LOG_ERROR("LUA", "panic: %s", lua_tostring(L, -1));
    return 0;
}

LuaVM* GetVM(lua_State* L) {
    return static_cast<LuaVM*>(lua_touserdata(L, lua_upvalueindex(1)));
}

uint8_t Byte(lua_State* L, int idx, int def) {
    lua_Integer v = luaL_optinteger(L, idx, def);
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return static_cast<uint8_t>(v);
}

/* ---- gfx ---- */
int L_gfx_clear(lua_State* L) {
    LuaVM* vm = GetVM(L);
    if (vm->m_batch != nullptr) vm->m_batch->Flush();
    graphics::Renderer::Clear(Color::Rgba(Byte(L, 1, 0), Byte(L, 2, 0), Byte(L, 3, 0), 255), true);
    return 0;
}

int L_gfx_rect(lua_State* L) {
    LuaVM* vm = GetVM(L);
    if (vm->m_batch != nullptr) {
        vm->m_batch->DrawRect(static_cast<float>(luaL_checknumber(L, 1)), static_cast<float>(luaL_checknumber(L, 2)),
                              static_cast<float>(luaL_checknumber(L, 3)), static_cast<float>(luaL_checknumber(L, 4)),
                              Color::Rgba(Byte(L, 5, 255), Byte(L, 6, 255), Byte(L, 7, 255), Byte(L, 8, 255)));
    }
    return 0;
}

int L_gfx_text(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const float x = static_cast<float>(luaL_checknumber(L, 1));
    const float y = static_cast<float>(luaL_checknumber(L, 2));
    const char* str = luaL_checkstring(L, 3);
    const float scale = static_cast<float>(luaL_optnumber(L, 7, 1.0));
    if (vm->m_batch != nullptr) vm->m_batch->Flush();
    graphics::Renderer::DrawText2D(x, y, scale, str, Color::Rgba(Byte(L, 4, 255), Byte(L, 5, 255), Byte(L, 6, 255), 255));
    return 0;
}

int L_gfx_size(lua_State* L) {
    lua_pushinteger(L, 640);
    lua_pushinteger(L, 480);
    return 2;
}

/* ---- input ---- */
bool ParseButton(const char* n, input::GamepadButton& out) {
    struct E { const char* name; input::GamepadButton btn; };
    static const E kMap[] = {
        {"up", input::GamepadButton::DPadUp},       {"down", input::GamepadButton::DPadDown},
        {"left", input::GamepadButton::DPadLeft},   {"right", input::GamepadButton::DPadRight},
        {"a", input::GamepadButton::Action1},       {"b", input::GamepadButton::Action2},
        {"x", input::GamepadButton::Action3},       {"y", input::GamepadButton::Action4},
        {"l", input::GamepadButton::ShoulderLeft},  {"r", input::GamepadButton::ShoulderRight},
        {"start", input::GamepadButton::Start},     {"back", input::GamepadButton::Back},
        {"home", input::GamepadButton::Home}
    };
    for (size_t i = 0; i < sizeof(kMap) / sizeof(kMap[0]); ++i) {
        if (strcmp(n, kMap[i].name) == 0) { out = kMap[i].btn; return true; }
    }
    return false;
}

const input::GamepadState& Pad() {
    return Application::GetInstance()->GetInput().GetGamepad(0);
}

int L_input_down(lua_State* L) {
    input::GamepadButton b;
    if (!ParseButton(luaL_checkstring(L, 1), b)) return luaL_argerror(L, 1, "unknown button");
    lua_pushboolean(L, Pad().IsDown(b));
    return 1;
}

int L_input_pressed(lua_State* L) {
    input::GamepadButton b;
    if (!ParseButton(luaL_checkstring(L, 1), b)) return luaL_argerror(L, 1, "unknown button");
    lua_pushboolean(L, Pad().IsPressed(b));
    return 1;
}

int L_input_connected(lua_State* L) {
    const lua_Integer i = luaL_optinteger(L, 1, 0);
    luaL_argcheck(L, i >= 0 && i < ZSDK_MAX_GAMEPADS, 1, "pad index out of range");
    lua_pushboolean(L, Application::GetInstance()->GetInput().GetGamepad(static_cast<uint8_t>(i)).IsConnected());
    return 1;
}

int L_input_stick(lua_State* L) {
    const Vec2 s = (luaL_optinteger(L, 1, 0) == 1) ? Pad().GetRightStick() : Pad().GetLeftStick();
    lua_pushnumber(L, s.x);
    lua_pushnumber(L, s.y);
    return 2;
}

/* ---- save ---- */
int L_save_write(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const lua_Integer slot = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    luaL_argcheck(L, slot >= 0 && slot < fs::kMaxSaveSlots, 1, "slot out of range");
    luaL_argcheck(L, len <= kLuaSaveMaxBytes, 2, "save data too large");
    const bool ok = vm->m_save != nullptr &&
        Succeeded(vm->m_save->SaveSlot(static_cast<uint8_t>(slot), data, static_cast<uint32_t>(len), kSaveScriptVersion));
    lua_pushboolean(L, ok);
    return 1;
}

int L_save_write_async(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const lua_Integer slot = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    luaL_argcheck(L, slot >= 0 && slot < fs::kMaxSaveSlots, 1, "slot out of range");
    luaL_argcheck(L, len > 0 && len <= kLuaSaveMaxBytes, 2, "save data size out of range");
    const bool ok = vm->m_save != nullptr &&
        Succeeded(vm->m_save->BeginSaveSlot(static_cast<uint8_t>(slot), data, static_cast<uint32_t>(len), kSaveScriptVersion));
    lua_pushboolean(L, ok);
    return 1;
}

int L_save_status(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const char* s = "idle";
    if (vm->m_save != nullptr) {
        switch (vm->m_save->GetAsyncStatus()) {
            case fs::AsyncSaveStatus::Pending: s = "pending"; break;
            case fs::AsyncSaveStatus::Done:    s = "done";    break;
            case fs::AsyncSaveStatus::Failed:  s = "failed";  break;
            default: break;
        }
    }
    lua_pushstring(L, s);
    return 1;
}

int L_save_read(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const lua_Integer slot = luaL_checkinteger(L, 1);
    luaL_argcheck(L, slot >= 0 && slot < fs::kMaxSaveSlots, 1, "slot out of range");
    static char buf[kLuaSaveMaxBytes];
    uint32_t got = 0;
    if (vm->m_save != nullptr &&
        Succeeded(vm->m_save->LoadSlot(static_cast<uint8_t>(slot), buf, sizeof(buf), got, kSaveScriptVersion))) {
        lua_pushlstring(L, buf, got);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

/* ---- require (script: mount only) ---- */
int L_require(lua_State* L) {
    LuaVM* vm = GetVM(L);
    const char* name = luaL_checkstring(L, 1);
    if (vm->m_vfs == nullptr) return luaL_error(L, "require: no filesystem bound");

    /* Cache table lives in the registry; scripts cannot reach it. */
    lua_getfield(L, LUA_REGISTRYINDEX, "zsdk.loaded");
    lua_getfield(L, -1, name);
    if (!lua_isnil(L, -1)) return 1;
    lua_pop(L, 1);

    char path[fs::Vfs::kMaxPathLen];
    size_t n = 0;
    const char* kPre = "script:/";
    for (; kPre[n] != '\0'; ++n) path[n] = kPre[n];
    for (size_t i = 0; name[i] != '\0'; ++i) {
        const char c = name[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok || n + 5u >= sizeof(path)) return luaL_error(L, "require: invalid module name '%s'", name);
        path[n++] = (c == '.') ? '/' : c;
    }
    if (name[0] == '\0' || name[0] == '.' || strstr(name, "..") != nullptr) {
        return luaL_error(L, "require: invalid module name '%s'", name);
    }
    path[n++] = '.'; path[n++] = 'l'; path[n++] = 'u'; path[n++] = 'a'; path[n] = '\0';

    void* buf = nullptr;
    uint32_t size = 0;
    if (Failed(vm->m_vfs->ReadAllBytes(path, &buf, size))) {
        return luaL_error(L, "module '%s' not found (%s)", name, path);
    }
    const int st = luaL_loadbufferx(L, static_cast<const char*>(buf), size, path, "t");
    fs::FileSystem::FreeBuffer(buf);
    if (st != LUA_OK) return lua_error(L);

    lua_pushstring(L, name);
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) { lua_pop(L, 1); lua_pushboolean(L, 1); }
    lua_pushvalue(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, "zsdk.loaded");
    lua_insert(L, -2);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
    return 1;
}

/* print(): newlib stdout is not wired on Zeebo (the stub path ends in abort), so route to the logger. */
int L_print(lua_State* L) {
    char line[ZSDK_DEBUG_LOG_LINE_LEN];
    size_t pos = 0;
    const int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        size_t len = 0;
        const char* str = luaL_tolstring(L, i, &len);
        if (i > 1 && pos + 1u < sizeof(line)) line[pos++] = '\t';
        for (size_t k = 0; k < len && pos + 1u < sizeof(line); ++k) line[pos++] = str[k];
        lua_pop(L, 1);
    }
    line[pos] = '\0';
    ZSDK_LOG_INFO("LUA", "%s", line);
    return 0;
}

/* ---- app ---- */
int L_app_quit(lua_State* L) {
    GetVM(L)->m_quit = true;
    return 0;
}

int L_app_log(lua_State* L) {
    ZSDK_LOG_INFO("LUA", "%s", luaL_tolstring(L, 1, nullptr));
    return 0;
}

int L_app_sdcard(lua_State* L) {
    LuaVM* vm = GetVM(L);
    lua_pushboolean(L, vm->m_save != nullptr && vm->m_save->IsUsingSdCard());
    return 1;
}

struct Fn { const char* name; lua_CFunction fn; };

void Register(lua_State* L, LuaVM* vm, const char* lib, const Fn* fns, size_t n) {
    lua_createtable(L, 0, static_cast<int>(n));
    for (size_t i = 0; i < n; ++i) {
        lua_pushlightuserdata(L, vm);
        lua_pushcclosure(L, fns[i].fn, 1);
        lua_setfield(L, -2, fns[i].name);
    }
    lua_setglobal(L, lib);
}

} // namespace

LuaVM::LuaVM()
    : m_impl(nullptr), m_batch(nullptr), m_save(nullptr), m_vfs(nullptr), m_used(0u),
      m_limit(kDefaultLuaMemoryLimit), m_quit(false), m_failed(false) {}

LuaVM::~LuaVM() { Shutdown(); }

Result LuaVM::Initialize(uint32_t memoryLimitBytes) {
    if (m_impl != nullptr) return Result::AlreadyInitialized;
    m_limit = memoryLimitBytes;
    m_used = 0u;
    m_quit = false;
    m_failed = false;
    m_impl = new Impl();
    m_impl->L = lua_newstate(LuaAlloc, this);
    if (m_impl->L == nullptr) {
        delete m_impl;
        m_impl = nullptr;
        return Result::OutOfMemory;
    }
    lua_State* L = m_impl->L;
    lua_atpanic(L, LuaPanic);

    luaL_requiref(L, "_G", luaopen_base, 1);          lua_pop(L, 1);
    luaL_requiref(L, "table", luaopen_table, 1);      lua_pop(L, 1);
    luaL_requiref(L, "string", luaopen_string, 1);    lua_pop(L, 1);
    luaL_requiref(L, "math", luaopen_math, 1);        lua_pop(L, 1);
    luaL_requiref(L, "utf8", luaopen_utf8, 1);        lua_pop(L, 1);
    luaL_requiref(L, "coroutine", luaopen_coroutine, 1); lua_pop(L, 1);

    /* Sandbox: scripts must not touch the filesystem or load arbitrary chunks from disk. */
    static const char* const kRemoved[] = {"dofile", "loadfile", "require"};
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "zsdk.loaded");
    for (size_t i = 0; i < sizeof(kRemoved) / sizeof(kRemoved[0]); ++i) {
        lua_pushnil(L);
        lua_setglobal(L, kRemoved[i]);
    }

    static const Fn kGfx[]   = {{"clear", L_gfx_clear}, {"rect", L_gfx_rect}, {"text", L_gfx_text}, {"size", L_gfx_size}};
    static const Fn kInput[] = {{"down", L_input_down}, {"pressed", L_input_pressed}, {"stick", L_input_stick}, {"connected", L_input_connected}};
    static const Fn kSave[]  = {{"write", L_save_write}, {"read", L_save_read},
                                {"write_async", L_save_write_async}, {"status", L_save_status}};
    static const Fn kApp[]   = {{"quit", L_app_quit}, {"log", L_app_log}, {"sdcard", L_app_sdcard}};
    lua_pushcfunction(L, L_print);
    lua_setglobal(L, "print");
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, L_require, 1);
    lua_setglobal(L, "require");
    Register(L, this, "gfx",   kGfx,   sizeof(kGfx) / sizeof(kGfx[0]));
    Register(L, this, "input", kInput, sizeof(kInput) / sizeof(kInput[0]));
    Register(L, this, "save",  kSave,  sizeof(kSave) / sizeof(kSave[0]));
    Register(L, this, "app",   kApp,   sizeof(kApp) / sizeof(kApp[0]));
    return Result::Ok;
}

void LuaVM::Shutdown() {
    if (m_impl != nullptr) {
        if (m_impl->L != nullptr) lua_close(m_impl->L);
        delete m_impl;
        m_impl = nullptr;
    }
}

void LuaVM::Bind(graphics::SpriteBatch2D* batch, fs::SaveGameManager* save) {
    m_batch = batch;
    m_save = save;
}

namespace {
bool PCall(LuaVM* vm, lua_State* L, int nargs, const char* what) {
    if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
        ZSDK_LOG_ERROR("LUA", "%s: %s", what, lua_tostring(L, -1));
        lua_pop(L, 1);
        vm->m_failed = true;
        return false;
    }
    return true;
}
}

Result LuaVM::LoadString(const char* chunkName, const char* source, size_t length) {
    if (m_impl == nullptr) return Result::NotInitialized;
    lua_State* L = m_impl->L;
    if (luaL_loadbufferx(L, source, length, chunkName, "t") != LUA_OK) {
        ZSDK_LOG_ERROR("LUA", "load: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        m_failed = true;
        return Result::Error;
    }
    if (!PCall(this, L, 0, "run")) return Result::Error;
    lua_getglobal(L, "init");
    if (lua_isfunction(L, -1)) {
        if (!PCall(this, L, 0, "init")) return Result::Error;
    } else {
        lua_pop(L, 1);
    }
    return Result::Ok;
}

Result LuaVM::LoadFile(const char* path) {
    if (m_impl == nullptr) return Result::NotInitialized;
    if (m_vfs == nullptr || path == nullptr) return Result::InvalidArg;
    void* buf = nullptr;
    uint32_t size = 0u;
    Result r = m_vfs->ReadAllBytes(path, &buf, size);
    if (Failed(r)) return r;
    r = LoadString(path, static_cast<const char*>(buf), size);
    fs::FileSystem::FreeBuffer(buf);
    return r;
}

void LuaVM::Update(float deltaSeconds) {
    if (m_save != nullptr) m_save->PumpAsync();
    if (m_impl == nullptr || m_failed) return;
    lua_State* L = m_impl->L;
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, deltaSeconds);
        PCall(this, L, 1, "update");
    } else {
        lua_pop(L, 1);
    }
}

void LuaVM::NotifyGamepad(uint8_t index, bool connected) {
    if (m_impl == nullptr || m_failed) return;
    lua_State* L = m_impl->L;
    lua_getglobal(L, "on_pad");
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, index);
        lua_pushboolean(L, connected);
        PCall(this, L, 2, "on_pad");
    } else {
        lua_pop(L, 1);
    }
}

void LuaVM::Draw() {
    if (m_impl == nullptr || m_failed) return;
    lua_State* L = m_impl->L;
    lua_getglobal(L, "draw");
    if (lua_isfunction(L, -1)) {
        PCall(this, L, 0, "draw");
    } else {
        lua_pop(L, 1);
    }
}

} // namespace script
} // namespace zsdk
