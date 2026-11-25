# 07 — Áudio, Rede, Bluetooth e Criptografia

Headers incluídos: `audio/audio.hpp`, `audio/audio_engine.hpp`, `audio/pcm_mixer.hpp`, `net/socket.hpp`, `net/http_client.hpp`, `net/network_mgr.hpp`, `net/bluetooth.hpp`, `security/crypto.hpp`, `security/license.hpp`.

---

## 1. Áudio & Streaming (`MediaStream`, `SoundEffect`, `AudioEngine`)

* **Zeebo:** Tanto `MediaStream` quanto `SoundEffect` encapsulam `IMedia` (`AEEMediaUtil_CreateMedia` com `IMEDIA_EnableChannelShare(TRUE)` obrigatório para *multisequencing* — Developer Guide 9.5.1.1), evitando `AEECLSID_SOUNDPLAYER` (`0x01002000`) que falha em múltiplas instâncias no console.
* **Windows:** Implementado com `winmm` (`mciSendStringA` para `MediaStream` MP3/WAV/MIDI e `PlaySoundA` + `waveOutSetVolume` para `SoundEffect`).
* **Limites de Hardware no `AudioEngine` (Seção 9.2):** Pool de **4 canais SFX** simultâneos (`kMaxSfxChannels = 4`, correspondente aos 4 decodificadores ADPCM/PCM/QCP do MSM7201A) e bloqueio automático de um segundo MP3 caso a BGM já esteja tocando MP3 no Decoder #1.

```cpp
zsdk::audio::AudioEngine audio;
audio.Initialize(app->GetNativePlatformContext());

audio.SetMasterVolume(90);
audio.PlayMusic("fs:/~/audio/tema.mp3", true); // BGM em loop
audio.PlaySfx("fs:/~/audio/pulo.wav");         // Pool de 4 canais SFX (limite MSM7201A)
```

* **Mixer PCM em Software com Pitch/Sample-Rate ao Vivo (`PcmMixer` em `audio/pcm_mixer.hpp`):**
  * Como o `IMedia` do BREW não suporta alteração de *sample-rate* ou *pitch* em tempo real sem esperar o próximo buffer, o `PcmMixer` fornece até **16 vozes PCM 16-bit** (mono ou estéreo) com acumulador de fase em ponto fixo 16.16 e interpolação linear.
  * `SetVoiceSampleRate(voice, hz)` e `SetVoicePitch(voice, pitchQ16)` têm efeito imediato na próxima chamada de `Mix(outStereo, frames)`.

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
