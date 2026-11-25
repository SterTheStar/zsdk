# 02 — Core, Ciclo de Vida, Tempo e Concorrência

Headers incluídos: `core/config.hpp`, `core/types.hpp`, `core/app.hpp`, `core/clock.hpp`, `core/math_fixed.hpp`, `core/thread.hpp`, `core/signal.hpp`, `core/system_info.hpp`.

---

## 1. Ciclo de Vida da Aplicação (`Application` & `IAppDelegate`)

O usuário implementa `zsdk::IAppDelegate` e registra sua classe com a macro `ZSDK_IMPLEMENT_APP`, sem precisar escrever `AEEClsCreateInstance` ou `WinMain`:

```cpp
#include "zsdk/zsdk.hpp"

class MeuJogo : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        auto* app = zsdk::Application::GetInstance();
        // Inicializar GLContext, SaveGameManager, AudioEngine...
        return zsdk::Result::Ok;
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        // Lógica de jogo com deltaTimeMs clampado em [1..250] ms
    }

    void OnRender() override {
        // Comandos Renderer + m_gl.SwapBuffers()
    }

    void OnSuspend() override { /* Pausa música/jogo quando BREW suspender (IHID é liberado automaticamente) */ }
    void OnResume() override  { /* Retoma quando voltar (IHID é recriado automaticamente) */ }

    void OnShutdown() override {
        // Liberar recursos determinísticos
    }
};

// Parâmetros: (Classe, Nome, FPS alvo, DebugOverlay inicial visível?)
ZSDK_IMPLEMENT_APP(MeuJogo, "MeuJogo", 60, true)
```

### Controle de FPS (`FpsCap`)
O Zeebo suporta **30 FPS** e **60 FPS** (o modo *Uncapped* foi removido por incompatibilidade prática com o timer cooperativo/VSync do BREW). Para saída de TV CRT no console real, o Developer Guide 0.97 (Seção 11.9) recomenda **30 FPS** (`FpsCap::Fps30`, usado como padrão no exemplo `01_spinning_cube` no Zeebo):

```cpp
auto* app = zsdk::Application::GetInstance();
app->SetFpsCap(zsdk::FpsCap::Fps30); // ou FpsCap::Fps60
uint16_t atual = app->GetTargetFps();
```

> No Zeebo (`zeebo_core.cpp`), o agendador desconta automaticamente o tempo gasto em `OnUpdate` + `OnRender` (`workMs`) antes de chamar `ISHELL_SetTimer`, trata `EVT_APP_NO_SLEEP` (evitando *throttling* de energia), trata o botão Power (`AVK_END` -> `RequestQuit()`), e libera/recria o `IHID` em `EVT_APP_SUSPEND`/`EVT_APP_RESUME` conforme o Zeebo Developer Guide 0.97 (Seções 4.2, 11.8 e 11.9). No Windows (`windows_backend.cpp`), o loop usa `timeBeginPeriod(1)` + *deadline* contínuo via `QueryPerformanceCounter` para travar exatamente em 30/60 FPS.

---

## 2. Tempo e Timers (`Clock` & `FrameTimer`)

* `zsdk::Clock::GetUptimeMs()` — Milissegundos monotônicos desde o boot (`GETUPTIMEMS` no BREW, `QueryPerformanceCounter` de 1 ms no Windows).
* `zsdk::Clock::GetWallTimeMs()` — Tempo de parede (`GETTIMEMS`).
* `zsdk::FrameTimer` — Calcula `deltaMs` (clamp `[1..250]` ms para física) e `FPS` real a cada 500 ms usando o tempo bruto sem clamp.

---

## 3. Matemática em Ponto Fixo (`Fixed16`)

Para máxima performance no ARM11 sem FPU pesada:

```cpp
zsdk::Fixed16 a = zsdk::FloatToFixed(2.5f);
zsdk::Fixed16 b = zsdk::IntToFixed(3);
zsdk::Fixed16 c = zsdk::FixedMul(a, b); // 7.5 em 16.16
zsdk::Fixed16 s = zsdk::FixedSinDeg(zsdk::IntToFixed(30)); // 0.5 em 16.16 (zero float)
zsdk::Fixed16 cs = zsdk::FixedCosDeg(zsdk::IntToFixed(60));
float f = zsdk::FixedToFloat(c);
```

---

## 4. Threads, Sinais e Callbacks Assíncronos

* **`zsdk::Thread` (`core/thread.hpp`):** Encapsula `IThread` cooperativo do BREW (`Start`, `Join`, `SleepMs`, `IsRunning`).
* **`zsdk::AsyncCallback` (`core/signal.hpp`):** Encapsula `AEECallback` + `ISHELL_Resume` para agendar callbacks sem alocação.
* **`zsdk::Signal` & `zsdk::SignalBus` (`core/signal.hpp`):** Encapsulam `ISignal`, `ISignalCtl` e `ISignalBus` do BREW 4.0.2.

---

## 5. Informações do Sistema (`SystemInfo`)

```cpp
zsdk::SystemMemoryInfo mem;
zsdk::SystemInfo::GetMemoryInfo(mem); // GETRAMFREE + GETFSFREE no Zeebo

zsdk::BatteryStatus bat;
zsdk::SystemInfo::GetBatteryStatus(app->GetNativePlatformContext(), bat);

zsdk::SystemInfo::SetBacklight(app->GetNativePlatformContext(), true, 80);
uint32_t lang = zsdk::SystemInfo::GetLanguageCode(app->GetNativePlatformContext());
```


## Detecção de gamepad (obrigatória no Zeebo)

O Developer Guide (§10) torna obrigatórios o tratamento de suspend/resume e a detecção de gamepad. `IAppDelegate` ganhou `OnGamepadConnected(index)` e `OnGamepadDisconnected(index)`: o `Application` compara a máscara de pads conectados a cada frame (`DispatchGamepadChanges()`, chamado pelos dois backends logo após `InputManager::Update`) e dispara os hooks, inclusive uma vez por pad já conectado no primeiro frame. Use-os para pausar e mostrar "conecte um controle".
