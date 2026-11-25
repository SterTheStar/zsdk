/**
 * @file lua_vm.hpp
 * @brief Embedded Lua 5.4 scripting VM for ZSDK (gamedev without a C++ toolchain).
 *
 * Sandboxed: only base/table/string/math/utf8/coroutine libraries are opened
 * (no io/os/package/debug; dofile/loadfile/require removed). Memory is capped.
 *
 * Script contract (all optional):
 *   function init()        -- once after load
 *   function update(dt)    -- dt in seconds
 *   function draw()        -- between SpriteBatch Begin/End (640x480 virtual)
 *
 * Lua API:
 *   gfx.clear(r,g,b)                      gfx.rect(x,y,w,h,r,g,b[,a])
 *   gfx.text(x,y,str,r,g,b[,scale])       gfx.size() -> 640, 480
 *   input.down(btn) / input.pressed(btn)  input.stick(0|1) -> x, y
 *   input.connected([pad]) -> bool        (optional hooks: on_pad(index, connected))
 *       btn: up down left right a b x y l r start back home
 *   save.write(slot, str) -> bool         save.read(slot) -> str | nil
 *   save.write_async(slot, str) -> bool   save.status() -> "idle"|"pending"|"done"|"failed"
 *   require("name")                        loads script:/name.lua once (dots = subdirs)
 *   app.quit()   app.log(str)   app.sdcard() -> bool
 * Colors are 0..255 integers.
 */

#ifndef ZSDK_SCRIPT_LUA_VM_HPP
#define ZSDK_SCRIPT_LUA_VM_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {

namespace graphics { class SpriteBatch2D; }
namespace fs { class SaveGameManager; class Vfs; }

namespace script {

static const uint32_t kDefaultLuaMemoryLimit = 2u * 1024u * 1024u;
static const uint32_t kLuaSaveMaxBytes       = 4096u;

class LuaVM : public NonCopyable {
public:
    LuaVM();
    ~LuaVM();

    /** @brief Creates the state and opens the sandboxed libs + zsdk bindings. */
    Result Initialize(uint32_t memoryLimitBytes = kDefaultLuaMemoryLimit);
    void   Shutdown();

    /** @brief Attaches the 2D batcher used by gfx.* and the save manager used by save.* (either may be null). */
    void Bind(graphics::SpriteBatch2D* batch, fs::SaveGameManager* save);

    /** @brief Enables require() and LoadFile(): scripts come from the Vfs "script:" mount. */
    void SetVfs(fs::Vfs* vfs) { m_vfs = vfs; }

    /** @brief Loads a script through the Vfs (e.g. "script:/main.lua") and runs it like LoadString(). */
    Result LoadFile(const char* path);

    /** @brief Compiles and runs a chunk, then calls init() if defined. */
    Result LoadString(const char* chunkName, const char* source, size_t length);

    /** @brief Pumps the async save (if any), then calls update(dt) if defined. Errors are logged and disable further calls. */
    void Update(float deltaSeconds);

    /** @brief Calls on_pad(index, connected) if defined; forward IAppDelegate gamepad hooks here. */
    void NotifyGamepad(uint8_t index, bool connected);

    /** @brief Calls draw() if defined. */
    void Draw();

    bool     HasError() const { return m_failed; }
    bool     QuitRequested() const { return m_quit; }
    uint32_t GetMemoryUsed() const { return m_used; }

    struct Impl;
    Impl*                    m_impl;
    graphics::SpriteBatch2D* m_batch;
    fs::SaveGameManager*     m_save;
    fs::Vfs*                 m_vfs;
    uint32_t                 m_used;
    uint32_t                 m_limit;
    bool                     m_quit;
    bool                     m_failed;
};

} // namespace script
} // namespace zsdk

#endif /* ZSDK_SCRIPT_LUA_VM_HPP */
