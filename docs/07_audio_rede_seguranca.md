# 07 Áudio, rede e segurança

O console fala com o mundo por três caminhos e a SDK trata cada um em um módulo separado. Áudio cobre o que sai pelo decodificador: música em streaming, efeitos curtos e mixagem PCM em software com pitch ao vivo. Rede cobre o que entra e sai por `ISockPort` e `IWeb`, sempre sem bloquear o laço do jogo. Segurança embrulha hash, HMAC, cifra simétrica, RSA e a licença do app sobre `IHashCtx`, `ICipher1`, `IRSA` e `ILicense`. Use este capítulo quando já existe um app rodando e você precisa somar som, sincronizar dados ou proteger um save; para o primeiro contato com a SDK, comece por [02 Core e async](./02_core_e_async.md); se a necessidade é só armazenar dados localmente, o capítulo certo é [06 Filesystem e savegame](./06_filesystem_e_savegame.md).

## Visão rápida

| Módulo | Classes | Backend Zeebo | O que resolve |
| :--- | :--- | :--- | :--- |
| `audio` | `AudioEngine`, `MediaStream`, `SoundEffect`, `PcmMixer` | `IMedia` com `IMEDIA_EnableChannelShare`, mixagem em software | BGM em loop, pool de 4 efeitos, pitch e sample rate ao vivo |
| `net` | `Socket`, `HttpClient`, `NetworkManager`, `BluetoothManager` | `ISockPort`, `IWeb`, `INetMgr`/`IWIFI`, `IBT` | TCP/UDP cliente, HTTP GET/POST assíncrono, DNS, SPP |
| `security` | `Random`, `Hash`, `Hmac`, `Cipher`, `Rsa`, `LicenseManager` | `IRandomSource`, `IHashCtx`, `ICipher1`, `IRSA`, `ILicense` | MD5/SHA-1, HMAC, AES/DES/ARC4, RSA, licença BREW |

## Pré-requisitos

Todos os `Initialize` dos três módulos recebem o contexto de plataforma, que é o `IShell*` no Zeebo. Pegue o valor uma vez, no início do app, por `zsdk::Application::GetInstance()->GetNativePlatformContext()`, e guarde. Áudio trabalha com caminhos que o BREW abre direto, então os arquivos ficam no diretório do app ou no cartão; os exemplos usam `fs:/~/audio/`, o mesmo prefixo `home:` que o [Vfs](./06_filesystem_e_savegame.md) monta. Rede exige uma conexão já configurada no console: a SDK consulta o estado, mas não configura a conexão.

O backend Windows existe para iteração rápida, e a paridade não é total. Antes de confiar numa execução no PC, confira:

| Recurso | Zeebo | Windows |
| :--- | :--- | :--- |
| `AudioEngine`, `MediaStream` por arquivo | completo | completo via `mciSendStringA` |
| `MediaStream::LoadFromMemory` | completo | `Result::NotSupported` |
| `PcmMixer` | completo | completo, é só software |
| `Socket`, `HttpClient`, `NetworkManager` | completo | `NotSupported` ou stubs sem efeito |
| Bluetooth SPP | conforme build | `IsSupported()` devolve `false` |
| `Hash`, `Hmac`, `Rsa` | completo | `NotSupported` |
| `LicenseManager` | consulta real | sempre válido, `999` dias |

## Áudio

### Conceitos

O `AudioEngine` é o dono de todos os canais. Ele mantém um `MediaStream` para a música e um array de `SoundEffect` do tamanho de `kMaxSfxChannels`, que vale `4`. O comentário no header liga esse número ao item 9.2 do Zeebo Developer Guide: quatro decodificadores ADPCM/PCM/QCP simultâneos mais um decodificador MP3 no canal 1.

`MediaStream` e `SoundEffect` embrulham `IMedia` criado por `AEEMediaUtil_CreateMedia`. Os dois chamam `IMEDIA_EnableChannelShare(TRUE)` logo depois de criar a mídia, porque sem isso o BREW entrega o decodificador com exclusividade e o segundo som falha (Developer Guide 9.5.1.1). A diferença entre as classes é o nível de controle: `MediaStream` registra notify, tem pausa, resume e volume; `SoundEffect` só toca, para e ajusta volume.

O formato é decidido pelo decodificador a partir da extensão ou dos bytes. O enum `MediaFormat` existe (`Auto`, `Wave`, `Mp3`, `Midi`, `Qcp`, `Aac`), mas o backend Zeebo ignora o parâmetro em `LoadFromMemory`, e o Windows recusa a chamada. Streaming e memória são caminhos distintos: `LoadFromFile` monta um `AEEMediaData` com `MMD_FILE_NAME`, então a música longa não ocupa RAM; `LoadFromMemory` monta `MMD_BUFFER` apontando para o seu buffer, sem copiar, então os bytes precisam continuar válidos durante toda a reprodução.

`PcmMixer` é o caminho oposto: não toca nada, só mistura. Ele mantém até `kMaxPcmVoices` (`16`) vozes em PCM 16 bits com acumulador de fase 16.16 e interpolação linear, e escreve o resultado em estéreo intercalado no buffer que você passar. Ele não se conecta ao `AudioEngine`: a música carregada por `IMedia` não entra como voz, e a mistura só existe dentro do buffer.

### Assinatura e ciclo de vida

`Initialize` recebe o contexto e prepara os quatro canais de efeito. O destrutor chama `Shutdown`, e `Shutdown` é idempotente, então não há problema em chamar antes do fim. O volume efetivo é sempre `canal * master / 100`, recalculado a cada mudança de qualquer um dos dois.

### Uso passo a passo

Passo 1, inicialização, música e efeitos. `PlayMusic` para e descarrega a faixa anterior antes de carregar a nova, então uma falha ali já perdeu a BGM. Cada `PlaySfx` ocupa o próximo canal do pool em round-robin, e o quinto efeito simultâneo reusa o canal mais antigo.

```cpp
#include "zsdk/audio/audio_engine.hpp"
#include "zsdk/debug/logger.hpp"

static zsdk::audio::AudioEngine g_audio;

void StartAudio(void* platformContext) {
    if (zsdk::Failed(g_audio.Initialize(platformContext))) {
        return;
    }
    g_audio.SetMasterVolume(100u);
    g_audio.SetMusicVolume(70u);
    const zsdk::Result r = g_audio.PlayMusic("fs:/~/audio/tema.qcp", true);
    if (zsdk::Failed(r)) {
        ZSDK_LOG_ERROR("GAME", "falha ao tocar a BGM: %d", static_cast<int>(r));
    }
}

void OnJump() {
    g_audio.PlaySfx("fs:/~/audio/pulo.wav");
}
```

Passo 2, pausa. O `AudioEngine` não expõe `PauseMusic`; para pausar sem perder a posição, controle um `MediaStream` você mesmo e use o engine só para os efeitos.

```cpp
#include "zsdk/audio/audio.hpp"

static zsdk::audio::MediaStream g_bgm;

void SetPaused(bool paused) {
    if (paused) {
        g_bgm.Pause();
    } else {
        g_bgm.Resume();
    }
}
```

Carregue esse `g_bgm` com `LoadFromFile`, `SetLooping(true)` e `Play()`, como no `AudioEngine`. Pausa e resume só existem em `MediaStream`; o `SoundEffect` não tem equivalente.

Passo 3, mixagem PCM. `Play` devolve o handle da voz ou `-1`. A partir daí, `SetVoicePitch` e `SetVoiceSampleRate` valem já na próxima chamada de `Mix`.

```cpp
#include "zsdk/audio/pcm_mixer.hpp"

static zsdk::audio::PcmMixer g_mixer(22050u);
static int16_t g_mixBuf[368u * 2u];

void StartEngineSound(const int16_t* pcm, uint32_t frames) {
    const int8_t voice = g_mixer.Play(pcm, frames, 11025u, 1u, false, 100u, 0x10000u);
    if (voice < 0) {
        return;
    }
    g_mixer.SetVoicePitch(voice, 0x18000u);
}

void MixOneFrame() {
    g_mixer.Mix(g_mixBuf, 368u);
}
```

O buffer de saída tem `2 * outFrames` amostras `int16_t`, intercaladas esquerda e direita. Com 368 frames, são `1.472` bytes, o suficiente para `1/60 s` a 22050 Hz.

### Referência da API

`AudioEngine` (`include/zsdk/audio/audio_engine.hpp`):

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(void* platformContext)` | `AlreadyInitialized` na segunda chamada |
| `PlayMusic` | `Result PlayMusic(const char* filePath, bool loop = true)` | para e descarrega a faixa anterior |
| `PlaySfx` | `Result PlaySfx(const char* filePath)` | round-robin nos 4 canais |
| `StopMusic`, `StopAllSfx` | `void` sem argumentos | `StopMusic` não descarrega o arquivo |
| `SetMasterVolume` | `void SetMasterVolume(uint8_t volumePercent)` | reaplica nos canais |
| `SetMusicVolume`, `SetSfxVolume` | `void` com `uint8_t volumePercent` | valor acima de `100` é truncado |

`MediaStream`, `SoundEffect` e `PcmMixer` (`include/zsdk/audio/`):

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `LoadFromFile`, `LoadFromMemory` | `Result` com contexto e caminho, ou buffer, tamanho e `MediaFormat` | memória exige buffer vivo |
| `Play`, `Stop`, `Pause`, `Resume` | `Result` sem argumentos | `NotInitialized` sem mídia carregada |
| `SetVolume`, `SetLooping` | `Result SetVolume(uint8_t)` e `void SetLooping(bool)` | `MM_PARM_PLAY_REPEAT` com `0` para repetir |
| `IsPlaying`, `IsLoaded` | `bool` | `IsPlaying` depende do notify do backend |
| `SoundEffect::PlayFile` | `Result PlayFile(const char* filePath)` | sem handle: não dá para parar um efeito só |
| `PcmMixer` | `explicit PcmMixer(uint32_t outputSampleRateHz = 22050u)` | `0` cai no padrão de 22050 Hz |
| `PcmMixer::Play` | `int8_t Play(const int16_t*, uint32_t, uint32_t, uint8_t = 1, bool = false, uint8_t = 100u, uint32_t = 0x10000u)` | `-1` sem voz livre |
| `PcmMixer::SetVoiceSampleRate`, `SetVoicePitch` | `Result` com `int8_t voice` e `uint32_t` | `0x10000` = 1.0x; `NotInitialized` se a voz parou |
| `PcmMixer::SetVoiceVolume`, `Mix` | `Result SetVoiceVolume(int8_t, uint8_t, uint8_t)` e `void Mix(int16_t*, uint32_t)` | saída 16 bits estéreo |

Na ABI C99 os mesmos símbolos são `zsdk_audio_*`, `zsdk_media_*`, `zsdk_sfx_*` e `zsdk_pcm_mixer_*`, declarados em `include/zsdk/c/zsdk_c_audio.h`.

### Limites e armadilhas

| Limite | Efeito prático |
| :--- | :--- |
| `kMaxSfxChannels = 4` | o quinto efeito reusa o próximo canal da rotação e corta o som que estava ali |
| 1 decodificador MP3 | `PlaySfx` recusa `.mp3` quando a BGM é `.mp3` |
| Sem pausa no `AudioEngine` | pausar exige um `MediaStream` próprio |
| Sem notify nos efeitos | não existe callback de fim de efeito |
| `PcmMixer` não tem saída | `Mix` só preenche o buffer; a SDK não entrega esse buffer ao hardware |
| `Initialize` ignora falhas | o retorno de cada `SoundEffect::Initialize` não é verificado |

`PcmMixer` custa uma passada por frame e por voz ativa. A 22050 Hz com 16 vozes são até `352.800` interpolações de voz por segundo de áudio, e esse trabalho entra no orçamento do frame que chamou `Mix`. O buffer de 368 frames precisa ser misturado a cada `1/60 s`, então trate `Mix` como parte do frame, não como algo eventual.

Latência não tem número medido nesta SDK. Nenhum caminho expõe tamanho de buffer ou prioridade de decodificador; a única janela que o jogo controla é a do `PcmMixer`. O [TODO.md](../TODO.md) registra que nada foi executado em hardware Zeebo, então os limites de voz do guia não foram confirmados no console para PCM.

### Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `PlayMusic` devolve `AudioError` | `AEEMediaUtil_CreateMedia` falhou | confira o caminho `fs:/~/` e a extensão |
| `Mix` devolve silêncio | buffer de amostras liberado antes | mantenha o PCM vivo enquanto a voz toca |
| `SetVoicePitch` devolve `NotInitialized` | a voz já terminou | teste `IsVoicePlaying` antes |

## Rede

### Conceitos

`Socket` embrulha `ISockPort` e nunca bloqueia. `Open` cria `AEECLSID_SOCKPORT` e chama `ISOCKPORT_OpenEx` com `AEE_AF_INET` e o tipo do protocolo. `ConnectIPv4` recebe o IP em ordem de host e devolve `Result::WouldBlock` quando o BREW ainda está conectando. `Send` e `Recv` devolvem a contagem de bytes, `0` com `Result::WouldBlock` quando não há progresso, ou `-1` com `Result::NetworkError`. Não existe API de servidor: sem `Bind`, `Listen` ou `Accept`, e sem endereço por datagrama, o socket UDP também precisa de `ConnectIPv4` e só conversa com aquele destino.

`HttpClient` embrulha `IWeb`. `Initialize` cria `AEECLSID_WEB`; `Get` e `Post` apenas disparam a requisição e voltam, e o desfecho chega em um `HttpResponseCallback` com o status e o corpo completo. O corpo é lido do `ISOURCE` da resposta em blocos de `2048` bytes e acumulado no heap até o teto configurado.

`NetworkManager` mantém `INetMgr` e `IWIFI`. Ele resolve nomes de forma assíncrona por `INETMGR_GetHostByName` e devolve até `AEEDNSMAXADDRS` endereços em ordem de host. `GetState` só verifica se a instância de `INetMgr` existe; não é um sinal vivo de conectividade.

`BluetoothManager` cobre o perfil SPP no papel de cliente. Ele abre `IBT`, pede o cliente SPP por `IBT_GetSPPClient` e conversa por `IBTSPPCLI_Read` e `IBTSPPCLI_Write`. Não há descoberta, pareamento nem outro perfil na API.

TLS não existe na SDK. Nem `Socket` nem `HttpClient` têm API de certificado, de chave pública ou de negociação. O `HttpClient` repassa a URL como está para o `IWeb`, então uma URL `https://` depende inteiramente da pilha BREW do aparelho, e o jogo não tem como verificar a identidade do servidor nem fixar uma chave. Na prática, token e senha viajam legíveis para quem estiver no caminho; sem TLS na pilha do aparelho, a única forma de proteger o payload é cifrar na aplicação com `Cipher`.

### Uso passo a passo

Passo 1, resolver o nome e abrir o TCP. O callback de DNS roda no laço de eventos do BREW, então `ConnectIPv4` pode ser chamado dali sem bloquear nada.

```cpp
#include "zsdk/net/network_mgr.hpp"
#include "zsdk/net/socket.hpp"
#include "zsdk/debug/logger.hpp"

static zsdk::net::NetworkManager g_net;
static zsdk::net::Socket         g_sock;
static uint16_t                  g_port = 7777u;

void OnResolved(zsdk::Result result, const uint32_t* addrs, uint32_t count, void*) {
    if (zsdk::Failed(result) || count == 0u) {
        return;
    }
    for (uint32_t i = 0u; i < count; ++i) {
        if (g_sock.ConnectIPv4(addrs[i], g_port) == zsdk::Result::Ok) {
            return;
        }
    }
}

bool BeginConnect(void* platformContext, const char* host) {
    if (zsdk::Failed(g_net.Initialize(platformContext))) {
        return false;
    }
    if (zsdk::Failed(g_sock.Open(platformContext, zsdk::net::SocketProtocol::Tcp))) {
        return false;
    }
    return zsdk::Succeeded(g_net.ResolveHost(host, OnResolved, nullptr));
}
```

Passo 2, bombear o socket uma vez por frame. O retorno precisa ser lido junto com o status: `0` com `WouldBlock` é normal, `-1` com `NetworkError` é conexão morta.

```cpp
void PumpSocket() {
    char buffer[512];
    zsdk::Result status = zsdk::Result::Ok;
    const int32_t n = g_sock.Recv(buffer, sizeof(buffer), &status);

    if (n > 0) {
        ZSDK_LOG_INFO("NET", "%d bytes recebidos", static_cast<int>(n));
        return;
    }
    if (status == zsdk::Result::WouldBlock) {
        return;
    }
    if (status == zsdk::Result::NetworkError) {
        g_sock.Close();
    }
}
```

`Send` pode enviar menos que o pedido, então compare o retorno com `bytes`, não só com `Failed`. Um envio parcial exige reenviar o restante no próximo frame.

Passo 3, uma requisição HTTP com tratamento de erro. O destino precisa viver mais que a chamada, porque o callback chega depois; use um buffer estático ou um campo do jogo.

```cpp
#include "zsdk/net/http_client.hpp"
#include "zsdk/debug/logger.hpp"

static zsdk::net::HttpClient g_http;
static char                 g_scoreText[64];

void OnScore(int32_t status, const void* body, uint32_t bodyBytes, void*) {
    if (status != 200 || !body) {
        ZSDK_LOG_WARN("NET", "resposta %d sem corpo", static_cast<int>(status));
        return;
    }
    const uint32_t n = (bodyBytes < 63u) ? bodyBytes : 63u;
    for (uint32_t i = 0u; i < n; ++i) {
        g_scoreText[i] = static_cast<const char*>(body)[i];
    }
    g_scoreText[n] = '\0';
}

bool InitHttp(void* platformContext) {
    if (zsdk::Failed(g_http.Initialize(platformContext))) {
        return false;
    }
    g_http.SetMaxResponseBytes(8u * 1024u);
    return true;
}

bool SendScore(const char* url, const char* json, uint32_t jsonBytes) {
    return zsdk::Succeeded(g_http.Post(url, "application/json", json, jsonBytes, OnScore, nullptr));
}
```

`Initialize` uma vez por app: a segunda chamada devolve `AlreadyInitialized` e derrubaria a requisição. O retorno de `Post` confirma apenas a entrega ao `IWeb`; status `404`, `500` e falha de rede só aparecem no callback, sempre com o código negativo para erro de transporte.

### Referência da API

`Socket` e `NetworkManager` (`include/zsdk/net/`):

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Socket::Open` | `Result Open(void* platformContext, SocketProtocol protocol)` | `Tcp` ou `Udp`, uma vez por socket |
| `Socket::ConnectIPv4` | `Result ConnectIPv4(uint32_t ipv4HostOrder, uint16_t port)` | `WouldBlock` enquanto conecta |
| `Socket::Send` | `int32_t Send(const void* data, uint32_t sizeBytes, Result* outStatus = nullptr)` | pode enviar menos que o pedido |
| `Socket::Recv` | `int32_t Recv(void* buffer, uint32_t maxBytes, Result* outStatus = nullptr)` | `0` e `WouldBlock` sem dados |
| `NetworkManager::GetState` | `NetworkState GetState() const` | `Disconnected`, `Connecting`, `Connected` |
| `NetworkManager::ResolveHost` | `Result ResolveHost(const char* hostname, DnsResolveCallback callback, void* userData)` | até 4 endereços, ordem de host |
| `NetworkManager::GetLocalIpAddress`, `IsWifiAvailable` | `uint32_t` e `bool` | IP local em ordem de host, `0` se ausente; Wi-Fi reporta a existência de `IWIFI` |

`HttpClient` e `BluetoothManager`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `HttpClient::Get` | `Result Get(const char* url, HttpResponseCallback callback, void* userData)` | uma requisição por vez |
| `HttpClient::Post` | `Result Post(const char*, const char* contentType, const void* body, uint32_t bodyBytes, HttpResponseCallback, void*)` | só `Content-Type` como cabeçalho |
| `HttpClient::SetMaxResponseBytes`, `CancelPending` | `void` com `uint32_t maxBytes` e sem argumentos | padrão de `262.144` bytes |
| `BluetoothManager::OpenSpp` | `Result OpenSpp(const char* portName)` | nome do serviço SPP, não MAC |
| `BluetoothManager::GetSppState`, `ReadSpp` | `SppState` e `int32_t ReadSpp(void*, uint32_t)` | `Opened` e `Connected` são distintos; leitura negativa em espera ou erro |
| `BluetoothManager::IsSupported`, `IsRadioEnabled` | `bool` | `IsSupported` reflete a instância `IBT`; o rádio vem de `IBT_GetRadioState` |

Na ABI C99, `zsdk_c_net.h` repete quase tudo com os nomes `zsdk_net_*`, `zsdk_socket_*` e `zsdk_http_*`. Duas diferenças importam: `zsdk_dns_cb_t` entrega um único endereço, não a lista, e `zsdk_net_is_available` reporta `IsWifiAvailable`.

### Limites e armadilhas

| Limite | Efeito prático |
| :--- | :--- |
| Sem TLS | tráfego em claro; não há verificação de servidor |
| Teto de 256 KiB | corpo maior é cortado sem erro |
| Uma requisição por vez | `Get` ou `Post` cancela a anterior |
| Sem timeout configurável | o prazo fica por conta do `IWeb` |
| `GetState` não é medição | devolve `Connected` com `INetMgr` presente |

O truncamento do corpo merece atenção: quando o acumulador chega ao teto, `AppendBodyBytes` recusa os bytes seguintes e a resposta é entregue com o status HTTP normal. Um `200` pode significar JSON incompleto, então `SetMaxResponseBytes` precisa caber no maior corpo esperado, e o servidor deve poder responder em partes pequenas.

O corpo passado ao callback é liberado quando o callback retorna. Copie o que precisar, ou trate o ponteiro como válido só dentro da função. O mesmo vale para o corpo do `Post`, que é copiado para o heap e liberado ao fim da requisição, então você pode passar um buffer temporário.

### Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| Callback chega com status `-1` | transporte falhou ou não há rede | trate como erro e repita com intervalo |
| Corpo cortado no meio | teto de `262.144` bytes | aumente `SetMaxResponseBytes` |
| Requisição anterior nunca responde | `Get`/`Post` cancelaram a anterior | espere o callback antes da próxima |
| `Recv` devolve `0` sempre | você ignorou `outStatus` | compare com `WouldBlock` |
| `GetState` diz `Connected` sem rede | só checa a instância `INetMgr` | use o resultado de uma requisição real |

## Segurança

### Conceitos

As primitivas implementadas cabem em uma lista curta. `Hash` faz MD5 (`16` bytes) e SHA-1 (`20` bytes). `Hmac` faz HMAC-MD5 e HMAC-SHA1 sobre as mesmas classes de contexto. `Cipher` faz AES-128, AES-192, AES-256, DES, 3DES e ARC4, nos modos ECB, CBC, CFB, CTR e fluxo (ARC4), com padding nenhum, zero ou PKCS7. `Rsa` faz exponenciação modular sobre `IRSA`. `Random` lê `AEECLSID_RANDOM` e cai em `GETRAND` se a instância não existir. Fora isso não existe SHA-256, AES-GCM, ChaCha20, derivação de chave (PBKDF2, scrypt, Argon2, HKDF) nem comparação em tempo constante, e não existe keystore: quem tem o `.mod` tem a chave.

Cada objeto cria a instância BREW na hora de começar. `Hash::Begin` cria `AEECLSID_MD5CTX` ou `AEECLSID_SHA1CTX`, e `Hash::Final` libera essa instância. Um `Final` por `Begin`: reutilizar o objeto exige novo `Begin`, e chamar `Final` duas vezes devolve `NotInitialized`.

A licença não tem formato próprio na SDK. `LicenseManager::CheckLicense` consulta a instância `AEECLSID_LICENSE` do BREW e devolve três campos: `valid`, `expired` e `remainingDays`. O que define a licença é o sistema do console, ligado ao app instalado, não um arquivo que a SDK interpreta.

### Uso passo a passo

Passo 1, checksum de save com MD5. MD5 detecta corrupção, não impede adulteração: recalcular é trivial. Grave os `16` bytes do digest na frente do payload com `Hash::Compute` e, na leitura, recalcule e compare. A comparação abaixo é escrita à mão porque a SDK não tem função de comparação; o laço não sai no primeiro byte diferente.

```cpp
#include "zsdk/security/crypto.hpp"

typedef zsdk::security::Hash Hash;

bool CheckSave(void* platformContext, const uint8_t* file, uint32_t fileBytes) {
    if (fileBytes <= Hash::kMd5DigestSize) {
        return false;
    }
    uint8_t expected[Hash::kMd5DigestSize];
    if (zsdk::Failed(Hash::Compute(platformContext, zsdk::security::HashAlgorithm::Md5,
                                   file + Hash::kMd5DigestSize,
                                   fileBytes - Hash::kMd5DigestSize,
                                   expected, sizeof(expected)))) {
        return false;
    }
    uint8_t diff = 0u;
    for (uint32_t i = 0u; i < Hash::kMd5DigestSize; ++i) {
        diff |= static_cast<uint8_t>(expected[i] ^ file[i]);
    }
    return diff == 0u;
}
```

Passo 2, derivar chave e cifrar. Sem KDF na SDK, a opção disponível é HMAC sobre senha e salt. Isso não substitui PBKDF2: uma única passada de HMAC-SHA1 é barata de atacar por força bruta, então trate a chave resultante como ofuscada, não como secreta.

```cpp
static void DeriveKey(void* platformContext, const void* pass, uint32_t passBytes,
                      const void* salt, uint32_t saltBytes, uint8_t* out16) {
    uint8_t mac[Hash::kSha1DigestSize];
    const zsdk::Result r = zsdk::security::Hmac::Compute(
        platformContext, zsdk::security::HashAlgorithm::Sha1,
        pass, passBytes, salt, saltBytes, mac, sizeof(mac));
    if (zsdk::Failed(r)) {
        return;
    }
    for (uint32_t i = 0u; i < 16u; ++i) {
        out16[i] = mac[i];
    }
}
```

Com a chave em mãos, o caminho de cifra é `Cipher::Initialize` com algoritmo, modo, direção, padding, chave e IV, depois `Process` para os bytes e `ProcessLast` para fechar o padding. AES-128 exige `16` bytes de chave; o IV de CBC também tem `16` bytes e precisa ser gravado junto do texto cifrado, porque sem ele a decifra não começa. O tipo `Cipher` cria uma instância `ICipher1` a cada `Initialize`, então reuse o objeto para o bloco inteiro em vez de criar um por registro.

Passo 3, licença. `CheckLicense` devolve `NotSupported` quando o console não expõe `AEECLSID_LICENSE`, e aí `valid` continua `false`. Trate a ausência como não licenciado, nunca como liberado.

```cpp
#include "zsdk/debug/logger.hpp"

void CheckAppLicense(void* platformContext) {
    zsdk::security::LicenseStatus status = {false, false, 0u};
    const zsdk::Result r = zsdk::security::LicenseManager::CheckLicense(platformContext, status);
    if (zsdk::Failed(r) || !status.valid || status.expired) {
        ZSDK_LOG_WARN("LIC", "licenca invalida ou expirada");
    }
}
```

### Referência da API

`include/zsdk/security/crypto.hpp`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Hash::Compute` | `static Result Compute(void* platformContext, HashAlgorithm, const void* data, uint32_t sizeBytes, void* outDigest, uint32_t digestBufferBytes, uint32_t* outDigestBytes = nullptr)` | MD5 ou SHA-1 |
| `Hash::Begin` / `Update` / `Final` | `Result` com o contexto e o buffer | streaming, um `Final` por `Begin` |
| `Hmac::Compute` | `static Result Compute(void* platformContext, HashAlgorithm, const void* key, uint32_t keyBytes, const void* data, uint32_t sizeBytes, void* outMac, uint32_t macBufferBytes, uint32_t* outMacBytes = nullptr)` | tag com o tamanho do hash |
| `Cipher::Initialize` | `Result Initialize(void* platformContext, CipherAlgorithm, CipherMode, CipherDirection, CipherPadding, const void* key, uint32_t keyBytes, const void* iv = nullptr, uint32_t ivBytes = 0u)` | IV opcional no papel |
| `Cipher::Process`, `ProcessLast` | `Result Process(const void* inData, uint32_t inBytes, void* outData, uint32_t outBufferBytes, uint32_t* outBytesWritten)` e `Result ProcessLast(void*, uint32_t, uint32_t*)` | saída com folga de um bloco; `ProcessLast` fecha o padding |
| `Cipher::GetBlockSize` | `uint32_t GetBlockSize() const` | `1` para ARC4, `0` sem inicializar |
| `Rsa::ModExp` | `static Result ModExp(void* platformContext, const void* modulus, uint32_t modulusBytes, const void* exponent, uint32_t exponentBytes, const void* inData, uint32_t inBytes, void* outData, uint32_t outBufferBytes, uint32_t* outBytesWritten = nullptr)` | síncrono, big-endian |
| `Random::GetBytes`, `GetUInt32` | `static Result GetBytes(void* platformContext, void* outBuffer, uint32_t sizeBytes)` e `static uint32_t GetUInt32(void* platformContext)` | `IRandomSource`, com fallback `GETRAND` |

`include/zsdk/security/license.hpp`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `LicenseManager::CheckLicense` | `static Result CheckLicense(void* platformContext, LicenseStatus& outStatus)` | `NotSupported` sem `AEECLSID_LICENSE` |
| `LicenseStatus` | `bool valid`, `bool expired`, `uint32_t remainingDays` | `valid` só vira `true` se `ILICENSE_GetInfo` funcionar; `remainingDays` é o valor bruto |

Na ABI C99, `zsdk_c_security.h` expõe `zsdk_random_bytes`, `zsdk_random_u32`, `zsdk_hash_compute`, `zsdk_hmac_compute`, `zsdk_rsa_mod_exp` e `zsdk_license_check`. `Cipher` não tem equivalente em C.

### Limites e armadilhas

| Limite | Efeito prático |
| :--- | :--- |
| Só MD5 e SHA-1 | nenhum hash moderno disponível |
| Sem KDF | derivação de chave fica com uma passada de HMAC |
| Sem comparação em tempo constante | a verificação precisa ser escrita no jogo |
| Sem keystore | a chave viaja dentro do `.mod` |
| `Rsa::ModExp` é síncrono | bloqueia o frame durante a operação |
| Windows sem hash, HMAC e RSA | validação de save não roda no PC |
| `Random` no PC é LCG | sequência previsível, não use para segurança no desktop |

`Rsa::ModExp` recebe o bloco de entrada do tamanho do módulo e pode devolver uma saída mais curta, porque o `IRSA_RSA` não preenche à esquerda: alinhe à direita pelo tamanho do módulo antes de comparar com uma assinatura. O cálculo é uma exponenciação modular de 1024 bits ou mais na thread que chamou, então faça isso na inicialização ou em uma `Thread`, nunca no meio de um frame.

### Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `Hash::Compute` devolve `NotSupported` | backend Windows | rode a validação no Zeebo ou no emulador |
| `Hash::Final` devolve `InvalidArg` | buffer menor que o digest | use `kMd5DigestSize` ou `kSha1DigestSize` |
| `Cipher::Initialize` devolve `InvalidArg` | chave com tamanho errado | AES-128 = `16`, AES-192 = `24`, AES-256 = `32` |
| `Cipher::Process` devolve `Error` | saída curta ou estado inválido | reserve `inBytes + GetBlockSize()` |
| `CheckLicense` devolve `NotSupported` | sem `AEECLSID_LICENSE` | trate como não licenciado |
| `valid` falso e `expired` falso | `ILICENSE_GetInfo` falhou | não use `valid` isolado |

## Veja também

- [06 Filesystem e savegame](./06_filesystem_e_savegame.md): onde o checksum deste capítulo entra e o limite de 64 KB do save.
- [02 Core e async](./02_core_e_async.md): `Thread` para tirar RSA do frame e o laço que bombeia socket.
- [03 Debug e profiler](./03_debug_e_profiler.md) e [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md): os subsistemas `Audio` e `Network` do `MemoryTracker` e o que é stub no Windows.
- Código: [audio_engine.hpp](../include/zsdk/audio/audio_engine.hpp), [pcm_mixer.hpp](../include/zsdk/audio/pcm_mixer.hpp), [crypto.hpp](../include/zsdk/security/crypto.hpp) e [TODO.md](../TODO.md), com as limitações ainda não validadas em hardware.
