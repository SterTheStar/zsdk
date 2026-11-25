/**
 * @file gl_platform.hpp
 * @brief Internal OpenGL ES 1.1 / Windows Desktop OpenGL compatibility header.
 */

#ifndef ZSDK_GRAPHICS_GL_PLATFORM_HPP
#define ZSDK_GRAPHICS_GL_PLATFORM_HPP

#include "zsdk/core/config.hpp"

#if ZSDK_PLATFORM_ZEEBO
    #include <gles/gles_1_1/gl.h>
#else
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <GL/gl.h>

    #ifndef GL_CLAMP_TO_EDGE
        #define GL_CLAMP_TO_EDGE 0x812F
    #endif
    #ifndef GL_UNSIGNED_SHORT_5_6_5
        #define GL_UNSIGNED_SHORT_5_6_5 0x8363
    #endif
    #ifndef GL_UNSIGNED_SHORT_4_4_4_4
        #define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
    #endif
    #ifndef GL_UNSIGNED_SHORT_5_5_5_1
        #define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
    #endif
    #ifndef GL_ARRAY_BUFFER
        #define GL_ARRAY_BUFFER 0x8892
    #endif
    #ifndef GL_ELEMENT_ARRAY_BUFFER
        #define GL_ELEMENT_ARRAY_BUFFER 0x8893
    #endif
    #ifndef GL_STATIC_DRAW
        #define GL_STATIC_DRAW 0x88E4
    #endif
    #ifndef GL_DYNAMIC_DRAW
        #define GL_DYNAMIC_DRAW 0x88E8
    #endif
    #ifndef GL_GENERATE_MIPMAP
        #define GL_GENERATE_MIPMAP 0x8191
    #endif
    #ifndef GL_TEXTURE0
        #define GL_TEXTURE0 0x84C0
    #endif

    typedef ptrdiff_t GLsizeiptr;
    typedef ptrdiff_t GLintptr;
    typedef int GLfixed;

    namespace zsdk_win_gl {
        typedef void (APIENTRY *PfnActiveTexture)(GLenum);
        typedef void (APIENTRY *PfnClientActiveTexture)(GLenum);
        typedef void (APIENTRY *PfnGenBuffers)(GLsizei, GLuint*);
        typedef void (APIENTRY *PfnBindBuffer)(GLenum, GLuint);
        typedef void (APIENTRY *PfnBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
        typedef void (APIENTRY *PfnBufferSubData)(GLenum, GLintptr, GLsizeiptr, const void*);
        typedef void (APIENTRY *PfnDeleteBuffers)(GLsizei, const GLuint*);
        typedef void (APIENTRY *PfnCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*);

        inline void glActiveTexture(GLenum tex) {
            static PfnActiveTexture fn = reinterpret_cast<PfnActiveTexture>(wglGetProcAddress("glActiveTexture"));
            if (fn) fn(tex);
        }
        inline void glClientActiveTexture(GLenum texture) {
            static PfnClientActiveTexture fn = reinterpret_cast<PfnClientActiveTexture>(
                wglGetProcAddress("glClientActiveTexture"));
            if (fn) fn(texture);
        }
        inline void glGenBuffers(GLsizei n, GLuint* buffers) {
            static PfnGenBuffers fn = reinterpret_cast<PfnGenBuffers>(wglGetProcAddress("glGenBuffers"));
            if (fn) fn(n, buffers);
        }
        inline void glBindBuffer(GLenum target, GLuint buffer) {
            static PfnBindBuffer fn = reinterpret_cast<PfnBindBuffer>(wglGetProcAddress("glBindBuffer"));
            if (fn) fn(target, buffer);
        }
        inline void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
            static PfnBufferData fn = reinterpret_cast<PfnBufferData>(wglGetProcAddress("glBufferData"));
            if (fn) fn(target, size, data, usage);
        }
        inline void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
            static PfnBufferSubData fn = reinterpret_cast<PfnBufferSubData>(wglGetProcAddress("glBufferSubData"));
            if (fn) fn(target, offset, size, data);
        }
        inline void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
            static PfnDeleteBuffers fn = reinterpret_cast<PfnDeleteBuffers>(wglGetProcAddress("glDeleteBuffers"));
            if (fn) fn(n, buffers);
        }
        inline void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                           GLsizei width, GLsizei height, GLint border,
                                           GLsizei imageSize, const void* data) {
            static PfnCompressedTexImage2D fn = reinterpret_cast<PfnCompressedTexImage2D>(
                wglGetProcAddress("glCompressedTexImage2D"));
            if (fn) fn(target, level, internalformat, width, height, border, imageSize, data);
        }
        inline void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
            ::glOrtho(l, r, b, t, n, f);
        }
        inline void glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
            ::glFrustum(l, r, b, t, n, f);
        }
        inline void glClipPlanef(GLenum plane, const GLfloat* equation) {
            if (!equation) return;
            GLdouble eq[4] = {
                static_cast<GLdouble>(equation[0]),
                static_cast<GLdouble>(equation[1]),
                static_cast<GLdouble>(equation[2]),
                static_cast<GLdouble>(equation[3])
            };
            ::glClipPlane(plane, eq);
        }
        inline void glTranslatex(GLfixed x, GLfixed y, GLfixed z) {
            const float inv = 1.0f / 65536.0f;
            ::glTranslatef(static_cast<GLfloat>(x) * inv,
                           static_cast<GLfloat>(y) * inv,
                           static_cast<GLfloat>(z) * inv);
        }
        inline void glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
            const float inv = 1.0f / 65536.0f;
            ::glRotatef(static_cast<GLfloat>(angle) * inv,
                        static_cast<GLfloat>(x) * inv,
                        static_cast<GLfloat>(y) * inv,
                        static_cast<GLfloat>(z) * inv);
        }
        inline void glScalex(GLfixed x, GLfixed y, GLfixed z) {
            const float inv = 1.0f / 65536.0f;
            ::glScalef(static_cast<GLfloat>(x) * inv,
                       static_cast<GLfloat>(y) * inv,
                       static_cast<GLfloat>(z) * inv);
        }
    } // namespace zsdk_win_gl

    using zsdk_win_gl::glActiveTexture;
    using zsdk_win_gl::glClientActiveTexture;
    using zsdk_win_gl::glGenBuffers;
    using zsdk_win_gl::glBindBuffer;
    using zsdk_win_gl::glBufferData;
    using zsdk_win_gl::glBufferSubData;
    using zsdk_win_gl::glDeleteBuffers;
    using zsdk_win_gl::glCompressedTexImage2D;
    using zsdk_win_gl::glOrthof;
    using zsdk_win_gl::glFrustumf;
    using zsdk_win_gl::glClipPlanef;
    using zsdk_win_gl::glTranslatex;
    using zsdk_win_gl::glRotatex;
    using zsdk_win_gl::glScalex;
#endif


namespace zsdk {
namespace graphics {
namespace glutil {

/**
 * @brief Clears any stale GL error so the next glGetError() reflects only the call under test.
 *        (Zeebo platform notes: a stale error makes error-checking code take its failure path
 *        with no other symptom.) Bounded loop: some drivers return the same error forever.
 */
inline void DrainErrors() {
    for (int i = 0; i < 8 && glGetError() != GL_NO_ERROR; ++i) {}
}

/**
 * @brief Forces byte alignment for pixel unpack (uploads) and restores the GL default (4).
 *        The default of 4 shears Alpha8 / RGB888 textures whose width * bpp is not a multiple of 4.
 */
struct UnpackAlign1 {
    UnpackAlign1()  { glPixelStorei(GL_UNPACK_ALIGNMENT, 1); }
    ~UnpackAlign1() { glPixelStorei(GL_UNPACK_ALIGNMENT, 4); }
};

/** @brief Same for pixel pack (glReadPixels). */
struct PackAlign1 {
    PackAlign1()  { glPixelStorei(GL_PACK_ALIGNMENT, 1); }
    ~PackAlign1() { glPixelStorei(GL_PACK_ALIGNMENT, 4); }
};

} // namespace glutil
} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_GL_PLATFORM_HPP */
