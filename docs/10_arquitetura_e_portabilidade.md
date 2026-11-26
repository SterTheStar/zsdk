# 10 Arquitetura e portabilidade

O ZSDK existe para resolver um problema específico: escrever um jogo para o Zeebo sem espalhar `IShell`, `IDisplay`, `IHID` e `EGLDisplay` pelo código do jogo, e sem perder a possibilidade de rodar a mesma base de código no Windows durante o desenvolvimento. Tudo no desenho da SDK sai dessa restrição, do orçamento de 32 MB de RAM e VRAM do console e do fato de a GPU implementar OpenGL ES 1.1 de função fixa. Esta página explica como as peças se encaixam e por que cada uma está onde está. Ela não ensina a usar nenhum módulo: para isso existem os capítulos 02 a 09.

## Visão rápida

| Camada | Onde vive | Responsabilidade | Depende de |
| :--- | :--- | :--- | :--- |
| Headers públicos | `include/zsdk/` | contrato estável em C++14 e ABI C99 | nada além de `stdint.h` |
| Serviços portáveis | `src/core/`, `src/graphics/`, `src/fs/`, `src/audio/`, `src/net/`, `src/security/`, `src/debug/`, `src/script/` | lógica que não conhece a plataforma | backends de plataforma |
| Backends | `src/platform/zeebo/`, `src/platform/windows/` | implementa o que toca o hardware | BREW, EGL, GLES, Win32 |
| Ponte C99 | `src/c/`, `include/zsdk/c/` | achata classes em funções com handles opacos | camada C++ portável |
| Bindings | `bindings/lua/`, `bindings/rust/` | expõe a ABI C para Lua e Rust | ponte C99 |

A regra de dependência é de mão única: backend conhece a plataforma, serviço portável conhece o backend, header público não conhece nenhum dos dois. Um `#include <AEEBase.h>` em `include/zsdk/` é bug de arquitetura, não escolha de estilo.

## O hardware que define o desenho

O Zeebo roda um ARM11 a 528 MHz com 32 MB compartilhados entre RAM e VRAM, sobre BREW 4.0.2 com extensões da Qualcomm. Três consequências moldam a SDK inteira.

### Orçamento de memória

Os 32 MB são um teto único: textura, malha, áudio, código carregado e heap do jogo competem pelo mesmo espaço. Por isso o `MemoryTracker` existe desde o primeiro dia e por isso a SDK evita alocação dinâmica em caminho crítico. O próprio binário do app entra nessa conta: o `.mod` do exemplo Lua fica em torno de 398 KB contra 169 KB do exemplo C++, diferença que só o runtime do Lua explica.

O `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` vale `32 MiB` e serve de referência para a barra do profiler e para o modo de orçamento estrito do `MemoryTracker`. Ver [04 Debug e profiler](./04_debug_e_profiler.md).

### Pipeline gráfico de função fixa

A GPU expõe OpenGL ES 1.1, sem shaders programáveis, com matrizes e iluminação por estágio fixo. Isso explica duas decisões que parecem arcaicas e são obrigatórias aqui: a matriz de projeção e a de visão são entregues por `glLoadMatrix` e não por uniform, e os formatos de vértice em [vertex_formats.hpp](../include/zsdk/graphics/vertex_formats.hpp) são fixos (`VertexP3N3C4`, `VertexP3C4U2`, `VertexP3N3U2`) porque não existe layout declarado por shader. Também explica `math_fixed.hpp`: a API aceita `GLfixed` nativamente, então uma SDK que só trabalhasse em `float` pagaria conversão em cada chamada.

O estado da GPU é caro de trocar. O `Renderer` mantém um cache de estado justamente para não reenviar matriz, blend, textura e ponteiro de vértice a cada draw call. Ver [06 Gráficos 3D e 2D](./06_graficos_3d_e_2d.md).

### Armazenamento e a guarda da NAND

O sistema de arquivos do console tem uma NAND de sistema que não pode ser tocada pelo jogo e um diretório privado por aplicativo em `fs:/~/`. A SDK aplica uma trava de compilação, `ZSDK_NAND_WRITE_GUARD`, que recusa escrita, remoção e criação de diretório fora de `fs:/~/` e `fs:/card0/`. Ela existe porque um `Remove` no caminho errado transforma um bug de lógica em console inutilizável.

O cartão SD aparece como `fs:/card0/` e é tratado como destino opcional de save. O `TODO.md` registra que esse caminho não está comprovado em hardware, então trate o save em SD como não suportado até validar no console. Ver [07 Filesystem e SaveGame](./07_filesystem_e_savegame.md).

## As camadas da SDK

### Headers públicos

`include/zsdk/zsdk.hpp` é o header guarda-chuva: inclui os 44 headers de módulo e nada da plataforma. Um arquivo de jogo que inclui só esse header compila tanto para ARM quanto para Windows, e é essa propriedade que dá sentido ao resto da arquitetura.

Os headers usam `namespace zsdk` com sub-namespaces por módulo (`zsdk::graphics`, `zsdk::fs`, `zsdk::input` e assim por diante). Os tipos comuns (`Result`, `Vec2`, `Vec3`, `Rect`, `Color`) ficam no namespace raiz, em [types.hpp](../include/zsdk/core/types.hpp). Não há exceção nem RTTI no código: todo erro atravessa a API como `Result`, um `enum class` de `int32_t` com valores negativos, e as funções auxiliares `Succeeded` e `Failed` evitam comparação textual em cada chamada.

### Serviços portáveis

Aqui mora o comportamento que não depende de plataforma: o cache de estado do `Renderer`, o cálculo de malha e textura, o formato de save com CRC32, o laço do `PcmMixer`, o parser do `HttpClient`, o `LuaVM`. Nenhum desses arquivos inclui header de BREW. Quando precisam de algo do sistema, chamam uma função do backend.

### Backends de plataforma

`src/platform/zeebo/` tem 19 arquivos, um por área (`zeebo_core.cpp`, `zeebo_gl_context.cpp`, `zeebo_input.cpp`, `zeebo_fs.cpp`, `zeebo_socket.cpp` e assim por diante). Eles traduzem a API da SDK para BREW: `IShell`, `IDisplay`, `IHID`, `IFile`, `ISocket`, `IThread`, `IBattery`.

`src/platform/windows/` tem um único arquivo, `windows_backend.cpp`. Ele cobre o suficiente para o jogo rodar no PC com OpenGL e Win32, mas não é um porte completo: parte das funções é stub que devolve `Result::NotSupported`. O `TODO.md` registra que `Thread` e outros pontos estão nesse estado. Use o backend Windows para iterar em lógica e renderização, não como prova de que a API funciona no console.

### A ponte C99 e os bindings

A ABI C99 em `include/zsdk/c/` é a base comum de três linguagens. Ela transforma cada classe em struct opaca mais funções `zsdk_modulo_acao`, com nomes estáveis, e o verificador `tools/check_bindings.py` compara a ABI contra os bindings Lua e Rust para garantir que nenhuma função some pelo caminho. Ver [Bindings](../bindings/README.md).

A existência dessa ponte é o que permite `zsdk_add_c_app`, `zsdk_add_lua_app` e `zsdk_add_rust_app` gerarem apps completos sem uma linha de C++ no diretório do jogo.

## O papel dos headers em relação à performance

Três macros aparecem em quase todos os headers e não são decoração.

`ZSDK_FORCE_INLINE` resolve para `__forceinline` no MSVC e `inline __attribute__((always_inline))` no GCC e Clang. É usada em acessores curtos para não pagar chamada em código que roda 60 vezes por segundo.

`ZSDK_ENABLE_DEBUG_OVERLAY` muda o binário, não só o comportamento. Com ela em `0`, o overlay e os logs `INFO` e `WARN` são removidos na compilação, não desligados em tempo de execução. É a diferença entre um release leve e um release que carrega um painel de debug inteiro.

`ZSDK_NAND_WRITE_GUARD` insere a checagem de caminho no `FileSystem`. Desligá-la é possível e documentado no capítulo 07, mas significa assumir a responsabilidade por qualquer escrita no caminho errado.

## Determinismo e custo previsível

O desenho evita três fontes clássicas de variação: exceções (que exigem tabela de unwind e tornam o caminho de erro imprevisível), RTTI (que infla o binário) e alocação por frame nos subsistemas críticos. `InputManager`, `DebugOverlay` e `SaveGameManager` são escritos para não chamar o alocador depois da inicialização.

O controle de frame fica no `FrameTimer` e no `FpsCap` do `Application`: o laço dorme o suficiente para não passar do alvo, o que no console reduz consumo e no Windows mantém o comportamento parecido com o do hardware. O delta entregue a `OnUpdate` é em milissegundos e já vem limitado, para que uma pausa longa do sistema não injete um salto gigante na simulação. Ver [03 Core e assíncrono](./03_core_e_async.md).

## Limites conhecidos

A honestidade sobre o que não foi validado faz parte da documentação. Os pontos abaixo estão abertos no `TODO.md` do repositório e devem ser tratados como risco, não como detalhe:

| Limite | Efeito prático |
| :--- | :--- |
| Nada foi executado em hardware Zeebo | os 60 FPS observados vêm do emulador HLE, não medem o console |
| Headers reais de HID ausentes | o mapa de botões e a normalização dos sticks nunca foram conferidos |
| Build MSVC não validado | a build Windows comprovada é MinGW-w64 |
| Stack de 32 KB por thread | arrays grandes em pilha são risco real, não teórico |
| Save em `fs:/card0/` sem prova | o caminho de SD pode não ser gravável no firmware |
| Memória do Lua fora do `MemoryTracker` | o consumo do script não aparece no overlay |
| Empacotamento no Linux | o pós-build chama `elf2mod.exe` e falha fora do Windows |

## Por que não há camada de abstração sobre a ABI C

Uma alternativa comum seria fazer os bindings Lua e Rust falarem com uma camada C separada, escrita à mão. A SDK não faz isso: a ABI C99 é a própria API, mantida em paralelo aos headers C++ e verificada por script. O custo é disciplina, porque cada função nova precisa aparecer nos três lugares. O ganho é que não existe uma segunda semântica para divergir da primeira, e o verificador transforma divergência em teste vermelho em vez de bug silencioso.

## Veja também

- [02 Build e empacotamento](./02_build_e_empacotamento.md) para transformar essas camadas em `.mod` e `.mif`.
- [03 Core e assíncrono](./03_core_e_async.md) para o ciclo de vida do `Application` e o laço de frame.
- [06 Gráficos 3D e 2D](./06_graficos_3d_e_2d.md) para o cache de estado e os formatos de vértice.
- [07 Filesystem e SaveGame](./07_filesystem_e_savegame.md) para a guarda da NAND em detalhe.
- [config.hpp](../include/zsdk/core/config.hpp) e [types.hpp](../include/zsdk/core/types.hpp) como fonte das macros e dos códigos de erro citados aqui.
