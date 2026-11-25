# Exemplo 03 — Cubo Girando + Painel Debug em Lua

Exemplo mínimo em Lua usando os bindings modulares `zsdk_lua`:
- `zsdk.core.set_overlay_visible(true)` para exibir o painel Debug/RAM/VRAM.
- `zsdk.graphics.draw_colored_mesh(verts, indices)` + `set_perspective`, `look_at`, `rotate` para desenhar 1 cubo 3D colorido girando (com a geometria definida no próprio script Lua).
- `zsdk.log.info(...)` para logar no console do Debug Overlay e no `DBGPRINTF`.

## Como compilar com Lua
```bash
cmake -S . -B build_lua -DZSDK_BUILD_LUA_BINDINGS=ON -DZSDK_LUA_INCLUDE_DIR=/caminho/para/lua/include
cmake --build build_lua
```
No host C/C++, registre o módulo no `lua_State*`:
```c
#include "zsdk_lua.h"
luaL_requiref(L, "zsdk", luaopen_zsdk, 1);
lua_pop(L, 1);
```
E execute `examples/03_lua_spinning_cube/main.lua` chamando `zsdk_on_init(w, h)`, `zsdk_on_update(dt_ms)` e `zsdk_on_render(w, h)`.
