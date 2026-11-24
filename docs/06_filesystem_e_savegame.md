# 06 — Filesystem, Proteção da NAND, `udata` / SaveGame e Banco de Dados

Headers incluídos: `fs/filesystem.hpp`, `fs/savegame.hpp`, `fs/archive_unzip.hpp`, `fs/local_storage.hpp`, `fs/ram_cache.hpp`, `fs/database.hpp`.

---

## 1. Filesystem & Proteção contra Escrita na NAND (`ZSDK_NAND_WRITE_GUARD`)

A classe `zsdk::fs::FileSystem` encapsula `IFileMgr` / `IFile` no BREW e `<stdio.h>` no Windows:

```cpp
zsdk::fs::FileSystem fs;
fs.Initialize(app->GetNativePlatformContext());

// Leitura completa rastreada pelo MemoryTracker (MemorySubsystem::Filesystem)
void* data = nullptr;
uint32_t size = 0;
if (zsdk::Succeeded(fs.ReadAllBytes("fs:/~/assets/level1.bin", &data, size))) {
    // usar data...
    zsdk::fs::FileSystem::FreeBuffer(data);
}

// Listagem de diretório sem alocações (IFILEMGR_EnumInit / EnumNext)
fs.EnumerateDirectory("fs:/~/assets", [](const zsdk::fs::FileEntry& e, void*) -> bool {
    return true; // continuar
}, nullptr);
```

### Trava de Segurança (`ZSDK_NAND_WRITE_GUARD = 1`)
Para proteger a NAND principal do Zeebo contra corrupção acidental:
* **Leitura (`FileMode::Read`)** é permitida normalmente.
* **Escrita, criação, remoção e `MkDir`** são **bloqueadas** (`Result::IoError`) se apontarem para `fs:/sys/`, `fs:/mif/`, `fs:/shared/`, `fs:/mod/` ou raiz `fs:/`.
* Escritas só são aceitas em **`fs:/~/`** (pasta do próprio jogo), **`fs:/card0/`** (SD Card) ou caminhos relativos (resolvidos em `fs:/~/`).
* O manifesto [zsdk_template.mif.in](../tools/mif/zsdk_template.mif.in) também remove `PL_SHARED_WRITE` (`0x07AF`) e inclui `0x0101414E` (`AEECLSID_CARD0_ACCESS`).

---

## 2. Sistema `udata` & SaveGame (`SaveGameManager`)

Seguem a padronização do Zeebo armazenando arquivos persistentes e slots de save dentro da pasta **`udata`** do próprio jogo (`fs:/~/udata/` no Zeebo, `udata/` no Windows), com verificação de **magic (`ZSV1`)**, **versão**, **CRC32**, checagem de espaço livre na Flash (`SystemInfo::GetMemoryInfo`) e trava de **64 KB** (`kMaxSaveDataBytes = 65536`, conforme Zeebo Developer Guide 5.1):

```cpp
zsdk::fs::SaveGameManager saveMgr;
saveMgr.Initialize(app->GetNativePlatformContext()); // Cria fs:/~/udata/ automaticamente

// 1. Salvar struct de progresso em um slot (0..15) -> fs:/~/udata/save_00.sav
MeuSave save = { /* ... */ };
saveMgr.SaveSlot(0, &save, sizeof(save), /*gameVersion=*/1);

// 2. Carregar e validar CRC32 + versão
uint32_t lidos = 0;
if (zsdk::Succeeded(saveMgr.LoadSlot(0, &save, sizeof(save), lidos, 1))) {
    // Save íntegro!
}

// 3. Arquivos livres do jogo dentro de udata/
saveMgr.WriteUserFile("profile.dat", &perfil, sizeof(perfil));
```

---

## 3. Descompressão, Cache e Banco de Dados

* **`UnzipStream` (`fs/archive_unzip.hpp`):** Descompressão streaming sobre `IUnzipAStream` (`AEEUnzipStream.h`).
* **`RamCache` (`fs/ram_cache.hpp`):** Cache em memória com limite de bytes sobre `IRamCache`.
* **`LocalStorage` (`fs/local_storage.hpp`):** Armazenamento chave-valor persistente.
* **`DatabaseManager` / `Database` / `DbRecord` (`fs/database.hpp`):** Wrapper completo sobre `IDBMgr` / `IDatabase` / `IDBRecord` (`AEEDB.h`) com buffer de stack de até 16 campos por registro.
