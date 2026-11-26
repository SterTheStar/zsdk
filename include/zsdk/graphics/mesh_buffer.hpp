/**
 * @file mesh_buffer.hpp
 * @brief RAII OpenGL ES 1.1 Vertex/Index Buffer Object (VBO) with VRAM tracking.
 */

#ifndef ZSDK_GRAPHICS_MESH_BUFFER_HPP
#define ZSDK_GRAPHICS_MESH_BUFFER_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

enum class BufferType : uint8_t {
    Vertex = 0,     /**< GL_ARRAY_BUFFER */
    Index           /**< GL_ELEMENT_ARRAY_BUFFER */
};

enum class BufferUsage : uint8_t {
    Static = 0,     /**< GL_STATIC_DRAW */
    Dynamic         /**< GL_DYNAMIC_DRAW */
};

/**
 * @brief Lightweight VBO/IBO wrapper with automatic VRAM accounting.
 */
class MeshBuffer : public NonCopyable {
public:
    MeshBuffer();
    ~MeshBuffer();

    Result Create(BufferType type, BufferUsage usage, uint32_t sizeBytes, const void* data);
    Result UpdateData(uint32_t offsetBytes, uint32_t sizeBytes, const void* data);
    void   Destroy();

    void Bind() const;
    static void Unbind(BufferType type);

    ZSDK_FORCE_INLINE uint32_t GetNativeId() const { return m_glId; }
    ZSDK_FORCE_INLINE uint32_t GetSizeBytes() const { return m_sizeBytes; }
    ZSDK_FORCE_INLINE bool     IsValid() const { return m_glId != 0u || m_ramData != nullptr; }
    ZSDK_FORCE_INLINE bool     IsHardwareVbo() const { return m_glId != 0u; }
    ZSDK_FORCE_INLINE const uint8_t* GetRamData() const { return m_ramData; }
    ZSDK_FORCE_INLINE const void* GetPointerAtOffset(int32_t offsetBytes) const {
        return m_ramData
            ? static_cast<const void*>(m_ramData + offsetBytes)
            : reinterpret_cast<const void*>(static_cast<uintptr_t>(offsetBytes));
    }

private:
    uint32_t    m_glId;
    uint32_t    m_sizeBytes;
    uint8_t*    m_ramData;
    BufferType  m_type;
    BufferUsage m_usage;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_MESH_BUFFER_HPP */
