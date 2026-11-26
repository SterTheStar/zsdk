# 06 Filesystem e save

O console grava saves numa NAND que o jogo só pode tocar dentro do próprio diretório, e o Zeebo Developer Guide limita a 64 KB o conjunto de dados de save. A camada `zsdk::fs` existe para que essas duas restrições não dependam da disciplina de quem escreve o jogo: `FileSystem` fala com o `IFileMgr` do BREW e aplica a trava de escrita, `Vfs` troca caminhos nativos por prefixos portáveis, e `SaveGameManager` grava slots com magic, versão e CRC32 num arquivo temporário que só substitui o antigo quando está completo. Use esses serviços para tudo que precisa sobreviver ao desligamento do console. Não os use para assets que caberiam dentro do `.mod`: ler megabytes da NAND custa frame e ocupa o mesmo heap de 32 MB que as texturas.

## Visão rápida

| Símbolo | O que faz | Precisa de `platformContext` | Estado no Windows |
| :--- | :--- | :--- | :--- |
| `FileSystem` | Abre, lê, escreve, remove, renomeia, mede espaço livre e enumera diretório | sim | completo |
| `File` | Handle RAII sobre `IFile*` (`std::FILE*` no PC) | não | completo |
| `Vfs` | Prefixos de montagem sobre um `FileSystem` interno | sim | completo, sem `sd:` |
| `SaveGameManager` | Diretório `udata/` e slots com header, versão e CRC32 | sim | completo |
| `DatabaseManager`, `Database`, `DbRecord` | Registros sobre `IDBMgr` / `IDatabase` / `IDBRecord` | sim | sem implementação |
| `LocalStorage` | Chave-valor simples, um arquivo por chave | sim | stub |
| `RamCache` | Cache em RAM com limite de bytes | sim | stub |
| `UnzipStream` | Descompressão streaming sobre `IUnzipStream` | sim | stub |

"Stub" quer dizer que o símbolo existe e liga, mas não persiste nada: `LocalStorage::Write` devolve `Result::Ok` e `Read` devolve `Result::NotFound`, `RamCache::Get` também devolve `NotFound`, e `UnzipStream::Read` devolve `-1`. `Database` é pior: `src/platform/windows/windows_backend.cpp` não define nenhum método da classe, então usar `DatabaseManager` num build Windows quebra no link, não em tempo de execução.

## Pré-requisitos

- Uma `zsdk::IAppDelegate` em execução, porque `Initialize` recebe o contexto de `Application::GetInstance()->GetNativePlatformContext()`, que é o `IShell*` no Zeebo e um ponteiro ignorado no Windows.
- No Zeebo, o `.mif` gerado a partir de [zsdk_template.mif.in](../tools/mif/zsdk_template.mif.in), que traz `0x07AF` (sem `PL_SHARED_WRITE`) e a dependência `0x0101414E` (`AEECLSID_CARD0_ACCESS`). Sem `PL_FILE` no conjunto, nenhuma abertura de arquivo funciona.
- Uma decisão sobre onde mora o dado: slot com CRC, arquivo livre em `udata/`, banco de registros ou chave-valor. São quatro coisas com garantias diferentes, e misturá-las produz arquivos que ninguém consegue diagnosticar depois.

## Conceitos

### O mapa de caminhos do console

O BREW expõe o armazenamento como uma árvore única com prefixo `fs:`. A ZSDK não inventa raízes: usa os caminhos que o firmware aceita e recusa o resto.

| Caminho | O que é | O jogo pode escrever |
| :--- | :--- | :--- |
| `fs:/~/` | Diretório privado do aplicativo | sim |
| `fs:/~/udata/` | Convenção da SDK para dados de usuário e slots | sim |
| `fs:/card0/` | Cartão SD | sim, não comprovado no console |
| `fs:/mod/<jogo>/` | Diretório do módulo na NAND | não pela SDK |
| `caminho relativo` | Resolvido pelo BREW dentro do diretório do app | sim |
| `Windows` | Caminho do sistema de arquivos, relativo ao diretório de trabalho | sim |

O `SaveGameManager` monta `fs:/~/udata/<arquivo>` no Zeebo e `udata/<arquivo>` no Windows, sempre com `BuildUdataPath`. O `TODO.md` registra que o guia do console descreve o save como `/mod/<jogo>/udata/` e que o código da SDK trata `fs:/~/` como sandbox do app, então qual dos dois o firmware mapeia para qual é uma suposição do projeto, não um fato do guia.

A forma portável do mesmo dado é um prefixo do `Vfs`: `udata:/save_00.sav`, `home:/assets/level1.bin`, `script:/main.lua`. No Zeebo o prefixo só troca a raiz; no Windows troca por um diretório relativo ao executável (`./`, `scripts/`, `udata/`).

### A trava de escrita na NAND

`ZSDK_NAND_WRITE_GUARD` está definido como `1` em [config.hpp](../include/zsdk/core/config.hpp) e é consultado por `IsWritePathAllowed` em [zeebo_fs.cpp](../src/platform/zeebo/zeebo_fs.cpp). Ele não olha nomes de diretórios de sistema: a regra é uma allowlist de prefixos, sem exceção.

| Operação | Passa a trava | Como falha |
| :--- | :--- | :--- |
| `Open` com `FileMode::Read` | sempre | não falha pela trava |
| `Open` com `Create`, `ReadWrite` ou `Append` | só em `fs:/~/` ou `fs:/card0/` | `Result::IoError` antes de abrir |
| `Remove`, `Rename`, `MkDir` | só em `fs:/~/` ou `fs:/card0/` | `Result::IoError` |

A comparação ignora maiúsculas e minúsculas, então `FS:/MOD/x` é bloqueado igual a `fs:/mod/x`. Caminho relativo passa direto porque o BREW o resolve dentro do diretório do app; caminho vazio é sempre bloqueado. `Rename` exige que origem e destino passem, o que impede mover algo do sistema para dentro do sandbox.

Para desligar a trava, compile com `-DZSDK_NAND_WRITE_GUARD=0`. O `#ifndef` em `config.hpp` respeita o valor já definido, então a definição da linha de comando vence. Não existe opção de CMake para isso: no `CMakeLists.txt` do seu projeto, é `target_compile_definitions(seu_alvo PRIVATE ZSDK_NAND_WRITE_GUARD=0)`. Desligar serve para depurar no emulador, não para release: o custo é um `Remove` no caminho errado tornar o console inutilizável.

### FileSystem, Vfs e SaveGameManager: qual usar

`Vfs::Initialize` instancia um `FileSystem` privado e `SaveGameManager::Initialize` faz o mesmo, então cada serviço tem sua própria conexão com o `IFileMgr`. `Vfs::Raw()` devolve a referência interna para `EnumerateDirectory`, que o `Vfs` não expõe. O `TODO.md` anota a duplicação como dívida: o `SaveGameManager` deveria usar o `Vfs`.

1. `SaveGameManager` se você quer o formato com header e CRC32, slots numerados e recuperação do `.tmp`. É o caminho para progresso de jogo e nada mais.
2. `Vfs` se você quer caminhos portáveis e tem vários arquivos e diretórios, ou um script Lua que precisa carregar do mesmo jeito no PC e no console. É também o que o `debug::FileLogSink` espera.
3. `FileSystem` cru se precisa enumerar diretório, medir espaço livre de verdade ou trabalhar com caminho nativo que já sabe qual é.

Não grave o mesmo dado com `FileSystem` e com `SaveGameManager`: os dois apontam para o mesmo diretório, mas só um escreve header e CRC.

### Header de save, versão e CRC32

Cada slot é um arquivo `udata/save_XX.sav` com 16 bytes de cabeçalho antes do payload.

| Campo | Tipo | Valor gravado |
| :--- | :--- | :--- |
| `magic` | `uint32_t` | `0x3156535A`, os bytes `ZSV1` |
| `version` | `uint32_t` | o `gameVersion` passado na chamada, padrão `1` |
| `payloadSize` | `uint32_t` | tamanho exato do payload |
| `crc32` | `uint32_t` | CRC32 do payload, polinômio `0xEDB88320`, refletido |

`LoadSlot` valida nesta ordem: magic, `version` igual ao `expectedVersion` recebido, `payloadSize` menor ou igual ao `maxBytes` do seu buffer, e por fim o CRC do que foi lido. Qualquer um dos quatro falhando vira `Result::IoError`. A versão não é migrada: um save da versão 1 lido com `expectedVersion = 2` é rejeitado, então a migração é sua.

A gravação é sempre por troca: `SaveSlot` escreve `udata/save_XX.tmp` inteiro, fecha o arquivo, remove o `save_XX.sav` anterior e renomeia o `.tmp`. A ordem importa porque no BREW `IFILEMGR_Rename` falha se o destino existe. Se a energia cair entre o `Remove` e o `Rename`, sobra um `.tmp` completo e válido, e `LoadSlot` o aceita como fallback com um `ZSDK_LOG_WARN`. O `TODO.md` diz que essa recuperação foi testada no emulador, mas não sob queda de energia real.

### Save assíncrono e o limite de frame

`BeginSaveSlot` copia até `kAsyncSaveMaxBytes` (4096 bytes) para um buffer interno e escreve na hora os 16 bytes do header zerados. `PumpAsync(budgetBytes)` grava até `budgetBytes` do payload por chamada; quando o último byte entra, volta ao início com `Seek`, regrava o header com magic, versão, tamanho e CRC corretos, fecha e faz o mesmo commit do caminho síncrono.

| Estado | Significado |
| :--- | :--- |
| `AsyncSaveStatus::Idle` | nenhum job iniciado, ou estado reposto por `Shutdown` |
| `AsyncSaveStatus::Pending` | ainda há bytes para escrever |
| `AsyncSaveStatus::Done` | o último job terminou, o arquivo foi trocado |
| `AsyncSaveStatus::Failed` | o último job falhou; o `.tmp` foi removido se o header não chegou a ser regravado |

O `LuaVM` chama `PumpAsync()` sozinho em `Update()`, então script não precisa bombear; em C++ o loop do job é seu. O que ainda bloqueia o frame: a escrita de 16 bytes no `BeginSaveSlot`, cada `IFILE_Write` de `PumpAsync` (síncrono, e a NAND pode gastar mais que o orçamento pedido), a cópia de até 4 KiB e a finalização com `Seek`, header, `Remove` e `Rename`. O `TODO.md` registra que o save assíncrono diluiu a escrita por frame mas não usa `IThread`, e que medir a travada real na NAND continua pendente.

### Assinatura e configuração

```cpp
#include "zsdk/fs/vfs.hpp"
#include "zsdk/fs/savegame.hpp"

class MyApp : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        void* ctx = zsdk::Application::GetInstance()->GetNativePlatformContext();
        // Vfs cria fs:/~/udata/ no Zeebo e udata/ no PC
        zsdk::Result r = m_vfs.Initialize(ctx, /*preferSdCard=*/false);
        if (zsdk::Failed(r)) {
            return r;
        }
        r = m_save.Initialize(ctx);
        if (zsdk::Failed(r)) {
            return r;
        }
        m_vfs.Mount("mods:", "fs:/~/mods/");
        return zsdk::Result::Ok;
    }

    void OnShutdown() override {
        m_save.Shutdown();
        m_vfs.Shutdown();
    }

private:
    zsdk::fs::Vfs             m_vfs;
    zsdk::fs::SaveGameManager m_save;
};
```

Chamar `Initialize` duas vezes devolve `Result::AlreadyInitialized`, e `Shutdown` é idempotente. `SaveGameManager::Initialize` ignora o resultado do `MkDir` que cria o diretório de `udata`, porque no BREW criar um diretório existente falha; se a criação der errado por outro motivo, o erro aparece na primeira abertura de arquivo, não na inicialização.

## Uso passo a passo

### Ler e escrever um arquivo binário

Toda escrita precisa de código de erro explícito: `File::Write` devolve quantos bytes saíram e `-1` quando o handle está inválido ou o buffer é nulo, e `File::Read` segue a mesma regra. Nunca confie que gravou o que pediu.

```cpp
#include "zsdk/fs/vfs.hpp"
#include "zsdk/debug/logger.hpp"

struct Config {
    uint32_t volumePercent;
    uint8_t  language;
};

static zsdk::Result SaveConfig(zsdk::fs::Vfs& vfs, const Config& cfg) {
    zsdk::fs::File f;
    const zsdk::Result r = vfs.Open("udata:/config.bin", zsdk::fs::FileMode::Create, f);
    if (zsdk::Failed(r)) {
        ZSDK_LOG_ERROR("CFG", "abertura falhou: %d", static_cast<int>(r));
        return r;
    }
    if (f.Write(&cfg, sizeof(cfg)) != static_cast<int32_t>(sizeof(cfg))) {
        return zsdk::Result::IoError;
    }
    return zsdk::Result::Ok;
}

static bool LoadConfig(zsdk::fs::Vfs& vfs, Config& out) {
    zsdk::fs::File f;
    if (zsdk::Failed(vfs.Open("udata:/config.bin", zsdk::fs::FileMode::Read, f))) {
        return false;
    }
    return f.Read(&out, sizeof(out)) == static_cast<int32_t>(sizeof(out));
}
```

`File` fecha no destrutor, então o `File f` local é o `Close` do par abrir/fechar. `FileMode::Create` não trunca explicitamente: no Zeebo vira `_OFM_CREATE | _OFM_READWRITE` e no Windows vira `"wb+"`. Se o novo conteúdo for menor que o antigo e você precisa que o arquivo encolha, chame `Remove` antes de abrir. `Flush` não faz nada no Zeebo (a escrita do `IFile` não tem camada de buffer) e é `fflush` no Windows.

### Salvar e carregar um slot

Este é o caminho do exemplo `01_spinning_cube`: uma struct de configurações, um slot e validação de tamanho depois da carga.

```cpp
#include "zsdk/fs/savegame.hpp"

static const uint32_t kSaveVersion = 2u;

struct Progress {
    uint32_t fase;
    uint32_t moedas;
    uint8_t  personagem;
};

static bool SaveProgress(zsdk::fs::SaveGameManager& saves, const Progress& p) {
    const zsdk::Result r = saves.SaveSlot(0, &p, sizeof(p), kSaveVersion);
    if (zsdk::Failed(r)) {
        ZSDK_LOG_ERROR("SAVE", "slot 0 nao gravou: %d", static_cast<int>(r));
        return false;
    }
    return true;
}

static bool LoadProgress(zsdk::fs::SaveGameManager& saves, Progress& out) {
    uint32_t lidos = 0u;
    const zsdk::Result r = saves.LoadSlot(0, &out, sizeof(out), lidos, kSaveVersion);
    return zsdk::Succeeded(r) && lidos == sizeof(out);
}
```

`LoadSlot` devolve em `outBytesRead` o tamanho declarado no header, que pode ser menor que `sizeof(out)` se você mudou a struct. Comparar com `sizeof` é o que distingue "save da versão certa" de "save de um layout antigo que passou no CRC". Se a struct crescer, suba o `kSaveVersion` em vez de aceitar o arquivo antigo.

### Salvar sem travar o frame

O job começa num evento de menu e termina alguns frames depois. Guarde um flag: `PumpAsync` já devolve o estado, mas esquecer de bombear é o erro mais comum.

```cpp
class SaveUi {
public:
    void RequestSave(const Progress& p) {
        m_pending = zsdk::Succeeded(m_saves.BeginSaveSlot(2, &p, sizeof(p), kSaveVersion));
    }

    void Update() {
        if (!m_pending) {
            return;
        }
        const zsdk::fs::AsyncSaveStatus st = m_saves.PumpAsync(1024u);
        if (st == zsdk::fs::AsyncSaveStatus::Failed) {
            ZSDK_LOG_ERROR("SAVE", "save assincrono do slot 2 falhou");
        }
        m_pending = (st == zsdk::fs::AsyncSaveStatus::Pending);
    }

private:
    zsdk::fs::SaveGameManager m_saves;
    bool                      m_pending = false;
};
```

`BeginSaveSlot` com um job `Pending` devolve `Result::WouldBlock`, então não há fila: um save por vez. `PumpAsync(0)` é tratado como `1` byte, o que levaria 4096 frames para terminar o pior caso de 4 KiB. Enquanto o job não termina, o `save_02.sav` no disco continua o antigo e o `.tmp` novo tem magic inválido, então um `LoadSlot` durante a gravação devolve o save anterior.

### Arquivos soltos, cache e chave-valor

Para dado que não precisa de CRC nem de slot, `WriteUserFile` e `ReadUserFile` embrulham a abertura e a trava de 64 KB. `profile.dat` é um arquivo comum em `udata/`, e `HasSlot` e `DeleteSlot` não o enxergam.

```cpp
static bool SaveProfile(zsdk::fs::SaveGameManager& saves, const Progress& p) {
    if (zsdk::Failed(saves.WriteUserFile("profile.dat", &p, sizeof(p)))) {
        return false;
    }
    Progress check{};
    uint32_t lidos = 0u;
    const zsdk::Result r = saves.ReadUserFile("profile.dat", &check, sizeof(check), lidos);
    return zsdk::Succeeded(r) && lidos == sizeof(check);
}
```

`WriteUserFile` recusa payload maior que `kMaxSaveDataBytes` com `Result::InvalidArg` e, quando os saves não estão no SD, consulta `SystemInfo::GetMemoryInfo` e falha com `Result::IoError` se `freeFsBytes` for maior que zero e menor que o payload mais 4 KiB. `ReadUserFile` lê até `maxBytes` sem comparar com o tamanho do arquivo: um arquivo maior é truncado silenciosamente no seu buffer, e o `outBytesRead` é a única pista.

`RamCache` e `LocalStorage` guardam bytes sob uma chave, sem formato nem validação. `LocalStorage` no Zeebo é um arquivo por chave, `zsdk_kv_<key>.bin` montado em `SNPRINTF` num buffer de 96 bytes, e a chave não passa por sanitização: uma chave com `/` muda o caminho de gravação e uma chave longa é truncada junto com o nome do arquivo. Use chaves curtas em `[a-z0-9_]`.

`RamCache::Clear` chama `IRAMCACHE_SetMaxSize(cache, 0)` e a classe não guarda o `maxBytes` recebido no `Initialize`, então não há caminho de volta para a capacidade original: só `Shutdown` seguido de `Initialize` reabrem o cache com espaço.

### Descomprimir um pacote

`UnzipStream` recebe um `IAStream*` do BREW, que a ZSDK não expõe a partir de `File` (`m_nativeFile` é privado). Na prática você abre o arquivo comprimido com o BREW direto, ou recebe o stream de outra camada, e entrega o ponteiro.

```cpp
zsdk::Result DecompressTo(void* ctx, void* nativeIAStream, uint8_t* out, uint32_t outBytes) {
    zsdk::fs::UnzipStream z;
    zsdk::Result r = z.Initialize(ctx);
    if (zsdk::Failed(r)) {
        return r;
    }
    if (zsdk::Failed(r = z.SetInputStream(nativeIAStream))) {
        z.Shutdown();
        return r;
    }
    uint32_t total = 0u;
    while (total < outBytes) {
        const int32_t n = z.Read(out + total, outBytes - total);
        if (n <= 0) {
            break;
        }
        total += static_cast<uint32_t>(n);
    }
    z.Shutdown();
    return (total == outBytes) ? zsdk::Result::Ok : zsdk::Result::IoError;
}
```

`Read` devolve `-1` para handle inválido ou buffer nulo e o que o `IUNZIPASTREAM_Read` devolver no resto, então o fim do stream pode chegar como `0`. Trate os dois como fim e compare o total com o tamanho esperado.

## Referência da API

### FileSystem e File

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(void* platformContext)` | Cria `AEECLSID_FILEMGR`; `InvalidArg` com contexto nulo |
| `Open` | `Result Open(const char* path, FileMode mode, File& outFile)` | `NotFound` para qualquer falha do `IFILEMGR_OpenFile`; `IoError` se a trava bloquear |
| `Exists` | `bool Exists(const char* path) const` | `IFILEMGR_Test` no Zeebo, `fopen` no Windows, que não abre diretório |
| `Rename` | `Result Rename(const char* srcPath, const char* dstPath)` | Falha se o destino existe no BREW; no Windows substitui |
| `GetFreeBytes` | `uint32_t GetFreeBytes(uint32_t* outTotal = nullptr) const` | `0` em erro, e `0` também é espaço livre legítimo |
| `EnumerateDirectory` | `Result EnumerateDirectory(const char* dirPath, bool directoriesOnly, EnumerateDirCallbackFn callback, void* userData = nullptr)` | `NotSupported` no Windows |
| `ReadAllBytes` | `Result ReadAllBytes(const char* path, void** outBuffer, uint32_t& outSize)` | Aloca via `MemoryTracker`; libere com `FreeBuffer` |
| `FreeBuffer` | `static void FreeBuffer(void* buffer)` | Par obrigatório de `ReadAllBytes` |
| `File::Read` | `int32_t Read(void* buffer, uint32_t bytes)` | `-1` para handle inválido ou `bytes == 0` |
| `File::Write` | `int32_t Write(const void* buffer, uint32_t bytes)` | Devolve os bytes gravados, não `Result` |
| `File::Seek` | `Result Seek(SeekOrigin origin, int32_t offset)` | `Start`, `Current`, `End` |
| `File::GetSize` | `uint32_t GetSize() const` | `IFILE_GetInfo`; `0` em erro, indistinguível de arquivo vazio |

`FileEntry` entrega `name` (até 64 bytes), `size`, `creationDate`, `isDirectory`, `isReadOnly` e `isHidden`. O callback devolve `true` para continuar e `false` para parar; nenhuma alocação acontece por entrada.

### Vfs

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(void* platformContext, bool preferSdCard = false)` | Instala `home:`, `script:` e `udata:` |
| `Mount` | `Result Mount(const char* prefix, const char* root)` | Prefixo de 2 a 12 caracteres terminando em `:`; substitui montagem existente |
| `Unmount` | `bool Unmount(const char* prefix)` | `false` se o prefixo não está montado |
| `Resolve` | `Result Resolve(const char* path, char* outNative, size_t maxLen) const` | `InvalidArg` para `..` ou overflow |
| `Rename` | `Result Rename(const char* srcPath, const char* dstPath)` | Resolve as duas pontas antes de renomear |
| `IsUsingSdCard` | `bool IsUsingSdCard() const` | Verdadeiro só com `preferSdCard` e cartão presente |
| `Raw` | `FileSystem& Raw()` | Acesso ao `FileSystem` interno, para `EnumerateDirectory` |

Os limites são `kMaxMounts = 8`, `kMaxPrefixLen = 12`, `kMaxRootLen = 64` e `kMaxPathLen = 128`, este último como buffer de caminho na pilha de cada operação. Prefixo não montado passa sem tradução, então `fs:/~/x` chega ao `FileSystem` como veio e `mods:/x` sem `Mount` seria um caminho nativo inválido. A recusa de `..` vale só para caminho montado: um caminho que já é nativo passa sem checagem e depende da trava da NAND.

### SaveGameManager

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(void* platformContext, bool preferSdCard = false)` | Cria o diretório de `udata`; ignora o resultado do `MkDir` |
| `SaveSlot` | `Result SaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion = 1u)` | `InvalidArg` sem dados, com `sizeBytes == 0` ou slot `>= 16`; `WouldBlock` se o mesmo slot está em job pendente |
| `LoadSlot` | `Result LoadSlot(uint8_t slotIndex, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead, uint32_t expectedVersion = 1u)` | Cai no `.tmp` se o `.sav` falhar |
| `BeginSaveSlot` | `Result BeginSaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion = 1u)` | Copia até 4096 bytes; `InvalidArg` acima disso |
| `PumpAsync` | `AsyncSaveStatus PumpAsync(uint32_t budgetBytes = 1024u)` | Também finaliza o job quando o último byte entra |
| `GetAsyncStatus` | `AsyncSaveStatus GetAsyncStatus() const` | Espelho do estado, sem escrever nada |
| `WriteUserFile` | `Result WriteUserFile(const char* filename, const void* data, uint32_t sizeBytes)` | Recusa acima de 64 KB |
| `ReadUserFile` | `Result ReadUserFile(const char* filename, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead)` | Trunca em `maxBytes` sem avisar |
| `HasSlot` / `DeleteSlot` | `bool HasSlot(uint8_t) const` / `Result DeleteSlot(uint8_t)` | Operam sobre `save_XX.sav` |
| `BuildUdataPath` | `void BuildUdataPath(const char* filename, char* outPath, size_t maxLen) const` | Monta o caminho completo de `udata/<filename>` |

As constantes são `kMaxSaveSlots = 16`, `kMaxSaveDataBytes = 65536`, `kAsyncSaveMaxBytes = 4096`, header de `16` bytes e margem de espaço livre de `4096` bytes além do payload. `SaveSlot` exige espaço livre maior ou igual a payload mais 16 mais 4096, senão devolve `Result::IoError` e loga `Not enough free space`. `BeginSaveSlot` na mesma situação marca o estado como `Failed` e devolve `IoError`, sem arquivo nenhum.

### Database

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `DatabaseManager::OpenDatabase` | `Result OpenDatabase(const char* path, bool createIfMissing, Database& outDb, uint32_t minSize = 0u, uint16_t minRecords = 0u)` | `IoError` quando cria e falha; `NotFound` quando `createIfMissing` é falso |
| `DatabaseManager::SetCacheSize` | `void SetCacheSize(int32_t bytes)` | Repassa para `IDBMGR_SetCacheSize` |
| `Database::CreateRecord` | `Result CreateRecord(const DbField* fields, int32_t fieldCount, DbRecord& outRecord)` | `OutOfMemory` se a conversão de mais de 16 campos não alocar |
| `Database::GetNextRecord` | `Result GetNextRecord(DbRecord& outRecord)` | `NotFound` no fim da enumeração |
| `DbRecord::Update` | `Result Update(const DbField* fields, int32_t fieldCount)` | Substitui todos os campos do registro |
| `DbRecord::NextField` | `DbFieldType NextField(uint8_t& outName, uint16_t& outLen)` | `DbFieldType::None` no fim |
| `DbRecord::GetField` | `const void* GetField(uint8_t& outName, DbFieldType& outType, uint16_t& outLen)` | Buffer interno do BREW; não libere |

`DbField` tem `type`, `name` (`uint8_t`), `dataLen` e `data`, e o `data` não é copiado: precisa continuar válido durante a chamada que o usa.

### LocalStorage, RamCache e UnzipStream

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `LocalStorage::Write` | `Result Write(const char* key, const void* data, uint32_t sizeBytes)` | Um arquivo por chave, sem header |
| `LocalStorage::Read` | `Result Read(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead)` | `NotFound` quando a chave não existe |
| `RamCache::Initialize` | `Result Initialize(void* platformContext, uint32_t maxBytes)` | `NotSupported` se `AEECLSID_RAMCACHE` não existir |
| `RamCache::Put` | `Result Put(const char* key, const void* data, uint32_t sizeBytes)` | `OutOfMemory` quando o limite estoura |
| `RamCache::Get` | `Result Get(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outSize)` | `NotFound` quando `IRAMCACHE_Find` falha |
| `UnzipStream::SetInputStream` | `Result SetInputStream(void* nativeIAStream)` | Exige `IAStream*` do BREW |
| `UnzipStream::Read` | `int32_t Read(void* outBuffer, uint32_t bytes)` | `-1` no stub do Windows |

### ABI C

`zsdk/c/zsdk_c_fs.h` cobre `FileSystem`, `File`, `SaveGameManager`, `LocalStorage` e `RamCache`. Não há ABI C para `Vfs`, `Database` nem `UnzipStream`, o que o `TODO.md` registra como pendência.

| C++ | C |
| :--- | :--- |
| `FileSystem::Open` + `File::Close` | `zsdk_file_open(fs, path, mode, &result)` + `zsdk_file_close(file)` |
| `FileSystem::ReadAllBytes` | `zsdk_fs_read_all_bytes(fs, path, &buf, &size)` |
| `SaveGameManager::SaveSlot` | `zsdk_savegame_save_slot(sg, slot, data, size, ver)` |
| `SaveGameManager::PumpAsync` | `zsdk_savegame_pump_async(sg, budget_bytes)` |
| `FileSystem::EnumerateDirectory` | `zsdk_fs_enumerate_dir(fs, dir, dirs_only, cb, user_data)` |

No C, `zsdk_result_t` compara com `ZSDK_C_OK`, e o status assíncrono vem como inteiro: `0` idle, `1` pending, `2` done, `3` failed.

## Limites e armadilhas

### Números do console

| Limite | Valor | Efeito |
| :--- | :--- | :--- |
| `payload de save` | `64 KiB` | `WriteUserFile` recusa acima; `SaveSlot` não impõe o teto sozinho |
| `filesystem` | `1 GB` compartilhado | saves e log disputam o mesmo espaço |
| `heap` | `32 MB` com o `.mod` carregado | um buffer de `ReadAllBytes` de 4 MB custa 12% do orçamento |
| `stack` | `32 KB` | `char path[128]` em `Vfs` e `SaveGameManager` conta, e o `TODO.md` pede auditoria |
| `slot assíncrono` | `4096` bytes | progresso maior precisa do caminho síncrono |

O teto de 64 KB vem do Zeebo Developer Guide §5.1 e está registrado no `TODO.md`, que também lembra que o `FileLogSink` tem teto de 64 KiB e pode consumir sozinho o espaço que o save precisa. Em release, desligue o log em arquivo ou baixe o `maxBytes`.

### O custo real de gravar

`SaveSlot` abre o `.tmp`, faz duas escritas (`IFILE_Write` do header e do payload) e depois `Remove` e `Rename`. Numa NAND com apagamento por bloco, essa é a operação mais cara do capítulo, e ela acontece no frame em que você chamou. O save assíncrono troca isso por uma escrita de 16 bytes no início, um pedaço por frame e a finalização: com 3 KB de payload e orçamento de 1 KB, são três frames mais a troca. Meça no console antes de prometer que não trava.

### Divergências Windows

| Comportamento | Zeebo | Windows |
| :--- | :--- | :--- |
| `EnumerateDirectory` | enumera via `IFILEMGR_EnumInit` | `Result::NotSupported` |
| `Rename` sobre destino existente | falha | substitui com `MoveFileExA` |
| `ReadAllBytes` de arquivo vazio | `Ok` com tamanho `0` | `OutOfMemory`, porque `Alloc(0)` devolve `nullptr` |
| `LocalStorage`, `RamCache`, `UnzipStream` | persistentes e reais | `Ok` sem persistir; `Read` devolve `-1` |
| `Database` | completa | símbolo ausente no link |

A linha do arquivo vazio é a que mais morde em teste: um `ReadAllBytes` de um arquivo de 0 byte passa no emulador e falha no PC, então trate `OutOfMemory` nesse caso como "arquivo vazio" ou abra o arquivo e confira `File::GetSize` antes de chamar `ReadAllBytes`.

### Corrupção de save e perda de dados

O que a SDK garante é estreito: um payload lido passa por magic, versão e CRC32, e a troca do arquivo é feita por `.tmp`, `Remove` e `Rename`. O que ela não garante é atomicidade. Entre o `Remove` do `save_XX.sav` e o `Rename` do `.tmp` existe uma janela em que o slot não existe, e é por isso que `LoadSlot` tenta o `.tmp`. Se o `.tmp` também não existir, o slot está perdido e a SDK devolve o erro da leitura do `.sav`.

Três formas de perder dado que a SDK não detecta:

1. Payload com padding não inicializado. O CRC cobre os bytes de padding também, então `Progress p;` sem inicializar grava lixo diferente a cada build e invalida qualquer verificação futura. Inicialize com `Progress p{};`.
2. Duas gravações do mesmo slot no mesmo frame. `SaveSlot` só devolve `WouldBlock` quando existe job assíncrono pendente para aquele slot; dois `SaveSlot` síncronos disputam o mesmo `.tmp` e o resultado depende de a segunda abertura truncar o arquivo ou não. Grave um slot por vez e espere o retorno de cada gravação.
3. Tratar todo erro como "não há save". `LoadSlot` devolve `Result::NotFound` quando o arquivo não existe e `Result::IoError` quando existe e não validou. Se o chamador gravar por cima em qualquer erro, um save de versão antiga é apagado no primeiro boot da versão nova. Decida explicitamente se `IoError` pode ser sobrescrito e guarde o arquivo antigo quando não puder.

Para reduzir o risco, alterne dois slots: grave sempre no slot que não foi lido por último e, na carga, tente os dois na ordem inversa da gravação. Isso dobra o espaço ocupado e não é um recurso da SDK, é uma convenção do seu jogo.

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `SaveSlot` devolve `Result::IoError` com log `Not enough free space` | espaço livre menor que payload mais 4112 bytes | libere espaço, apague slots antigos, baixe o `FileLogSink` |
| `Open` devolve `Result::NotFound` para caminho que existe | `IFILEMGR_OpenFile` falhou por qualquer motivo | confira a trava de NAND no log `FS` e o retorno de `GetFreeBytes` |
| `Open` com `Create` devolve `Result::IoError` | trava de NAND bloqueou o caminho | grave em `fs:/~/` ou `fs:/card0/`, nunca em `fs:/mod/` |
| `LoadSlot` devolve `IoError` e o arquivo existe | versão diferente do `expectedVersion`, tamanho acima do seu buffer ou CRC quebrado | leia com a versão antiga e migre, ou aumente o buffer |
| `LoadSlot` devolve erro e apareceu um `ZSDK_LOG_WARN` de recuperação | `.sav` sumiu entre `Remove` e `Rename` | nada a fazer, o `.tmp` foi aceito |
| `BeginSaveSlot` devolve `Result::WouldBlock` | job anterior ainda em `Pending` | continue bombeando com `PumpAsync` antes de pedir outro |
| `BeginSaveSlot` devolve `InvalidArg` | payload acima de 4096 bytes ou slot `>= 16` | use `SaveSlot` para payload grande |
| `RamCache::Put` falha depois de um `Clear` | `Clear` deixou o limite em `0` | refaça `Shutdown` e `Initialize` |
| `Vfs::Resolve` devolve `InvalidArg` | caminho com `..` ou maior que 128 bytes | encurte o caminho ou monte uma raiz menor |
| `DatabaseManager` não liga no build Windows | `zeebo_database.cpp` só entra com `ZSDK_BUILD_ZEEBO` | use `LocalStorage` no PC ou isole o banco com `#if ZSDK_PLATFORM_ZEEBO` |

## Veja também

- [01 Build e empacotamento](./01_build_e_empacotamento.md) para o `.mif`, os privilégios e as flags de `config.hpp`.
- [03 Debug e profiler](./03_debug_e_profiler.md) para o `FileLogSink`, que grava pelo `Vfs` e divide o espaço do save.
- [07 Áudio, rede e segurança](./07_audio_rede_seguranca.md) para os caminhos de áudio em `fs:/~/audio/`.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md) para o orçamento de memória e o estado dos backends.
- Código: [filesystem.hpp](../include/zsdk/fs/filesystem.hpp), [savegame.cpp](../src/fs/savegame.cpp), [vfs.cpp](../src/fs/vfs.cpp), [zeebo_fs.cpp](../src/platform/zeebo/zeebo_fs.cpp) e as limitações abertas no [TODO.md](../TODO.md).
