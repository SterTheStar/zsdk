# 04 — Input, Controles (Z-Pad / Boomerang / Xbox) e Teclado/Mouse

Headers incluídos: `input/input_types.hpp`, `input/input.hpp`, `input/keyboard_mouse.hpp`, `input/virtual_keyboard.hpp`.

---

## 1. Mapeamento Oficial de Botões (1:1 com `AEEHIDButtons.h`)

No Zeebo, os botões são mapeados via `IHID` / `IHIDDevice` (`AEEHIDButtons.h`, `AEEHIDThumbsticks.h`, `AEEHIDDevice_Joystick.h`, com `IHIDDevice_SetExclusiveLevel(1)` e recuperação de fila `dropped`) com fallback completo para eventos `AVK_*` e `AVK_GP_1..4/5/6/SL/SR` (`AEEVCodes.h`). No Windows, são mapeados via **XInput** (controles Xbox P1–P4) + teclado no P1.

| Enum `GamepadButton` | Posição Física | Zeebo Z-Pad | Xbox (Windows) | Teclado (Windows) |
| :--- | :--- | :--- | :--- | :--- |
| `DPadUp` / `Down` / `Left` / `Right` | Direcional | `ZEEBO_BUTTON_DPAD_*` | D-Pad | Setas |
| `Action1` | Sul (baixo) | Botão **1** (`SOUTH`) | **A** | `Z` |
| `Action2` | Oeste (esquerda) | Botão **2** (`WEST`) | **X** | `X` |
| `Action3` | Norte (cima) | Botão **3** (`NORTH`) | **Y** | `C` |
| `Action4` | Leste (direita) | Botão **4** (`EAST`) | **B** | `V` |
| `ShoulderLeft` / `Right` | Ombros | **ZL** / **ZR** | **LB** / **RB** | — |
| `TriggerLeft` / `Right` | Gatilhos | Shoulder Lower | **LT** / **RT** | — |
| `Start` / `Back` / `Home` | Centro | Start / Back / Home (`AEEHIDBUTTON_9`) | Start / Back / — | `Enter` / `Backspace` / `Esc` |
| `LeftThumb` / `RightThumb` | Clique Analógico | L3 / R3 | L3 / R3 | — |

> **Atenção ao botão central do Z-Pad:** No Z-Pad padrão, o botão central envia `ZEEBO_BUTTON_HOME` (`AEEHIDBUTTON_9`), portanto trate tanto `Start` quanto `Home` caso queira compatibilidade com ambos em menus.

---

## 2. Consultando Estados por Frame (Zero Alocação)

```cpp
void OnUpdate(uint32_t dtMs) override {
    auto* app = zsdk::Application::GetInstance();
    const auto& p1 = app->GetInput().GetGamepad(0); // P1..P4 (0..3)

    if (p1.IsPressed(zsdk::input::GamepadButton::Action1)) {
        // Disparado apenas no frame em que o botão foi pressionado
    }
    if (p1.IsDown(zsdk::input::GamepadButton::Action4)) {
        // Enquanto estiver segurando
    }
    if (p1.IsReleased(zsdk::input::GamepadButton::Action1)) {
        // Apenas no frame em que soltou
    }

    uint32_t tempoSeguradoMs = p1.GetHoldDurationMs(zsdk::input::GamepadButton::Action2);

    // Combos / Atalhos
    const uint32_t combo = zsdk::input::ButtonMask(zsdk::input::GamepadButton::Start) |
                           zsdk::input::ButtonMask(zsdk::input::GamepadButton::ShoulderLeft);
    if (p1.IsComboJustTriggered(combo)) {
        // Combo acionado!
    }
}
```

---

## 3. Analógicos, Deadzones, Movimento (Boomerang) e Rumble

* **Analógicos (`GetLeftStick()`, `GetRightStick()`):** Retornam `Vec2` normalizado em `[-1.0 .. 1.0]` com deadzone radial configurável:
  ```cpp
  auto& mutPad = app->GetInput().GetGamepadMutable(0);
  mutPad.SetAnalogConfig({0.18f, 0.95f}); // innerDeadzone, outerDeadzone
  ```
* **Sensores de Movimento (`GetMotion()`):** Expõe `MotionState` (`supported`, `acceleration` de `nAX/Y/Z`, `angularVelocity` de `nVRx/y/z`, `rotation` de `nRx/y/z` via `IHIDDevice_GetPositionState`).
* **Vibração (`SetRumble`):**
  ```cpp
  app->GetInput().SetRumble(0, 32000, 32000); // Intensidade 0..65535 (motor esq/dir)
  ```

---

## 4. Teclado e Mouse HID (`KeyboardMouseManager`)

Para dispositivos USB/Bluetooth HID no Zeebo ou teclado/mouse no Windows:

```cpp
zsdk::input::KeyboardMouseManager km;
km.Initialize(app->GetNativePlatformContext());
km.Update();
if (km.HasKeyboard() && km.IsKeyDown(0x04)) { /* ... */ }
```


## Home, Start e AVK_CLR

* Home (curto) gera `AVK_CLR`; o Developer Guide manda tratá-lo como pausa dentro do jogo (Home longo > 3 s abre o popup do sistema e gera suspend). O caminho de teclas agora mapeia `AVK_CLR` para `GamepadButton::Home`, igual ao caminho HID (antes virava `Back`, então o mesmo botão físico tinha duas identidades).
* O helper oficial `AEEHIDButton_GetNextButtonEvent` marca `Start` e os cliques dos thumbsticks como "não disponíveis no Zeebo"; `Start` só chega via `AVK_SELECT` (clique do thumbstick direito, §6.4).

---

## 5. Teclado Virtual em Tela (`VirtualKeyboard`)

Para entrada de texto com Z-Pad ou teclado (metade inferior da tela 640x480 com safe area para CRT, sem dependência de `Zeebo.lib` externa):

```cpp
zsdk::input::VirtualKeyboard vkb;
vkb.Open("NOME DO JOGADOR", "ZEEBO", 16);
// No OnUpdate(): vkb.Update(p1);
// No OnRender(): vkb.Render(w, h);
if (vkb.IsSubmitted()) {
    const char* texto = vkb.GetText();
}
```
