/**
 * @file zeebo_database.cpp
 * @brief Qualcomm BREW / Zeebo IDBMgr, IDatabase, and IDBRecord implementation.
 */

#include "zsdk/fs/database.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEClassIDs.h"
#include "AEEDB.h"

namespace zsdk {
namespace fs {

namespace {

constexpr int32_t kStackFieldCapacity = 16;

AEEDBFieldType MapToBrewFieldType(DbFieldType type) {
    switch (type) {
        case DbFieldType::Byte:    return AEEDB_FT_BYTE;
        case DbFieldType::Word:    return AEEDB_FT_WORD;
        case DbFieldType::DWord:   return AEEDB_FT_DWORD;
        case DbFieldType::String:  return AEEDB_FT_STRING;
        case DbFieldType::Binary:  return AEEDB_FT_BINARY;
        case DbFieldType::Phone:   return AEEDB_FT_PHONE;
        case DbFieldType::Bitmap:  return AEEDB_FT_BITMAP;
        case DbFieldType::Complex: return AEEDB_FT_COMPLEX;
        case DbFieldType::None:
        default:                   return AEEDB_FT_NONE;
    }
}

DbFieldType MapFromBrewFieldType(AEEDBFieldType type) {
    switch (type) {
        case AEEDB_FT_BYTE:    return DbFieldType::Byte;
        case AEEDB_FT_WORD:    return DbFieldType::Word;
        case AEEDB_FT_DWORD:   return DbFieldType::DWord;
        case AEEDB_FT_STRING:  return DbFieldType::String;
        case AEEDB_FT_BINARY:  return DbFieldType::Binary;
        case AEEDB_FT_PHONE:   return DbFieldType::Phone;
        case AEEDB_FT_BITMAP:  return DbFieldType::Bitmap;
        case AEEDB_FT_COMPLEX: return DbFieldType::Complex;
        case AEEDB_FT_NONE:
        default:               return DbFieldType::None;
    }
}

void FillBrewFields(const DbField* src, AEEDBField* dst, int32_t count) {
    for (int32_t i = 0; i < count; ++i) {
        dst[i].fType    = MapToBrewFieldType(src[i].type);
        dst[i].fName    = static_cast<AEEDBFieldName>(src[i].name);
        dst[i].wDataLen = src[i].dataLen;
        dst[i].pBuffer  = src[i].data;
    }
}

/**
 * @brief Helper to convert DbField array to AEEDBField array without heap allocation
 *        for typical field counts (<= 16).
 */
class ScopedBrewFields : public NonCopyable {
public:
    ScopedBrewFields()
        : m_heapPtr(nullptr)
        , m_ptr(nullptr) {
    }

    ~ScopedBrewFields() {
        if (m_heapPtr) {
            debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, m_heapPtr);
            m_heapPtr = nullptr;
        }
    }

    AEEDBField* Convert(const DbField* fields, int32_t count) {
        if (!fields || count <= 0) {
            return nullptr;
        }
        if (count <= kStackFieldCapacity) {
            m_ptr = m_stack;
        } else {
            m_heapPtr = static_cast<AEEDBField*>(debug::MemoryTracker::Alloc(
                debug::MemorySubsystem::Filesystem,
                static_cast<size_t>(count) * sizeof(AEEDBField)));
            if (!m_heapPtr) {
                return nullptr;
            }
            m_ptr = m_heapPtr;
        }
        FillBrewFields(fields, m_ptr, count);
        return m_ptr;
    }

private:
    AEEDBField  m_stack[kStackFieldCapacity];
    AEEDBField* m_heapPtr;
    AEEDBField* m_ptr;
};

} // namespace

// -----------------------------------------------------------------------------
// DbRecord
// -----------------------------------------------------------------------------

DbRecord::DbRecord()
    : m_nativeRecord(nullptr) {
}

DbRecord::~DbRecord() {
    Close();
}

bool DbRecord::IsOpen() const {
    return m_nativeRecord != nullptr;
}

void DbRecord::Close() {
    if (m_nativeRecord) {
        IDBRECORD_Release(static_cast<IDBRecord*>(m_nativeRecord));
        m_nativeRecord = nullptr;
    }
}

uint16_t DbRecord::GetId() const {
    if (!m_nativeRecord) {
        return AEE_DB_RECID_NULL;
    }
    return IDBRECORD_GetID(static_cast<IDBRecord*>(m_nativeRecord));
}

void DbRecord::Reset() {
    if (m_nativeRecord) {
        IDBRECORD_Reset(static_cast<IDBRecord*>(m_nativeRecord));
    }
}

Result DbRecord::Update(const DbField* fields, int32_t fieldCount) {
    if (!m_nativeRecord) {
        return Result::NotInitialized;
    }
    if (!fields || fieldCount <= 0) {
        return Result::InvalidArg;
    }
    ScopedBrewFields converter;
    AEEDBField* brewFields = converter.Convert(fields, fieldCount);
    if (!brewFields) {
        return Result::OutOfMemory;
    }
    const int res = IDBRECORD_Update(static_cast<IDBRecord*>(m_nativeRecord),
                                     brewFields, static_cast<int>(fieldCount));
    return (res == SUCCESS) ? Result::Ok : Result::IoError;
}

Result DbRecord::Remove() {
    if (!m_nativeRecord) {
        return Result::NotInitialized;
    }
    const int res = IDBRECORD_Remove(static_cast<IDBRecord*>(m_nativeRecord));
    if (res == SUCCESS) {
        /* IDBRECORD_Remove frees the underlying IDBRecord object on success */
        m_nativeRecord = nullptr;
        return Result::Ok;
    }
    return Result::IoError;
}

DbFieldType DbRecord::NextField(uint8_t& outName, uint16_t& outLen) {
    outName = 0u;
    outLen  = 0u;
    if (!m_nativeRecord) {
        return DbFieldType::None;
    }
    AEEDBFieldName name = 0;
    uint16 len = 0u;
    const AEEDBFieldType ft = IDBRECORD_NextField(
        static_cast<IDBRecord*>(m_nativeRecord), &name, &len);
    outName = static_cast<uint8_t>(name);
    outLen  = len;
    return MapFromBrewFieldType(ft);
}

const void* DbRecord::GetField(uint8_t& outName, DbFieldType& outType, uint16_t& outLen) {
    outName = 0u;
    outType = DbFieldType::None;
    outLen  = 0u;
    if (!m_nativeRecord) {
        return nullptr;
    }
    AEEDBFieldName name = 0;
    AEEDBFieldType ft   = AEEDB_FT_NONE;
    uint16 len          = 0u;
    byte* ptr = IDBRECORD_GetField(
        static_cast<IDBRecord*>(m_nativeRecord), &name, &ft, &len);
    outName = static_cast<uint8_t>(name);
    outType = MapFromBrewFieldType(ft);
    outLen  = len;
    return ptr;
}

bool DbRecord::GetFieldWord(uint16_t& outValue) {
    outValue = 0u;
    if (!m_nativeRecord) {
        return false;
    }
    uint16 val = 0u;
    if (!IDBRECORD_GetFieldWord(static_cast<IDBRecord*>(m_nativeRecord), &val)) {
        return false;
    }
    outValue = val;
    return true;
}

bool DbRecord::GetFieldDWord(uint32_t& outValue) {
    outValue = 0u;
    if (!m_nativeRecord) {
        return false;
    }
    uint32 val = 0u;
    if (!IDBRECORD_GetFieldDWord(static_cast<IDBRecord*>(m_nativeRecord), &val)) {
        return false;
    }
    outValue = val;
    return true;
}

const uint16_t* DbRecord::GetFieldString() {
    if (!m_nativeRecord) {
        return nullptr;
    }
    return reinterpret_cast<const uint16_t*>(
        IDBRECORD_GetFieldString(static_cast<IDBRecord*>(m_nativeRecord)));
}

// -----------------------------------------------------------------------------
// Database
// -----------------------------------------------------------------------------

Database::Database()
    : m_nativeDb(nullptr) {
}

Database::~Database() {
    Close();
}

bool Database::IsOpen() const {
    return m_nativeDb != nullptr;
}

void Database::Close() {
    if (m_nativeDb) {
        IDATABASE_Release(static_cast<IDatabase*>(m_nativeDb));
        m_nativeDb = nullptr;
    }
}

uint16_t Database::GetRecordCount() const {
    if (!m_nativeDb) {
        return 0u;
    }
    return IDATABASE_GetRecordCount(static_cast<IDatabase*>(m_nativeDb));
}

void Database::Reset() {
    if (m_nativeDb) {
        IDATABASE_Reset(static_cast<IDatabase*>(m_nativeDb));
    }
}

Result Database::GetNextRecord(DbRecord& outRecord) {
    if (!m_nativeDb) {
        return Result::NotInitialized;
    }
    outRecord.Close();
    IDBRecord* rec = IDATABASE_GetNextRecord(static_cast<IDatabase*>(m_nativeDb));
    if (!rec) {
        return Result::NotFound;
    }
    outRecord.m_nativeRecord = rec;
    return Result::Ok;
}

Result Database::GetRecordById(uint16_t recordId, DbRecord& outRecord) {
    if (!m_nativeDb) {
        return Result::NotInitialized;
    }
    outRecord.Close();
    IDBRecord* rec = IDATABASE_GetRecordByID(static_cast<IDatabase*>(m_nativeDb), recordId);
    if (!rec) {
        return Result::NotFound;
    }
    outRecord.m_nativeRecord = rec;
    return Result::Ok;
}

Result Database::CreateRecord(const DbField* fields, int32_t fieldCount, DbRecord& outRecord) {
    if (!m_nativeDb) {
        return Result::NotInitialized;
    }
    if (!fields || fieldCount <= 0) {
        return Result::InvalidArg;
    }
    outRecord.Close();

    ScopedBrewFields converter;
    AEEDBField* brewFields = converter.Convert(fields, fieldCount);
    if (!brewFields) {
        return Result::OutOfMemory;
    }

    IDBRecord* rec = IDATABASE_CreateRecord(static_cast<IDatabase*>(m_nativeDb),
                                            brewFields, static_cast<int>(fieldCount));
    if (!rec) {
        return Result::IoError;
    }
    outRecord.m_nativeRecord = rec;
    return Result::Ok;
}

// -----------------------------------------------------------------------------
// DatabaseManager
// -----------------------------------------------------------------------------

struct DatabaseManager::Impl {
    IShell* shell;
    IDBMgr* dbMgr;
};

DatabaseManager::DatabaseManager()
    : m_impl(nullptr) {
}

DatabaseManager::~DatabaseManager() {
    Shutdown();
}

Result DatabaseManager::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Filesystem, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->shell = shell;
    m_impl->dbMgr = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_DBMGR,
                              reinterpret_cast<void**>(&m_impl->dbMgr)) != AEE_SUCCESS || !m_impl->dbMgr) {
        ZSDK_LOG_ERROR("DB", "Failed to create AEECLSID_DBMGR");
        Shutdown();
        return Result::IoError;
    }

    return Result::Ok;
}

void DatabaseManager::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->dbMgr) {
        IDBMGR_Release(m_impl->dbMgr);
        m_impl->dbMgr = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, m_impl);
    m_impl = nullptr;
}

Result DatabaseManager::OpenDatabase(const char* path,
                                     bool createIfMissing,
                                     Database& outDb,
                                     uint32_t minSize,
                                     uint16_t minRecords) {
    if (!m_impl || !m_impl->dbMgr) {
        return Result::NotInitialized;
    }
    if (!path) {
        return Result::InvalidArg;
    }
    outDb.Close();

    IDatabase* db = nullptr;
    if (minSize > 0u || minRecords > 0u) {
        db = IDBMGR_OpenDatabaseEx(m_impl->dbMgr, path,
                                   createIfMissing ? TRUE : FALSE,
                                   minSize, minRecords);
    } else {
        db = IDBMGR_OpenDatabase(m_impl->dbMgr, path,
                                 createIfMissing ? TRUE : FALSE);
    }

    if (!db) {
        return createIfMissing ? Result::IoError : Result::NotFound;
    }
    outDb.m_nativeDb = db;
    return Result::Ok;
}

Result DatabaseManager::Remove(const char* path) {
    if (!m_impl || !m_impl->dbMgr) {
        return Result::NotInitialized;
    }
    if (!path) {
        return Result::InvalidArg;
    }
    return (IDBMGR_Remove(m_impl->dbMgr, path) == SUCCESS) ? Result::Ok : Result::IoError;
}

void DatabaseManager::SetCacheSize(int32_t bytes) {
    if (m_impl && m_impl->dbMgr) {
        IDBMGR_SetCacheSize(m_impl->dbMgr, static_cast<int>(bytes));
    }
}

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
