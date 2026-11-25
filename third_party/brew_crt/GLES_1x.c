/*=============================================================================

    FILE:         GLES_1x.c

    SERVICES:     OpenGL ES

    DESCRIPTION:  Provides the OpenGL ES 1.1 bindings which are implemented
                  using the GL interfaces

===============================================================================
        Copyright © 2006 QUALCOMM Incorporated.
		All Rights Reserved.
		QUALCOMM Proprietary/GTDR
=============================================================================*/


/*----------------------------------------------------------------------------*
 *                      I N C L U D E   F I L E S                             *
 *----------------------------------------------------------------------------*/
#include "AEEStdLib.h"
#include "AEEGL.h"		// Legacy, deprecated
#include "AEEGLES10.h"
#include "AEEGLES11.h"

#include "AEEEGL11.h"
#include "EGL_1x.h"
#include "GLES_1x.h"
#include "GLES_1x_support.h"    // GLES function definition macros are here


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
 *                            G L O B A L S                                   *
 *----------------------------------------------------------------------------*/
#if defined(__ARMCC_VERSION) && !defined(AEE_STATIC)
IGL *const gpIGL;		//deprecated, here for backward compatiable
IGLES10 *const gpIGLES10;       //gl 1.0 instance
IGLES11 *const gpIGLES11;       //gl 1.1 inatance
#define GPIGL (*(IGL**)&gpIGL)
#define GPIGLES10 (*(IGLES10**)&gpIGLES10)
#define GPIGLES11 (*(IGLES11**)& gpIGLES11)
#else
IGL *gpIGL = NULL;
IGLES10 *gpIGLES10 = NULL;
IGLES11 *gpIGLES11 = NULL;
#define GPIGL (gpIGL)
#define GPIGLES10 (gpIGLES10)
#define GPIGLES11 (gpIGLES11)
#endif

/*
 * GL init state flag
 */
GV_DECLARE(GLboolean, gbGLInitDone, GL_FALSE);
#define GBGLINITDONE GV_ACCESS(GLboolean, gbGLInitDone)

static void *g_gles10Vtbl = NULL;
static void *g_gles10Release = NULL;
static void *g_gles11Vtbl = NULL;
static void *g_gles11Release = NULL;


/*----------------------------------------------------------------------------*
 *                            T Y P E D E F S                                 *
 *----------------------------------------------------------------------------*/
#ifndef GL_OES_VERSION_1_1
typedef int GLsizeiptr;
typedef int GLintptr;
#endif  // GL_OES_VERSION_1_1

#ifdef OES_OBI_WRAPPER
typedef struct _IGLWRAPPER *IGLWrapper_p;

typedef struct _IGLWRAPPER
{
  AEEVTBL(IGL)  *pvt;
  uint32        m_uRef;
} IGLWrapper;
#endif

/*----------------------------------------------------------------------------*
 *                            INTERNAL ROUTINES                               *
 *----------------------------------------------------------------------------*/
#ifdef OES_OBI_WRAPPER
uint32 GLES_1x_AddRef(IGL *po)
{
  return 1;
}

uint32 GLES_1x_Release(IGL *po)
{
  IGLWrapper_p w_p = (IGLWrapper_p)po;

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

int GLES_1x_QueryInterface(IGL *po, AEECLSID idReq, void **ppo)
{
  return SUCCESS;
}
#endif

/*----------------------------------------------------------------------------*
 *             E X T E R N A L   U T I L I T Y    F U N C T I O N S           *
 *----------------------------------------------------------------------------*/
extern IEGL *EGL_GetEGLInterfacePtr(void);

extern IEGL10 *EGL_GetQEGLInterfacePtr(void);

/*----------------------------------------------------------------------------*
 *                      I N I T  F U N C T I O N S                            *
 *----------------------------------------------------------------------------*/
//this function is for legacy only
//the new version of GL interface does not allow to create instance from IShell 
int GLES_Init(IShell *pIShell)
{
#ifdef OES_OBI_WRAPPER
  IGLWrapper_p pIGLFake = NULL;
  AEEVTBL(IGL) *pvt = NULL;
#endif

  /*
   * Make sure we haven't already done this
   */
  if (GL_TRUE == GBGLINITDONE)
  {
    return SUCCESS;
  }

  GPIGL = NULL;
  GPIGLES10 = NULL;
  GPIGLES11 = NULL;

  if (EGL_GetQEGLInterfacePtr() != NULL)
  {
    IEGL10_QueryInterface(EGL_GetQEGLInterfacePtr(), AEEIID_GLES10,
                          (void **)&(GPIGLES10));
    IEGL10_QueryInterface(EGL_GetQEGLInterfacePtr(), AEEIID_GLES11,
                          (void **)&(GPIGLES11));

    g_gles10Vtbl = (GPIGLES10 && AEEGETPVTBL(GPIGLES10, IGLES10)) ? (void *)AEEGETPVTBL(GPIGLES10, IGLES10) : NULL;
    g_gles10Release = g_gles10Vtbl ? (void *)AEEGETPVTBL(GPIGLES10, IGLES10)->Release : NULL;
    g_gles11Vtbl = (GPIGLES11 && AEEGETPVTBL(GPIGLES11, IGLES11)) ? (void *)AEEGETPVTBL(GPIGLES11, IGLES11) : NULL;
    g_gles11Release = g_gles11Vtbl ? (void *)AEEGETPVTBL(GPIGLES11, IGLES11)->Release : NULL;

#ifdef OES_OBI_WRAPPER
    // Create 'fake' old interface
#ifndef __ARMCC_VERSION
#pragma message ( "***** Building OES_OBI_WRAPPER *****" )
#endif

    // 1st create the wrapper struct
    pIGLFake = (IGLWrapper_p)MALLOC(sizeof(IGLWrapper));
    if (!pIGLFake)
    {
      return EFAILED;
    }

    // 2nd alloc the vtable space
    pvt = (AEEVTBL(IGL)*)MALLOC(sizeof(AEEVTBL(IGL)));
    if (!pvt)
    {
      return EFAILED;
    }

    // 3rd stuff the vtable
    /*
     * These are faked locally
     */
    pvt->AddRef = GLES_1x_AddRef;
    pvt->Release = GLES_1x_Release;
    pvt->QueryInterface = GLES_1x_QueryInterface;
    /*
     * These all point to the new SHIM functions which will use the NBI
     */
    pvt->glActiveTexture = glActiveTexture;
    pvt->glAlphaFuncx = glAlphaFuncx;
    pvt->glBindTexture = glBindTexture;
    pvt->glBlendFunc = glBlendFunc;
    pvt->glClear = glClear;
    pvt->glClearColorx = glClearColorx;
    pvt->glClearDepthx = glClearDepthx;
    pvt->glClearStencil = glClearStencil;
    pvt->glClientActiveTexture = glClientActiveTexture;
    pvt->glColor4x = glColor4x;
    pvt->glColorMask = glColorMask;
    pvt->glColorPointer = glColorPointer;
    pvt->glCompressedTexImage2D = glCompressedTexImage2D;
    pvt->glCompressedTexSubImage2D = glCompressedTexSubImage2D;
    pvt->glCopyTexImage2D = glCopyTexImage2D;
    pvt->glCopyTexSubImage2D = glCopyTexSubImage2D;
    pvt->glCullFace = glCullFace;
    pvt->glDeleteTextures = glDeleteTextures;
    pvt->glDepthFunc = glDepthFunc;
    pvt->glDepthMask = glDepthMask;
    pvt->glDepthRangex = glDepthRangex;
    pvt->glDisable = glDisable;
    pvt->glDisableClientState = glDisableClientState;
    pvt->glDrawArrays = glDrawArrays;
    pvt->glDrawElements = glDrawElements;
    pvt->glEnable = glEnable;
    pvt->glEnableClientState = glEnableClientState;
    pvt->glFinish = glFinish;
    pvt->glFlush = glFlush;
    pvt->glFogx = glFogx;
    pvt->glFogxv = glFogxv;
    pvt->glFrontFace = glFrontFace;
    pvt->glFrustumx = glFrustumx;
    pvt->glGenTextures = glGenTextures;
    pvt->glGetError = glGetError;
    pvt->glGetIntegerv = glGetIntegerv;
    pvt->glGetString = glGetString;
    pvt->glHint = glHint;
    pvt->glLightModelx = glLightModelx;
    pvt->glLightModelxv = glLightModelxv;
    pvt->glLightx = glLightx;
    pvt->glLightxv = glLightxv;
    pvt->glLineWidthx = glLineWidthx;
    pvt->glLoadIdentity = glLoadIdentity;
    pvt->glLoadMatrixx = glLoadMatrixx;
    pvt->glLogicOp = glLogicOp;
    pvt->glMaterialx = glMaterialx;
    pvt->glMaterialxv = glMaterialxv;
    pvt->glMatrixMode = glMatrixMode;
    pvt->glMultMatrixx = glMultMatrixx;
    pvt->glMultiTexCoord4x = glMultiTexCoord4x;
    pvt->glNormal3x = glNormal3x;
    pvt->glNormalPointer = glNormalPointer;
    pvt->glOrthox = glOrthox;
    pvt->glPixelStorei = glPixelStorei;
    pvt->glPointSizex = glPointSizex;
    pvt->glPolygonOffsetx = glPolygonOffsetx;
    pvt->glPopMatrix = glPopMatrix;
    pvt->glPushMatrix = glPushMatrix;
    pvt->glReadPixels = glReadPixels;
    pvt->glRotatex = glRotatex;
    pvt->glSampleCoveragex = glSampleCoveragex;
    pvt->glScalex = glScalex;
    pvt->glScissor = glScissor;
    pvt->glShadeModel = glShadeModel;
    pvt->glStencilFunc = glStencilFunc;
    pvt->glStencilMask = glStencilMask;
    pvt->glStencilOp = glStencilOp;
    pvt->glTexCoordPointer = glTexCoordPointer;
    pvt->glTexEnvx = glTexEnvx;
    pvt->glTexEnvxv = glTexEnvxv;
    pvt->glTexImage2D = glTexImage2D;
    pvt->glTexParameterx = glTexParameterx;
    pvt->glTexSubImage2D = glTexSubImage2D;
    pvt->glTranslatex = glTranslatex;
    pvt->glVertexPointer = glVertexPointer;
    pvt->glViewport = glViewport;

    // 4th hookup the vtable and set the reference count
    pIGLFake->pvt = pvt;
    pIGLFake->m_uRef = 1;
    // Stash it so we can free it later
    GPIGL = (IGL *)pIGLFake;
#endif
  }
  else
  {
    // Try the legacy GL interface
    if (ISHELL_CreateInstance(pIShell, AEECLSID_GL, (void **)&(GPIGL)) !=
        SUCCESS)
    {
      // Failure, no supported OpenGL ES detected
      return EFAILED;
    }
  }

  GBGLINITDONE = GL_TRUE;

  return SUCCESS;
}


/*----------------------------------------------------------------------------*
 *                R E L E A S E  F U N C T I O N S                            *
 *----------------------------------------------------------------------------*/
void GLES_Release(void)
{
  /*
   * Bail if we didn't init gl
   */
  if (GL_FALSE == GBGLINITDONE)
  {
    return;
  }

  if (NULL != GPIGL)
  {
    /*
     * If we have an NBI interface pointer then GPIGL points at our
     * faked wrapper struct... otherwise it's an OBI instance
     *
     * if it's our faked version, the GLES_1x_Release function above
     * is called to free our fake interface and vtable. Otherwise
     * the normal IGL_Release method is called.
     */
    IGL_Release(GPIGL);
    GPIGL = NULL;
  }
  if (NULL != GPIGLES11)
  {
    AEEVTBL(IGLES11) *pvt11 = AEEGETPVTBL(GPIGLES11, IGLES11);
    if (NULL != pvt11 &&
        (void *)pvt11 == g_gles11Vtbl &&
        NULL != pvt11->Release &&
        (void *)pvt11->Release == g_gles11Release &&
        (void *)pvt11->Release != (void *)pvt11->ClipPlanef)
    {
      IGLES11_Release(GPIGLES11);
    }
    GPIGLES11 = NULL;
    g_gles11Vtbl = NULL;
    g_gles11Release = NULL;
  }
  if (NULL != GPIGLES10)
  {
    AEEVTBL(IGLES10) *pvt10 = AEEGETPVTBL(GPIGLES10, IGLES10);
    if (NULL != pvt10 &&
        (void *)pvt10 == g_gles10Vtbl &&
        NULL != pvt10->Release &&
        (void *)pvt10->Release == g_gles10Release)
    {
      IGLES10_Release(GPIGLES10);
    }
    GPIGLES10 = NULL;
    g_gles10Vtbl = NULL;
    g_gles10Release = NULL;
  }
  GBGLINITDONE = GL_FALSE;

}

/*----------------------------------------------------------------------------*
 *                     GL 1.0  FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/
/*
 * glActiveTexture
 */
GLCL_FUNC_1_ARGS(ActiveTexture,
                 GLenum, texture)

/*
 * glAlphaFunc
 */
GLCM_FUNC_2_ARGS(AlphaFunc,
                 GLenum, func, GLclampf, ref)

/*
 * glAlphaFuncx 
 */
GLCL_FUNC_2_ARGS(AlphaFuncx,
                 GLenum, func, GLclampx, ref)

/*
 * glBindTexture 
 */
GLCL_FUNC_2_ARGS(BindTexture,
                 GLenum, target, GLuint, texture)

/*
 * glBlendFunc 
 */
GLCL_FUNC_2_ARGS(BlendFunc,
                 GLenum, sfactor, GLenum, dfactor)

/*
 * glClear 
 */
GLCL_FUNC_1_ARGS(Clear,
                 GLbitfield, mask)

/*
 * glClearColor 
 */
GLCM_FUNC_4_ARGS(ClearColor,
                 GLclampf, red, GLclampf, green, GLclampf, blue,
                 GLclampf, alpha)

/*
 * glClearColorx
 */
GLCL_FUNC_4_ARGS(ClearColorx,
                 GLclampx, red, GLclampx, green, GLclampx, blue,
                 GLclampx, alpha)

/*
 * glClearDepthf 
 */
GLCM_FUNC_1_ARGS(ClearDepthf,
                 GLclampf, depth)

/*
 * glClearDepthx 
 */
GLCL_FUNC_1_ARGS(ClearDepthx,
                 GLclampx, depth)

/*
 * glClearStencil 
 */
GLCL_FUNC_1_ARGS(ClearStencil,
                 GLint, s)

/*
 * glClientActiveTexture 
 */
GLCL_FUNC_1_ARGS(ClientActiveTexture,
                 GLenum, texture)

/*
 * glColor4f 
 */
GLCM_FUNC_4_ARGS(Color4f,
                 GLfloat, red, GLfloat, green, GLfloat, blue, GLfloat, alpha)

/*
 * glColor4x 
 */
GLCL_FUNC_4_ARGS(Color4x,
                 GLfixed, red, GLfixed, green, GLfixed, blue, GLfixed, alpha)

/*
 * glColorMask 
 */
GLCL_FUNC_4_ARGS(ColorMask,
                 GLboolean, red, GLboolean, green, GLboolean, blue,
                 GLboolean, alpha)

/*
 * glColorPointer 
 */
GLCL_FUNC_4_ARGS(ColorPointer,
                 GLint, size, GLenum, type, GLsizei, stride,
                 const GLvoid* ,pointer)

/*
 * glCompressedTexImage2D 
 */
GLCL_FUNC_8_ARGS(CompressedTexImage2D,
                GLenum, target, GLint, level, GLenum, internalformat,
                GLsizei, width, GLsizei, height, GLint, border,
                GLsizei, imageSize, const GLvoid* ,data)

/*
 * glCompressedTexSubImage2D 
 */
GLCL_FUNC_9_ARGS(CompressedTexSubImage2D,
                 GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset,
                 GLsizei, width, GLsizei, height, GLenum, format,
                 GLsizei, imageSize, const GLvoid* ,data)

/*
 * glCopyTexImage2D 
 */
GLCL_FUNC_8_ARGS(CopyTexImage2D,
                 GLenum, target, GLint, level, GLenum, internalformat,
                 GLint, x, GLint, y, GLsizei, width, GLsizei, height,
                 GLint, border)

/*
 * glCopyTexSubImage2D 
 */
GLCL_FUNC_8_ARGS(CopyTexSubImage2D,
                 GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset,
                 GLint, x, GLint, y, GLsizei, width, GLsizei, height)

/*
 * glCullFace 
 */
GLCL_FUNC_1_ARGS(CullFace,
                 GLenum, mode)

/*
 * glDeleteTextures 
 */
GLCL_FUNC_2_ARGS(DeleteTextures,
                 GLsizei, n, const GLuint*, textures)

/*
 * glDepthFunc 
 */
GLCL_FUNC_1_ARGS(DepthFunc,
                 GLenum, func)

/*
 * glDepthMask 
 */
GLCL_FUNC_1_ARGS(DepthMask,
                 GLboolean, flag)

/*
 * glDepthRangef 
 */
GLCM_FUNC_2_ARGS(DepthRangef,
                 GLclampf, zNear, GLclampf, zFar)

/*
 * glDepthRangex 
 */
GLCL_FUNC_2_ARGS(DepthRangex,
                 GLclampx, zNear, GLclampx, zFar)

/*
 * glDisable 
 */
GLCL_FUNC_1_ARGS(Disable,
                 GLenum, cap)

/*
 * glDisableClientState 
 */
GLCL_FUNC_1_ARGS(DisableClientState,
                 GLenum, array)

/*
 * glDrawArrays 
 */
GLCL_FUNC_3_ARGS(DrawArrays,
                 GLenum, mode, GLint, first, GLsizei, count)

/*
 * glDrawElements 
 */
GLCL_FUNC_4_ARGS(DrawElements,
                 GLenum, mode, GLsizei, count, GLenum, type,
                 const GLvoid*, indices)

/*
 * glEnable 
 */
GLCL_FUNC_1_ARGS(Enable,
                 GLenum, cap)

/*
 * glEnableClientState 
 */
GLCL_FUNC_1_ARGS(EnableClientState,
                 GLenum, array)

/*
 * glFinish 
 */
GLCL_FUNC_0_ARGS(Finish)
                 

/*
 * glFlush 
 */
GLCL_FUNC_0_ARGS(Flush)

/*
 * glFogf 
 */
GLCM_FUNC_2_ARGS(Fogf,
                 GLenum, pname, GLfloat, param)

/*
 * glFogfv 
 */
GLCM_FUNC_2_ARGS(Fogfv,
                 GLenum, pname, const GLfloat*, params)

/*
 * glFogx 
 */
GLCL_FUNC_2_ARGS(Fogx,
                 GLenum, pname, GLfixed, param)

/*
 * glFogxv 
 */
GLCL_FUNC_2_ARGS(Fogxv,
                 GLenum, pname, const GLfixed*, params)

/*
 * glFrontFace 
 */
GLCL_FUNC_1_ARGS(FrontFace,
                 GLenum, mode)

/*
 * glFrustumf 
 */
GLCM_FUNC_6_ARGS(Frustumf,
                 GLfloat, left, GLfloat, right, GLfloat, bottom,
                 GLfloat, top, GLfloat, zNear, GLfloat, zFar)

/*
 * glFrustumx 
 */
GLCL_FUNC_6_ARGS(Frustumx,
                 GLfixed, left, GLfixed, right, GLfixed, bottom,
                 GLfixed, top, GLfixed, zNear, GLfixed, zFar)

/*
 * glGenTextures 
 */
GLCL_FUNC_2_ARGS(GenTextures,
                 GLsizei, n, GLuint*, textures)

/*
 * glGetError
 */
GL_API GLenum GL_APIENTRY glGetError(void)
{
  GLenum ret;

  if (NULL != GPIGLES11)
  {
    IGLES11_GetError(GPIGLES11, &ret);
    return ret;
  }
  else if (NULL != GPIGLES10)
  {
    IGLES10_GetError(GPIGLES10, &ret);
    return ret;
  }
  else if (NULL != GPIGL)
  {
    return IGL_glGetError(GPIGL);
  }

  return GL_INVALID_OPERATION;
}

/*
 * glGetIntegerv 
 */
GLCL_FUNC_2_ARGS(GetIntegerv,
                 GLenum, pname, GLint*, params)

/*
 * glGetString
 */
GL_API const GLubyte * GL_APIENTRY glGetString(GLenum name)
{
  const GLubyte *ret;

  if (NULL != GPIGLES11)
  {
    IGLES11_GetString(GPIGLES11, name, &ret);
    return ret;
  }
  else if (NULL != GPIGLES10 )
  {
    IGLES10_GetString(GPIGLES10, name, &ret);
    return ret;
  }
  else if (NULL != GPIGL)
  {
    return IGL_glGetString(GPIGL, name);
  }

  return NULL;
}

/*
 * glHint 
 */
GLCL_FUNC_2_ARGS(Hint,
                 GLenum, target, GLenum, mode)

/*
 * glLightModelf 
 */
GLCM_FUNC_2_ARGS(LightModelf,
                 GLenum, pname, GLfloat, param)

/*
 * glLightModelfv 
 */
GLCM_FUNC_2_ARGS(LightModelfv,
                 GLenum, pname, const GLfloat*, params)

/*
 * glLightModelx 
 */
GLCL_FUNC_2_ARGS(LightModelx,
                 GLenum, pname, GLfixed, param)

/*
 * glLightModelxv 
 */
GLCL_FUNC_2_ARGS(LightModelxv,
                 GLenum, pname, const GLfixed*, params)

/*
 * glLightf 
 */
GLCM_FUNC_3_ARGS(Lightf,
                 GLenum, light, GLenum, pname, GLfloat, param)

/*
 * glLightfv 
 */
GLCM_FUNC_3_ARGS(Lightfv,
                 GLenum, light, GLenum, pname, const GLfloat*, params)

/*
 * glLightx 
 */
GLCL_FUNC_3_ARGS(Lightx,
                 GLenum, light, GLenum, pname, GLfixed, param)

/*
 * glLightxv 
 */
GLCL_FUNC_3_ARGS(Lightxv,
                 GLenum, light, GLenum, pname, const GLfixed*, params)

/*
 * glLineWidth 
 */
GLCM_FUNC_1_ARGS(LineWidth,
                 GLfloat, width)

/*
 * glLineWidthx 
 */
GLCL_FUNC_1_ARGS(LineWidthx,
                 GLfixed, width)

/*
 * glLoadIdentity 
 */
GLCL_FUNC_0_ARGS(LoadIdentity)
                 
/*
 * glLoadMatrixf 
 */
GLCM_FUNC_1_ARGS(LoadMatrixf,
                 const GLfloat*, m)

/*
 * glLoadMatrixx 
 */
GLCL_FUNC_1_ARGS(LoadMatrixx,
                 const GLfixed*, m)

/*
 * glLogicOp 
 */
GLCL_FUNC_1_ARGS(LogicOp,
                 GLenum, opcode)

/*
 * glMaterialf 
 */
GLCM_FUNC_3_ARGS(Materialf,
                 GLenum, face, GLenum, pname, GLfloat, param)

/*
 * glMaterialfv 
 */
GLCM_FUNC_3_ARGS(Materialfv,
                 GLenum, face, GLenum, pname, const GLfloat*, params)

/*
 * glMaterialx 
 */
GLCL_FUNC_3_ARGS(Materialx,
                 GLenum, face, GLenum, pname, GLfixed, param)

/*
 * glMaterialxv 
 */
GLCL_FUNC_3_ARGS(Materialxv,
                 GLenum, face, GLenum, pname, const GLfixed*, params)

/*
 * glMatrixMode 
 */
GLCL_FUNC_1_ARGS(MatrixMode,
                 GLenum, mode)

/*
 * glMultMatrixf 
 */
GLCM_FUNC_1_ARGS(MultMatrixf,
                 const GLfloat*, m)

/*
 * glMultMatrixx 
 */
GLCL_FUNC_1_ARGS(MultMatrixx,
                 const GLfixed*, m)

/*
 * glMultiTexCoord4f 
 */
GLCM_FUNC_5_ARGS(MultiTexCoord4f,
                 GLenum, target, GLfloat, s, GLfloat, t, GLfloat, r, GLfloat, q)

/*
 * glMultiTexCoord4x 
 */
GLCL_FUNC_5_ARGS(MultiTexCoord4x,
                 GLenum, target, GLfixed, s, GLfixed, t, GLfixed, r, GLfixed, q)

/*
 * glNormal3f 
 */
GLCM_FUNC_3_ARGS(Normal3f,
                 GLfloat, nx, GLfloat, ny, GLfloat, nz)

/*
 * glNormal3x 
 */
GLCL_FUNC_3_ARGS(Normal3x,
                 GLfixed, nx, GLfixed, ny, GLfixed, nz)

/*
 * glNormalPointer 
 */
GLCL_FUNC_3_ARGS(NormalPointer,
                 GLenum, type, GLsizei, stride, const GLvoid*, pointer)

/*
 * glOrthof 
 */
GLCM_FUNC_6_ARGS(Orthof,
                 GLfloat, left, GLfloat, right, GLfloat, bottom,
                 GLfloat, top, GLfloat, zNear, GLfloat, zFar)

/*
 * glOrthox 
 */
GLCL_FUNC_6_ARGS(Orthox,
                 GLfixed, left, GLfixed, right, GLfixed, bottom,
                 GLfixed, top, GLfixed, zNear, GLfixed, zFar)

/*
 * glPixelStorei 
 */
GLCL_FUNC_2_ARGS(PixelStorei,
                 GLenum, pname, GLint, param)

/*
 * glPointSize 
 */
GLCM_FUNC_1_ARGS(PointSize,
                 GLfloat, size)

/*
 * glPointSizex 
 */
GLCL_FUNC_1_ARGS(PointSizex,
                 GLfixed, size)

/*
 * glPolygonOffset 
 */
GLCM_FUNC_2_ARGS(PolygonOffset,
                 GLfloat, factor, GLfloat, units)

/*
 * glPolygonOffsetx 
 */
GLCL_FUNC_2_ARGS(PolygonOffsetx,
                 GLfixed, factor, GLfixed, units)

/*
 * gl
 */
GLCL_FUNC_0_ARGS(PopMatrix)

/*
 * glPushMatrix 
 */
GLCL_FUNC_0_ARGS(PushMatrix)

/*
 * glReadPixels 
 */
GLCL_FUNC_7_ARGS(ReadPixels,
                 GLint, x, GLint, y, GLsizei, width, GLsizei, height,
                 GLenum, format, GLenum, type, GLvoid*, pixels)

/*
 * glRotatef 
 */
GLCM_FUNC_4_ARGS(Rotatef,
                 GLfloat, angle, GLfloat, x, GLfloat, y, GLfloat, z)

/*
 * glRotatex 
 */
GLCL_FUNC_4_ARGS(Rotatex,
                 GLfixed, angle, GLfixed, x, GLfixed, y, GLfixed, z)

/*
 * glSampleCoverage 
 */
GLCM_FUNC_2_ARGS(SampleCoverage,
                 GLclampf, value, GLboolean, invert)

/*
 * glSampleCoveragex 
 */
GLCL_FUNC_2_ARGS(SampleCoveragex,
                 GLclampx, value, GLboolean, invert)

/*
 * glScalef 
 */
GLCM_FUNC_3_ARGS(Scalef,
                 GLfloat, x, GLfloat, y, GLfloat, z)

/*
 * glScalex 
 */
GLCL_FUNC_3_ARGS(Scalex,
                 GLfixed, x, GLfixed, y, GLfixed, z)

/*
 * glScissor 
 */
GLCL_FUNC_4_ARGS(Scissor,
                 GLint, x, GLint, y, GLsizei, width, GLsizei, height)

/*
 * glShadeModel 
 */
GLCL_FUNC_1_ARGS(ShadeModel,
                 GLenum, mode)

/*
 * glStencilFunc 
 */
GLCL_FUNC_3_ARGS(StencilFunc,
                 GLenum, func, GLint, ref, GLuint, mask)

/*
 * glStencilMask 
 */
GLCL_FUNC_1_ARGS(StencilMask,
                 GLuint, mask)

/*
 * glStencilOp 
 */
GLCL_FUNC_3_ARGS(StencilOp,
                 GLenum, fail, GLenum, zfail, GLenum, zpass)

/*
 * glTexCoordPointer 
 */
GLCL_FUNC_4_ARGS(TexCoordPointer,
                 GLint, size, GLenum, type, GLsizei, stride,
                 const GLvoid*, pointer)

/*
 * glTexEnvf 
 */
GLCM_FUNC_3_ARGS(TexEnvf,
                 GLenum, target, GLenum, pname, GLfloat, param)

/*
 * glTexEnvfv 
 */
GLCM_FUNC_3_ARGS(TexEnvfv,
                 GLenum, target, GLenum, pname, const GLfloat*, params)

/*
 * glTexEnvx 
 */
GLCL_FUNC_3_ARGS(TexEnvx,
                 GLenum, target, GLenum, pname, GLfixed, param)

/*
 * glTexEnvxv 
 */
GLCL_FUNC_3_ARGS(TexEnvxv,
                 GLenum, target, GLenum, pname, const GLfixed*, params)

/*
 * glTexImage2D 
 */
GLCL_FUNC_9_ARGS(TexImage2D,
                 GLenum, target, GLint, level, GLint, internalformat,
                 GLsizei, width, GLsizei, height, GLint, border, GLenum, format,
                 GLenum, type, const GLvoid*, pixels)

/*
 * glTexParameterf 
 */
GLCM_FUNC_3_ARGS(TexParameterf,
                 GLenum, target, GLenum, pname, GLfloat, param)

/*
 * glTexParameterx 
 */
GLCL_FUNC_3_ARGS(TexParameterx,
                 GLenum, target, GLenum, pname, GLfixed, param)

/*
 * glTexSubImage2D 
 */
GLCL_FUNC_9_ARGS(TexSubImage2D,
                 GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset,
                 GLsizei, width, GLsizei, height, GLenum, format, GLenum, type,
                 const GLvoid*, pixels)

/*
 * glTranslatef 
 */
GLCM_FUNC_3_ARGS(Translatef,
                 GLfloat, x, GLfloat, y, GLfloat, z)

/*
 * glTranslatex 
 */
GLCL_FUNC_3_ARGS(Translatex,
                 GLfixed, x, GLfixed, y, GLfixed, z)

/*
 * glVertexPointer 
 */
GLCL_FUNC_4_ARGS(VertexPointer,
                 GLint, size, GLenum, type, GLsizei, stride,
                 const GLvoid*, pointer)

/*
 * glViewport 
 */
GLCL_FUNC_4_ARGS(Viewport,
                 GLint, x, GLint, y, GLsizei, width, GLsizei, height)

/*----------------------------------------------------------------------------*
 *                     GL 1.1  FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/

/*
 * glClipPlanef
 */
GL11_FUNC_2_ARGS(ClipPlanef,
                 GLenum, plane, const GLfloat*, equation)

/*
 * glGetClipPlanef
 */
GL11_FUNC_2_ARGS(GetClipPlanef,
                 GLenum, pname, GLfloat*, eqn)

/*
 * glGetFloatv
 */
GL11_FUNC_2_ARGS(GetFloatv,
                 GLenum, pname, GLfloat*, params)

/*
 * glGetLightfv
 */
GL11_FUNC_3_ARGS(GetLightfv,
                 GLenum, light, GLenum, pname, GLfloat*, params)

/*
 * glGetMaterialfv
 */
GL11_FUNC_3_ARGS(GetMaterialfv,
                 GLenum, face, GLenum, pname, GLfloat*, params)

/*
 * glGetTexEnvfv
 */
GL11_FUNC_3_ARGS(GetTexEnvfv,
                 GLenum, env, GLenum, pname, GLfloat*, params)

/*
 * glGetTexParameterfv
 */
GL11_FUNC_3_ARGS(GetTexParameterfv,
                 GLenum, target, GLenum, pname, GLfloat*, params)

/*
 * glPointParameterf
 */
GL11_FUNC_2_ARGS(PointParameterf,
                 GLenum, pname, GLfloat, param)

/*
 * glPointParameterfv
 */
GL11_FUNC_2_ARGS(PointParameterfv,
                 GLenum, pname, const GLfloat*, params)

/*
 * glTexParameterfv
 */
GL11_FUNC_3_ARGS(TexParameterfv,
                 GLenum, target, GLenum, pname, const GLfloat*, params)

/*
 * glBindBuffer
 */
GL11_FUNC_2_ARGS(BindBuffer,
                 GLenum, target, GLuint, buffer)

/*
 * glBufferData
 */
GL11_FUNC_4_ARGS(BufferData,
                 GLenum, target, GLsizeiptr, size, const GLvoid*, data,
                 GLenum, usage)

/*
 * glBufferSubData
 */
GL11_FUNC_4_ARGS(BufferSubData,
                 GLenum, target, GLintptr, offset, GLsizeiptr, size,
                 const GLvoid*, data)

/*
 * glClipPlanex
 */
GL11_FUNC_2_ARGS(ClipPlanex,
                 GLenum, plane, const GLfixed*, equation)

/*
 * glColor4ub
 */
GL11_FUNC_4_ARGS(Color4ub,
                 GLubyte, red, GLubyte, green, GLubyte, blue, GLubyte, alpha)

/*
 * glDeleteBuffers
 */
GL11_FUNC_2_ARGS(DeleteBuffers,
                 GLsizei, n, const GLuint*, buffers)

/*
 * glGetBooleanv
 */
GL11_FUNC_2_ARGS(GetBooleanv,
                 GLenum, pname, GLboolean*, params)

/*
 * glGetBufferParameteriv
 */
GL11_FUNC_3_ARGS(GetBufferParameteriv,
                 GLenum, target, GLenum, pname, GLint*, params)

/*
 * glGetClipPlanex
 */
GL11_FUNC_2_ARGS(GetClipPlanex,
                 GLenum, pname, GLfixed*, eqn)

/*
 * glGenBuffers
 */
GL11_FUNC_2_ARGS(GenBuffers,
                 GLsizei, n, GLuint*, buffers)

/*
 * glGetFixedv
 */
GL11_FUNC_2_ARGS(GetFixedv,
                 GLenum, pname, GLfixed*, params)

/*
 * glGetLightxv
 */
GL11_FUNC_3_ARGS(GetLightxv,
                 GLenum, light, GLenum, pname, GLfixed*, params)

/*
 * glGetMaterialxv
 */
GL11_FUNC_3_ARGS(GetMaterialxv,
                 GLenum, face, GLenum, pname, GLfixed*, params)

/*
 * glGetPointerv
 */
GL11_FUNC_2_ARGS(GetPointerv,
                 GLenum, pname, void**, params)

/*
 * glGetTexEnviv
 */
GL11_FUNC_3_ARGS(GetTexEnviv,
                 GLenum, env, GLenum, pname, GLint*, params)

/*
 * glGetTexEnvxv
 */
GL11_FUNC_3_ARGS(GetTexEnvxv,
                 GLenum, env, GLenum, pname, GLfixed*, params)

/*
 * glGetTexParameteriv
 */
GL11_FUNC_3_ARGS(GetTexParameteriv,
                 GLenum, target, GLenum, pname, GLint*, params)

/*
 * glGetTexParameterxv
 */
GL11_FUNC_3_ARGS(GetTexParameterxv,
                 GLenum, target, GLenum, pname, GLfixed*, params)

/*
 * glIsBuffer
 */
GL_API GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
  GLboolean ret = FALSE;

  if (NULL != GPIGLES11)
  {
    IGLES11_IsBuffer(GPIGLES11, buffer, &ret);
  }
  return ret;
}

/*
 * glIsEnabled
 */
GL_API GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
  GLboolean ret = FALSE;

  if (NULL != GPIGLES11)
  {
    IGLES11_IsEnabled(GPIGLES11, cap, &ret);
  }
  return ret;
}

/*
 * glIsTexture
 */
GL_API GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
  GLboolean ret = FALSE;

  if (NULL != GPIGLES11)
  {
    IGLES11_IsTexture(GPIGLES11, texture, &ret);
  }
  return ret;
}

/*
 * glPointParameterx
 */
GL11_FUNC_2_ARGS(PointParameterx,
                 GLenum, pname, GLfixed, param)

/*
 * glPointParameterxv
 */
GL11_FUNC_2_ARGS(PointParameterxv,
                 GLenum, pname, const GLfixed*, params)

/*
 * glPointSizePointerOES
 */
GL11_FUNC_3_ARGS(PointSizePointerOES,
                 GLenum, type, GLsizei, stride, const GLvoid*, pointer)

/*
 * glTexEnvi
 */
GL11_FUNC_3_ARGS(TexEnvi,
                 GLenum, target, GLenum, pname, GLint, param)

/*
 * glTexEnviv
 */
GL11_FUNC_3_ARGS(TexEnviv,
                GLenum, target, GLenum, pname, const GLint*, params)


/*
 * glTexParameteri
 */
GL11_FUNC_3_ARGS(TexParameteri,
                 GLenum, target, GLenum, pname, GLint, param)

/*
 * glTexParameteriv
 */
GL11_FUNC_3_ARGS(TexParameteriv,
                 GLenum, target, GLenum, pname, const GLint*, params)

/*
 * glTexParameterxv
 */
GL11_FUNC_3_ARGS(TexParameterxv,
                 GLenum, target, GLenum, pname, const GLfixed*, params)

