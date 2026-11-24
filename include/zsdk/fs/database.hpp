/**
 * @file database.hpp
 * @brief Structured record database abstraction over BREW IDBMgr / IDatabase / IDBRecord.
 *
 * Isolates AEEDB.h types from public user headers and routes any temporary
 * heap allocations through debug::MemoryTracker under MemorySubsystem::Filesystem.
 */

#ifndef ZSDK_FS_DATABASE_HPP
#define ZSDK_FS_DATABASE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace fs {

/**
 * @brief Field data types supported by BREW record databases (AEEDBFieldType).
 */
enum class DbFieldType : uint8_t {
    None = 0,   /**< AEEDB_FT_NONE */
    Byte,       /**< AEEDB_FT_BYTE (8-bit value) */
    Word,       /**< AEEDB_FT_WORD (16-bit value) */
    DWord,      /**< AEEDB_FT_DWORD (32-bit value) */
    String,     /**< AEEDB_FT_STRING (null-terminated UTF-16 / AECHAR array) */
    Binary,     /**< AEEDB_FT_BINARY (raw byte buffer) */
    Phone,      /**< AEEDB_FT_PHONE */
    Bitmap,     /**< AEEDB_FT_BITMAP */
    Complex     /**< AEEDB_FT_COMPLEX */
};

/**
 * @brief Descriptor for a single field in a database record (mirrors AEEDBField).
 */
struct DbField {
    DbFieldType type;       /**< Field value type. */
    uint8_t     name;       /**< Application-defined or AEEDBFIELD_* field identifier. */
    uint16_t    dataLen;    /**< Size of @p data in bytes. */
    void*       data;       /**< Pointer to field payload (non-owning). */
};

/**
 * @brief RAII wrapper around a BREW IDBRecord*.
 */
class DbRecord : public NonCopyable {
public:
    DbRecord();
    ~DbRecord();

    /** @brief Returns true if this handle currently holds an open record. */
    bool IsOpen() const;

    /** @brief Releases the underlying IDBRecord*. Safe to call multiple times. */
    void Close();

    /** @brief Returns the unique 16-bit record ID, or 0xFFFF if closed. */
    uint16_t GetId() const;

    /** @brief Resets the field iteration cursor to the first field of the record. */
    void Reset();

    /**
     * @brief Replaces all fields of the record with the supplied field array.
     * @param fields     Array of field descriptors.
     * @param fieldCount Number of elements in @p fields.
     * @return Result::Ok on success, or an error code.
     */
    Result Update(const DbField* fields, int32_t fieldCount);

    /**
     * @brief Deletes this record from its parent database and invalidates the handle.
     * @return Result::Ok on success, or Result::IoError on failure.
     */
    Result Remove();

    /**
     * @brief Advances to the next field in the record.
     * @param outName Receives the field name identifier.
     * @param outLen  Receives the field data length in bytes.
     * @return Field type, or DbFieldType::None when no more fields remain.
     */
    DbFieldType NextField(uint8_t& outName, uint16_t& outLen);

    /**
     * @brief Reads the current field's raw data pointer and metadata.
     * @param outName Receives the field name identifier.
     * @param outType Receives the field type.
     * @param outLen  Receives the field data length in bytes.
     * @return Pointer to internal field buffer, or nullptr if unavailable.
     */
    const void* GetField(uint8_t& outName, DbFieldType& outType, uint16_t& outLen);

    /** @brief Reads the current field as a 16-bit unsigned integer. */
    bool GetFieldWord(uint16_t& outValue);

    /** @brief Reads the current field as a 32-bit unsigned integer. */
    bool GetFieldDWord(uint32_t& outValue);

    /** @brief Reads the current field as a null-terminated UTF-16 (AECHAR) string. */
    const uint16_t* GetFieldString();

private:
    friend class Database;
    void* m_nativeRecord;
};

/**
 * @brief RAII wrapper around a BREW IDatabase*.
 */
class Database : public NonCopyable {
public:
    Database();
    ~Database();

    /** @brief Returns true if this handle currently holds an open database. */
    bool IsOpen() const;

    /** @brief Releases the underlying IDatabase*. Safe to call multiple times. */
    void Close();

    /** @brief Returns the total number of records in the database. */
    uint16_t GetRecordCount() const;

    /** @brief Resets the record enumeration cursor to the beginning. */
    void Reset();

    /**
     * @brief Opens the next record in enumeration order.
     * @param outRecord Receives the opened record handle.
     * @return Result::Ok if a record was retrieved, Result::NotFound at end of database.
     */
    Result GetNextRecord(DbRecord& outRecord);

    /**
     * @brief Opens a specific record by its 16-bit record ID.
     * @param recordId  Record identifier obtained from DbRecord::GetId().
     * @param outRecord Receives the opened record handle.
     * @return Result::Ok if found, Result::NotFound otherwise.
     */
    Result GetRecordById(uint16_t recordId, DbRecord& outRecord);

    /**
     * @brief Creates a new record populated with @p fields and opens it in @p outRecord.
     * @param fields     Array of field descriptors.
     * @param fieldCount Number of elements in @p fields.
     * @param outRecord  Receives the newly created record handle.
     * @return Result::Ok on success, or an error code.
     */
    Result CreateRecord(const DbField* fields, int32_t fieldCount, DbRecord& outRecord);

private:
    friend class DatabaseManager;
    void* m_nativeDb;
};

/**
 * @brief Database manager service wrapping BREW IDBMgr*.
 */
class DatabaseManager : public NonCopyable {
public:
    DatabaseManager();
    ~DatabaseManager();

    /**
     * @brief Initializes IDBMgr from the platform IShell* context.
     * @param platformContext Pointer to BREW IShell*.
     * @return Result::Ok on success, or an error code.
     */
    Result Initialize(void* platformContext);

    /** @brief Releases IDBMgr and internal state. */
    void Shutdown();

    /**
     * @brief Opens or creates a BREW database file.
     * @param path            Database file path (e.g. "fs:/~/save.db").
     * @param createIfMissing If true, creates the database if it does not exist.
     * @param outDb           Receives the opened Database handle.
     * @param minSize         Optional minimum preallocated size in bytes (0 for default).
     * @param minRecords      Optional minimum record count hint (0 for default).
     * @return Result::Ok on success, Result::NotFound or Result::IoError on failure.
     */
    Result OpenDatabase(const char* path,
                        bool createIfMissing,
                        Database& outDb,
                        uint32_t minSize = 0u,
                        uint16_t minRecords = 0u);

    /** @brief Removes a database file from storage. */
    Result Remove(const char* path);

    /** @brief Hints the internal database cache size in bytes. */
    void SetCacheSize(int32_t bytes);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_DATABASE_HPP */
