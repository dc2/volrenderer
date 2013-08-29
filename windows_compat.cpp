#include "windows_compat.h"

#ifdef _WIN32

PFNGLTEXIMAGE3DPROC glTexImage3D;
PFNGLACTIVETEXTUREPROC glActiveTexture;

void initGLExt()
{
    glTexImage3D = (PFNGLTEXIMAGE3DPROC) wglGetProcAddress("glTexImage3D");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC) wglGetProcAddress("glActiveTexture");
}

#endif
