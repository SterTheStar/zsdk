# 02 Core, ciclo de vida, tempo e concorrência

O núcleo do ZSDK entrega o que todo app do Zeebo precisa antes do primeiro pixel: um ponto de entrada que esconde `AEEClsCreateInstance` e `WinMain`, um laço de quadro com alvo fixo de 30 ou 60 FPS, um relógio monotônico, matemática inteira compatível com o `GLfixed` do OpenGL ES 1.1, threads e callbacks assíncronos sobre o BREW, e consultas de bateria, memória e idioma. Nenhuma dessas assinaturas expõe tipo do BREW, então o mesmo `main.cpp` compila para o console e para o Windows. Use este capítulo ao montar o esqueleto do jogo: o delegate, o alvo de FPS e o tratamento de suspensão são decisões que afetam todos os outros módulos.

## Visão rápida

| Símbolo | Papel |
| :--- | :--- |
| `zsdk::Application` | Instância única que guarda configuração, delegate, `InputManager` e `FrameTimer` |
| `zsdk::IAppDelegate` | Interface que o jogo implementa: `OnInit`, `OnUpdate`, `OnRender`, `OnShutdown` |
| `ZSDK_IMPLEMENT_APP` | Macro que define o delegate estático e `zsdk::CreateUserApp` |
| `zsdk::FpsCap` | Presets de 30 e 60 FPS |
| `zsdk::Clock` | Uptime monotônico e tempo de parede |
| `zsdk::FrameTimer` | Delta de quadro clampado e FPS medido numa janela de 500 ms |
| `zsdk::Fixed16` | Escalar 16.16 e helpers de seno, cosseno, multiplicação e divisão |
| `zsdk::Thread` | Encapsulamento de `IThread` do BREW |
| `zsdk::AsyncCallback`, `Signal`, `SignalBus` | Callback adiado e sinais 1:1 e fan-out |
| `zsdk::SystemInfo` | RAM livre, filesystem livre, bateria, backlight e código de idioma |

## Pré-requisitos

O build precisa estar funcionando, com `ZSDK_BUILD_ZEEBO` ligado para o console ou desligado para o Windows, como descreve [01 Build e empacotamento](./01_build_e_empacotamento.md). Todo o núcleo chega por `#include "zsdk/zsdk.hpp"`. O CMake já compila a SDK com `-fno-exceptions` e `-fno-rtti` (MSVC usa `/GR- /EHs-c-`), então erro se propaga por `zsdk::Result`, nunca por `throw`.

## Conceitos

### O delegate e o ponto de entrada

O jogo não escreve `AEEClsCreateInstance` nem `WinMain`. Ele implementa `zsdk::IAppDelegate` e registra a classe com `ZSDK_IMPLEMENT_APP`, que gera duas coisas: um objeto estático `g_zsdkUserDelegate` da sua classe e a função `zsdk::CreateUserApp(AppConfig&)`, o único símbolo que o backend chama para obter o jogo e a configuração. Ela preenche os quatro campos de `AppConfig` com os argumentos da macro, usa `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` no orçamento de memória e devolve o endereço do objeto estático.

Duas consequências práticas saem dessa definição. A primeira: `DelegateClass` precisa ter construtor padrão sem argumentos, porque o objeto estático é construído antes de qualquer `OnInit`. A segunda: o delegate é único por binário, então um app que queira dois modos de jogo escolhe dentro de `OnInit`, não com duas instâncias da classe.

Em seguida o backend cria o `Application`, chama `Configure` com a configuração devolvida e só então dispara os callbacks. `Application::Configure` também repassa `memoryBudgetBytes` para `debug::MemoryTracker::SetBudgetBytes`, o que faz o orçamento padrão de 32 MiB aparecer no overlay de memória.

### O laço do BREW: callbacks de timer

No Zeebo não existe laço. O BREW chama o handler de eventos do applet, e o ZSDK instala um timer de um disparo (`ISHELL_SetTimer`) para produzir cada quadro. A ordem dentro de `EVT_APP_START`, em `zeebo_core.cpp`:

1. Lê `AEEDeviceInfo` para descobrir `cxScreen` e `cyScreen`.
2. Zera o `FrameTimer` e inicializa o `InputManager` com o `IShell*`.
3. Inicializa o `DebugOverlay` e aplica a visibilidade vinda da configuração.
4. Chama `OnInit()`. Se devolver erro, o handler retorna `FALSE` e o applet é fechado.
5. Calcula `frameIntervalMs = 1000 / fps`, marca o timer como ativo e agenda o primeiro quadro.

Cada disparo do timer faz, nesta ordem: `FrameTimer::Tick`, `InputManager::Update`, `Application::DispatchGamepadChanges`, `DebugOverlay::Update`, `OnUpdate(dt)` e, se o jogo não pediu saída, `OnRender()`. Depois desconta o tempo de trabalho do intervalo alvo e reagenda:

```cpp
const uint32_t nowMs = GETUPTIMEMS();
const uint32_t workMs = (nowMs >= frameStartMs) ? (nowMs - frameStartMs) : 0u;
uint32_t nextDelayMs = (ps->frameIntervalMs > workMs)
                           ? (ps->frameIntervalMs - workMs)
                           : 1u;
if (nextDelayMs == 0u) {
    nextDelayMs = 1u;
}
ISHELL_SetTimer(ps->applet.m_pIShell,
                static_cast<int32>(nextDelayMs),
                FrameTickCallback, ps);
```

Isso significa que o intervalo é respeitado entre o início de um quadro e o início do próximo, não somado ao trabalho. Um quadro que já gastou mais que o intervalo inteiro reagenda com 1 ms e não tenta recuperar o atraso.

Os eventos de sistema fecham o ciclo. `EVT_APP_STOP` cancela o timer e chama `OnShutdown`, seguido de `DebugOverlay::Shutdown` e `InputManager::Shutdown`. `EVT_APP_SUSPEND` cancela o timer, chama `OnSuspend` e libera o `IHID`; `EVT_APP_RESUME` reinicializa o `IHID`, reinicia o `FrameTimer`, chama `OnResume` e reagenda. `EVT_APP_NO_SLEEP` responde `TRUE` enquanto o jogo está ativo, o que evita o rebaixamento de clock do BREW. Uma tecla `AVK_END` chamando `RequestQuit` e `ISHELL_CloseApplet` encerra o app.

### O laço do Windows: deadline e bomba de mensagens

No Windows o backend cria uma janela de 640x480, um `HDC`, inicializa o `InputManager` com o `HWND` e chama `OnInit()`. Se qualquer um dos dois falhar, a janela é destruída e `Run` retorna 1 sem chamar `OnShutdown`. Com tudo pronto, o backend sobe a resolução do timer do sistema para 1 ms (`timeBeginPeriod(1)`), zera o `FrameTimer` e entra no laço:

1. Drena a fila com `PeekMessageA`. `WM_QUIT` vira `RequestQuit`.
2. Se a janela não está suspensa: `Tick`, `Update` da entrada, `DispatchGamepadChanges`, `DebugOverlay::Update`, `OnUpdate` e `OnRender`.
3. Avança o deadline em `frequência / fps` ticks do `QueryPerformanceCounter`. Se o atraso passou de dois quadros, o deadline é ressincronizado para o agora, para não haver rajada de recuperação.
4. Dorme até faltarem 2 ms, então gira no `QueryPerformanceCounter` até o deadline.

`WM_ACTIVATE` é o gancho de suspensão: perder o foco chama `OnSuspend`, ganhar chama `OnResume`. Diferente do Zeebo, o `InputManager` continua inicializado durante a suspensão e o laço continua girando, apenas sem chamar `OnUpdate` nem `OnRender`. `WM_CLOSE` e `WM_DESTROY` marcam `RequestQuit`. Ao sair do laço, o backend restaura `timeEndPeriod(1)` e só então chama `OnShutdown`, `DebugOverlay::Shutdown` e `InputManager::Shutdown`.

A diferença estrutural entre os dois backends está em quem controla o ritmo. No BREW, o app devolve o controle ao sistema e o timer decide quando chamar de novo, com resolução de 1 ms e sem garantia de pontualidade. No Windows, o app controla o tempo com um deadline absoluto e só devolve o controle ao sistema entre quadros. Nos dois casos o jogo vê a mesma sequência de callbacks.

### Por que ponto fixo, threads e sinais existem

A GPU do Zeebo recebe coordenadas em `GLfixed` no OpenGL ES 1.1, e converter `float` para inteiro em cada vértice custa trabalho dentro de um laço de 16 ms. `Fixed16` é `int32_t` em formato 16.16, binário-compatível com `GLfixed`, então o mesmo número serve para calcular e para enviar. Soma e subtração são instruções inteiras, e `FixedSinDeg` calcula o seno com a aproximação inteira de Bhaskara I, que o header descreve como zero operações de ponto flutuante no ARM11.

Threads existem porque o app do Zeebo tem stack pequeno e porque gravação em NAND pode travar. O wrapper `Thread` cria um `IThread` do BREW com o tamanho de stack pedido, o que permite tirar trabalho pesado do quadro principal. Sinais existem porque o BREW é cooperativo: APIs assíncronas devolvem antes de terminar e avisam por callback. `AsyncCallback` agenda uma função para rodar num passo posterior do mesmo apartamento, e `Signal` faz o mesmo com semântica de evento, com um `SignalBus` para acordar vários interessados de uma vez. Nada disso cria um segundo fluxo de execução na lógica do jogo: os callbacks rodam no laço do BREW.

### Assinatura e configuração

`Application` não é criado pelo jogo. O backend instancia (`static zsdk::Application g_zsdkApp` no Zeebo, uma variável de pilha em `Run` no Windows), chama `Configure` e expõe o ponteiro por `GetInstance()`, que nunca é nulo dentro dos callbacks. A configuração vem de `AppConfig`:

```cpp
struct AppConfig {
    const char* name;
    uint16_t    targetFps;
    uint32_t    memoryBudgetBytes;
    bool        enableDebugOverlay;
};
```

Não há `Shutdown` para chamar em `Application`. O que precisa ser liberado são os recursos do jogo, dentro de `OnShutdown`, e a ordem importa: libere primeiro os objetos que dependem de contexto gráfico (`GLContext`, filas de sprite) e por último o que apenas lê arquivos. No Zeebo o `InputManager` é desligado pelo backend depois do seu `OnShutdown`; não o desligue também.

## Uso passo a passo

### Registrar o app

Um `main.cpp` mínimo, com o ciclo completo:

```cpp
#include "zsdk/zsdk.hpp"

class MeuJogo : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        auto* app = zsdk::Application::GetInstance();
        ZSDK_LOG_INFO("APP", "tela %dx%d", app->GetScreenWidth(), app->GetScreenHeight());
        return zsdk::Result::Ok;
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        auto* app = zsdk::Application::GetInstance();
        const zsdk::input::GamepadState& pad = app->GetInput().GetGamepad(0);
        if (pad.IsPressed(zsdk::input::GamepadButton::Home)) {
            app->RequestQuit();
        }
        m_elapsedMs += deltaTimeMs;
    }

    void OnRender() override {}

    void OnShutdown() override {
        ZSDK_LOG_INFO("APP", "rodou %u ms", m_elapsedMs);
    }

private:
    uint32_t m_elapsedMs = 0u;
};

ZSDK_IMPLEMENT_APP(MeuJogo, "MeuJogo", 30, true)
```

O nome é usado como título da janela no Windows; no Zeebo ele fica só na configuração, porque o nome exibido vem do `.mif`. O terceiro argumento é o alvo de FPS, o quarto é a visibilidade inicial do overlay. O exemplo `01_spinning_cube` usa `ZSDK_PLATFORM_ZEEBO` para escolher 30 no console e 60 no Windows.

### Ler tempo e trocar o alvo de FPS

`FrameTimer` é acessado por referência constante, então o jogo lê o delta e o FPS, mas não reescreve o relógio:

```cpp
void OnRender() override {
    auto* app = zsdk::Application::GetInstance();
    const zsdk::FrameTimer& timer = app->GetTimer();

    static uint32_t s_report = 0u;
    s_report += timer.GetDeltaMs();
    if (s_report >= 1000u) {
        s_report = 0u;
        ZSDK_LOG_INFO("APP", "%.1f FPS, dt %.3f s", timer.GetFps(), timer.GetDeltaSeconds());
    }

    /* 30 FPS em TV CRT, 60 FPS quando o jogo cabe no orçamento de 16 ms. */
    if (app->GetTargetFps() != 30u) {
        app->SetFpsCap(zsdk::FpsCap::Fps30);
    }
}
```

`SetFpsCap` é um atalho de `SetTargetFps`: no Zeebo ele recalcula o intervalo do timer na hora, no Windows o laço lê `GetTargetFps()` a cada quadro e ajusta o deadline. Valores fora de 30 e 60 são aceitos pelo tipo `uint16_t`, mas ficam sem preset; `FpsCap` existe para deixar a intenção explícita.

### Usar ponto fixo numa rotação

Ângulos em graus, coordenadas em 16.16 e nenhuma conversão para `float` no meio do caminho:

```cpp
void OnRender() override {
    static zsdk::Fixed16 s_angleDeg = zsdk::IntToFixed(0);
    s_angleDeg += zsdk::FloatToFixed(0.75f);
    if (s_angleDeg >= zsdk::IntToFixed(360)) {
        s_angleDeg -= zsdk::IntToFixed(360);
    }

    const zsdk::Fixed16 px = zsdk::FixedMul(zsdk::FixedSinDeg(s_angleDeg), zsdk::kFixedHalf);
    const zsdk::Fixed16 py = zsdk::FixedMul(zsdk::FixedCosDeg(s_angleDeg), zsdk::kFixedHalf);

    ZSDK_LOG_INFO("FIXED", "px %ld py %ld", static_cast<long>(px), static_cast<long>(py));
}
```

`FixedSinDeg` já normaliza o ângulo para uma volta completa, então ângulos acumulados além de 360 funcionam. Para plotar ou comparar com `float`, converta só na borda com `FixedToFloat`, nunca dentro de um laço por vértice.

### Tirar trabalho do quadro com Thread

O entry point é uma função livre com um `void*`, e o `stackSizeBytes` é o padrão de 16 KiB se você não passar nada:

```cpp
struct LoadJob {
    zsdk::Thread  thread;
    volatile bool done;
};

static void LoadEntry(void* userData) {
    LoadJob* job = static_cast<LoadJob*>(userData);
    /* Trabalho que não pode somar ao orçamento do quadro: abrir arquivos, decodificar. */
    job->done = true;
}

zsdk::Result StartJob(LoadJob& job) {
    job.done = false;
    return job.thread.Start(zsdk::Application::GetInstance()->GetNativePlatformContext(),
                            LoadEntry, &job, 24u * 1024u);
}
```

`Thread::~Thread` chama `Join()`, então o dono do objeto espera o fim ao sair de escopo. Use `IsRunning()` para saber se o entry ainda não retornou e `Join()` explicitamente quando precisar do resultado antes de continuar.

### Agendar callbacks e sinais

`AsyncCallback` adia uma função para um passo posterior do laço, sem alocar por disparo. Ele cancela o pendente na destruição, o que evita chamar ponteiro pendurado:

```cpp
class MusicFade {
public:
    zsdk::Result Initialize(void* platformContext) {
        return m_callback.Initialize(platformContext, MusicFade::OnFadeDone, this);
    }
    zsdk::Result Start() { return m_callback.Schedule(); }
    bool Pending() const { return m_callback.IsPending(); }

private:
    static void OnFadeDone(void* userData) {
        static_cast<MusicFade*>(userData)->m_finished = true;
    }

    zsdk::AsyncCallback m_callback;
    bool                m_finished = false;
};
```

`Signal` segue o mesmo modelo, mas o disparo é um evento e a fonte precisa ser reabilitada depois de cada entrega. `SignalBus` agrupa sinais para acordar todos de uma vez:

```cpp
zsdk::Signal frameReady;
zsdk::Signal audioReady;
zsdk::SignalBus bus;

void Setup(void* ctx) {
    frameReady.Initialize(ctx, OnFrame, nullptr);
    audioReady.Initialize(ctx, OnAudio, nullptr);

    bus.Initialize(ctx);
    bus.Add(frameReady);
    bus.Add(audioReady);
}

void OnFrame(void*) {}
void OnAudio(void*) {}

void Tick() {
    bus.Strobe();               /* acorda os dois sinais habilitados */
    frameReady.Enable();        /* rearma depois da entrega */
    audioReady.Enable();
}
```

`Strobe` equivale a `Set` seguido de `Clear` e entrega só aos sinais habilitados no momento. Se um `Signal` foi entregue e você não chamar `Enable`, os próximos `Set` ficam sem efeito.

### Consultar hardware

`SystemInfo` mistura chamadas sem contexto e chamadas que precisam do `IShell*` ou `HWND` devolvido por `GetNativePlatformContext()`:

```cpp
void ReportHardware() {
    zsdk::SystemMemoryInfo mem;
    if (zsdk::Succeeded(zsdk::SystemInfo::GetMemoryInfo(mem))) {
        ZSDK_LOG_INFO("SYS", "RAM livre %u de %u, maior bloco %u",
                      mem.freeRamBytes, mem.totalRamBytes, mem.largestFreeBlockBytes);
    }

    auto* app = zsdk::Application::GetInstance();
    void* ctx = app->GetNativePlatformContext();

    zsdk::BatteryStatus bat;
    if (zsdk::Succeeded(zsdk::SystemInfo::GetBatteryStatus(ctx, bat)) && bat.present) {
        ZSDK_LOG_INFO("SYS", "bateria %u%% carregando %d", bat.levelPercent, bat.charging ? 1 : 0);
    }

    zsdk::SystemInfo::SetBacklight(ctx, true, 80u);
    ZSDK_LOG_INFO("SYS", "idioma 0x%08X", zsdk::SystemInfo::GetLanguageCode(ctx));
}
```

Chame isso no `OnInit` ou a cada alguns segundos, nunca por quadro: cada consulta cria e libera instâncias BREW (`IBattery`, `IBacklight`) e lê o heap do sistema.

### Ajustar o build com as macros de config.hpp

`config.hpp` decide comportamento em tempo de compilação e todas as macros aceitam `-D` do CMake, porque são guardadas por `#ifndef`:

| Macro | Padrão | Efeito |
| :--- | :--- | :--- |
| `ZSDK_PLATFORM_ZEEBO` | `1` fora de `_WIN32` | Seleciona todo o backend BREW |
| `ZSDK_PLATFORM_WINDOWS` | `1` com `_WIN32` | Seleciona o backend Win32 |
| `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` | `33554432` | Orçamento passado ao `MemoryTracker` |
| `ZSDK_MAX_GAMEPADS` | `4` | Pads varridos por `DispatchGamepadChanges` |
| `ZSDK_NAND_WRITE_GUARD` | `1` | Bloqueia escrita fora de `fs:/card0/` e `fs:/~/` |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | `1`, ou `0` com `NDEBUG` | Compila o overlay e o contador de draw calls |
| `ZSDK_ENABLE_MEMORY_TRACKING` | Igual ao overlay | Contabiliza alocação por subsistema |
| `ZSDK_ENABLE_LOGGING` | `1` | Compila `ZSDK_LOG_INFO` e `ZSDK_LOG_WARN` |
| `ZSDK_DEBUG_LOG_CAPACITY` | `32` | Linhas no buffer circular do logger |
| `ZSDK_DEBUG_LOG_LINE_LEN` | `96` | Caracteres por linha, incluindo o terminador |

A detecção de plataforma é automática: `_WIN32` sem `AEE_SIMULATOR` vira Windows, qualquer outro caso (incluindo o simulador BREW) vira Zeebo. Como o CMake passa `ZSDK_PLATFORM_ZEEBO` e `ZSDK_PLATFORM_WINDOWS` explicitamente por alvo, o valor definido ali ganha da detecção.

## Referência da API

### `IAppDelegate`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `OnInit` | `Result OnInit()` | Erro aborta o applet no Zeebo e fecha a janela no Windows |
| `OnUpdate` | `void OnUpdate(uint32_t deltaTimeMs)` | Delta já clampado em `[1..250]` ms |
| `OnRender` | `void OnRender()` | Não é chamado se `RequestQuit` ocorreu durante o `OnUpdate` |
| `OnSuspend` | `void OnSuspend()` | Zeebo: `IHID` liberado depois. Windows: perda de foco |
| `OnResume` | `void OnResume()` | Zeebo: `IHID` recriado e timer reiniciado antes |
| `OnGamepadConnected` | `void OnGamepadConnected(uint8_t playerIndex)` | Dispara para cada pad já conectado no primeiro quadro |
| `OnGamepadDisconnected` | `void OnGamepadDisconnected(uint8_t playerIndex)` | Pause o jogo e peça para reconectar |
| `OnShutdown` | `void OnShutdown()` | Chamado uma vez, com a entrada ainda viva no Windows |

### `Application`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `GetInstance` | `static Application* GetInstance()` | `nullptr` só antes de o backend construir a instância |
| `Configure` | `void Configure(const AppConfig&, IAppDelegate*)` | Chamado pelo backend, não pelo jogo |
| `GetInput` | `input::InputManager& GetInput()` | Versão `const` disponível |
| `GetTimer` | `const FrameTimer& GetTimer() const` | Somente leitura |
| `GetNativePlatformContext` | `void* GetNativePlatformContext() const` | `IShell*` no Zeebo, `HWND` no Windows |
| `RequestQuit` | `void RequestQuit()` | Encerra no fim do quadro corrente |
| `IsQuitRequested` | `bool IsQuitRequested() const` | Consultável no meio do `OnUpdate` |
| `GetScreenWidth` | `int32_t GetScreenWidth() const` | 640 se a plataforma ainda não informou |
| `GetScreenHeight` | `int32_t GetScreenHeight() const` | 480 se a plataforma ainda não informou |
| `SetTargetFps` | `void SetTargetFps(uint16_t fps)` | `0` vira 60 no Zeebo e sem limite no Windows |
| `SetFpsCap` | `void SetFpsCap(FpsCap cap)` | Atalho para `SetTargetFps` |
| `GetTargetFps` | `uint16_t GetTargetFps() const` | Valor atual da configuração |
| `DispatchGamepadChanges` | `void DispatchGamepadChanges()` | Chamado pelos backends após `InputManager::Update` |

### `Clock` e `FrameTimer`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Clock::GetUptimeMs` | `static uint32_t GetUptimeMs()` | `GETUPTIMEMS` no Zeebo, QPC desde a primeira chamada no Windows |
| `Clock::GetWallTimeMs` | `static uint32_t GetWallTimeMs()` | `GETTIMEMS` no Zeebo, `GetTickCount` no Windows |
| `FrameTimer::Reset` | `void Reset()` | Zera base, delta em 16 ms e FPS em 60 |
| `FrameTimer::Tick` | `uint32_t Tick()` | Devolve o delta e alimenta a janela de FPS |
| `FrameTimer::GetDeltaMs` | `uint32_t GetDeltaMs() const` | Último delta clampado |
| `FrameTimer::GetDeltaSeconds` | `float GetDeltaSeconds() const` | Delta em segundos |
| `FrameTimer::GetFps` | `float GetFps() const` | Calculado a cada 500 ms |

### Ponto fixo

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Fixed16` | `typedef int32_t Fixed16` | Formato 16.16, compatível com `GLfixed` |
| `kFixedOne` | `0x00010000` | Representa 1.0 |
| `kFixedHalf` | `0x00008000` | Representa 0.5 |
| `IntToFixed` | `Fixed16 IntToFixed(int32_t v)` | `v << 16`; seguro para valor absoluto abaixo de 32768 |
| `FixedToInt` | `int32_t FixedToInt(Fixed16 f)` | `f >> 16`, trunca em direção ao negativo |
| `FloatToFixed` | `Fixed16 FloatToFixed(float v)` | Converte na borda do sistema, não no laço |
| `FixedToFloat` | `float FixedToFloat(Fixed16 f)` | Inverso de `FloatToFixed` |
| `FixedMul` | `Fixed16 FixedMul(Fixed16 a, Fixed16 b)` | Usa `int64_t` intermediário |
| `FixedDiv` | `Fixed16 FixedDiv(Fixed16 a, Fixed16 b)` | Sem guarda de divisão por zero |
| `FixedSinDeg` | `Fixed16 FixedSinDeg(Fixed16 angleDeg)` | Bhaskara I inteiro, erro máximo 0.0016 |
| `FixedCosDeg` | `Fixed16 FixedCosDeg(Fixed16 angleDeg)` | `FixedSinDeg(angleDeg + 90)` |

### `Thread`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Start` | `Result Start(void* platformContext, ThreadEntryFn entry, void* userData, uint32_t stackSizeBytes = 16384u)` | `platformContext` é o `IShell*` |
| `Join` | `void Join()` | Bloqueia até o entry retornar; chamado no destrutor |
| `SleepMs` | `void SleepMs(uint32_t ms)` | Zeebo: ignora `ms` e suspende a thread; Windows: `Sleep(ms)` |
| `IsRunning` | `bool IsRunning() const` | `false` no Windows, que roda tudo inline |

### Sinais

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `AsyncCallback::Initialize` | `Result Initialize(void* platformContext, SignalCallbackFn callback, void* userData)` | `SignalCallbackFn` é `void (*)(void*)` |
| `AsyncCallback::Schedule` | `Result Schedule()` | Agenda via `ISHELL_Resume` |
| `AsyncCallback::Cancel` | `void Cancel()` | Seguro quando não há pendente |
| `AsyncCallback::IsPending` | `bool IsPending() const` | Usa `CALLBACK_IsQueued` |
| `AsyncCallback::GetNativeCallback` | `void* GetNativeCallback() const` | `AEECallback*` para APIs assíncronas do BREW |
| `Signal::Initialize` | `Result Initialize(void* platformContext, SignalCallbackFn callback, void* userData)` | Cria por `AEECLSID_SignalCBFactory` |
| `Signal::Set` | `Result Set()` | Entrega assíncrona ao callback |
| `Signal::Enable` | `Result Enable()` | Rearma depois da entrega |
| `Signal::Detach` | `Result Detach()` | `Set` posteriores viram no-op |
| `Signal::IsValid` | `bool IsValid() const` | `true` só com `ISignal` e `ISignalCtl` vivos |
| `SignalBus::Add` / `Remove` | `Result Add(Signal&)` / `Result Remove(Signal&)` | Registra o `ISignal` do sinal |
| `SignalBus::Set` / `Clear` / `Strobe` | `Result Set()`, `Result Clear()`, `Result Strobe()` | `Strobe` acorda os habilitados e volta ao estado limpo |

### `SystemInfo`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `GetMemoryInfo` | `static Result GetMemoryInfo(SystemMemoryInfo& outInfo)` | `GETRAMFREE` mais `GETFSFREE` no Zeebo |
| `GetBatteryStatus` | `static Result GetBatteryStatus(void* platformContext, BatteryStatus& outStatus)` | Windows devolve `present = false` |
| `SetBacklight` | `static Result SetBacklight(void* platformContext, bool enabled, uint8_t levelPercent = 100u)` | Nível só é aplicado quando habilita |
| `GetLanguageCode` | `static uint32_t GetLanguageCode(void* platformContext)` | `dwLang` do `AEEDeviceInfo`; 0 no Windows |

`SystemMemoryInfo` traz `freeRamBytes`, `totalRamBytes`, `largestFreeBlockBytes`, `freeFsBytes` e `totalFsBytes`. `BatteryStatus` traz `present`, `charging` e `levelPercent` de 0 a 100.

## Limites e armadilhas

O app do Zeebo tem **32 KB de stack**, número do Zeebo Developer Guide §5.1 registrado em [TODO.md](../TODO.md). Toda variável local grande dentro de `OnUpdate` ou `OnRender` sai desse orçamento, e o estouro não é detectado: o app morre. Arrays grandes pertencem ao membro da classe ou ao heap, e `Thread::Start` aceita stack maior justamente para tirar trabalho do quadro.

O heap é de **32 MB** e o `.mod` inteiro é carregado nele, então o orçamento que o `MemoryTracker` mostra (32 MiB por padrão) inclui o próprio binário. Um `.mod` de 400 KB já come o equivalente a 400 KB do orçamento antes do primeiro `new`.

A resolução do timer do BREW é de 1 ms, e o intervalo é calculado como `1000 / fps`: 30 FPS viram 33 ms e 60 FPS viram 16 ms, o que na prática permite até 62.5 quadros por segundo. Não espere pontualidade de microssegundos, e não presuma que `OnUpdate` roda exatamente N vezes por segundo.

`FrameTimer::Tick` clampa o delta em `[1..250]` ms. O piso de 1 ms evita delta zero em quadros consecutivos no mesmo milissegundo; o teto de 250 ms evita que um stall de gravação jogue a simulação para frente de uma vez. O FPS, por outro lado, usa o delta bruto sem clamp, então ele mede o que aconteceu de verdade.

No Windows, `WM_ACTIVATE` não reinicia o `FrameTimer`. O primeiro `Tick` depois de recuperar o foco soma todo o tempo em segundo plano e chega ao jogo como 250 ms. Trate o primeiro `OnUpdate` após `OnResume` como inválido e descarte o passo de simulação.

`Thread::SleepMs` no Zeebo descarta o argumento e chama `ITHREAD_Suspend`, que suspende a thread até outra chamar `ITHREAD_Resume`. Não use `SleepMs` como espera cronometrada: ela não acorda sozinha. No Windows, `Thread::Start` executa o entry na thread que chamou, `Join` não faz nada e `IsRunning` sempre devolve `false`; o paralelismo real só existe no console.

Os símbolos de `signal.hpp` só têm implementação no backend Zeebo (`zeebo_signal.cpp`, dentro de `#if ZSDK_PLATFORM_ZEEBO`). Um app Windows que chame `AsyncCallback::Schedule`, `Signal::Set` ou `SignalBus::Strobe` não linka. No Windows, `Thread` e `SystemInfo` têm stubs em `windows_backend.cpp`, conforme registrado em [TODO.md](../TODO.md).

`SetTargetFps(0)` tem dois significados. No Zeebo, zero é substituído por 60 e o timer roda a 60 FPS. No Windows, zero desliga o deadline e o laço roda sem limite, consumindo um núcleo inteiro. Use 30 ou 60.

`FixedDiv` não checa divisor zero: `FixedDiv(a, 0)` é divisão inteira por zero, com resultado indefinido e possivelmente trap do processador, não um `Result`. Teste o divisor antes. `IntToFixed` desloca 16 bits, então inteiros acima de 32767 estouram o sinal antes mesmo de qualquer conta.

`Clock::GetUptimeMs` devolve `uint32_t` em milissegundos, o que dá cerca de 49.7 dias antes de voltar a zero. O `FrameTimer` sobrevive à volta devolvendo um delta de 1 ms por um quadro, mas contas próprias de tempo precisam considerar isso.

Nada disso foi validado em hardware Zeebo: os relatórios de FPS vêm do emulador, não do console. O estado atual está em [TODO.md](../TODO.md).

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `OnInit` devolve erro e o app fecha logo depois do START | O handler de `EVT_APP_START` retorna `FALSE` e o BREW fecha o applet | Procure `CORE: OnInit failed` no log e devolva `Result::Ok` só depois de criar todos os recursos |
| Primeiro quadro depois de voltar ao Windows dá um salto de 250 ms | `WM_ACTIVATE` não reinicia o `FrameTimer` | Ignore o delta do primeiro `OnUpdate` após `OnResume` |
| `Signal` ou `AsyncCallback` falham na linkagem do build Windows | Implementação existe só no backend Zeebo | Compile a lógica assíncrona com `#if ZSDK_PLATFORM_ZEEBO` e use um caminho direto no Windows |
| `Thread::SleepMs(16)` trava o jogo no Zeebo | O `ms` é ignorado e `ITHREAD_Suspend` não tem tempo de retomada | Use o timer do quadro ou `ISHELL_SetTimer` para esperar |
| Um laço de rotação trava ou desenha errado com `Fixed16` | `float` cru passado onde se espera 16.16 | Converta com `FloatToFixed` na fronteira e mantenha inteiro no laço |
| FPS medido muito abaixo do alvo sem trabalho aparente | O timer do BREW foi reagendado com 1 ms porque o quadro estourou o intervalo | Reduza o trabalho por quadro; o backend não recupera atraso |
| `OnGamepadConnected` não dispara no primeiro quadro | `DispatchGamepadChanges` foi chamado antes de `InputManager::Update` | Chame depois do `Update`; os backends já fazem isso |
| Um `Signal` para de acordar depois do primeiro `Set` | O sinal foi entregue e desabilitado automaticamente | Chame `Enable()` depois de cada entrega |

## Veja também

* [01 Build e empacotamento](./01_build_e_empacotamento.md): flags de compilação, alvos de 30 e 60 FPS por plataforma e o `.mod`.
* [03 Debug e profiler](./03_debug_e_profiler.md): o `MemoryTracker` que recebe o orçamento de `AppConfig` e o `DebugOverlay` chamado no laço.
* [04 Input e controles](./04_input_controles.md): o `InputManager` atualizado antes do `OnUpdate` e o hotplug por trás de `DispatchGamepadChanges`.
* [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md): como os dois backends implementam as mesmas classes.
* Código: [app.hpp](../include/zsdk/core/app.hpp), [clock.hpp](../include/zsdk/core/clock.hpp), [math_fixed.hpp](../include/zsdk/core/math_fixed.hpp), [zeebo_core.cpp](../src/platform/zeebo/zeebo_core.cpp) e [windows_backend.cpp](../src/platform/windows/windows_backend.cpp).
