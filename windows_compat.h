#ifndef _USREXT_H
#define _USREXT_H

#ifdef _WIN32

#include<windows.h>

#include <stdio.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "windows_compat_glext.h"

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLTEXIMAGE3DPROC glTexImage3D;

void initGLExt();

#endif
#endif
