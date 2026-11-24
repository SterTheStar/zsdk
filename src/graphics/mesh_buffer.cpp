/**
 * @file mesh_buffer.cpp
 * @brief OpenGL ES 1.1 Vertex/Index Buffer Object implementation with VRAM tracking.
 */

#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "gl_platform.hpp"

namespace zsdk {
namespace graphics {

namespace {

GLenum ToGlTarget(BufferType t) {
    return (t == BufferType::Index) ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
}

GLenum ToGlUsage(BufferUsage u) {
    return (u == BufferUsage::Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
}

} // namespace

MeshBuffer::MeshBuffer()
    : m_glId(0u)
    , m_sizeBytes(0u)
    , m_type(BufferType::Vertex)
    , m_usage(BufferUsage::Static) {
}

MeshBuffer::~MeshBuffer() {
    Destroy();
}

Result MeshBuffer::Create(BufferType type, BufferUsage usage, uint32_t sizeBytes, const void* data) {
    if (sizeBytes == 0u) {
        return Result::InvalidArg;
    }
    if (m_glId != 0u) {
        Destroy();
    }
    if (!debug::MemoryTracker::CanAllocateVram(sizeBytes)) {
        return Result::OutOfMemory;
    }

    GLuint id = 0u;
    glGenBuffers(1, &id);
    if (id == 0u) {
        return Result::GraphicsError;
    }

    const GLenum target = ToGlTarget(type);
    glBindBuffer(target, id);
    glBufferData(target, static_cast<GLsizeiptr>(sizeBytes), data, ToGlUsage(usage));

    if (glGetError() != GL_NO_ERROR) {
        glDeleteBuffers(1, &id);
        return Result::GraphicsError;
    }

    m_glId = static_cast<uint32_t>(id);
    m_sizeBytes = sizeBytes;
    m_type = type;
    m_usage = usage;

    debug::MemoryTracker::RecordVramAlloc(m_sizeBytes);
    return Result::Ok;
}

Result MeshBuffer::UpdateData(uint32_t offsetBytes, uint32_t sizeBytes, const void* data) {
    if (m_glId == 0u || !data || offsetBytes + sizeBytes > m_sizeBytes) {
        return Result::InvalidArg;
    }
    glBindBuffer(ToGlTarget(m_type), static_cast<GLuint>(m_glId));
    glBufferSubData(ToGlTarget(m_type), static_cast<GLintptr>(offsetBytes),
                    static_cast<GLsizeiptr>(sizeBytes), data);
    return (glGetError() == GL_NO_ERROR) ? Result::Ok : Result::GraphicsError;
}

void MeshBuffer::Destroy() {
    if (m_glId != 0u) {
        const GLuint id = static_cast<GLuint>(m_glId);
        glDeleteBuffers(1, &id);
        debug::MemoryTracker::RecordVramFree(m_sizeBytes);
        m_glId = 0u;
        m_sizeBytes = 0u;
    }
}

void MeshBuffer::Bind() const {
    glBindBuffer(ToGlTarget(m_type), static_cast<GLuint>(m_glId));
}

void MeshBuffer::Unbind(BufferType type) {
    glBindBuffer(ToGlTarget(type), 0u);
}

} // namespace graphics
} // namespace zsdk
