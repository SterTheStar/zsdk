/**
 * @file texture.cpp
 * @brief OpenGL ES 1.1 Texture implementation with automatic VRAM tracking.
 */

#include "zsdk/graphics/texture.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "gl_platform.hpp"

#if ZSDK_PLATFORM_ZEEBO
    #include <gles/egl.h>
    #include <gles/glESext.h>
#endif

namespace zsdk {
namespace graphics {

namespace {

void MapFormat(TextureFormat fmt, GLenum& outGlFormat, GLenum& outGlType, uint32_t& outBpp) {
    switch (fmt) {
        case TextureFormat::Alpha8:
            outGlFormat = GL_ALPHA; outGlType = GL_UNSIGNED_BYTE; outBpp = 1u; break;
        case TextureFormat::RGB565:
            outGlFormat = GL_RGB;   outGlType = GL_UNSIGNED_SHORT_5_6_5; outBpp = 2u; break;
        case TextureFormat::RGBA4444:
            outGlFormat = GL_RGBA;  outGlType = GL_UNSIGNED_SHORT_4_4_4_4; outBpp = 2u; break;
        case TextureFormat::RGBA5551:
            outGlFormat = GL_RGBA;  outGlType = GL_UNSIGNED_SHORT_5_5_5_1; outBpp = 2u; break;
        case TextureFormat::RGB888:
            outGlFormat = GL_RGB;   outGlType = GL_UNSIGNED_BYTE; outBpp = 3u; break;
        case TextureFormat::RGBA8888:
        default:
            outGlFormat = GL_RGBA;  outGlType = GL_UNSIGNED_BYTE; outBpp = 4u; break;
    }
}

} // namespace

Texture::Texture()
    : m_glId(0u)
    , m_width(0u)
    , m_height(0u)
    , m_format(TextureFormat::RGBA8888)
    , m_vramBytes(0u) {
}

Texture::~Texture() {
    Destroy();
}

Result Texture::Create(uint16_t width, uint16_t height, TextureFormat format,
                       const void* pixelData, TextureFilter filter) {
    if (width == 0u || height == 0u) {
        return Result::InvalidArg;
    }
    if (m_glId != 0u) {
        Destroy();
    }

    GLenum glFmt = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;
    uint32_t bpp = 4u;
    MapFormat(format, glFmt, glType, bpp);

    const uint32_t vramBytes = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * bpp;
    if (!debug::MemoryTracker::CanAllocateVram(vramBytes)) {
        return Result::OutOfMemory;
    }

    GLuint id = 0u;
    glGenTextures(1, &id);
    if (id == 0u) {
        return Result::GraphicsError;
    }

    const GLint glFilter = (filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(glFmt),
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 0, glFmt, glType, pixelData);

    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &id);
        return Result::GraphicsError;
    }

    m_glId = static_cast<uint32_t>(id);
    m_width = width;
    m_height = height;
    m_format = format;
    m_vramBytes = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * bpp;

    debug::MemoryTracker::RecordVramAlloc(m_vramBytes);
    return Result::Ok;
}

Result Texture::CreateCompressed(uint16_t width, uint16_t height, uint32_t glInternalFormat,
                                 uint32_t imageSize, const void* compressedData,
                                 TextureFilter filter) {
    if (width == 0u || height == 0u || imageSize == 0u || !compressedData) {
        return Result::InvalidArg;
    }
    if (m_glId != 0u) {
        Destroy();
    }
    if (!debug::MemoryTracker::CanAllocateVram(imageSize)) {
        return Result::OutOfMemory;
    }

    GLuint id = 0u;
    glGenTextures(1, &id);
    if (id == 0u) {
        return Result::GraphicsError;
    }

    const GLint glFilter = (filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(glInternalFormat),
                           static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                           0, static_cast<GLsizei>(imageSize), compressedData);

    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &id);
        return Result::GraphicsError;
    }

    m_glId = static_cast<uint32_t>(id);
    m_width = width;
    m_height = height;
    m_format = TextureFormat::RGBA8888;
    m_vramBytes = imageSize;

    debug::MemoryTracker::RecordVramAlloc(m_vramBytes);
    return Result::Ok;
}

Result Texture::UpdateSubImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                               const void* pixelData) {
    if (m_glId == 0u || !pixelData) {
        return Result::InvalidArg;
    }

    GLenum glFmt = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;
    uint32_t bpp = 4u;
    MapFormat(m_format, glFmt, glType, bpp);
    ZSDK_UNUSED(bpp);

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_glId));
    glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(x), static_cast<GLint>(y),
                    static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                    glFmt, glType, pixelData);

    return (glGetError() == GL_NO_ERROR) ? Result::Ok : Result::GraphicsError;
}

Result Texture::GenerateMipmaps(TextureFilter filter) {
    if (m_glId == 0u) {
        return Result::NotInitialized;
    }

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_glId));

    const GLint minFilter = (filter == TextureFilter::Nearest)
        ? GL_NEAREST_MIPMAP_NEAREST
        : GL_LINEAR_MIPMAP_LINEAR;
    const GLint magFilter = (filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

#if ZSDK_PLATFORM_ZEEBO
    static bool s_resolved = false;
    static PFNGLGENERATEMIPMAPOESPROC s_genMipmap = nullptr;
    if (!s_resolved) {
        s_genMipmap = reinterpret_cast<PFNGLGENERATEMIPMAPOESPROC>(
            eglGetProcAddress("glGenerateMipmapOES"));
        s_resolved = true;
    }
    if (s_genMipmap) {
        s_genMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }
#else
    #ifdef GL_GENERATE_MIPMAP
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    #endif
#endif

    return (glGetError() == GL_NO_ERROR) ? Result::Ok : Result::GraphicsError;
}

Result Texture::ReadFramebuffer(int32_t x, int32_t y, uint16_t width, uint16_t height,
                                TextureFormat format, void* outPixels) {
    if (width == 0u || height == 0u || !outPixels) {
        return Result::InvalidArg;
    }

    GLenum glFmt = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;
    uint32_t bpp = 4u;
    MapFormat(format, glFmt, glType, bpp);
    ZSDK_UNUSED(bpp);

    glReadPixels(static_cast<GLint>(x), static_cast<GLint>(y),
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                 glFmt, glType, outPixels);

    return (glGetError() == GL_NO_ERROR) ? Result::Ok : Result::GraphicsError;
}

void Texture::Destroy() {
    if (m_glId != 0u) {
        const GLuint id = static_cast<GLuint>(m_glId);
        glDeleteTextures(1, &id);
        debug::MemoryTracker::RecordVramFree(m_vramBytes);
        m_glId = 0u;
        m_width = 0u;
        m_height = 0u;
        m_vramBytes = 0u;
    }
}

void Texture::Bind(uint8_t textureUnit) const {
    if (textureUnit > 0u) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
    }
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_glId));
}

} // namespace graphics
} // namespace zsdk
