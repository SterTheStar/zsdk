# Bindings C, Lua e Rust

A ABI C99 é a única superfície pública da SDK que sobrevive a uma mudança de linguagem. Ela existe porque o núcleo é C++14 sem exceções e sem RTTI: um objeto do núcleo não pode atravessar o limite do módulo BREW nem ser chamado por um `lua_State`, e templates não têm layout estável entre compiladores. A solução é uma camada fina de funções `extern "C"` com tipos de tamanho fixo, que o C chama direto e que o Lua e o Rust chamam por FFI. Você precisa deste capítulo se escreve em C99, Lua ou Rust; se fica em C++, `zsdk/zsdk.hpp` é suficiente e a ABI só importa quando você quer expor algo. Toda afirmação aqui vem dos headers em [include/zsdk/c](../include/zsdk/c), das implementações em [src/c](../src/c) e do verificador [tools/check_bindings.py](../tools/check_bindings.py).

## Visão rápida

| Camada | Onde vive | Papel |
| :--- | :--- | :--- |
| contrato | `include/zsdk/c/*.h` | 302 funções declaradas com `ZSDK_C_API`, 13 enumerações espelhadas |
| ponte | `src/c/*.cpp` | implementa cada função em `extern "C"` chamando o núcleo C++ |
| host | `src/c/host/zsdk_c_host.cpp` | transforma os cinco callbacks de app em um `IAppDelegate` |
| Lua | `bindings/lua/` | módulo `zsdk` com uma tabela por subsistema |
| Rust bruto | `bindings/rust/zsdk-sys/` | `extern "C"` e tipos `#[repr(C)]`, `#![no_std]` |
| Rust seguro | `bindings/rust/zsdk/` | structs com `Drop` e `Result<T, Error>` |
| verificação | `tools/check_bindings.py` | compara declaração, implementação, binding e paridade de enum |

## Pré-requisitos

O binding Lua precisa dos headers de Lua: o CMake usa `third_party/lua` quando `ZSDK_ENABLE_LUA_VM` está ligado, e `ZSDK_LUA_INCLUDE_DIR` serve para apontar uma instalação externa. O binding Rust precisa de `cargo` e, para o alvo de console, da toolchain `armv5te-unknown-linux-gnueabi`. O verificador precisa de Python 3 e roda sem argumentos a partir da raiz do repositório.

## Conceitos

### Por que uma ABI C99 e não uma API C++

Uma função `extern "C"` tem nome sem mangling, chamada sem ajuste de objeto e retorno por registrador segundo a convenção da plataforma. É o único contrato que o `cifc` do BREW, um `lua_State` e um crate `no_std` conseguem consumir sem compartilhar compilador. O preço é que a ABI não pode expor classe, referência, template nem exceção: tudo vira ponteiro opaco, `struct` de campos públicos ou enumeração inteira.

Os tipos opacos são declarados incompletos, como `typedef struct zsdk_fs_t zsdk_fs_t;`. O chamador só manipula o ponteiro, nunca conhece o tamanho, e por isso o layout interno de `FileSystem` não vaza para o header. O ciclo de vida é explícito em pares: `zsdk_fs_create` com `zsdk_fs_destroy`, `zsdk_fs_init` com `zsdk_fs_shutdown`.

### Convenções de nomenclatura e de tipos

Três regras cobrem quase todo o header. A primeira é o prefixo `zsdk_<subsistema>_` mais o verbo, com o substantivo depois quando ajuda: `zsdk_fs_get_free_bytes`, `zsdk_texture_create_from_atitc`. A segunda é que construtores não inicializam e `init` não constrói: `create` devolve ponteiro ou `NULL`, `init` devolve `zsdk_result_t`. A terceira é que erro de domínio volta como `zsdk_result_t` negativo, contagem de bytes volta como `int32_t`, e consulta booleana volta como `int` valendo 0 ou 1.

| Padrão | Exemplo | O que o chamador faz com o retorno |
| :--- | :--- | :--- |
| `zsdk_result_t` | `zsdk_fs_mkdir(fs, "fs:/~/saves")` | compara com `ZSDK_C_OK`, trata negativo como erro |
| ponteiro opaco | `zsdk_texture_create()` | testa `NULL`, libera com o `destroy` correspondente |
| `int` booleano | `zsdk_fs_exists(fs, path)` | usa em condição, nunca guarda como erro |
| `int32_t` de bytes | `zsdk_file_read(file, buf, n)` | negativo é falha, positivo é o que foi lido |
| saída por ponteiro | `zsdk_input_left_stick(in, p, &x, &y)` | passa o endereço, a função não aloca |

Sobre posse: buffer devolvido por `zsdk_fs_read_all_bytes` pertence a você e volta por `zsdk_fs_free_buffer`; string `const char*` devolvida por `zsdk_vkbd_get_text` aponta para o buffer interno do objeto e continua válida até o texto ser editado; `zsdk_gl_context_t`, `zsdk_texture_t` e os demais handles pertencem a você e são liberados com `destroy`.

As enumerações são espelhadas uma a uma, porque o valor numérico atravessa a ABI e o Rust o lê como `#[repr(u32)]` ou `#[repr(i32)]`. Mudar a ordem de um enumerador quebra C, Lua e Rust ao mesmo tempo, e é o que o verificador barra.

### Como a mesma ABI serve três linguagens

O C chama a função como ela é. O Lua não pode: cada função vira um `static int l_<sub>_<nome>(lua_State*)` que converte os argumentos da pilha, chama a ABI e empurra o retorno. Objetos com `create`/`destroy` viram userdata com metatabela e `__gc`, e o conjunto é publicado como `zsdk.<subsistema>.<nome>`.

O Rust tem duas camadas de propósito. `zsdk-sys` é o `extern "C"` literal, sem açúcar, com os tipos incompletos como `#[repr(C)] pub struct zsdk_fs_t { _private: [u8; 0] }`. `zsdk` embrulha isso em tipos que implementam `Drop` e devolvem `Result<T, Error>`, onde `Error` é o próprio `ZsdkResult`. Como o crate é `no_std`, não há alocador: toda saída vai para buffer do chamador, e leitura de arquivo inteiro devolve um `FsBuffer` que libera no `Drop`.

## Referência da API

### Funções por grupo

As contagens abaixo saíram dos headers, com `ZSDK_C_API` como marcador de declaração, e a coluna Lua e Rust mede quantas dessas funções aparecem citadas em cada camada, exatamente o que o verificador exige.

| Header | Funções | Chamadas no Lua | Declaradas no Rust |
| :--- | ---: | ---: | ---: |
| `zsdk_c_core.h` | 14 | 14 | 14 |
| `zsdk_c_log.h` | 3 | 1 | 2 |
| `zsdk_c_input.h` | 36 | 36 | 36 |
| `zsdk_c_audio.h` | 46 | 46 | 46 |
| `zsdk_c_graphics.h` | 122 | 122 | 122 |
| `zsdk_c_fs.h` | 50 | 50 | 50 |
| `zsdk_c_net.h` | 25 | 22 | 25 |
| `zsdk_c_security.h` | 6 | 6 | 6 |
| total | 302 | 297 | 301 |

Cada header se divide por classe, e a soma fecha com a tabela acima. `zsdk_c_graphics.h` concentra 17 funções de `GLContext`, 16 de `Texture`, 8 de `MeshBuffer`, 64 de `Renderer` (estado, matriz e desenho), 11 de `SpriteBatch2D` e 6 de `Frustum`. `zsdk_c_fs.h` cobre 12 de `FileSystem` e enumeração de diretório, 8 de `File`, 7 de `LocalStorage`, 16 de `SaveGameManager` e 7 de `RamCache`. `zsdk_c_audio.h` divide 11 funções em `AudioEngine`, 14 em `PcmMixer`, 13 em `MediaStream` e 8 em `SoundEffect`. `zsdk_c_net.h` traz 9 de `NetworkManager`, DNS e cancelamento, 8 de `HttpClient` e 8 de `Socket`. `zsdk_c_input.h` tem 16 de `InputManager`, 10 de `KeyboardMouseManager` e 10 de `VirtualKeyboard`.

### Enumerações compartilhadas

| C | C++ | Rust | Header |
| :--- | :--- | :--- | :--- |
| `zsdk_result_t` | `zsdk::Result` | `ZsdkResult` | `zsdk_c_types.h` |
| `zsdk_log_level_t` | `zsdk::debug::LogLevel` | `ZsdkLogLevel` | `zsdk_c_types.h` |
| `zsdk_button_t` | `zsdk::input::GamepadButton` | `ZsdkButton` | `zsdk_c_input.h` |
| `zsdk_blend_mode_t` | `zsdk::graphics::BlendMode` | `ZsdkBlendMode` | `zsdk_c_graphics.h` |
| `zsdk_primitive_type_t` | `zsdk::graphics::PrimitiveType` | `ZsdkPrimitiveType` | `zsdk_c_graphics.h` |
| `zsdk_tex_format_t` | `zsdk::graphics::TextureFormat` | `ZsdkTexFormat` | `zsdk_c_graphics.h` |
| `zsdk_tex_filter_t` | `zsdk::graphics::TextureFilter` | `ZsdkTexFilter` | `zsdk_c_graphics.h` |
| `zsdk_tex_wrap_t` | `zsdk::graphics::TextureWrap` | `ZsdkTexWrap` | `zsdk_c_graphics.h` |
| `zsdk_file_mode_t` | `zsdk::fs::FileMode` | `ZsdkFileMode` | `zsdk_c_fs.h` |
| `zsdk_seek_origin_t` | `zsdk::fs::SeekOrigin` | `ZsdkSeekOrigin` | `zsdk_c_fs.h` |
| `zsdk_net_state_t` | `zsdk::net::NetworkState` | `ZsdkNetState` | `zsdk_c_net.h` |
| `zsdk_socket_proto_t` | `zsdk::net::SocketProtocol` | `ZsdkSocketProto` | `zsdk_c_net.h` |
| `zsdk_hash_algo_t` | `zsdk::security::HashAlgorithm` | `ZsdkHashAlgo` | `zsdk_c_security.h` |

`zsdk::input::GamepadButton` não tem o sentinela `Count` que existe em `zsdk_button_t` e em `ZsdkButton`; o verificador remove o último item dos dois lados nesse caso, e é a única exceção de contagem.

## O verificador de paridade

`python tools/check_bindings.py` faz seis verificações e imprime uma linha de resumo. A saída real na árvore atual é:

```text
[ZSDK BINDINGS CHECK OK] 8 subsystems, 13 enums 1:1 parity, 302 C ABI functions, 297/302 Lua, 301/302 Rust-sys, 300/302 Rust-safe, 256/273 C++ methods bridged.
```

As verificações são estruturais: para cada subsistema de `include/zsdk/<nome>` ele exige header `zsdk_c_<nome>.h` incluído no guarda-chuva, implementação `src/c/zsdk_c_<nome>.cpp` listada no `CMakeLists.txt`, arquivo `zsdk_lua_<nome>.c` com `luaopen_zsdk_<nome>` registrado no loader e listado no CMake do binding, módulo em `zsdk-sys` e módulo em `zsdk`. Depois compara enumeração por enumeração entre C++, C e Rust, exige que cada `ZSDK_C_API` declarada tenha definição em `src/c`, e exige que o nome de cada função apareça no código Lua e no Rust.

Uma exclusão explícita existe para funções que não têm binding equivalente. No Lua ficam de fora `zsdk_log`, `zsdk_log_v`, `zsdk_http_get`, `zsdk_http_post` e `zsdk_net_resolve_host`, todas com `va_list` ou callback. No `zsdk-sys` fica de fora só `zsdk_log_v`, e no `zsdk` seguro ficam `zsdk_log` e `zsdk_log_v`. A consequência prática: em Rust use `zsdk_log_msg`, que recebe string já formatada.

O que ele não faz: não valida argumentos, ordem, nem semântica. Uma chamada Lua com os parâmetros trocados passa na verificação, porque a busca é textual pelo nome da função. Também não mede cobertura do núcleo C++: a métrica de 256 de 273 métodos é um indicador de lacuna calculado só sobre os headers monitorados em `TRACKED_CPP_HEADERS`, e `--cpp-gaps` lista, por header, os métodos que nenhum arquivo de `src/c` chama. Subsistemas fora dessa lista, como `fs::Vfs` e `graphics::Frame`, não entram nem no numerador nem no denominador.

## Binding Lua

```lua
local fs = zsdk.fs.new_fs(zsdk_platform_context)

if fs:exists("fs:/~/udata/highscore.txt") then
    local text, err = fs:read_text("fs:/~/udata/highscore.txt")
    if text then
        zsdk.log.info("SAVE", "recorde lido: " .. text)
    else
        zsdk.log.warn("SAVE", "leitura falhou: " .. tostring(err))
    end
end

local f = fs:open("fs:/~/udata/highscore.txt", 2)
if f then
    f:write("1234")
    f:close()
end
```

O `zsdk_platform_context` é uma `lightuserdata` que o host publica como global; é o mesmo `IShell*` que o C chama de `platform_ctx`. O modo `2` é `ZSDK_FILE_CREATE`. Falha de `open` devolve `nil` mais o código, e o `__gc` do userdata garante o `destroy` se você esquecer o `close`.

O módulo se registra com `luaL_requiref(L, "zsdk", luaopen_zsdk, 1)`, e `luaopen_zsdk` monta uma tabela com oito campos, um por subsistema, chamando `luaopen_zsdk_<nome>` para cada. O header [bindings/lua/include/zsdk_lua.h](../bindings/lua/include/zsdk_lua.h) declara também `zsdk_lua_push_input_view`, que empurra um `InputManager` existente como userdata sem dono, usada pelo host para entregar ao script o mesmo input do frame.

## Binding Rust

```rust
use core::ffi::{CStr, c_void};
use zsdk::fs::FileSystem;
use zsdk::log;

pub fn log_manifest(ctx: *mut c_void) {
    let mut fs = match FileSystem::new(ctx) {
        Ok(fs) => fs,
        Err(_) => return,
    };
    let path = CStr::from_bytes_with_nul(b"fs:/~/manifest.txt\0").unwrap();
    if let Ok(bytes) = fs.read_all_bytes(path) {
        log::info(c"FS", if bytes.is_empty() { c"manifest vazio" } else { c"manifest lido" });
    }
}
```

`FileSystem::new` cria e inicializa em uma chamada; o `Drop` faz `shutdown` e `destroy`. `read_all_bytes` devolve `FsBuffer`, que implementa `Deref<Target = [u8]>` e libera com `zsdk_fs_free_buffer` no `Drop`, o que resolve o par `read`/`free` exigido pela ABI sem vazar. O crate inteiro é `#![no_std]` com `panic = "abort"` nos perfis de dev e release, coerente com o `-fno-exceptions` do núcleo.

Para linkar contra a biblioteca gerada pelo CMake, exporte `ZSDK_LINK_ZSDK=1` e, se o build não estiver em `build_zeebo` ou `build_windows`, aponte `ZSDK_LIB_DIR`. O `build.rs` decide o diretório pelo `TARGET` do cargo e, no Windows, acrescenta `user32`, `gdi32`, `opengl32`, `winmm`, `ws2_32`, `advapi32` e `shell32`.

## Como portar uma função nova

1. Declare em `include/zsdk/c/zsdk_c_<sub>.h`, dentro de `ZSDK_C_EXTERN_BEGIN`, começando com `ZSDK_C_API`. Use apenas tipo de tamanho fixo e ponteiro opaco.
2. Implemente em `src/c/zsdk_c_<sub>.cpp`, com `extern "C"` e sem deixar exceção atravessar: converta `Result` de volta para `zsdk_result_t`.
3. Acrescente o handler em `bindings/lua/src/zsdk_lua_<sub>.c` e registre-o na tabela `luaL_Reg` do módulo. O nome precisa existir como `static int`.
4. Declare o `extern "C"` correspondente em `bindings/rust/zsdk-sys/src/<sub>.rs`, com o mesmo nome e os mesmos tipos.
5. Embrulhe em `bindings/rust/zsdk/src/<safe>.rs`, devolvendo `Result` e liberando no `Drop` quando houver recurso.
6. Rode `python tools/check_bindings.py`. A linha de resumo mostra a nova contagem e o mesmo número deve aparecer nas três colunas.
7. Se a função introduz enumeração nova, acrescente a tupla em `SHARED_ENUMS` no verificador para que a paridade numérica seja checada.
8. Atualize o host, se a função precisar de `platform_ctx`: `Application::GetNativePlatformContext()` é a fonte desse ponteiro.

## Limites e armadilhas

`va_list` não atravessa a FFI. `zsdk_log` e `zsdk_log_v` existem para C, mas o `zsdk-sys` exclui `zsdk_log_v` e o Lua chama apenas `zsdk_log_msg`. Em qualquer linguagem que não seja C, formate antes de chamar.

Callback HTTP e DNS não tem binding Lua. `zsdk_http_get`, `zsdk_http_post` e `zsdk_net_resolve_host` recebem ponteiro de função e `user_data`, o que não tem representação barata na pilha do Lua; hoje só o C e o Rust os usam.

O binding Lua não é registrado automaticamente dentro do `LuaVM`. O [TODO.md](../TODO.md) registra que a API do `LuaVM` (`gfx`, `input`, `save`, `app`) é separada do módulo `zsdk`, e que unir as duas exige registrar `luaopen_zsdk` no estado criado pelo `LuaVM` quando `ZSDK_BUILD_LUA_BINDINGS` está ligado.

Sem alocador no Rust, o tamanho do buffer é decisão do chamador. `no_std` significa que você calcula o pior caso antes: uma leitura de save precisa de `max_bytes` explícito, e uma `ZsdkResult` negativa é o único sinal de que o buffer era pequeno demais.

O verificador é textual e não substitui um teste. Ele garante que a função foi citada nas três camadas, não que a chamada funciona; o teste de compilação real da ABI é `tests/c_abi_smoke_test.c`, que o CMake compila como objeto C99 com `ZSDK_ENABLE_TESTS`.

## Veja também

- [01 Build e empacotamento](../docs/01_build_e_empacotamento.md): as macros que produzem o app C, Lua e Rust.
- [08 Scripting Lua](../docs/08_scripting_lua.md): o `LuaVM` e o `require` sobre o `Vfs`, que é outro caminho de script.
- [11 Referência da API](../docs/11_referencia_da_api.md): mapa de headers e códigos de resultado.
- [12 Solução de problemas](../docs/12_solucao_de_problemas.md): o que fazer quando o verificador acusa paridade quebrada.
- [tools/check_bindings.py](../tools/check_bindings.py): a lista de checagens e o dicionário de subsistemas.
