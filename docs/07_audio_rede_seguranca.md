# 07 — Áudio, Rede, Bluetooth e Criptografia

Headers incluídos: `audio/audio.hpp`, `audio/audio_engine.hpp`, `net/socket.hpp`, `net/http_client.hpp`, `net/network_mgr.hpp`, `net/bluetooth.hpp`, `security/crypto.hpp`, `security/license.hpp`.

---

## 1. Áudio & Streaming (`MediaStream`, `SoundEffect`, `AudioEngine`)

* **Zeebo:** Encapsula `IMedia` (`AEEMediaUtil_CreateMedia`) e `ISoundPlayer` (`AEECLSID_SOUNDPLAYER`).
* **Windows:** Implementado com `winmm` (`mciSendStringA` para `MediaStream` MP3/WAV/MIDI e `PlaySoundA` + `waveOutSetVolume` para `SoundEffect`).

```cpp
zsdk::audio::AudioEngine audio;
audio.Initialize(app->GetNativePlatformContext());

audio.SetMasterVolume(90);
audio.PlayMusic("fs:/~/audio/tema.mp3", true); // BGM em loop
audio.PlaySfx("fs:/~/audio/pulo.wav");         // Pool de 8 canais SFX
```

---

## 2. Rede, HTTP e Bluetooth

* **`Socket` (`net/socket.hpp`):** Sockets TCP/UDP não-bloqueantes sobre `ISockPort` (`Open`, `ConnectIPv4`, `Send`, `Recv`, tratando `Result::WouldBlock`).
* **`NetworkManager` (`net/network_mgr.hpp`):** Estado da rede (`INetMgr` / `IWIFI`), resolução DNS assíncrona (`ResolveHost` via `INETMGR_GetHostByName`) e IP local (`GetLocalIpAddress`).
* **`HttpClient` (`net/http_client.hpp`):** Requisições assíncronas `Get()` e `Post()` sobre `IWeb`, lendo o corpo completo da resposta via `ISOURCE_Read` em `pisMessage`.
* **`BluetoothManager` (`net/bluetooth.hpp`):** Consulta de rádio (`IBT`) e comunicação serial SPP (`OpenSpp`, `ReadSpp`, `WriteSpp` via `IBTSPPClient`).

---

## 3. Segurança, Criptografia e Licença

* **`Random` (`security/crypto.hpp`):** Geração de bytes aleatórios via `AEECLSID_RANDOM` (`ISource`) com fallback `GETRAND`.
* **`Hash` & `Hmac` (`security/crypto.hpp`):** MD5, SHA-1 e HMAC-MD5 / HMAC-SHA1 sobre `IHashCtx`.
* **`Cipher` (`security/crypto.hpp`):** Cifras simétricas AES-128/192/256, DES, 3DES e ARC4 sobre `ICipherFactory` / `ICipher1`.
* **`Rsa` (`security/crypto.hpp`):** Exponenciação modular RSA (`ModExp`) sobre `IRSA`.
* **`LicenseManager` (`security/license.hpp`):** Verificação de expiração e validade da licença BREW (`ILicense`).


## Vozes de SFX

`ZSDK_MAX_SFX_CHANNELS` (padrão **4** no Zeebo, 8 no PC) define `audio::kMaxSfxChannels`. No Zeebo a receita documentada do mixer BREW é 1 stream MIDI/MMF/PMD + até 4 de QCP/AMR/ADPCM (todos do mesmo tipo); não confirmado para PCM.
