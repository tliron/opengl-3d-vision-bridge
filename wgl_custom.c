#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "wgl_custom.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>

static void* AppleGLGetProcAddress (const GLubyte *name)
{
  static const struct mach_header* image = NULL;
  NSSymbol symbol;
  char* symbolName;
  if (NULL == image)
  {
    image = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", NSADDIMAGE_OPTION_RETURN_ON_ERROR);
  }
  /* prepend a '_' for the Unix C symbol mangling convention */
  symbolName = malloc(strlen((const char*)name) + 2);
  strcpy(symbolName+1, (const char*)name);
  symbolName[0] = '_';
  symbol = NULL;
  /* if (NSIsSymbolNameDefined(symbolName))
	 symbol = NSLookupAndBindSymbol(symbolName); */
  symbol = image ? NSLookupSymbolInImage(image, symbolName, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR) : NULL;
  free(symbolName);
  return symbol ? NSAddressOfSymbol(symbol) : NULL;
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined (__sun)
#include <dlfcn.h>
#include <stdio.h>

static void* SunGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable: 4055)
#pragma warning(disable: 4054)
#endif

static int TestPointer(const PROC pTest)
{
	ptrdiff_t iTest;
	if(!pTest) return 0;
	iTest = (ptrdiff_t)pTest;
	
	if(iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1) return 0;
	
	return 1;
}

static PROC WinGetProcAddress(const char *name)
{
	HMODULE glMod = NULL;
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if(TestPointer(pFunc))
	{
		return pFunc;
	}
	glMod = GetModuleHandleA("OpenGL32.dll");
	return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}
	
#define IntGetProcAddress(name) WinGetProcAddress(name)
#else
	#if defined(__APPLE__)
		#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
	#else
		#if defined(__sgi) || defined(__sun)
			#define IntGetProcAddress(name) SunGetProcAddress(name)
		#else /* GLX */
		    #include <GL/glx.h>

			#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte*)name)
		#endif
	#endif
#endif

int wgl_ext_NV_present_video = wgl_LOAD_FAILED;
int wgl_ext_NV_video_output = wgl_LOAD_FAILED;
int wgl_ext_NV_gpu_affinity = wgl_LOAD_FAILED;
int wgl_ext_NV_video_capture = wgl_LOAD_FAILED;
int wgl_ext_NV_copy_image = wgl_LOAD_FAILED;
int wgl_ext_NV_multisample_coverage = wgl_LOAD_FAILED;
int wgl_ext_NV_DX_interop = wgl_LOAD_FAILED;
int wgl_ext_NV_DX_interop2 = wgl_LOAD_FAILED;

BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoDeviceNV)(HDC, unsigned int, HVIDEOOUTPUTDEVICENV, const int *) = NULL;
int (CODEGEN_FUNCPTR *_ptrc_wglEnumerateVideoDevicesNV)(HDC, HVIDEOOUTPUTDEVICENV *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryCurrentContextNV)(int, int *) = NULL;

static int Load_NV_present_video()
{
	int numFailed = 0;
	_ptrc_wglBindVideoDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, unsigned int, HVIDEOOUTPUTDEVICENV, const int *))IntGetProcAddress("wglBindVideoDeviceNV");
	if(!_ptrc_wglBindVideoDeviceNV) numFailed++;
	_ptrc_wglEnumerateVideoDevicesNV = (int (CODEGEN_FUNCPTR *)(HDC, HVIDEOOUTPUTDEVICENV *))IntGetProcAddress("wglEnumerateVideoDevicesNV");
	if(!_ptrc_wglEnumerateVideoDevicesNV) numFailed++;
	_ptrc_wglQueryCurrentContextNV = (BOOL (CODEGEN_FUNCPTR *)(int, int *))IntGetProcAddress("wglQueryCurrentContextNV");
	if(!_ptrc_wglQueryCurrentContextNV) numFailed++;
	return numFailed;
}

BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoImageNV)(HPVIDEODEV, HPBUFFERARB, int) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetVideoDeviceNV)(HDC, int, HPVIDEODEV *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetVideoInfoNV)(HPVIDEODEV, unsigned long *, unsigned long *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoDeviceNV)(HPVIDEODEV) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoImageNV)(HPBUFFERARB, int) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglSendPbufferToVideoNV)(HPBUFFERARB, int, unsigned long *, BOOL) = NULL;

static int Load_NV_video_output()
{
	int numFailed = 0;
	_ptrc_wglBindVideoImageNV = (BOOL (CODEGEN_FUNCPTR *)(HPVIDEODEV, HPBUFFERARB, int))IntGetProcAddress("wglBindVideoImageNV");
	if(!_ptrc_wglBindVideoImageNV) numFailed++;
	_ptrc_wglGetVideoDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, int, HPVIDEODEV *))IntGetProcAddress("wglGetVideoDeviceNV");
	if(!_ptrc_wglGetVideoDeviceNV) numFailed++;
	_ptrc_wglGetVideoInfoNV = (BOOL (CODEGEN_FUNCPTR *)(HPVIDEODEV, unsigned long *, unsigned long *))IntGetProcAddress("wglGetVideoInfoNV");
	if(!_ptrc_wglGetVideoInfoNV) numFailed++;
	_ptrc_wglReleaseVideoDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HPVIDEODEV))IntGetProcAddress("wglReleaseVideoDeviceNV");
	if(!_ptrc_wglReleaseVideoDeviceNV) numFailed++;
	_ptrc_wglReleaseVideoImageNV = (BOOL (CODEGEN_FUNCPTR *)(HPBUFFERARB, int))IntGetProcAddress("wglReleaseVideoImageNV");
	if(!_ptrc_wglReleaseVideoImageNV) numFailed++;
	_ptrc_wglSendPbufferToVideoNV = (BOOL (CODEGEN_FUNCPTR *)(HPBUFFERARB, int, unsigned long *, BOOL))IntGetProcAddress("wglSendPbufferToVideoNV");
	if(!_ptrc_wglSendPbufferToVideoNV) numFailed++;
	return numFailed;
}

HDC (CODEGEN_FUNCPTR *_ptrc_wglCreateAffinityDCNV)(const HGPUNV *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDeleteDCNV)(HDC) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpuDevicesNV)(HGPUNV, UINT, PGPU_DEVICE) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpusFromAffinityDCNV)(HDC, UINT, HGPUNV *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpusNV)(UINT, HGPUNV *) = NULL;

static int Load_NV_gpu_affinity()
{
	int numFailed = 0;
	_ptrc_wglCreateAffinityDCNV = (HDC (CODEGEN_FUNCPTR *)(const HGPUNV *))IntGetProcAddress("wglCreateAffinityDCNV");
	if(!_ptrc_wglCreateAffinityDCNV) numFailed++;
	_ptrc_wglDeleteDCNV = (BOOL (CODEGEN_FUNCPTR *)(HDC))IntGetProcAddress("wglDeleteDCNV");
	if(!_ptrc_wglDeleteDCNV) numFailed++;
	_ptrc_wglEnumGpuDevicesNV = (BOOL (CODEGEN_FUNCPTR *)(HGPUNV, UINT, PGPU_DEVICE))IntGetProcAddress("wglEnumGpuDevicesNV");
	if(!_ptrc_wglEnumGpuDevicesNV) numFailed++;
	_ptrc_wglEnumGpusFromAffinityDCNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, UINT, HGPUNV *))IntGetProcAddress("wglEnumGpusFromAffinityDCNV");
	if(!_ptrc_wglEnumGpusFromAffinityDCNV) numFailed++;
	_ptrc_wglEnumGpusNV = (BOOL (CODEGEN_FUNCPTR *)(UINT, HGPUNV *))IntGetProcAddress("wglEnumGpusNV");
	if(!_ptrc_wglEnumGpusNV) numFailed++;
	return numFailed;
}

BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoCaptureDeviceNV)(UINT, HVIDEOINPUTDEVICENV) = NULL;
UINT (CODEGEN_FUNCPTR *_ptrc_wglEnumerateVideoCaptureDevicesNV)(HDC, HVIDEOINPUTDEVICENV *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglLockVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV, int, int *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV) = NULL;

static int Load_NV_video_capture()
{
	int numFailed = 0;
	_ptrc_wglBindVideoCaptureDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(UINT, HVIDEOINPUTDEVICENV))IntGetProcAddress("wglBindVideoCaptureDeviceNV");
	if(!_ptrc_wglBindVideoCaptureDeviceNV) numFailed++;
	_ptrc_wglEnumerateVideoCaptureDevicesNV = (UINT (CODEGEN_FUNCPTR *)(HDC, HVIDEOINPUTDEVICENV *))IntGetProcAddress("wglEnumerateVideoCaptureDevicesNV");
	if(!_ptrc_wglEnumerateVideoCaptureDevicesNV) numFailed++;
	_ptrc_wglLockVideoCaptureDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, HVIDEOINPUTDEVICENV))IntGetProcAddress("wglLockVideoCaptureDeviceNV");
	if(!_ptrc_wglLockVideoCaptureDeviceNV) numFailed++;
	_ptrc_wglQueryVideoCaptureDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, HVIDEOINPUTDEVICENV, int, int *))IntGetProcAddress("wglQueryVideoCaptureDeviceNV");
	if(!_ptrc_wglQueryVideoCaptureDeviceNV) numFailed++;
	_ptrc_wglReleaseVideoCaptureDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HDC, HVIDEOINPUTDEVICENV))IntGetProcAddress("wglReleaseVideoCaptureDeviceNV");
	if(!_ptrc_wglReleaseVideoCaptureDeviceNV) numFailed++;
	return numFailed;
}

BOOL (CODEGEN_FUNCPTR *_ptrc_wglCopyImageSubDataNV)(HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei) = NULL;

static int Load_NV_copy_image()
{
	int numFailed = 0;
	_ptrc_wglCopyImageSubDataNV = (BOOL (CODEGEN_FUNCPTR *)(HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei))IntGetProcAddress("wglCopyImageSubDataNV");
	if(!_ptrc_wglCopyImageSubDataNV) numFailed++;
	return numFailed;
}

BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXCloseDeviceNV)(HANDLE) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXLockObjectsNV)(HANDLE, GLint, HANDLE *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXObjectAccessNV)(HANDLE, GLenum) = NULL;
HANDLE (CODEGEN_FUNCPTR *_ptrc_wglDXOpenDeviceNV)(void *) = NULL;
HANDLE (CODEGEN_FUNCPTR *_ptrc_wglDXRegisterObjectNV)(HANDLE, void *, GLuint, GLenum, GLenum) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXSetResourceShareHandleNV)(void *, HANDLE) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXUnlockObjectsNV)(HANDLE, GLint, HANDLE *) = NULL;
BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXUnregisterObjectNV)(HANDLE, HANDLE) = NULL;

static int Load_NV_DX_interop()
{
	int numFailed = 0;
	_ptrc_wglDXCloseDeviceNV = (BOOL (CODEGEN_FUNCPTR *)(HANDLE))IntGetProcAddress("wglDXCloseDeviceNV");
	if(!_ptrc_wglDXCloseDeviceNV) numFailed++;
	_ptrc_wglDXLockObjectsNV = (BOOL (CODEGEN_FUNCPTR *)(HANDLE, GLint, HANDLE *))IntGetProcAddress("wglDXLockObjectsNV");
	if(!_ptrc_wglDXLockObjectsNV) numFailed++;
	_ptrc_wglDXObjectAccessNV = (BOOL (CODEGEN_FUNCPTR *)(HANDLE, GLenum))IntGetProcAddress("wglDXObjectAccessNV");
	if(!_ptrc_wglDXObjectAccessNV) numFailed++;
	_ptrc_wglDXOpenDeviceNV = (HANDLE (CODEGEN_FUNCPTR *)(void *))IntGetProcAddress("wglDXOpenDeviceNV");
	if(!_ptrc_wglDXOpenDeviceNV) numFailed++;
	_ptrc_wglDXRegisterObjectNV = (HANDLE (CODEGEN_FUNCPTR *)(HANDLE, void *, GLuint, GLenum, GLenum))IntGetProcAddress("wglDXRegisterObjectNV");
	if(!_ptrc_wglDXRegisterObjectNV) numFailed++;
	_ptrc_wglDXSetResourceShareHandleNV = (BOOL (CODEGEN_FUNCPTR *)(void *, HANDLE))IntGetProcAddress("wglDXSetResourceShareHandleNV");
	if(!_ptrc_wglDXSetResourceShareHandleNV) numFailed++;
	_ptrc_wglDXUnlockObjectsNV = (BOOL (CODEGEN_FUNCPTR *)(HANDLE, GLint, HANDLE *))IntGetProcAddress("wglDXUnlockObjectsNV");
	if(!_ptrc_wglDXUnlockObjectsNV) numFailed++;
	_ptrc_wglDXUnregisterObjectNV = (BOOL (CODEGEN_FUNCPTR *)(HANDLE, HANDLE))IntGetProcAddress("wglDXUnregisterObjectNV");
	if(!_ptrc_wglDXUnregisterObjectNV) numFailed++;
	return numFailed;
}


static const char * (CODEGEN_FUNCPTR *_ptrc_wglGetExtensionsStringARB)(HDC) = NULL;

typedef int (*PFN_LOADFUNCPOINTERS)();
typedef struct wgl_StrToExtMap_s
{
	char *extensionName;
	int *extensionVariable;
	PFN_LOADFUNCPOINTERS LoadExtension;
} wgl_StrToExtMap;

static wgl_StrToExtMap ExtensionMap[8] = {
	{"WGL_NV_present_video", &wgl_ext_NV_present_video, Load_NV_present_video},
	{"WGL_NV_video_output", &wgl_ext_NV_video_output, Load_NV_video_output},
	{"WGL_NV_gpu_affinity", &wgl_ext_NV_gpu_affinity, Load_NV_gpu_affinity},
	{"WGL_NV_video_capture", &wgl_ext_NV_video_capture, Load_NV_video_capture},
	{"WGL_NV_copy_image", &wgl_ext_NV_copy_image, Load_NV_copy_image},
	{"WGL_NV_multisample_coverage", &wgl_ext_NV_multisample_coverage, NULL},
	{"WGL_NV_DX_interop", &wgl_ext_NV_DX_interop, Load_NV_DX_interop},
	{"WGL_NV_DX_interop2", &wgl_ext_NV_DX_interop2, NULL},
};

static int g_extensionMapSize = 8;

static wgl_StrToExtMap *FindExtEntry(const char *extensionName)
{
	int loop;
	wgl_StrToExtMap *currLoc = ExtensionMap;
	for(loop = 0; loop < g_extensionMapSize; ++loop, ++currLoc)
	{
		if(strcmp(extensionName, currLoc->extensionName) == 0)
			return currLoc;
	}
	
	return NULL;
}

static void ClearExtensionVars()
{
	wgl_ext_NV_present_video = wgl_LOAD_FAILED;
	wgl_ext_NV_video_output = wgl_LOAD_FAILED;
	wgl_ext_NV_gpu_affinity = wgl_LOAD_FAILED;
	wgl_ext_NV_video_capture = wgl_LOAD_FAILED;
	wgl_ext_NV_copy_image = wgl_LOAD_FAILED;
	wgl_ext_NV_multisample_coverage = wgl_LOAD_FAILED;
	wgl_ext_NV_DX_interop = wgl_LOAD_FAILED;
	wgl_ext_NV_DX_interop2 = wgl_LOAD_FAILED;
}


static void LoadExtByName(const char *extensionName)
{
	wgl_StrToExtMap *entry = NULL;
	entry = FindExtEntry(extensionName);
	if(entry)
	{
		if(entry->LoadExtension)
		{
			int numFailed = entry->LoadExtension();
			if(numFailed == 0)
			{
				*(entry->extensionVariable) = wgl_LOAD_SUCCEEDED;
			}
			else
			{
				*(entry->extensionVariable) = wgl_LOAD_SUCCEEDED + numFailed;
			}
		}
		else
		{
			*(entry->extensionVariable) = wgl_LOAD_SUCCEEDED;
		}
	}
}


static void ProcExtsFromExtString(const char *strExtList)
{
	size_t iExtListLen = strlen(strExtList);
	const char *strExtListEnd = strExtList + iExtListLen;
	const char *strCurrPos = strExtList;
	char strWorkBuff[256];

	while(*strCurrPos)
	{
		/*Get the extension at our position.*/
		int iStrLen = 0;
		const char *strEndStr = strchr(strCurrPos, ' ');
		int iStop = 0;
		if(strEndStr == NULL)
		{
			strEndStr = strExtListEnd;
			iStop = 1;
		}

		iStrLen = (int)((ptrdiff_t)strEndStr - (ptrdiff_t)strCurrPos);

		if(iStrLen > 255)
			return;

		strncpy(strWorkBuff, strCurrPos, iStrLen);
		strWorkBuff[iStrLen] = '\0';

		LoadExtByName(strWorkBuff);

		strCurrPos = strEndStr + 1;
		if(iStop) break;
	}
}

int wgl_LoadFunctions(HDC hdc)
{
	ClearExtensionVars();
	
	_ptrc_wglGetExtensionsStringARB = (const char * (CODEGEN_FUNCPTR *)(HDC))IntGetProcAddress("wglGetExtensionsStringARB");
	if(!_ptrc_wglGetExtensionsStringARB) return wgl_LOAD_FAILED;
	
	ProcExtsFromExtString((const char *)_ptrc_wglGetExtensionsStringARB(hdc));
	return wgl_LOAD_SUCCEEDED;
}

