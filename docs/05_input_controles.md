# 05 Input e controles

A entrada do Zeebo chega por dois canais diferentes, o `IHID` com fila de botões e posição analógica, e os eventos `AVK_*` que o BREW entrega como teclado, e o mesmo botão físico pode aparecer nos dois. O `InputManager` resolve isso com uma API única de polling: cada slot de jogador tem um `GamepadState` atualizado uma vez por frame, com máscara de botões, transições de pressionado e solto, duração de toque, sticks normalizados com deadzone e estado de movimento. No Windows, o mesmo código roda sobre XInput mais teclado, o que permite testar o jogo no PC sem tocar no código de jogo. Use este capítulo quando precisar ler controle, rumble ou teclado; para entrada de texto, prefira o `VirtualKeyboard`, que já converte D-Pad e botões em letras.

## Visão rápida

| Peça | Header | O que entrega |
| :--- | :--- | :--- |
| `GamepadState` | `input/input.hpp` | Estado de um slot: botões, holds, sticks, movimento |
| `InputManager` | `input/input.hpp` | Até `ZSDK_MAX_GAMEPADS` slots, hotplug, update por frame, rumble |
| `GamepadButton` | `input/input_types.hpp` | Enum lógico, independente do controle físico |
| `AnalogConfig` | `input/input_types.hpp` | Deadzone interna e externa dos sticks |
| `MotionState` | `input/input_types.hpp` | Aceleração, velocidade angular e rotação brutas |
| `KeyboardMouseManager` | `input/keyboard_mouse.hpp` | Teclado e mouse HID no Zeebo; só teclado no Windows |
| `VirtualKeyboard` | `input/virtual_keyboard.hpp` | Teclado em tela na metade inferior do 640x480 |

## Pré-requisitos

Em C++, o caminho normal é o `Application`, que já possui um `InputManager` e o atualiza antes do seu `OnUpdate`. Pegue a referência com `zsdk::Application::GetInstance()->GetInput()`. A ordem importa: o backend chama `InputManager::Update`, depois `DispatchGamepadChanges` e só então o seu `OnUpdate`, então `IsPressed` e `IsReleased` descrevem a transição entre o frame anterior e este.

Para criar um `InputManager` fora do `Application`, o `Initialize` precisa do contexto de plataforma: `IShell*` no Zeebo, `HWND` no Windows. `Application::GetNativePlatformContext()` devolve o valor certo para cada backend. Em C, o host gerado pelo `zsdk_add_c_app` já cria e alimenta tudo; o seu `zsdk_c_app_update` recebe um `const zsdk_input_t*` que é uma view sobre o `InputManager` do app.

## Conceitos

### Estado por frame, não callbacks

`GamepadState` é um valor, não um serviço: ele guarda `m_currentMask` e `m_previousMask`, um vetor de tempo de toque por botão, os dois sticks já normalizados, a configuração de deadzone e o `MotionState`. `InputManager::Update` começa chamando `BeginFrame` em todos os slots, o que copia a máscara atual para a anterior e soma `deltaTimeMs` aos botões que estavam pressionados (zerando os demais), depois drena as filas de dispositivo e grava o novo estado.

Dessa ordem saem as leituras que o jogo usa:

| Chamada | Verdadeira quando |
| :--- | :--- |
| `IsDown` | O botão está na máscara atual |
| `IsPressed` | Está na atual e não estava na anterior |
| `IsReleased` | Não está na atual e estava na anterior |
| `GetHoldDurationMs` | Tempo acumulado desde o toque, em ms, zerado ao soltar |

`IsComboJustTriggered(mask)` exige que todos os botões da máscara estejam pressionados agora e que pelo menos um deles não estivesse antes. É a forma certa de detectar atalho sem disparar de novo enquanto o jogador segura a combinação, e é o que o `DebugOverlay` usa para abrir e fechar.

`GetHoldDurationMs` avança pelo `deltaTimeMs` que você passa em `Update`, não pelo relógio. Se o jogo chamar `Update` duas vezes no mesmo frame, o hold dobra e as transições se perdem, porque a segunda chamada iguala atual e anterior.

### Três famílias de controle, um enum só

O jogo nunca vê botão de hardware. `GamepadButton` tem 17 valores mais `Count`, e `ButtonMask(button)` converte cada um em um bit para as máscaras de combo. A tradução no Zeebo acontece em `ApplyHidButtonEvent`, que escolhe o mapa pelo perfil do dispositivo detectado em `DetectControllerProfile`:

| Perfil | Critério | Consequência |
| :--- | :--- | :--- |
| `Zeebo` | VID `0x1EAA` (Dragon), `0x1A5C` (Z-Pad), `0x15A2` (Boomerang), VID `0` ou dispositivo Bluetooth | IDs nativos de `AEEHIDButtons.h`, com UID como reserva |
| `PlayStation` | VID `0x054C` | Mapa por índice de botão do DualShock/DualSense |
| `GenericHid` | Qualquer outro | UID primeiro, depois o mesmo índice numérico do mapa PlayStation |

O mapa nativo, direto de `MapHidButtonToLogical`, é o que vale para o Z-Pad:

| Enum | Constante de hardware | Posição |
| :--- | :--- | :--- |
| `DPadUp` / `DPadDown` / `DPadLeft` / `DPadRight` | `ZEEBO_BUTTON_DPAD_*` | Direcional |
| `Action1` | `ZEEBO_BUTTON_SOUTH` | Botão 1, sul |
| `Action2` | `ZEEBO_BUTTON_WEST` | Botão 2, oeste |
| `Action3` | `ZEEBO_BUTTON_NORTH` | Botão 3, norte |
| `Action4` | `ZEEBO_BUTTON_EAST` | Botão 4, leste |
| `ShoulderLeft` / `ShoulderRight` | `ZEEBO_BUTTON_SHOULDER_LEFT` / `_RIGHT` | ZL e ZR |
| `TriggerLeft` / `TriggerRight` | `AEEHIDBUTTON_7` / `AEEHIDBUTTON_8` | Gatilhos |
| `Start` | `AEEHIDBUTTON_10` | Iniciar |
| `LeftThumb` / `RightThumb` | `AEEHIDBUTTON_11` / `AEEHIDBUTTON_12` | Clique dos analógicos |
| `Home` | `ZEEBO_BUTTON_HOME` | Botão central |

Se o ID não casa, `MapHidButtonToLogical` ainda tenta o UID de `AEEHIDDevice_Joystick.h`: `AEEUID_HIDJoystick_DPad_*`, `Start`, `Back`, `Left_Thumbstick`, `Right_Thumbstick`, `Left_Shoulder_Upper`, `Right_Shoulder_Upper`, `Left_Shoulder_Lower` e `Right_Shoulder_Lower`. É esse fallback que cobre controles USB genéricos no perfil Zeebo.

Quando `info.nButtonMax - info.nButtonMin >= 3`, o evento não é botão e sim hat switch. `ApplyHatSwitchDpad` normaliza POV de 4 e de 8 direções para os quatro bits do direcional, então um direcional analógico de controle de PC vira `DPadUp`, `DPadDown`, `DPadLeft` e `DPadRight` como qualquer outro.

No Windows o mapa é por bitmask do XInput: `A` em `Action1`, `X` em `Action2`, `Y` em `Action3`, `B` em `Action4`, `LB` e `RB` nos ombros, `LT` e `RT` nos gatilhos, `Start` e `Back` no centro, `L3` e `R3` nos cliques. Os gatilhos contam como pressionados acima de 30 de 255, e `Home` é sempre `false` nesse caminho. O teclado soma no jogador 1: setas no direcional, `Z`, `X`, `C` e `V` nos quatro botões de ação, `Enter` em `Start`, `Backspace` em `Back` e `Esc` em `Home`. Não existe tecla mapeada para ombro ou gatilho.

No caminho de teclas do BREW, `MapAvkToLogical` converte `AVK_UP`, `AVK_DOWN`, `AVK_LEFT` e `AVK_RIGHT` no direcional, `AVK_SELECT` em `Start`, `AVK_CLR` em `Home`, `AVK_SOFT1` e `AVK_SOFT2` em `Action1` e `Action2`, e, quando o SDK define `AVK_GP_1`, a série `AVK_GP_1` a `AVK_GP_4` nos quatro botões, `AVK_GP_5` e `AVK_GP_6` nos ombros, `AVK_GP_SL` e `AVK_GP_SR` nos gatilhos.

### Deadzone radial e normalização dos sticks

O bruto do hardware é `int16_t` e chega ao `GamepadState` por `SetRawSticks`, que aplica `NormalizeStickWithDeadzone` em cada stick. A conta é radial, não por eixo: divide o bruto por 32767, satura em `-1.0`, compara a magnitude ao quadrado com o quadrado da deadzone interna e, se passar, reescala `(mag - inner) / (outer - inner)` e projeta o resultado de volta na direção original.

Os padrões vêm do construtor: `innerDeadzone` 0.15 e `outerDeadzone` 0.95. Qualquer magnitude abaixo de 0.15 vira `{0, 0}` exatamente, o que elimina o drift do stick em repouso. A faixa entre 0.15 e 0.95 é esticada para 0 a 1, então o jogador ganha resolução útil logo acima da zona morta, e qualquer coisa acima de 0.95 vira 1. Se `outerDeadzone` for menor que `innerDeadzone`, o código usa 1.0 no lugar da externa.

O sinal do eixo Y é padronizado como positivo para cima. No Zeebo o bridge nega o Y do Z-Pad (`NegateAxis16`) e o Y dos perfis com limites conhecidos (`NormalizeAxisRange` com `invert`), o que faz o `+1` do `GetLeftStick().y` significar "para cima" em qualquer controle. No Windows os sticks do XInput já chegam com essa convenção.

### Hotplug, suspensão e os hooks do app

No Zeebo o `InputManager` drena `IHID_GetNextConnectEvent` a cada frame. Evento de conexão com `nDeviceType == AEEUID_HID_Joystick_Device` vira `AttachDeviceSlot`, que reserva o primeiro slot livre, cria o `IHIDDevice`, chama `IHIDDevice_SetExclusiveLevel(dev, 1)` para não receber em dobro os eventos `AVK` padrão (Developer Guide 6.3.6), cria o mapeamento de thumbstick e lê os limites mínimo e máximo de posição. Evento com status zero, ou cujo device info não pôde ser lido, cai em `DetachDeviceSlot`, que destrói o mapeamento, solta o dispositivo e marca o slot como desconectado.

Se a fila de conexão sinalizar `dropped`, o bridge esvazia o resto da fila e reenumera com `IHID_GetConnectedDevices`, que é o procedimento do Developer Guide 6.2.2 para não perder um controle conectado durante um pico de eventos.

O `Application` observa o resultado desse trabalho em `DispatchGamepadChanges`, que compara a máscara atual de conectados com a do último frame e chama `IAppDelegate::OnGamepadConnected` ou `OnGamepadDisconnected` só na transição. Os dois são virtuais com corpo vazio, então sobrescrever é opcional.

O ciclo de vida do BREW também mexe no input. Em `EVT_APP_SUSPEND` o app chama `InputManager::Shutdown`, liberando todos os `IHIDDevice` (Developer Guide 11.9), e em `EVT_APP_RESUME` chama `Initialize` de novo. Nada disso preserva estado: ao voltar, os slots são reenumerados e um controle que estava no slot 2 pode reaparecer no 1.

### Assinatura e configuração

```cpp
#include "zsdk/input/input.hpp"

static zsdk::input::InputManager g_input;

zsdk::Result InitInput(void* platformContext) {
    const zsdk::Result r = g_input.Initialize(platformContext);
    if (zsdk::Failed(r)) {
        return r;
    }
    zsdk::input::AnalogConfig cfg;
    cfg.innerDeadzone = 0.18f;
    cfg.outerDeadzone = 0.96f;
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        g_input.GetGamepadMutable(i).SetAnalogConfig(cfg);
    }
    return zsdk::Result::Ok;
}

void FrameInput(uint32_t deltaTimeMs) {
    g_input.Update(deltaTimeMs);
}
```

No Zeebo, `Initialize` devolve `Result::AlreadyInitialized` na segunda chamada e `Result::InvalidArg` com contexto nulo; no Windows ele aceita ser chamado de novo, apenas reconfigura o `HWND` e recarrega o XInput. `Shutdown` libera tudo e é seguro chamar duas vezes. `GetGamepad` e `GetGamepadMutable` devolvem o slot 0 para qualquer índice fora de `[0, ZSDK_MAX_GAMEPADS)`, então um índice errado não estoura memória, ele lê o jogador 1.

## Uso passo a passo

Passo 1, leitura de menu. Use `IsPressed` para navegação e `IsDown` para repetição contínua, e reserve `IsComboJustTriggered` para atalhos, porque ele não repete enquanto o jogador segura.

```cpp
#include "zsdk/debug/logger.hpp"
#include "zsdk/input/input.hpp"

static uint8_t g_cursor = 0u;

void UpdateMenu(const zsdk::input::GamepadState& pad) {
    if (pad.IsPressed(zsdk::input::GamepadButton::DPadDown)) {
        g_cursor = static_cast<uint8_t>((g_cursor + 1u) % 10u);
    }
    if (pad.IsPressed(zsdk::input::GamepadButton::DPadUp)) {
        g_cursor = static_cast<uint8_t>((g_cursor == 0u) ? 9u : (g_cursor - 1u));
    }
    if (pad.IsPressed(zsdk::input::GamepadButton::Action1)) {
        ZSDK_LOG_INFO("MENU", "item %u", g_cursor);
    }

    const uint32_t combo = zsdk::input::ButtonMask(zsdk::input::GamepadButton::ShoulderLeft) |
                           zsdk::input::ButtonMask(zsdk::input::GamepadButton::ShoulderRight);
    if (pad.IsComboJustTriggered(combo)) {
        ZSDK_LOG_INFO("MENU", "atalho dos dois ombros");
    }
}
```

Passo 2, sticks. A saída já está em `[-1, 1]` com deadzone aplicada, então o jogo só precisa multiplicar pela velocidade e pelo delta em segundos. Repare que o eixo Y positivo é para cima, então subir no mundo é `+y` direto.

```cpp
static float g_yaw = 0.0f;
static float g_pitch = 0.0f;
static float g_playerX = 0.0f;
static float g_playerY = 0.0f;

void UpdateCameraAndPlayer(const zsdk::input::GamepadState& pad, float dtSeconds) {
    const zsdk::Vec2 ls = pad.GetLeftStick();
    const zsdk::Vec2 rs = pad.GetRightStick();

    g_playerX += ls.x * 3.0f * dtSeconds;
    g_playerY += ls.y * 3.0f * dtSeconds;
    g_yaw   -= rs.x * 120.0f * dtSeconds;
    g_pitch += rs.y * 90.0f * dtSeconds;
}
```

Passo 3, rumble com duração. `SetRumble` manda a intensidade para os dois motores e não tem noção de tempo: nada no `InputManager` desliga a vibração sozinho, então o jogo precisa zerar quando o efeito acaba.

```cpp
#include "zsdk/input/input.hpp"

static uint32_t g_rumbleMsLeft = 0u;

void StartImpactRumble(zsdk::input::InputManager& input, uint32_t durationMs) {
    if (zsdk::Failed(input.SetRumble(0u, 48000u, 32000u))) {
        g_rumbleMsLeft = 0u;
        return;
    }
    g_rumbleMsLeft = durationMs;
}

void UpdateRumble(zsdk::input::InputManager& input, uint32_t deltaTimeMs) {
    if (g_rumbleMsLeft == 0u) {
        return;
    }
    if (deltaTimeMs >= g_rumbleMsLeft) {
        g_rumbleMsLeft = 0u;
        input.SetRumble(0u, 0u, 0u);
    } else {
        g_rumbleMsLeft -= deltaTimeMs;
    }
}
```

Passo 4, reagir a conexão e desconexão. O gancho é o lugar certo para reaplicar configuração de analógico e para pausar o jogo, porque no Zeebo o controle pode ser removido com o jogo rodando.

```cpp
#include "zsdk/core/app.hpp"
#include "zsdk/debug/logger.hpp"
#include "zsdk/input/input.hpp"

class GameApp : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override { return zsdk::Result::Ok; }
    void OnRender() override {}
    void OnShutdown() override {}

    void OnUpdate(uint32_t deltaTimeMs) override {
        if (m_paused) {
            return;
        }
        ZSDK_UNUSED(deltaTimeMs);
    }

    void OnGamepadConnected(uint8_t playerIndex) override {
        ZSDK_LOG_INFO("GAME", "P%u conectado", playerIndex + 1u);
        zsdk::input::AnalogConfig cfg;
        cfg.innerDeadzone = 0.18f;
        cfg.outerDeadzone = 0.96f;
        zsdk::Application::GetInstance()->GetInput()
            .GetGamepadMutable(playerIndex).SetAnalogConfig(cfg);
        m_paused = false;
    }

    void OnGamepadDisconnected(uint8_t playerIndex) override {
        ZSDK_LOG_WARN("GAME", "P%u removido", playerIndex + 1u);
        m_paused = true;
    }

private:
    bool m_paused = false;
};
```

Passo 5, teclado e mouse. `KeyboardMouseManager` é um polling separado do `InputManager`, com a sua própria instância de `IHID` no Zeebo, e não entra nas máscaras de `GamepadState`.

```cpp
#include "zsdk/input/keyboard_mouse.hpp"

static zsdk::input::KeyboardMouseManager g_kbm;
static bool g_debugOpen = false;

void InitKeyboardMouse(void* platformContext) {
    g_kbm.Initialize(platformContext);
}

void UpdateKeyboardMouse() {
    g_kbm.Update();
    if (g_kbm.HasKeyboard() && g_kbm.IsKeyDown(0x04u)) { /* HID usage da tecla A */
        g_debugOpen = true;
    }
    const zsdk::Vec2 delta = g_kbm.GetMouseDelta();
    ZSDK_UNUSED(delta);
}
```

Passo 6, teclado virtual. `Open` arma o teclado, `Update` consome o pad, `Render` desenha na metade inferior da tela de 640x480, e `IsSubmitted`, `IsCancelled` e `GetText` dizem o que aconteceu. O buffer é interno e continua válido até o próximo `Open`.

```cpp
#include "zsdk/debug/logger.hpp"
#include "zsdk/input/virtual_keyboard.hpp"

static zsdk::input::VirtualKeyboard g_vkb;

void OpenNameEntry() {
    g_vkb.Open("NOME DO JOGADOR", "ZEEBO", 16u);
}

void UpdateNameEntry(const zsdk::input::GamepadState& pad) {
    if (!g_vkb.IsActive()) {
        return;
    }
    g_vkb.Update(pad);
    if (g_vkb.IsSubmitted()) {
        ZSDK_LOG_INFO("GAME", "nome: %s", g_vkb.GetText());
    } else if (g_vkb.IsCancelled()) {
        ZSDK_LOG_INFO("GAME", "entrada cancelada");
    }
}

void RenderNameEntry(int32_t width, int32_t height) {
    g_vkb.Render(width, height);
}
```

O grid tem 4 linhas por 10 colunas. O D-Pad move a seleção, `Action1` digita o caractere, `Action2` apaga, `Action3` insere espaço, `Start` confirma e `Back` cancela. As células `^`, `<` e `>` da última linha viram `CAPS` (alterna entre o grid minúsculo e o maiúsculo), `DEL` (apaga) e `OK` (confirma). A caixa alta troca o hífen da terceira linha por sublinhado.

Passo 7, o mesmo pela ABI C. A view de input vem do host, então o seu código C nunca cria um `InputManager`.

```c
#include "zsdk/c/zsdk_c.h"

void UpdatePlayerC(const zsdk_input_t* input) {
    float lx = 0.0f;
    float ly = 0.0f;
    zsdk_input_left_stick(input, 0u, &lx, &ly);

    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION1)) {
        ZSDK_C_LOG_INFO("CUBE_C", "acao em (%.0f, %.0f)", (double)lx, (double)ly);
    }
    if (zsdk_input_connected_count(input) == 0u) {
        ZSDK_C_LOG_WARN("CUBE_C", "nenhum controle");
    }
}
```

## Referência da API

`GamepadState`, com as consultas inline exceto `GetHoldDurationMs` e `SetAnalogConfig`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `IsConnected` | `bool IsConnected() const` | Slot ocupado agora |
| `IsDown` | `bool IsDown(GamepadButton button) const` | Estado contínuo |
| `IsPressed` | `bool IsPressed(GamepadButton button) const` | Só no frame da descida |
| `IsReleased` | `bool IsReleased(GamepadButton button) const` | Só no frame da subida |
| `IsComboJustTriggered` | `bool IsComboJustTriggered(uint32_t comboMask) const` | Combinação completa e nova |
| `GetHoldDurationMs` | `uint32_t GetHoldDurationMs(GamepadButton button) const` | Soma `deltaTimeMs` desde o toque |
| `GetLeftStick` | `Vec2 GetLeftStick() const` | Normalizado com deadzone |
| `GetRightStick` | `Vec2 GetRightStick() const` | Normalizado com deadzone |
| `GetMotion` | `const MotionState& GetMotion() const` | Valores brutos de `IHIDDevice_GetPositionState` |
| `SetAnalogConfig` | `void SetAnalogConfig(const AnalogConfig& config)` | Vale a partir do próximo `SetRawSticks` |

`InputManager`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(void* platformContext)` | `IShell*` no Zeebo, `HWND` no Windows |
| `Shutdown` | `void Shutdown()` | Libera dispositivos, mapeamentos e o `Impl` |
| `Update` | `void Update(uint32_t deltaTimeMs)` | Hotplug, filas e `BeginFrame` de todos os slots |
| `HandlePlatformEvent` | `bool HandlePlatformEvent(uint32_t eventCode, uint16_t keyCode, uint32_t param)` | `EVT_KEY*`/`WM_KEY*` no jogador 1 |
| `GetGamepad` | `const GamepadState& GetGamepad(uint8_t playerIndex = 0) const` | Índice inválido cai no slot 0 |
| `GetGamepadMutable` | `GamepadState& GetGamepadMutable(uint8_t playerIndex = 0)` | Para `SetAnalogConfig` |
| `GetConnectedCount` | `uint8_t GetConnectedCount() const` | Quantos slots estão conectados |
| `SetRumble` | `Result SetRumble(uint8_t playerIndex, uint16_t leftMotor, uint16_t rightMotor)` | `0..65535` por motor |

`KeyboardMouseManager` e `VirtualKeyboard`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `KeyboardMouseManager::Initialize` | `Result Initialize(void* platformContext)` | Cria `IHID` próprio no Zeebo; no Windows só devolve `Ok` |
| `KeyboardMouseManager::Update` | `void Update()` | Drena teclado e mouse HID; no Windows é vazio |
| `KeyboardMouseManager::IsKeyDown` | `bool IsKeyDown(uint16_t hidKeyCode) const` | HID usage no Zeebo, código virtual no Windows |
| `KeyboardMouseManager::GetMouseDelta` | `Vec2 GetMouseDelta() const` | Delta do mouse; no Windows sempre `{0, 0}` |
| `KeyboardMouseManager::GetMouseButtonsMask` | `uint8_t GetMouseButtonsMask() const` | Um bit por botão do dispositivo; no Windows sempre `0` |
| `KeyboardMouseManager::HasKeyboard` | `bool HasKeyboard() const` | Dispositivo de teclado aberto |
| `VirtualKeyboard::Open` | `void Open(const char* title, const char* initialText = "", uint8_t maxLen = 32u)` | `maxLen` limitado a 63 |
| `VirtualKeyboard::Update` | `void Update(const GamepadState& pad)` | Consome D-Pad, `Action1`, `Action2`, `Action3`, `Start` e `Back` |
| `VirtualKeyboard::Render` | `void Render(int32_t viewportWidth, int32_t viewportHeight) const` | Painel em coordenadas de 640x480 |
| `VirtualKeyboard::GetText` | `const char* GetText() const` | Buffer interno, válido até o próximo `Open` |

ABI C em `zsdk/c/zsdk_c_input.h`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `zsdk_input_init` | `zsdk_result_t zsdk_input_init(zsdk_input_t* input, void* platform_ctx)` | Cria o `InputManager` do objeto |
| `zsdk_input_update` | `void zsdk_input_update(zsdk_input_t* input, uint32_t delta_ms)` | Um passo por frame |
| `zsdk_input_is_down` | `int zsdk_input_is_down(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn)` | Espelho de `IsDown` |
| `zsdk_input_left_stick` | `void zsdk_input_left_stick(const zsdk_input_t* input, uint8_t player, float* out_x, float* out_y)` | Escreve nos ponteiros, aceita nulo |
| `zsdk_input_motion` | `int zsdk_input_motion(const zsdk_input_t* input, uint8_t player, float* out_ax, float* out_ay, float* out_az, float* out_gx, float* out_gy, float* out_gz)` | Devolve `1` se `supported` |
| `zsdk_input_is_combo_triggered` | `int zsdk_input_is_combo_triggered(const zsdk_input_t* input, uint8_t player, uint32_t combo_mask)` | Máscara montada como `1u << ZSDK_BTN_*` |
| `zsdk_input_set_rumble` | `zsdk_result_t zsdk_input_set_rumble(zsdk_input_t* input, uint8_t player, uint16_t left_motor, uint16_t right_motor)` | Mesmos motores |

## Limites e armadilhas

`ZSDK_MAX_GAMEPADS` vale 4, e o compromisso não é o mesmo nas duas plataformas. No Windows os quatro índices do XInput são consultados. No Zeebo o limite é a enumeração de `IHID_GetConnectedDevices`, e o [TODO.md](../TODO.md) registra que o material do console fala em no máximo 2 controles, enquanto a SDK aceita 4, o que ainda precisa ser conferido em hardware.

`AttachDeviceSlot` reinicializa o slot com `pads[i] = GamepadState()`, o que apaga qualquer `SetAnalogConfig` que você tenha aplicado antes da conexão. Por isso a configuração de deadzone deve ser reaplicada em `OnGamepadConnected`, nunca só no `OnInit`.

No Zeebo, qualquer tecla `AVK` mapeada marca o jogador 1 como conectado, porque o caminho de teclado existe justamente para quando o `IHID` não está disponível. Isso significa que `OnGamepadConnected(0)` pode disparar num ambiente sem controle plugado, e que o jogo não deve tratar "conectado" como "existe controle físico". No Windows o efeito é parecido e permanente: o jogador 1 é sempre marcado conectado, com ou sem controle, porque o teclado alimenta o slot.

`MotionState::supported` é verdadeiro sempre que `IHIDDevice_GetPositionState` devolveu sucesso no frame, não quando existe acelerômetro. Num Z-Pad, que também devolve posição para os sticks, o flag pode estar ligado com aceleração zerada. `acceleration`, `angularVelocity` e `rotation` são os inteiros de `AEEHIDPositionInfo` convertidos para `float`, sem unidade normalizada: trate como valor bruto do dispositivo, não como g nem graus por segundo.

`SetRumble` não tem estado nem timeout. O valor fica no dispositivo até outra chamada, então um efeito que esquece de zerar deixa o controle vibrando depois de sair da tela. O retorno pode ser `Result::DeviceDisconnected` (slot vazio), `Result::NotSupported` (o dispositivo recusou o pedido, ou não há XInput carregado no Windows) ou `Result::InvalidArg` (índice fora da faixa). No Windows a função carrega `xinput1_4.dll`, `xinput9_1_0.dll` e `xinput1_3.dll` nesta ordem, por `LoadLibraryA`, sem link estático.

No Windows, `KeyboardMouseManager` é praticamente inerte: `Update` não faz nada, `GetMouseDelta` devolve `{0, 0}`, `GetMouseButtonsMask` devolve `0`, e `IsKeyDown` consulta `GetAsyncKeyState` direto com o código de tecla virtual, não HID. Ler o mouse no PC exige outro caminho, e o mesmo código que funciona no Zeebo com HID usage não funciona ali.

O `VirtualKeyboard` cabe em 63 caracteres (`kVirtualKeyboardMaxChars`), o título em 31, e o buffer é interno ao objeto, então um segundo `Open` sobrescreve o texto anterior. Ele renderiza em coordenadas fixas de 640x480 com o painel na metade inferior, então em resolução diferente a área é escalada pelo `Begin2DVirtual`, não reposicionada.

O `InputManager` do Windows usa um `Impl` estático (`static Impl s_inputImpl`), o que impede duas instâncias independentes no PC. No Zeebo cada `InputManager` aloca o seu `Impl` pelo `MemoryTracker` no subsistema `Input` e libera em `Shutdown`.

A detecção de controle foi exercitada no emulador com e sem pad, mas a remoção em tempo de execução não foi testada, porque o emulador usado não a simula pela linha de comando, e o [TODO.md](../TODO.md) registra essa lacuna. Tratar `OnGamepadDisconnected` como pausa é a recomendação, mas a sequência completa em hardware ainda não foi validada.

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| Objeto anda sozinho com o stick solto | `innerDeadzone` baixa demais para um stick gasto | Subir a interna para 0.18 ou mais com `SetAnalogConfig` |
| Deadzone configurada no `OnInit` não vale depois | `AttachDeviceSlot` recria o `GamepadState` do slot | Reaplicar em `OnGamepadConnected` |
| `IsPressed` nunca fica verdadeiro | `Update` não é chamado, ou é chamado duas vezes no frame | Chamar uma vez por frame, antes da leitura |
| Jogo pausa no meio da partida sem ninguém mexer | `OnGamepadDisconnected` disparou porque o `IHID` perdeu o device numa reenumeração ou no ciclo suspend/resume | Reconferir `GetConnectedCount` antes de pausar e tratar o slot 0 como teclado |
| Controle vibra depois do impacto | Ninguém chamou `SetRumble(0, 0, 0)` | Zerar no fim do efeito, como no passo 3 |
| Combo do overlay não funciona com teclado no Windows | A tabela de teclas do PC não tem ombro nem gatilho | Usar controle de Xbox ou `DebugOverlay::SetVisible` |
| Movimento sempre zerado em controle sem sensor | `supported` reflete leitura de posição, não hardware de movimento | Checar os valores além do flag |
| Stick para cima lido como negativo | Bridge próprio sem a inversão de Y | Manter Y positivo para cima, como `NegateAxis16` faz |
| `IsKeyDown(0x04)` funciona no Zeebo e não no PC | No Windows o parâmetro é código virtual, não HID usage | Separar o código de leitura por plataforma |
| Texto do `VirtualKeyboard` cortado | `maxLen` acima de 63 é reduzido ao limite | Pedir até 63 caracteres e validar o tamanho |

## Veja também

- [03 Core e async](./03_core_e_async.md): `Application`, `IAppDelegate` e o laço onde o `InputManager` é atualizado.
- [04 Debug e profiler](./04_debug_e_profiler.md): o overlay que abre com ombro esquerdo mais D-Pad para baixo.
- [01 Primeiros passos](./01_primeiros_passos.md): o primeiro app lendo o jogador 1 nas quatro linguagens.
- [11 Referência da API](./11_referencia_da_api.md): mapa de headers e assinaturas resumidas.
- [input.hpp](../include/zsdk/input/input.hpp), [input_types.hpp](../include/zsdk/input/input_types.hpp) e [zeebo_input.cpp](../src/platform/zeebo/zeebo_input.cpp): o estado, o enum e o mapa de botões completo.
