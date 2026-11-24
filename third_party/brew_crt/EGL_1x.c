/*=============================================================================

    FILE:         EGL_1x.c

    SERVICES:     OpenGL ES (EGL only)

    DESCRIPTION:  Provides the EGL 1.x bindings which are implemented
                  using the EGL interfaces
	  
===============================================================================
        Copyright © 2006 QUALCOMM Incorporated.
		All Rights Reserved.
		QUALCOMM Proprietary/GTDR
=============================================================================*/


/*-----------------------------------------------------------------------------*
 *                      I N C L U D E   F I L E S                              *
 *-----------------------------------------------------------------------------*/
#include "AEEStdLib.h"
#include "AEEGL.h"              // Legacy, deprecated
#include "AEECLSID_QEGL.bid"
#include "AEEEGL11.h"

#include "EGL_1x.h"
#include "EGL_1x_support.h"     // egl function definition macros are here
#include "GLES_1x.h"


/*-----------------------------------------------------------------------------*
 *                            M A C R O S                                      *
 *-----------------------------------------------------------------------------*/
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


/*-----------------------------------------------------------------------------*
 *                            G L O B A L S                                    *
 *-----------------------------------------------------------------------------*/
#if defined(__ARMCC_VERSION) && !defined(AEE_STATIC)
static IEGL *const gpIEGL;
static IEGL10 *const gpIQEGL;
static IEGL11 *const gpIQEGL11;
static IShell *const gpIShell;
#define GPIEGL (*(IEGL**)&gpIEGL)
#define GPIQEGL (*(IEGL10**)&gpIQEGL)
#define GPIQEGL11 (*(IEGL11**)&gpIQEGL11)
#define GPISHELL (*(IShell**)&gpIShell)
#else
static IEGL *gpIEGL = NULL;
static IEGL10 *gpIQEGL = NULL;
static IEGL11 *gpIQEGL11 = NULL;
static IShell *gpIShell = NULL;
#define GPIEGL (gpIEGL)
#define GPIQEGL (gpIQEGL)
#define GPIQEGL11 (gpIQEGL11)
#define GPISHELL (gpIShell)
#endif


/*
 * EGL init state flag
 */
GV_DECLARE(GLboolean, gbEGLInitDone, GL_FALSE);
#define GBEGLINITDONE GV_ACCESS(GLboolean, gbEGLInitDone)


/*-----------------------------------------------------------------------------*
 *                            T Y P E D E F S                                  *
 *-----------------------------------------------------------------------------*/
#ifdef OES_OBI_WRAPPER
typedef struct _IEGLWRAPPER *IEGLWrapper_p;

typedef struct _IEGLWRAPPER
{
  AEEVTBL(IEGL) *pvt;
  uint32        m_uRef;
} IEGLWrapper;
#endif

/*-----------------------------------------------------------------------------*
 *                            EXTERNAL ROUTINES                                *
 *-----------------------------------------------------------------------------*/
extern void FreeExtList(void);
extern void ReleaseNBI(void);

/*-----------------------------------------------------------------------------*
 *                            INTERNAL ROUTINES                                *
 *-----------------------------------------------------------------------------*/
#ifdef OES_OBI_WRAPPER
uint32 EGL_1x_AddRef(IEGL *po)
{
  return 1;
}

uint32 EGL_1x_Release(IEGL *po)
{
  IEGLWrapper_p w_p = (IEGLWrapper_p)po;

  if (w_p)
  {
    if (w_p->pvt)
    {
      FREE(w_p->pvt);
    }
    FREE(w_p);
  }
  return 0;
}

int EGL_1x_QueryInterface(IEGL *po, AEECLSID idReq, void **ppo)
{
  return SUCCESS;
}
#endif


/*-----------------------------------------------------------------------------*
 *                      I N I T  F U N C T I O N S                             *
 *-----------------------------------------------------------------------------*/
int EGL_Init(IShell *pIShell)
{
#ifdef OES_OBI_WRAPPER
  IEGLWrapper *pIEGLFake = NULL;
  AEEVTBL(IEGL) *pvt = NULL;
#endif

  /*
   * Make sure we haven't already done this
   */
  if (GL_TRUE == GBEGLINITDONE)
  {
    return SUCCESS;
  }

  GPIEGL = NULL;
  GPIQEGL = NULL;
  GPIQEGL11 = NULL;
  GPISHELL = pIShell;

  ISHELL_AddRef(pIShell);
  if (ISHELL_CreateInstance(pIShell, AEECLSID_QEGL, (void **)&(GPIQEGL)) !=
      SUCCESS)
  {
    // Try the Legacy interface
    if (ISHELL_CreateInstance(pIShell, AEECLSID_EGL, (void **)&(GPIEGL)) !=
        SUCCESS)
    {
      // Failure, no supported OpenGL ES detected
      return EFAILED;
    }
  }
  else
  {
    // Create QEGL11 if available
    IEGL10_QueryInterface(GPIQEGL, AEEIID_EGL11, (void **)&(GPIQEGL11));
#ifdef OES_OBI_WRAPPER
    // Create 'fake' OBI [Old BREW Interface]
#ifndef __ARMCC_VERSION
#pragma message ( "***** Building OES_OBI_WRAPPER *****" )
#endif

    // 1st create the wrapper struct
    pIEGLFake = (IEGLWrapper_p)MALLOC(sizeof(IEGLWrapper));
    if (!pIEGLFake)
    {
      return EFAILED;
    }

    // 2nd alloc the vtable space
    pvt = (AEEVTBL(IEGL)*)MALLOC(sizeof(AEEVTBL(IEGL)));
    if (!pvt)
    {
      return EFAILED;
    }

    // 3rd stuff the vtable
    /*
     * These are faked locally
     */
    pvt->AddRef = EGL_1x_AddRef;
    pvt->Release = EGL_1x_Release;
    pvt->QueryInterface = EGL_1x_QueryInterface;
    /*
     * These all point to the new SHIM functions which will use the NBI
     */
    pvt->eglGetError = eglGetError;

    pvt->eglGetDisplay = eglGetDisplay;
    pvt->eglInitialize = eglInitialize;
    pvt->eglTerminate = eglTerminate;
    pvt->eglQueryString = eglQueryString;
    pvt->eglGetProcAddress = eglGetProcAddress;

    pvt->eglGetConfigs = eglGetConfigs;
    pvt->eglChooseConfig = eglChooseConfig;
    pvt->eglGetConfigAttrib = eglGetConfigAttrib;

    pvt->eglCreateWindowSurface = eglCreateWindowSurface;
    pvt->eglCreatePixmapSurface = eglCreatePixmapSurface;
    pvt->eglCreatePbufferSurface = eglCreatePbufferSurface;
    pvt->eglDestroySurface = eglDestroySurface;
    pvt->eglQuerySurface = eglQuerySurface;

    pvt->eglCreateContext = eglCreateContext;
    pvt->eglDestroyContext = eglDestroyContext;
    pvt->eglMakeCurrent = eglMakeCurrent;
    pvt->eglGetCurrentContext = eglGetCurrentContext;
    pvt->eglGetCurrentSurface = eglGetCurrentSurface;
    pvt->eglGetCurrentDisplay = eglGetCurrentDisplay;
    pvt->eglQueryContext = eglQueryContext;

    pvt->eglWaitGL = eglWaitGL;
    pvt->eglWaitNative = eglWaitNative;
    pvt->eglSwapBuffers = eglSwapBuffers;
    pvt->eglCopyBuffers = eglCopyBuffers;

    // 4th hookup the vtable & set the reference count
    pIEGLFake->pvt = pvt;
    pIEGLFake->m_uRef = 1;
    // Stash it so we can free it later
    GPIEGL = (IEGL *)pIEGLFake;
#endif
  }

  GLES_Init(pIShell);

  GBEGLINITDONE = GL_TRUE;

  return SUCCESS;
}


/*-----------------------------------------------------------------------------*
 *                R E L E A S E  F U N C T I O N S                             *
 *-----------------------------------------------------------------------------*/
void EGL_Release(void)
{
  /*
   * Bail if we didn't init egl
   */
  if (GL_FALSE == GBEGLINITDONE)
  {
    return;
  }

  GLES_Release();
  if (NULL != GPIEGL)
  {
    /*
     * If we have an NBI interface pointer then GPIEGL points at our
     * faked wrapper struct... otherwise it's an OBI instance
     *
     * if it's our faked version, the EGL_1x_Release function above
     * is called to free our fake interface and vtable. Otherwise
     * the normal IEGL_Release method is called.
     */
    IEGL_Release(GPIEGL);
    GPIEGL = NULL;
  }
  if (NULL != GPIQEGL11)
  {
    IEGL11_Release(GPIQEGL11);
    GPIQEGL11 = NULL;
  }
  if (NULL != GPIQEGL)
  {
    ReleaseNBI();
    FreeExtList();
    IEGL10_Release(GPIQEGL);
    GPIQEGL = NULL;
  }
  if (NULL != GPISHELL)
  {
    ISHELL_Release(GPISHELL);
    GPISHELL = NULL;
  }

  GBEGLINITDONE = GL_FALSE;
}


/*-----------------------------------------------------------------------------*
 *                U T I L I T Y  F U N C T I O N S                             *
 *-----------------------------------------------------------------------------*/
IEGL *EGL_GetEGLInterfacePtr(void)
{
  return GPIEGL;
}

IEGL10 *EGL_GetQEGLInterfacePtr(void)
{
  return GPIQEGL;
}


/*-----------------------------------------------------------------------------*
 *                     EGL  FUNCTIONS                                          *
 *-----------------------------------------------------------------------------*/

/*
 * eglGetError
 */
EGL_FUNC_0_ARGS(EGLint, GetError, EGL_NOT_INITIALIZED)

/*
 * eglGetDisplay
 */
EGLDisplay eglGetDisplay (NativeDisplayType display)
{
  EGLDisplay ret = NULL;
  IDisplay *disp_p = NULL;
  IBitmap *bm_p = NULL;
  IDIB *dibm_p = NULL;

  if ((NULL != GPIQEGL) && (NULL != GPISHELL))
  {
    if (display != EGL_DEFAULT_DISPLAY)
    {
      disp_p = display;
    }
    else if (ISHELL_CreateInstance(GPISHELL, AEECLSID_DISPLAY1,
                                   (void**)(&disp_p)) != SUCCESS)
    {
      return EGL_NO_DISPLAY;
    }

    IDISPLAY_GetDeviceBitmap(disp_p, &bm_p);
    IBITMAP_QueryInterface(bm_p, AEECLSID_DIB, (void**)&dibm_p);

    IEGL10_GetDisplay(GPIQEGL, (IQueryInterface *)dibm_p, &ret);

    IDIB_Release(dibm_p);
    IBITMAP_Release(bm_p);
    if (display == EGL_DEFAULT_DISPLAY)
    {
      IDISPLAY_Release(disp_p);
    }
    return ret;
  }
  if (NULL != GPIEGL)
  {
    return (IEGL_eglGetDisplay(GPIEGL, display));
  }

  return EGL_NO_DISPLAY;
}

/*
 * eglInitialize
 */
EGL_FUNC_3_ARGS(EGLBoolean, Initialize, EGL_FALSE,
                EGLDisplay, dpy, EGLint*, major, EGLint*, minor)
/*
 * eglTerminate
 */
EGL_FUNC_1_ARGS(EGLBoolean, Terminate, EGL_FALSE,
                EGLDisplay, dpy)

/*
 * eglQueryString
 */
EGL_FUNC_2_ARGS(const char*, QueryString, NULL,
                EGLDisplay, dpy, EGLint, name)

/*
 * eglGetConfigs
 */
EGL_FUNC_4_ARGS(EGLBoolean, GetConfigs, EGL_FALSE,
                EGLDisplay, dpy, EGLConfig*, configs, EGLint, config_size,
                EGLint*, num_config)

/*
 * eglChooseConfig
 */
EGL_FUNC_5_ARGS(EGLBoolean, ChooseConfig, EGL_FALSE,
                EGLDisplay, dpy, const EGLint*, attrib_list,
                EGLConfig*, configs, EGLint, config_size, EGLint*, num_config)

/*
 * eglGetConfigAttrib
 */
EGL_FUNC_4_ARGS(EGLBoolean, GetConfigAttrib, EGL_FALSE,
                EGLDisplay, dpy, EGLConfig, config, EGLint, attribute,
                EGLint*, value)

/*
 * eglCreateWindowSurface
 */
EGL_NTFUNC_4_ARGS(EGLSurface, CreateWindowSurface, EGL_NO_SURFACE,
                  EGLDisplay, dpy, EGLConfig, config, NativeWindowType, window,
                  const EGLint*, attrib_list)

/*
 * eglCreatePixmapSurface
 */
EGL_NTFUNC_4_ARGS(EGLSurface, CreatePixmapSurface, EGL_NO_SURFACE,
                  EGLDisplay, dpy, EGLConfig, config, NativePixmapType, pixmap,
                  const EGLint*, attrib_list)

/*
 * eglCreatePbufferSurface
 */
EGL_FUNC_3_ARGS(EGLSurface, CreatePbufferSurface, EGL_NO_SURFACE,
                EGLDisplay, dpy, EGLConfig, config,
                const EGLint*, attrib_list)

/*
 * eglDestroySurface
 */
EGL_FUNC_2_ARGS(EGLBoolean, DestroySurface, EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surface)

/*
 * eglQuerySurface
 */
EGL_FUNC_4_ARGS(EGLBoolean, QuerySurface, EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, surface, EGLint, attribute,
                EGLint*, value)

/*
 * eglCreateContext
 */
EGL_FUNC_4_ARGS(EGLContext, CreateContext, EGL_NO_CONTEXT,
                EGLDisplay, dpy, EGLConfig, config, EGLContext, share_list,
                const EGLint*, attrib_list)

/*
 * eglDestroyContext
 */
EGL_FUNC_2_ARGS(EGLBoolean, DestroyContext, EGL_FALSE,
                EGLDisplay, dpy, EGLContext, ctx)

/*
 * eglMakeCurrent
 */
EGL_FUNC_4_ARGS(EGLBoolean, MakeCurrent, EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, draw, EGLSurface, read,
                EGLContext, ctx)

/*
 * eglGetCurrentContext
 */
EGL_FUNC_0_ARGS(EGLContext, GetCurrentContext, EGL_NO_CONTEXT)

/*
 * eglGetCurrentSurface
 */
EGL_FUNC_1_ARGS(EGLSurface, GetCurrentSurface, EGL_NO_SURFACE,
                EGLint, readdraw)

/*
 * eglGetCurrentDisplay
 */
EGL_FUNC_0_ARGS(EGLDisplay, GetCurrentDisplay, EGL_NO_DISPLAY)

/*
 * eglQueryContext
 */
EGL_FUNC_4_ARGS(EGLBoolean, QueryContext, EGL_FALSE,
                EGLDisplay, dpy, EGLContext, ctx, EGLint, attribute,
                EGLint*, value)

/*
 * eglWaitGL
 */
EGL_FUNC_0_ARGS(EGLBoolean, WaitGL, EGL_FALSE)

/*
 * eglWaitNative
 */
EGL_FUNC_1_ARGS(EGLBoolean, WaitNative, EGL_FALSE,
                EGLint, engine)

/*
 * eglSwapBuffers
 */
EGL_FUNC_2_ARGS(EGLBoolean, SwapBuffers, EGL_FALSE,
                EGLDisplay, dpy, EGLSurface, draw)

/*
 * eglCopyBuffers
 */
EGL_NTFUNC_3_ARGS(EGLBoolean, CopyBuffers, EGL_FALSE,
                  EGLDisplay, dpy, EGLSurface, surface, NativePixmapType, target)


/*
 * Begin EGL 1.1 stuff
 */
/*
 * eglSurfaceAttrib
 */
EGL11_FUNC_4_ARGS(EGLBoolean, SurfaceAttrib, EGL_FALSE,
                  EGLDisplay, dpy, EGLSurface, surface, EGLint, attribute,
                  EGLint, value)

/*
 * eglBindTexImage
 */
EGL11_FUNC_3_ARGS(EGLBoolean, BindTexImage, EGL_FALSE,
                  EGLDisplay, dpy, EGLSurface, surface, EGLint, buffer)

/*
 * eglReleaseTexImage
 */
EGL11_FUNC_3_ARGS(EGLBoolean, ReleaseTexImage, EGL_FALSE,
                  EGLDisplay, dpy, EGLSurface, surface, EGLint, buffer)

/*
 * eglSwapInterval
 */
EGL11_FUNC_2_ARGS(EGLBoolean, SwapInterval, EGL_FALSE,
                  EGLDisplay, dpy, EGLint, interval)
