/*=============================================================================

    FILE:         GLES_ext.c

    SERVICES:     OpenGL ES

    DESCRIPTION:  Provides the OpenGL ES extension bindings which are 
                  implemented using the GL interfaces

===============================================================================
        Copyright © 2006 QUALCOMM Incorporated.
		All Rights Reserved.
		QUALCOMM Proprietary/GTDR
=============================================================================*/


/*----------------------------------------------------------------------------*
 *                      I N C L U D E   F I L E S                             *
 *----------------------------------------------------------------------------*/
#include "gles/gles_1_1/gl.h"
#include "gles/EGLext.h"
#include "gles/glESext.h"
#undef GL_OES_VERSION_1_1

#include "AEEGL.h"		// Legacy, deprecated
#include "AEECLSID_QEGL.bid"
#include "AEEEGL10.h"
#include "AEEEGL11.h"
#include "AEEEGLGetColorBuffer.h"
#include "AEEEGLGetColorBufferInfo.h"
#include "AEEEGLGetPowerLevel.h"
#include "AEEEGLOESSwapInterval.h"
#include "AEEEGLSurfaceManip.h"

#include "AEEGLES10.h"
#include "AEEGLES11.h"
#include "AEEGLES10Ext.h"
#include "AEEGLES11Ext.h"
#include "AEEGLES11ExtPak.h"
#include "AEEGLESImageonExt.h"

#include "EGL_1x.h"
#include "GLES_ext_support.h"    // GLES extension function def macros are here

#include "AEEStdlib.h"

/*----------------------------------------------------------------------------*
 *             E X T E R N A L   U T I L I T Y    F U N C T I O N S           *
 *----------------------------------------------------------------------------*/
extern IEGL *EGL_GetEGLInterfacePtr(void);

extern IEGL10 *EGL_GetQEGLInterfacePtr(void);


/*----------------------------------------------------------------------------*
 *                            M A C R O S                                     *
 *----------------------------------------------------------------------------*/
#if defined(__ARMCC_VERSION) && !defined(AEE_STATIC)
#define GVP_DECLARE(TYPE,NAME)          static TYPE *const NAME
#define GVP_ACCESS(TYPE,NAME)           (*(TYPE**)&(NAME))
#define GV_DECLARE(TYPE,NAME,INITVAL)   static TYPE const NAME
#define GV_ACCESS(TYPE,NAME)            (*(TYPE*)&(NAME))
#else
#define GVP_DECLARE(TYPE,NAME)          static TYPE *NAME = NULL
#define GVP_ACCESS(TYPE,NAME)           (NAME)
#define GV_DECLARE(TYPE,NAME,INITVAL)   static TYPE NAME = INITVAL
#define GV_ACCESS(TYPE,NAME)            (NAME)
#endif


/*----------------------------------------------------------------------------*
 *                            T Y P E D E F S                                 *
 *----------------------------------------------------------------------------*/

typedef struct _ExtTblRec* ExtTblRec_p;

typedef struct _ExtTblRec
{
  const char *name;
  const char *alias1;
  const char *alias2;
  const void *func_p;
} ExtTblRec_t;

typedef struct _KnownExtTblRec* KnownExtTblRec_p;

typedef struct _KnownExtTblRec
{
  const char *name;
  const char *alias1;
  const char *alias2;
  const char *func_list;
} KnownExtTblRec_t;

#ifndef EGL_VERSION_1_3 // Versions 1.0 - 1.2
typedef void (*PFN_GETPROCADDRESS)();
#else // Versions 1.3+
typedef void (*PFN_GETPROCADDRESS)(void);
#endif

/*----------------------------------------------------------------------------*
 *                            G L O B A L S                                   *
 *----------------------------------------------------------------------------*/

/*
 * Various and sundry interface pointers
 */
#if defined(__ARMCC_VERSION) && !defined(AEE_STATIC)
/*
 * The pragmas below prevent a stupid warning from the RVCT compiler
 */
#pragma push
#pragma diag_suppress 550
static IEGL *const gpIEGL;
#pragma pop
static IEGL10 *const gpIQEGL;
//static IEGL11 *const gpIQEGL11;
#define GPIEGL (*(IEGL**)&gpIEGL)
#define GPIQEGL (*(IEGL10**)&gpIQEGL)
#define GPIQEGL11 (*(IEGL11**)&gpIQEGL11)
#else
static IEGL *gpIEGL;
static IEGL10 *gpIQEGL;
//static IEGL11 *gpIQEGL11;
#define GPIEGL (gpIEGL)
#define GPIQEGL (gpIQEGL)
#define GPIQEGL11 (gpIQEGL11)
#endif

GVP_DECLARE(IEGLOESSwapInterval, gpIEGLOSI);
#define GPIEGLOSI GVP_ACCESS(IEGLOESSwapInterval, gpIEGLOSI)

GVP_DECLARE(IEGLGetColorBuffer, gpIEGLGCB);
#define GPIEGLGCB GVP_ACCESS(IEGLGetColorBuffer, gpIEGLGCB)

GVP_DECLARE(IEGLGetColorBufferInfo, gpIEGLGCBI);
#define GPIEGLGCBI GVP_ACCESS(IEGLGetColorBufferInfo, gpIEGLGCBI)

GVP_DECLARE(IEGLGetPowerLevel, gpIEGLGPL);
#define GPIEGLGPL GVP_ACCESS(IEGLGetPowerLevel, gpIEGLGPL)

GVP_DECLARE(IEGLSurfaceManip, gpIEGLSMAN);
#define GPIEGLSMAN GVP_ACCESS(IEGLSurfaceManip, gpIEGLSMAN)


GVP_DECLARE(IGLES10Ext, gpIGLES10Ext);
#define GPIGLES10EXT GVP_ACCESS(IGLES10Ext, gpIGLES10Ext)

GVP_DECLARE(IGLES11Ext, gpIGLES11Ext);
#define GPIGLES11EXT GVP_ACCESS(IGLES11Ext, gpIGLES11Ext)

GVP_DECLARE(IGLES11ExtPak, gpIGLES11ExtPak);
#define GPIGLES11EXTPAK GVP_ACCESS(IGLES11ExtPak, gpIGLES11ExtPak)

GVP_DECLARE(IGLESImageonExt, gpIGLESImageonExt);
#define GPIGLESIMAGEONEXT GVP_ACCESS(IGLESImageonExt, gpIGLESImageonExt)

/*
 * Extension table pointers
 */
GVP_DECLARE(ExtTblRec_t, extension_list);
#define GPEXTLST GVP_ACCESS(ExtTblRec_t, extension_list)

GVP_DECLARE(KnownExtTblRec_t, known_egl_extension_list);
#define GPEGLEXTLST GVP_ACCESS(KnownExtTblRec_t, known_egl_extension_list)

GVP_DECLARE(KnownExtTblRec_t, known_gles_extension_list);
#define GPGLESEXTLST GVP_ACCESS(KnownExtTblRec_t, known_gles_extension_list)

/*
 * Init state flag
 */
GV_DECLARE(GLboolean, gbInitDone, GL_FALSE);
#define GBINITDONE GV_ACCESS(GLboolean, gbInitDone)


/*----------------------------------------------------------------------------*
 *                      P R I V A T E    F U N C T I O N S                    *
 *----------------------------------------------------------------------------*/
/*====================
 *
 * EGL Extensions
 *
 */
/***********
 *
 * EGL_OES_swap_control extension
 *
 ***********/
/*
 * eglSwapIntervalOES
 */
EGL_FUNC_2_ARGS(GPIEGLOSI, OESSWAPINTERVAL, EGLBoolean, SwapIntervalOES,
                EGL_FALSE,
                EGLDisplay, dpy, int, interval)

/*
 * eglGetSwapIntervalOES
 */
EGL_FUNC_1_ARGS(GPIEGLOSI, OESSWAPINTERVAL, int, GetSwapIntervalOES, 0,
                EGLDisplay, dpy)

/***********
 *
 * EGL_QUALCOMM_color_buffer extension
 *
 ***********/
EGL_FUNC_0_ARGS(GPIEGLGCB, GETCOLORBUFFER, void*, GetColorBuffer, NULL)

EGL_FUNC_2_ARGS(GPIEGLGCBI, GETCOLORBUFFERINFO, EGLBoolean, GetColorBufferInfo,
                EGL_FALSE,
                EGLint, attribute, EGLint*, value)

/***********
 *
 * EGL_QUALCOMM_power_level extension
 *
 ***********/
EGL_FUNC_0_ARGS(GPIEGLGPL, GETPOWERLEVEL, int, GetPowerLevel, -1)

/***********
 *
 * EGL_QUALCOMM_surface_scale extension
 *
 ***********/
/*
 * eglSurfaceScaleEnable
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceScaleEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, enable)

/*
 * eglSetSurfaceScale
 *
 * can't use macros due to rect pointer typecast requirement
 */
static EGLBoolean EXT_eglSetSurfaceScale(EGLDisplay dpy, EGLSurface surf,
                                         EGLSurfaceScaleRect *src,
                                         EGLSurfaceScaleRect *dst)
{
  EGLBoolean ret;

  if (GPIEGLSMAN)
  {
    IEGLSURFACEMANIP_SetSurfaceScale(GPIEGLSMAN, dpy, surf,
                                     (AEEEGLSurfaceScaleRect *)src,
                                     (AEEEGLSurfaceScaleRect *)dst, &ret);
    return ret;
  }
  return EGL_FALSE;
}

/*
 * eglGetSurfaceScale
 *
 * can't use macros due to rect pointer typecast requirement
 */
static EGLBoolean EXT_eglGetSurfaceScale(EGLDisplay dpy, EGLSurface surf,
                                         EGLBoolean *enabled,
                                         EGLSurfaceScaleRect *src,
                                         EGLSurfaceScaleRect *dst)
{
  EGLBoolean ret;

  if (GPIEGLSMAN)
  {
    IEGLSURFACEMANIP_GetSurfaceScale(GPIEGLSMAN, dpy, surf, enabled,
                                     (AEEEGLSurfaceScaleRect *)src,
                                     (AEEEGLSurfaceScaleRect *)dst, &ret);
    return ret;
  }
  return EGL_FALSE;
}

/*
 * eglGetSurfaceScaleCaps
 *
 * can't use macros due to caps pointer typecast requirement
 */
static EGLBoolean EXT_eglGetSurfaceScaleCaps(EGLDisplay dpy, EGLSurface surf,
                                             EGLSurfaceScaleCaps *param)
{
  EGLBoolean ret;

  if (GPIEGLSMAN)
  {
    IEGLSURFACEMANIP_GetSurfaceScaleCaps(GPIEGLSMAN, dpy, surf,
                                         (AEEEGLSurfaceScaleCaps *)param, &ret);
    return ret;
  }
  return EGL_FALSE;
}

/***********
 *
 * EGL_QUALCOMM_surface_rotate extension
 *
 ***********/
/*
 * eglSurfaceRotateEnable
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceRotateEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, enable)

/*
 * eglSetSurfaceRotate
 */
EGL_FUNC_5_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SetSurfaceRotate,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, rot90,
                EGLBoolean, hmirror, EGLBoolean, vmirror)

/*
 * eglGetSurfaceRotate
 */
EGL_FUNC_6_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceRotate,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, enabled,
                EGLBoolean*, rot90, EGLBoolean*, hmirror, EGLBoolean*, vmirror)

/*
 * eglGetSurfaceRotateCaps
 */
EGL_FUNC_5_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceRotateCaps,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, rot90,
                EGLBoolean*, hmirror, EGLBoolean*, vmirror)

/***********
 *
 * EGL_QUALCOMM_surface_transparency extension
 *
 ***********/
/*
 * eglSurfaceTransparencyEnable
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceTransparencyEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, enable)

/*
 * eglSetSurfaceTransparency
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SetSurfaceTransparency,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLint, alpha)

/*
 * eglGetSurfaceTransparency
 */
EGL_FUNC_4_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceTransparency,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, enabled,
                EGLint*, alpha)

/*
 * eglSetSurfaceTransparencyMap
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SetSurfaceTransparencyMap,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLSurface, alphasurf)

/*
 * eglGetSurfaceTransparencyMap
 */
EGL_FUNC_4_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceTransparencyMap,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, enabled,
                EGLSurface*, alphasurf)

/*
 * eglGetSurfaceTransparencyCaps
 */
EGL_FUNC_5_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean,
                GetSurfaceTransparencyCaps, EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, alphacomb,
                EGLBoolean*, constalpha, EGLBoolean*, alphamap)

/***********
 *
 * EGL_QUALCOMM_surface_color_key extension
 *
 ***********/
/*
 * eglSurfaceColorKeyEnable
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceColorKeyEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, enable)

/*
 * eglSetSurfaceColorKey
 */
EGL_FUNC_5_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SetSurfaceColorKey,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLint, red,
                EGLint, green, EGLint, blue)

/*
 * eglGetSurfaceColorKey
 */
EGL_FUNC_6_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceColorKey,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, enabled,
                EGLint*, red, EGLint*, green, EGLint*, blue)

/***********
 *
 * EGL_QUALCOMM_surface_overlay extension
 *
 ***********/
/*
 * eglCreateCompositeSurface
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLSurface, CreateCompositeSurface,
                EGL_NO_SURFACE,
                EGLDisplay, dpy, EGLSurface, win, const EGLint*, attrib_list)

/*
 * eglSurfaceOverlayEnable
 */
EGL_FUNC_3_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceOverlayEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean, enable)

/*
 * eglSurfaceOverlayLayerEnable
 */
EGL_FUNC_4_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceOverlayLayerEnable,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLint, layer,
                EGLBoolean, enable)

/*
 * eglSurfaceOverlayBind
 */
EGL_FUNC_5_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, SurfaceOverlayBind,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, comp_surf, EGLSurface, surf,
                EGLint, layer, EGLBoolean, enable)

/*
 * eglGetSurfaceOverlayBinding
 */
EGL_FUNC_4_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceOverlayBinding,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLSurface*, comp_surf,
                EGLint*, layer)

/*
 * eglGetSurfaceOverlay
 */
EGL_FUNC_4_ARGS(GPIEGLSMAN, SURFACEMANIP, EGLBoolean, GetSurfaceOverlay,
                EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surf, EGLBoolean*, layer_enable,
                EGLBoolean*, surf_enable)

/*
 * eglGetSurfaceOverlayCaps
 *
 * can't use macros due to caps pointer typecast requirement
 */
static EGLBoolean EXT_eglGetSurfaceOverlayCaps(EGLDisplay dpy, EGLSurface surf,
                                               EGLCompositeSurfaceCaps *param)
{
  EGLBoolean ret;

  if (GPIEGLSMAN)
  {
    IEGLSURFACEMANIP_GetSurfaceOverlayCaps(GPIEGLSMAN, dpy, surf,
                                           (AEEEGLCompositeSurfaceCaps *)param,
                                           &ret);
    return ret;
  }
  return EGL_FALSE;
}


/*====================
 *
 * GLES 1.0 Optional Extensions
 *
 */
/***********
 *
 * OES_query_matrix extension
 *
 ***********/
static GLbitfield EXT_glQueryMatrixxOES(GLfixed *mantissa, GLint *exponent)
{
  GLbitfield ret;

  if (GPIGLES10EXT)
  {
    IGLES10EXT_QueryMatrixxOES(GPIGLES10EXT, mantissa, exponent, &ret);
    return ret;
  }

  return 0;
}

/*====================
 *
 * GLES 1.1 Optional Extensions
 *
 */
/***********
 *
 * OES_matrix_palette extension
 *
 ***********/

/*
 * glCurrentPaletteMatrixOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXT, 11EXT, CurrentPaletteMatrixOES,
                 GLuint, matrixpaletteindex)

/*
 * glLoadPaletteFromModelViewMatrixOES
 */
GLES_FUNC_0_ARGS(GPIGLES11EXT, 11EXT, LoadPaletteFromModelViewMatrixOES)

/*
 * glMatrixIndexPointerOES
 */
GLES_FUNC_4_ARGS(GPIGLES11EXT, 11EXT, MatrixIndexPointerOES,
                 GLint, size, GLenum, type, GLsizei, stride,
                 const GLvoid*, pointer)

/*
 * glWeightPointerOES
 */
GLES_FUNC_4_ARGS(GPIGLES11EXT, 11EXT, WeightPointerOES,
                 GLint, size, GLenum, type, GLsizei, stride,
                 const GLvoid*, pointer)

/***********
 *
 * OES_draw_texture extension
 *
 ***********/

/*
 * glDrawTexsOES
 */
GLES_FUNC_5_ARGS(GPIGLES11EXT, 11EXT, DrawTexsOES,
                 GLshort, x, GLshort, y, GLshort, z,
                 GLshort, width, GLshort, height)

/*
 * glDrawTexiOES
 */
GLES_FUNC_5_ARGS(GPIGLES11EXT, 11EXT, DrawTexiOES,
                 GLint, x, GLint, y, GLint, z,
                 GLint, width, GLint, height)

/*
 * glDrawTexxOES
 */
GLES_FUNC_5_ARGS(GPIGLES11EXT, 11EXT, DrawTexxOES,
                 GLfixed, x, GLfixed, y, GLfixed, z,
                 GLfixed, width, GLfixed, height)

/*
 * glDrawTexsvOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXT, 11EXT, DrawTexsvOES,
                 const GLshort*, coords)

/*
 * glDrawTexivOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXT, 11EXT, DrawTexivOES,
                 const GLint*, coords)

/*
 * glDrawTexxvOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXT, 11EXT, DrawTexxvOES,
                 const GLfixed*, coords)

/*
 * glDrawTexfOES
 */
GLES_FUNC_5_ARGS(GPIGLES11EXT, 11EXT, DrawTexfOES,
                 GLfloat, x, GLfloat, y, GLfloat, z,
                 GLfloat, width, GLfloat, height)

/*
 * glDrawTexfvOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXT, 11EXT, DrawTexfvOES,
                 const GLfloat*, coords)

/*====================
 *
 * GLES 1.1 Extension Pak Extensions
 *
 */
/***********
 *
 * OES_texture_cube_map extension
 *
 ***********/

/*
 * glGetTexGeniv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GetTexGeniv,
                 GLenum, coord, GLenum, pname, GLint*, params)

/*
 * glGetTexGenxv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GetTexGenxv,
                 GLenum, coord, GLenum, pname, GLfixed*, params)

/*
 * glTexGeni
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGeni,
                 GLenum, coord, GLenum, pname, GLint, param)

/*
 * glTexGenx
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGenx,
                 GLenum, coord, GLenum, pname, GLfixed, param)

/*
 * glTexGeniv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGeniv,
                 GLenum, coord, GLenum, pname, const GLint*, params)

/*
 * glTexGenxv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGenxv,
                 GLenum, coord, GLenum, pname, const GLfixed*, params)

/*
 * glGetTexGenfv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GetTexGenfv,
                 GLenum, coord, GLenum, pname, GLfloat*, params)

/*
 * glTexGenf
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGenf,
                 GLenum, coord, GLenum, pname, GLfloat, param)

/*
 * glTexGenfv
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, TexGenfv,
                 GLenum, coord, GLenum, pname, const GLfloat*, params)

/***********
 *
 * OES_blend_subtract extension
 *
 ***********/

/*
 * glBlendEquation
 */
GLES_FUNC_1_ARGS(GPIGLES11EXTPAK, 11EXTPAK, BlendEquation,
                 GLenum, mode)

/***********
 *
 * OES_blend_func_separate extension
 *
 ***********/

/*
 * glBlendFuncSeparate
 */
GLES_FUNC_4_ARGS(GPIGLES11EXTPAK, 11EXTPAK, BlendFuncSeparate,
                 GLenum, srcRGB, GLenum, dstRGB,
                 GLenum, srcAlpha, GLenum, dstAlpha)

/***********
 *
 * OES_blend_equation_separate extension
 *
 ***********/

/*
 * glBlendEquationSeparate
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, BlendEquationSeparate,
                 GLenum, modeRGB, GLenum, modeAlpha)

/***********
 *
 * OES_framebuffer_object extension
 *
 ***********/

/*
 * glBindFramebufferOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, BindFramebufferOES,
                 GLenum, target, GLuint, framebuffer)

/*
 * glBindRenderbufferOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, BindRenderbufferOES,
                 GLenum, target, GLuint, renderbuffer)

/*
 * glCheckFramebufferStatusOES
 */
static GLenum EXT_glCheckFramebufferStatusOES(GLenum target)
{
  GLenum ret;

  if (GPIGLES11EXTPAK)
  {
    IGLES11EXTPAK_CheckFramebufferStatusOES(GPIGLES11EXTPAK, target, &ret);
    return ret;
  }
  return 0x8CDD;        // FIXME: replace when enums are in glESext.h
}

/*
 * glDeleteFramebuffersOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, DeleteFramebuffersOES,
                 GLsizei, n, const GLuint*, framebuffers)

/*
 * glDeleteRenderbuffersOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, DeleteRenderbuffersOES,
                 GLsizei, n, const GLuint*, renderbuffers)

/*
 * glFramebufferRenderbufferOES
 */
GLES_FUNC_4_ARGS(GPIGLES11EXTPAK, 11EXTPAK, FramebufferRenderbufferOES,
                 GLenum, target, GLenum, attachment,
                 GLenum, renderbuffertarget, GLuint, renderbuffer)

/*
 * glFramebufferTexture2DOES
 */
GLES_FUNC_5_ARGS(GPIGLES11EXTPAK, 11EXTPAK, FramebufferTexture2DOES,
                 GLenum, target, GLenum, attachment, GLenum, textarget,
                 GLuint, texture, GLint, level)

/*
 * glGenerateMipmapOES
 */
GLES_FUNC_1_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GenerateMipmapOES,
                 GLenum, target)

/*
 * glGenFramebuffersOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GenFramebuffersOES,
                 GLsizei, n, GLuint*, framebuffers)

/*
 * glGenRenderbuffersOES
 */
GLES_FUNC_2_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GenRenderbuffersOES,
                 GLsizei, n, GLuint*, renderbuffers)

/*
 * glGetFramebufferAttachmentParameterivOES
 */
GLES_FUNC_4_ARGS(GPIGLES11EXTPAK, 11EXTPAK,
                 GetFramebufferAttachmentParameterivOES,
                 GLenum, target, GLenum, attachment, GLenum, pname,
                 GLint*, params)

/*
 * glGetRenderbufferParameterivOES
 */
GLES_FUNC_3_ARGS(GPIGLES11EXTPAK, 11EXTPAK, GetRenderbufferParameterivOES,
                 GLenum, target, GLenum, pname, GLint*, params)

/*
 * glIsFramebufferOES
 */
static GLboolean EXT_glIsFramebufferOES(GLuint framebuffer)
{
  GLboolean ret;

  if (GPIGLES11EXTPAK)
  {
    IGLES11EXTPAK_IsFramebufferOES(GPIGLES11EXTPAK, framebuffer, &ret);
    return ret;
  }
  return GL_FALSE;
}

/*
 * glIsRenderbufferOES
 */
static GLboolean EXT_glIsRenderbufferOES(GLuint renderbuffer)
{
  GLboolean ret;

  if (GPIGLES11EXTPAK)
  {
    IGLES11EXTPAK_IsRenderbufferOES(GPIGLES11EXTPAK, renderbuffer, &ret);
    return ret;
  }
  return GL_FALSE;
}

/*
 * glRenderbufferStorageOES
 */
GLES_FUNC_4_ARGS(GPIGLES11EXTPAK, 11EXTPAK, RenderbufferStorageOES,
                 GLenum, target, GLenum, internalformat,
                 GLsizei, width, GLsizei, height)

/*====================
 *
 * Imageon Specific Extensions
 *
 */

/***********
 *
 * OES_point_size_array extension (GLES 1.1 Required extension)
 *
 ***********/

/*
 * glPointSizePointerOES
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, PointSizePointerOES,
                 GLenum, type, GLsizei, stride, const GLvoid*, pointer)

/***********
 *
 * EXT_blend_equation_separate extension
 *
 ***********/

/*
 * glBlendEquationSeparateEXT
 */
GLES_FUNC_2_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BlendEquationSeparateEXT,
                 GLenum, modeRGB, GLenum, modeAlpha)

/***********
 *
 * EXT_blend_func_separate extension
 *
 ***********/

/*
 * glBlendFuncSeparateEXT
 */
GLES_FUNC_4_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BlendFuncSeparateEXT,
                 GLenum, srcRGB, GLenum, dstRGB,
                 GLenum, srcAlpha, GLenum, dstAlpha)

/***********
 *
 * EXT_blend_minmax extension
 *
 ***********/

/*
 * glBlendEquationEXT
 */
GLES_FUNC_1_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BlendEquationEXT,
                 GLenum, mode) 

/***********
 *
 * QUALCOMM_vertexbuffer_object extension
 * (subset of ARB_vertex_buffer_object with ATI additions)
 *
 ***********/

/*
 * glBindBufferQUALCOMM
 */
GLES_FUNC_2_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BindBufferQUALCOMM,
                 GLenum, target, GLuint, buffer)

/*
 * glDeleteBuffersQUALCOMM
 */
GLES_FUNC_2_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, DeleteBuffersQUALCOMM,
                 GLsizei, n, const GLuint*, buffers)

/*
 * glGenBuffersQUALCOMM
 */
GLES_FUNC_2_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GenBuffersQUALCOMM,
                 GLsizei, n, GLuint*, buffers)

/*
 * glBufferDataQUALCOMM
 */
GLES_FUNC_4_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BufferDataQUALCOMM,
                 GLenum, target, GLsizeiptr, size, const GLvoid*, data,
                 GLenum, usage)

/*
 * glBufferSubDataQUALCOMM
 */
GLES_FUNC_4_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BufferSubDataQUALCOMM,
                 GLenum, target, GLintptr, offset, GLsizeiptr, size,
                 const GLvoid*, data)

/*
 * glIsBufferQUALCOMM
 */
static GLboolean EXT_glIsBufferQUALCOMM(GLuint buffer)
{
  GLboolean ret;

  if (GPIGLESIMAGEONEXT)
  {
    IGLESIMAGEONEXT_IsBufferQUALCOMM(GPIGLESIMAGEONEXT, buffer, &ret);
    return ret;
  }
  return GL_FALSE;
}

/*
 * glBufferDataATI
 */
GLES_FUNC_6_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, BufferDataATI,
                 GLenum, target, GLsizei, numvertices,
                 GLsizei, vertexelemsize, GLsizeiptrARB, size,
                 const GLvoid*, data, GLenum, usage)

/*
 * glMeshListATI
 */
GLES_FUNC_5_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, MeshListATI,
                 GLint, numlists, GLenum*, mode, GLsizei*, count,
                 GLenum*, type, const GLvoid**, indices)

/*
 * glDrawVertexBufferObjectATI
 */
GLES_FUNC_0_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, DrawVertexBufferObjectATI)
                 

/***********
 *
 * Miscellaneous JSR184, draw_texture and VBO support stuff
 *
 ***********/

/*
 * glGetPointerv
 */
GLES_FUNC_2_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GetPointerv,
                 GLenum, pname, void**, params)

/*
 * glTexEnvi
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexEnvi,
                 GLenum, target, GLenum, pname, GLint, param)

/*
 * glTexEnviv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexEnviv,
                 GLenum, target, GLenum, pname, const GLint*, params)

/*
 * glTexParameteri
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexParameteri,
                 GLenum, target, GLenum, pname, GLint, param)

/*
 * glTexParameteriv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexParameteriv,
                 GLenum, target, GLenum, pname, const GLint*, params)

/*
 * glTexParameterfv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexParameterfv,
                 GLenum, target, GLenum, pname, const GLfloat*, params)

/*
 * glTexParameterxv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, TexParameterxv,
                 GLenum, target, GLenum, pname, const GLfixed*, params)

/*
 * glGetMaterialfv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GetMaterialfv,
                 GLenum, face, GLenum, pname, GLfloat*, params)

/*
 * glGetTexParameteriv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GetTexParameteriv,
                 GLenum, target, GLenum, pname, GLint*, params)

/*
 * glGetTexParameterfv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GetTexParameterfv,
                 GLenum, target, GLenum, pname, GLfloat*, params)

/*
 * glGetTexParameterxv
 */
GLES_FUNC_3_ARGS(GPIGLESIMAGEONEXT, IMAGEONEXT, GetTexParameterxv,
                 GLenum, target, GLenum, pname, GLfixed*, params)


/*----------------------------------------------------------------------------*
 *              E X T E N S I O N   F U N C T I O N   T A B L E               *
 *----------------------------------------------------------------------------*/
static int InitKnownExtList(void)
{
  int i = 0;
  int egl_ext_cnt = 10;          // # extensions + 1 for NULL terminator
  int gles_ext_cnt = 15;        // # extensions + 1 for NULL terminator

  // Allocate space for the known extension lists
  GPEGLEXTLST = (KnownExtTblRec_t*)MALLOC(egl_ext_cnt * sizeof(KnownExtTblRec_t));
  if (!GPEGLEXTLST)
  {
    return 0;
  }
  GPGLESEXTLST = (KnownExtTblRec_t*)MALLOC(gles_ext_cnt * sizeof(KnownExtTblRec_t));
  if (!GPGLESEXTLST)
  {
    return 0;
  }

  /*
   * The known extension tables - lists all extensions known to this version
   * of eglGetProcAddress.  This list will probably include items not actually
   * supported by a particular driver.
   *
   * Each table must be terminated by an entry with the name field set to
   * NULL.
   *
   * To avoid false positives in the string matching:
   *
   *   The extension name must have a trailing space.
   *
   *   The function list must include a space after *every* function name
   *   including the last one in the string.
   */

  // EGL Extensions
  EGL_EXT_TBL_ENTRY("EGL_EXT_swap_control ",
                    "eglSwapIntervalOES eglGetSwapIntervalOES ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_get_color_buffer ",
                    "eglGetColorBufferQUALCOMM ");
EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_get_color_buffer_info ",
                    "eglGetColorBufferInfoQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_get_power_level ",
                    "eglGetPowerLevelQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_surface_scale ",
                    "eglSurfaceScaleEnableQUALCOMM "
                    "eglSetSurfaceScaleQUALCOMM "
                    "eglGetSurfaceScaleQUALCOMM "
                    "eglGetSurfaceScaleCapsQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_surface_rotate ",
                    "eglSurfaceRotateEnableQUALCOMM "
                    "eglSetSurfaceRotateQUALCOMM "
                    "eglGetSurfaceRotateQUALCOMM "
                    "eglGetSurfaceRotateCapsQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_surface_transparency ",
                    "eglSurfaceTransparencyEnableQUALCOMM "
                    "eglSetSurfaceTransparencyQUALCOMM "
                    "eglGetSurfaceTransparencyQUALCOMM "
                    "eglSetSurfaceTransparencyMapQUALCOMM "
                    "eglGetSurfaceTransparencyMapQUALCOMM "
                    "eglGetSurfaceTransparencyCapsQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_surface_color_key ",
                    "eglSurfaceColorKeyEnableQUALCOMM "
                    "eglSetSurfaceColorKeyQUALCOMM "
                    "eglGetSurfaceColorKeyQUALCOMM ");
  EGL_EXT_TBL_ENTRY("EGL_QUALCOMM_surface_overlay ",
                    "eglCreateCompositeSurfaceQUALCOMM "
                    "eglSurfaceOverlayEnableQUALCOMM "
                    "eglSurfaceOverlayLayerEnableQUALCOMM "
                    "eglSurfaceOverlayBindQUALCOMM "
                    "eglGetSurfaceOverlayBindingQUALCOMM "
                    "eglGetSurfaceOverlayQUALCOMM "
                    "eglGetSurfaceOverlayCapsQUALCOMM ");

  // GLES Extensions
  i = 0;
  GLES_EXT_TBL_ENTRY("GL_OES_query_matrix ",
                     "glQueryMatrixxOES ");
  GLES_EXT_TBL_ENTRY("GL_OES_matrix_palette ",
                     "glCurrentPaletteMatrixOES "
                     "glLoadPaletteFromModelViewMatrixOES "
                     "glMatrixIndexPointerOES "
                     "glWeightPointerOES ");
  GLES_EXT_TBL_ENTRY("GL_OES_draw_texture ",
                     "glDrawTexsOES "
                     "glDrawTexiOES "
                     "glDrawTexxOES "
                     "glDrawTexsvOES "
                     "glDrawTexivOES "
                     "glDrawTexxvOES "
                     "glDrawTexfOES "
                     "glDrawTexfvOES ");
  GLES_EXT_TBL_ENTRY("GL_OES_texture_cube_map ",
                     "glGetTexGeniv "
                     "glGetTexGenxv "
                     "glTexGeni "
                     "glTexGenx "
                     "glTexGeniv "
                     "glTexGenxv "
                     "glGetTexGenfv "
                     "glTexGenf "
                     "glTexGenfv ");
  GLES_EXT_TBL_ENTRY("GL_OES_blend_subtract ",
                     "glBlendEquation ");
  GLES_EXT_TBL_ENTRY("GL_OES_blend_func_separate ",
                     "glBlendFuncSeparate ");
  GLES_EXT_TBL_ENTRY("GL_OES_blend_equation_separate ",
                     "glBlendEquationSeparate ");
  GLES_EXT_TBL_ENTRY("GL_OES_framebuffer_object ",
                     "glBindFramebufferOES "
                     "glBindRenderbufferOES "
                     "glCheckFramebufferStatusOES "
                     "glDeleteFramebuffersOES "
                     "glDeleteRenderbuffersOES "
                     "glFramebufferRenderbufferOES "
                     "glFramebufferTexture2DOES "
                     "glGenerateMipmapOES "
                     "glGenFramebuffersOES "
                     "glGenRenderbuffersOES "
                     "glGetFramebufferAttachmentParameterivOES "
                     "glGetRenderbufferParameterivOES "
                     "glIsFramebufferOES "
                     "glIsRenderbufferOES "
                     "glRenderbufferStorageOES ");
  GLES_EXT_TBL_ENTRY("GL_OES_point_size_array ",
                     "glPointSizePointerOES ");
  GLES_EXT_TBL_ENTRY("GL_EXT_blend_minmax ",
                     "glBlendEquationEXT ");
  GLES_EXT_TBL_ENTRY("GL_EXT_blend_func_separate ",
                     "glBlendFuncSeparateEXT ");
  GLES_EXT_TBL_ENTRY("GL_EXT_blend_equation_separate ",
                     "glBlendEquationSeparateEXT ");
  ALIASED_GLES_EXT_TBL_ENTRY("GL_QUALCOMM_vertex_buffer_object ",
                             "glBindBufferQUALCOMM "
                             "glDeleteBuffersQUALCOMM "
                             "glGenBuffersQUALCOMM "
                             "glBufferDataQUALCOMM "
                             "glBufferSubDataQUALCOMM "
                             "glIsBufferQUALCOMM "
                             "glBufferDataATI "
                             "glMeshListATI "
                             "glDrawVertexBufferObjectATI ",
                             "GL_OES_vertex_buffer_object ",
                             "GL_ARB_vertex_buffer_object ");
  GLES_EXT_TBL_ENTRY("GL_ATI_imageon_misc ",
                     "glGetPointerv "
                     "glTexEnvi "
                     "glTexEnviv "
                     "glTexParameteri "
                     "glTexParameteriv "
                     "glTexParameterfv "
                     "glTexParameterxv "
                     "glGetMaterialfv "
                     "glGetTexParameteriv "
                     "glGetTexParameterfv "
                     "glGetTexParameterxv ");

  return 1;
}

static void patch_ext_list(const char *func_list_p) 
{
  char search_func_name[128];
  int i = 0;
  char *fn_p = NULL;
  char *fncopy_p = NULL;
  ExtTblRec_p search_p = NULL;
  int found = 0;

  fn_p = (char *)func_list_p;
  while ((unsigned int)(fn_p - func_list_p) <
         (unsigned int)STRLEN(func_list_p))
  {
    // copy the function name up to the next space or NUL
    fncopy_p = search_func_name;
    i = 0;
    while ((i < 127) && (*fn_p != ' ') && (*fn_p != '\0'))
    {
      *(fncopy_p++) = *(fn_p++);
      i++;
    }
    fn_p++;         // advance past the space
    *fncopy_p = '\0';

    // find the function name in the table
    search_p = GPEXTLST;
    found = 0;
    while (search_p->name != NULL)
    {
      if (!STRNCMP(search_func_name, search_p->name,
                   STRLEN(search_p->name)))
      {
        found++;
        break;
      }
      search_p++;
    }
    if (!found)
    {
      /*
       * This is an error... the runtime extension table should have
       * entries for all functions in the known extension lists.
       */

      // for now, just continue the outer while loop
      continue;
    }

    // zero the associated function pointer
    search_p->func_p = NULL;
  }
}

static void process_known_ext_list(KnownExtTblRec_p ket_p,
                                   const char *driver_ExtStr_p)
{
  KnownExtTblRec_p ext_search_p = ket_p;

  while (ext_search_p->name)
  {
    // check for extension name
    if (STRSTR(driver_ExtStr_p, ext_search_p->name))
    {
      ext_search_p++;
      continue;
    }
    // check for extension 1st alias if set
    if (ext_search_p->alias1)
    {
      if (STRSTR(driver_ExtStr_p, ext_search_p->alias1))
      {
        ext_search_p++;
        continue;
      }
    }
    // check for extension 2nd alias if set
    if (ext_search_p->alias2)
    {
      if (STRSTR(driver_ExtStr_p, ext_search_p->alias2))
      {
        ext_search_p++;
        continue;
      }
    }

    // Extension was not found so set all associated function pointers
    // to NULL
    patch_ext_list((const char *)ext_search_p->func_list);
    ext_search_p++;
  }
}

static int InitExtList(int SMan_V1, int Imag_V1)
{
  int i = 0;
  int ext_cnt = 94;     // # extensions + 1 for NULL terminator
  EGLDisplay dpy = eglGetCurrentDisplay();

  // Allocate space for the extension list
  GPEXTLST = (ExtTblRec_t*)MALLOC(ext_cnt * sizeof(ExtTblRec_t));
  if (!GPEXTLST)
  {
    return 0;
  }

  // ==================
  //
  // Initialize the full table
  //
  // ==================

  // EGL Extensions
  EXT_TBL_ENTRY(egl, SwapIntervalOES);
  EXT_TBL_ENTRY(egl, GetSwapIntervalOES);
  NAMED_EXT_TBL_ENTRY(egl, GetColorBuffer, "eglGetColorBufferQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetColorBufferInfo, "eglGetColorBufferInfoQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetPowerLevel, "eglGetPowerLevelQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceScaleEnable, "eglSurfaceScaleEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SetSurfaceScale, "eglSetSurfaceScaleQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceScale, "eglGetSurfaceScaleQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceScaleCaps,
                      "eglGetSurfaceScaleCapsQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceRotateEnable,
                      "eglSurfaceRotateEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SetSurfaceRotate, "eglSetSurfaceRotateQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceRotate, "eglGetSurfaceRotateQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceRotateCaps,
                      "eglGetSurfaceRotateCapsQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceTransparencyEnable,
                      "eglSurfaceTransparencyEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SetSurfaceTransparency,
                      "eglSetSurfaceTransparencyQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceTransparency,
                      "eglGetSurfaceTransparencyQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SetSurfaceTransparencyMap,
                      "eglSetSurfaceTransparencyMapQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceTransparencyMap,
                      "eglGetSurfaceTransparencyMapQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceTransparencyCaps,
                      "eglGetSurfaceTransparencyCapsQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceColorKeyEnable,
                      "eglSurfaceColorKeyEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SetSurfaceColorKey, "eglSetSurfaceColorKeyQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceColorKey, "eglGetSurfaceColorKeyQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, CreateCompositeSurface,
                      "eglCreateCompositeSurfaceQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceOverlayEnable,
                      "eglSurfaceOverlayEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceOverlayLayerEnable,
                      "eglSurfaceOverlayLayerEnableQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, SurfaceOverlayBind, "eglSurfaceOverlayBindQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceOverlayBinding,
                      "eglGetSurfaceOverlayBindingQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceOverlay, "eglGetSurfaceOverlayQUALCOMM");
  NAMED_EXT_TBL_ENTRY(egl, GetSurfaceOverlayCaps,
                      "eglGetSurfaceOverlayCapsQUALCOMM");
  // GLES 1.0 Extensions
  EXT_TBL_ENTRY(gl, QueryMatrixxOES);
  // GLES 1.1 Extensions
  EXT_TBL_ENTRY(gl, CurrentPaletteMatrixOES);
  EXT_TBL_ENTRY(gl, LoadPaletteFromModelViewMatrixOES);
  EXT_TBL_ENTRY(gl, MatrixIndexPointerOES);
  EXT_TBL_ENTRY(gl, WeightPointerOES);
  EXT_TBL_ENTRY(gl, DrawTexsOES);
  EXT_TBL_ENTRY(gl, DrawTexiOES);
  EXT_TBL_ENTRY(gl, DrawTexxOES);
  EXT_TBL_ENTRY(gl, DrawTexsvOES);
  EXT_TBL_ENTRY(gl, DrawTexivOES);
  EXT_TBL_ENTRY(gl, DrawTexxvOES);
  EXT_TBL_ENTRY(gl, DrawTexfOES);
  EXT_TBL_ENTRY(gl, DrawTexfvOES);
  // GLES 1.1 Extension Pak Extensions
  EXT_TBL_ENTRY(gl, GetTexGeniv);
  EXT_TBL_ENTRY(gl, GetTexGenxv);
  EXT_TBL_ENTRY(gl, TexGeniv);
  EXT_TBL_ENTRY(gl, TexGeni);
  EXT_TBL_ENTRY(gl, TexGenxv);
  EXT_TBL_ENTRY(gl, TexGenx);
  EXT_TBL_ENTRY(gl, GetTexGenfv);
  EXT_TBL_ENTRY(gl, TexGenfv);
  EXT_TBL_ENTRY(gl, TexGenf);
  EXT_TBL_ENTRY(gl, BlendEquationSeparate);
  EXT_TBL_ENTRY(gl, BlendEquation);
  EXT_TBL_ENTRY(gl, BlendFuncSeparate);
  EXT_TBL_ENTRY(gl, BindFramebufferOES);
  EXT_TBL_ENTRY(gl, BindRenderbufferOES);
  EXT_TBL_ENTRY(gl, CheckFramebufferStatusOES);
  EXT_TBL_ENTRY(gl, DeleteFramebuffersOES);
  EXT_TBL_ENTRY(gl, DeleteRenderbuffersOES);
  EXT_TBL_ENTRY(gl, FramebufferRenderbufferOES);
  EXT_TBL_ENTRY(gl, FramebufferTexture2DOES);
  EXT_TBL_ENTRY(gl, GenerateMipmapOES);
  EXT_TBL_ENTRY(gl, GenFramebuffersOES);
  EXT_TBL_ENTRY(gl, GenRenderbuffersOES);
  EXT_TBL_ENTRY(gl, GetFramebufferAttachmentParameterivOES);
  EXT_TBL_ENTRY(gl, GetRenderbufferParameterivOES);
  EXT_TBL_ENTRY(gl, IsFramebufferOES);
  EXT_TBL_ENTRY(gl, IsRenderbufferOES);
  EXT_TBL_ENTRY(gl, RenderbufferStorageOES);
  // Imageon Specific Extensions
  EXT_TBL_ENTRY(gl, PointSizePointerOES);
  EXT_TBL_ENTRY(gl, BlendEquationSeparateEXT);
  EXT_TBL_ENTRY(gl, BlendFuncSeparateEXT);
  EXT_TBL_ENTRY(gl, BlendEquationEXT);
  ALIASED_EXT_TBL_ENTRY(gl, BindBufferQUALCOMM, BindBufferOES, BindBufferARB);
  ALIASED_EXT_TBL_ENTRY(gl, DeleteBuffersQUALCOMM, DeleteBuffersOES,
                        DeleteBuffersARB);
  ALIASED_EXT_TBL_ENTRY(gl, GenBuffersQUALCOMM, GenBuffersOES, GenBuffersARB);
  ALIASED_EXT_TBL_ENTRY(gl, BufferDataQUALCOMM, BufferDataOES, BufferDataARB);
  ALIASED_EXT_TBL_ENTRY(gl, BufferSubDataQUALCOMM, BufferSubDataOES,
                        BufferSubDataARB);
  ALIASED_EXT_TBL_ENTRY(gl, IsBufferQUALCOMM, IsBufferOES, IsBufferARB);
  EXT_TBL_ENTRY(gl, BufferDataATI);             // deprecated
  EXT_TBL_ENTRY(gl, MeshListATI);               // deprecated
  EXT_TBL_ENTRY(gl, DrawVertexBufferObjectATI); // deprecated
  EXT_TBL_ENTRY(gl, GetPointerv);
  EXT_TBL_ENTRY(gl, TexEnviv);
  EXT_TBL_ENTRY(gl, TexEnvi);
  EXT_TBL_ENTRY(gl, TexParameteriv);
  EXT_TBL_ENTRY(gl, TexParameteri);
  EXT_TBL_ENTRY(gl, TexParameterfv);
  EXT_TBL_ENTRY(gl, TexParameterxv);
  EXT_TBL_ENTRY(gl, GetMaterialfv);
  EXT_TBL_ENTRY(gl, GetTexParameteriv);
  EXT_TBL_ENTRY(gl, GetTexParameterfv);
  EXT_TBL_ENTRY(gl, GetTexParameterxv);

  // ==================
  //
  // Get the extension strings back from the driver and
  // NULL the entries for unsupported extensions
  //
  // If this isn't done, the returned function pointers will
  // call through the BREW interface to a function in the OEM
  // layer which returns EUNSUPPORTED.  However, the return
  // value is currently inacessible.
  //
  // ==================

  InitKnownExtList();
  
  // Reconcile the known extension list against those actually supported
  // by the driver provided the interfaces have been initialized and
  // a display has been created.  Otherwise, all queries will return a
  // non-NULL value, but functions not actually supported by the
  // driver will return EUNSUPPORTED (this currently gets dropped in
  // the OEM layer).
  if (dpy)
  {
    process_known_ext_list(GPEGLEXTLST, eglQueryString(dpy, EGL_EXTENSIONS));

    process_known_ext_list(GPGLESEXTLST,
                           (const char *)glGetString(GL_EXTENSIONS));
  }

  if (SMan_V1)
  {
    patch_ext_list((const char *)"eglSurfaceColorKeyEnableQUALCOMM "
                                 "eglSetSurfaceColorKeyQUALCOMM "
                                 "eglGetSurfaceColorKeyQUALCOMM "
                                 "eglCreateCompositeSurfaceQUALCOMM "
                                 "eglSurfaceOverlayEnableQUALCOMM "
                                 "eglSurfaceOverlayLayerEnableQUALCOMM "
                                 "eglSurfaceOverlayBindQUALCOMM "
                                 "eglGetSurfaceOverlayBindingQUALCOMM "
                                 "eglGetSurfaceOverlayQUALCOMM "
                                 "eglGetSurfaceOverlayCapsQUALCOMM ");
  }

  if (Imag_V1)
  {
    patch_ext_list((const char *)"glGetMaterialfv glGetTexParameteriv "
                   "glGetTexParameterfv glGetTexParameterxv ");
  }
  return 1;
}

/*----------------------------------------------------------------------------*
 *                      P U B L I C    F U N C T I O N S                      *
 *----------------------------------------------------------------------------*/
void FreeExtList(void)
{
  if (GBINITDONE == GL_FALSE)
  {
    return;
  }
  if (GPEXTLST)
  {
    FREE(GPEGLEXTLST);
    FREE(GPGLESEXTLST);
    FREE(GPEXTLST);
  }
  GBINITDONE = GL_FALSE;
}

void ReleaseNBI(void)
{
  if (GPIEGLOSI)
  {
    IEGLOESSWAPINTERVAL_Release(GPIEGLOSI);
    GPIEGLOSI = NULL;
  }
  if (GPIEGLGCB)
  {
    IEGLGETCOLORBUFFER_Release(GPIEGLGCB);
    GPIEGLGCB = NULL;
  }
  if (GPIEGLGCBI)
  {
    IEGLGETCOLORBUFFERINFO_Release(GPIEGLGCBI);
    GPIEGLGCBI = NULL;
  }
  if (GPIEGLGPL)
  {
    IEGLGETPOWERLEVEL_Release(GPIEGLGPL);
    GPIEGLGPL = NULL;
  }
  if (GPIEGLSMAN)
  {
    IEGLSURFACEMANIP_Release(GPIEGLSMAN);
    GPIEGLSMAN = NULL;
  }
  if (GPIGLES10EXT)
  {
    IGLES10EXT_Release(GPIGLES10EXT);
    GPIGLES10EXT = NULL;
  }
  if (GPIGLES11EXT)
  {
    IGLES11EXT_Release(GPIGLES11EXT);
    GPIGLES11EXT = NULL;
  }
  if (GPIGLES11EXTPAK)
  {
    IGLES11EXTPAK_Release(GPIGLES11EXTPAK);
    GPIGLES11EXTPAK = NULL;
  }
  if (GPIGLESIMAGEONEXT)
  {
    IGLESIMAGEONEXT_Release(GPIGLESIMAGEONEXT);
    GPIGLESIMAGEONEXT = NULL;
  }
}

GL_API void (* GL_APIENTRY eglGetProcAddress (const char *procname))()
{
  void *fp = NULL;
  ExtTblRec_p search_p = NULL;
  char *egl_ext_str = NULL;
  char *gl_ext_str = NULL;
  EGLDisplay dpy = NULL;
  int SMan_V1 = 0;
  int Imag_V1 = 0;

  /*
   * Initialize the various interface pointers if required
   */
  if (GBINITDONE == GL_FALSE)
  {
    GPIEGL = NULL;
    GPIQEGL = NULL;
    //GPIQEGL11 = NULL;

    // Get the basic QEGL interface pointers
    GPIEGL = EGL_GetEGLInterfacePtr();
    GPIQEGL = EGL_GetQEGLInterfacePtr();
    if (GPIQEGL)
    {
      // Get the EGL extension interface pointers
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLOESSWAPINTERVAL,
                                (void**)&GPIEGLOSI) != SUCCESS)
      {
        GPIEGLOSI = NULL;
      }
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLGETCOLORBUFFER,
                                (void**)&GPIEGLGCB) != SUCCESS)
      {
        GPIEGLGCB = NULL;
      }
     if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLGETCOLORBUFFERINFO,
                                (void**)&gpIEGLGCBI) != SUCCESS)
      {
        gpIEGLGCBI = NULL;
      }

      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLGETPOWERLEVEL,
                                (void**)&GPIEGLGPL) != SUCCESS)
      {
        GPIEGLGPL = NULL;
      }
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLSURFACEMANIP,
                                (void**)&GPIEGLSMAN) != SUCCESS)
      {
        GPIEGLSMAN = NULL;
      }
      if (!GPIEGLSMAN)
      {
        dpy = eglGetCurrentDisplay();
        if (dpy)
        {
          egl_ext_str = (char *)eglQueryString(dpy, EGL_EXTENSIONS);
          if (STRSTR(egl_ext_str, "EGL_QUALCOMM_surface_scale "))
          {
            // Try the pre-production interface
            if (IEGL10_QueryInterface(GPIQEGL, AEEIID_EGLSURFACEMANIP_V1,
                                  (void**)&GPIEGLSMAN) != SUCCESS)
            {
              GPIEGLSMAN = NULL;
            }
            if (!GPIEGLSMAN)
            {
              DBGPRINTF("eglGetProcAddress (NBI) - "
                        "could not get EGLSurfaceManip interface");
            }
            else
            {
              SMan_V1++;
              DBGPRINTF("eglGetProcAddress (NBI) - "
                        "got V1 EGLSurfaceManip interface");
            }
          }
          else
          {
            DBGPRINTF("eglGetProcAddress (NBI) - "
                      "platform does not support EGLSurfaceManip interface");
          }
        }
        else
        {
          DBGPRINTF("eglGetProcAddress (NBI) - could not get display");
        }
      }
      else
      {
        DBGPRINTF("eglGetProcAddress (NBI) - "
                  "got V2 EGLSurfaceManip interface");
      }

      // Get the EGL extension interface pointers
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_GLES10EXT,
                                (void**)&GPIGLES10EXT) != SUCCESS)
      {
        GPIGLES10EXT = NULL;
      }
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_GLES11EXT,
                                (void**)&GPIGLES11EXT) != SUCCESS)
      {
        GPIGLES11EXT = NULL;
      }
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_GLES11EXTPAK,
                                (void**)&GPIGLES11EXTPAK) != SUCCESS)
      {
        GPIGLES11EXTPAK = NULL;
      }
      if (IEGL10_QueryInterface(GPIQEGL, AEEIID_GLESIMAGEONEXT,
                                (void**)&GPIGLESIMAGEONEXT) != SUCCESS)
      {
        GPIGLESIMAGEONEXT = NULL;
      }
      if (!GPIGLESIMAGEONEXT)
      {
        gl_ext_str = (char *)glGetString(GL_EXTENSIONS);
        if (STRSTR(gl_ext_str, "GL_ATI_imageon_misc "))
        {
          // Try the pre-production interface
          if (IEGL10_QueryInterface(GPIQEGL, AEEIID_GLESIMAGEONEXT_V1,
                                    (void**)&GPIGLESIMAGEONEXT) != SUCCESS)
          {
            GPIGLESIMAGEONEXT = NULL;
          }
          if (!GPIGLESIMAGEONEXT)
          {
            DBGPRINTF("eglGetProcAddress (NBI) - "
                      "could not get GLESImageonExt interface");
          }
          else
          {
            Imag_V1++;
            DBGPRINTF("eglGetProcAddress (NBI) - "
                      "got V1 GLESImageonExt interface");
          }
        }
        else
        {
          DBGPRINTF("eglGetProcAddress (NBI) - "
                    "platform does not support GLESImageonExt interface");
        }
      }
      else
      {
        DBGPRINTF("eglGetProcAddress (NBI) - "
                  "got V2 GLESImageonExt interface");
      }
    }

    // Setup the function pointer tables
    if (!InitExtList(SMan_V1, Imag_V1))
    {
      return (PFN_GETPROCADDRESS)fp;
    }

    GBINITDONE = GL_TRUE;
  }
 
  /*
   * Search the table for the specified procname
   */
  search_p = GPEXTLST;
  while (search_p->name != NULL)
  {
    // check name field first
    if (!STRNCMP(procname, search_p->name, STRLEN(search_p->name)))
    {
      fp = (void *)search_p->func_p;
      break;
    }
    // check 1st alias if set
    if (search_p->alias1)
    {
      if (!STRNCMP(procname, search_p->alias1, STRLEN(search_p->alias1)))
      {
        fp = (void *)search_p->func_p;
        break;
      }
    }
    // check 2nd alias if set
    if (search_p->alias2)
    {
      if (!STRNCMP(procname, search_p->alias2, STRLEN(search_p->alias2)))
      {
        fp = (void *)search_p->func_p;
        break;
      }
    }
    search_p++;
  }

  /*
   * If the wrapper is active we need to prevent an infinite loop
   */
#ifndef OES_OBI_WRAPPER
  if (!fp && GPIEGL)
  {
    /*
     * Bummer! - Try the old BREW interface
     */
    fp = (void *)IEGL_eglGetProcAddress(GPIEGL, procname);
  }
#endif

  return (PFN_GETPROCADDRESS)fp;
}

