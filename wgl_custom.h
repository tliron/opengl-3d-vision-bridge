#ifndef POINTER_C_GENERATED_HEADER_WINDOWSGL_H
#define POINTER_C_GENERATED_HEADER_WINDOWSGL_H

#ifdef __wglext_h_
#error Attempt to include auto-generated WGL header after wglext.h
#endif

#define __wglext_h_

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

#ifdef CODEGEN_FUNCPTR
#undef CODEGEN_FUNCPTR
#endif /*CODEGEN_FUNCPTR*/
#define CODEGEN_FUNCPTR WINAPI

#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
#define GLvoid void

#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/


#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS


#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/


struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};
DECLARE_HANDLE(HPBUFFERARB);
DECLARE_HANDLE(HPBUFFEREXT);
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
DECLARE_HANDLE(HPVIDEODEV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE *PGPU_DEVICE;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

extern int wgl_ext_NV_present_video;
extern int wgl_ext_NV_video_output;
extern int wgl_ext_NV_gpu_affinity;
extern int wgl_ext_NV_video_capture;
extern int wgl_ext_NV_copy_image;
extern int wgl_ext_NV_multisample_coverage;
extern int wgl_ext_NV_DX_interop;
extern int wgl_ext_NV_DX_interop2;

#define WGL_NUM_VIDEO_SLOTS_NV 0x20F0

#define WGL_BIND_TO_VIDEO_RGBA_NV 0x20C1
#define WGL_BIND_TO_VIDEO_RGB_AND_DEPTH_NV 0x20C2
#define WGL_BIND_TO_VIDEO_RGB_NV 0x20C0
#define WGL_VIDEO_OUT_ALPHA_NV 0x20C4
#define WGL_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define WGL_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define WGL_VIDEO_OUT_COLOR_NV 0x20C3
#define WGL_VIDEO_OUT_DEPTH_NV 0x20C5
#define WGL_VIDEO_OUT_FIELD_1 0x20C9
#define WGL_VIDEO_OUT_FIELD_2 0x20CA
#define WGL_VIDEO_OUT_FRAME 0x20C8
#define WGL_VIDEO_OUT_STACKED_FIELDS_1_2 0x20CB
#define WGL_VIDEO_OUT_STACKED_FIELDS_2_1 0x20CC

#define WGL_ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV 0x20D0
#define WGL_ERROR_MISSING_AFFINITY_MASK_NV 0x20D1

#define WGL_NUM_VIDEO_CAPTURE_SLOTS_NV 0x20CF
#define WGL_UNIQUE_ID_NV 0x20CE

#define WGL_COLOR_SAMPLES_NV 0x20B9
#define WGL_COVERAGE_SAMPLES_NV 0x2042

#define WGL_ACCESS_READ_ONLY_NV 0x00000000
#define WGL_ACCESS_READ_WRITE_NV 0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV 0x00000002

#ifndef WGL_NV_present_video
#define WGL_NV_present_video 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoDeviceNV)(HDC, unsigned int, HVIDEOOUTPUTDEVICENV, const int *);
#define wglBindVideoDeviceNV _ptrc_wglBindVideoDeviceNV
extern int (CODEGEN_FUNCPTR *_ptrc_wglEnumerateVideoDevicesNV)(HDC, HVIDEOOUTPUTDEVICENV *);
#define wglEnumerateVideoDevicesNV _ptrc_wglEnumerateVideoDevicesNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryCurrentContextNV)(int, int *);
#define wglQueryCurrentContextNV _ptrc_wglQueryCurrentContextNV
#endif /*WGL_NV_present_video*/ 

#ifndef WGL_NV_video_output
#define WGL_NV_video_output 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoImageNV)(HPVIDEODEV, HPBUFFERARB, int);
#define wglBindVideoImageNV _ptrc_wglBindVideoImageNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetVideoDeviceNV)(HDC, int, HPVIDEODEV *);
#define wglGetVideoDeviceNV _ptrc_wglGetVideoDeviceNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglGetVideoInfoNV)(HPVIDEODEV, unsigned long *, unsigned long *);
#define wglGetVideoInfoNV _ptrc_wglGetVideoInfoNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoDeviceNV)(HPVIDEODEV);
#define wglReleaseVideoDeviceNV _ptrc_wglReleaseVideoDeviceNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoImageNV)(HPBUFFERARB, int);
#define wglReleaseVideoImageNV _ptrc_wglReleaseVideoImageNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglSendPbufferToVideoNV)(HPBUFFERARB, int, unsigned long *, BOOL);
#define wglSendPbufferToVideoNV _ptrc_wglSendPbufferToVideoNV
#endif /*WGL_NV_video_output*/ 

#ifndef WGL_NV_gpu_affinity
#define WGL_NV_gpu_affinity 1
extern HDC (CODEGEN_FUNCPTR *_ptrc_wglCreateAffinityDCNV)(const HGPUNV *);
#define wglCreateAffinityDCNV _ptrc_wglCreateAffinityDCNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDeleteDCNV)(HDC);
#define wglDeleteDCNV _ptrc_wglDeleteDCNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpuDevicesNV)(HGPUNV, UINT, PGPU_DEVICE);
#define wglEnumGpuDevicesNV _ptrc_wglEnumGpuDevicesNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpusFromAffinityDCNV)(HDC, UINT, HGPUNV *);
#define wglEnumGpusFromAffinityDCNV _ptrc_wglEnumGpusFromAffinityDCNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglEnumGpusNV)(UINT, HGPUNV *);
#define wglEnumGpusNV _ptrc_wglEnumGpusNV
#endif /*WGL_NV_gpu_affinity*/ 

#ifndef WGL_NV_video_capture
#define WGL_NV_video_capture 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglBindVideoCaptureDeviceNV)(UINT, HVIDEOINPUTDEVICENV);
#define wglBindVideoCaptureDeviceNV _ptrc_wglBindVideoCaptureDeviceNV
extern UINT (CODEGEN_FUNCPTR *_ptrc_wglEnumerateVideoCaptureDevicesNV)(HDC, HVIDEOINPUTDEVICENV *);
#define wglEnumerateVideoCaptureDevicesNV _ptrc_wglEnumerateVideoCaptureDevicesNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglLockVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV);
#define wglLockVideoCaptureDeviceNV _ptrc_wglLockVideoCaptureDeviceNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglQueryVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV, int, int *);
#define wglQueryVideoCaptureDeviceNV _ptrc_wglQueryVideoCaptureDeviceNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglReleaseVideoCaptureDeviceNV)(HDC, HVIDEOINPUTDEVICENV);
#define wglReleaseVideoCaptureDeviceNV _ptrc_wglReleaseVideoCaptureDeviceNV
#endif /*WGL_NV_video_capture*/ 

#ifndef WGL_NV_copy_image
#define WGL_NV_copy_image 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglCopyImageSubDataNV)(HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, HGLRC, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei);
#define wglCopyImageSubDataNV _ptrc_wglCopyImageSubDataNV
#endif /*WGL_NV_copy_image*/ 


#ifndef WGL_NV_DX_interop
#define WGL_NV_DX_interop 1
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXCloseDeviceNV)(HANDLE);
#define wglDXCloseDeviceNV _ptrc_wglDXCloseDeviceNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXLockObjectsNV)(HANDLE, GLint, HANDLE *);
#define wglDXLockObjectsNV _ptrc_wglDXLockObjectsNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXObjectAccessNV)(HANDLE, GLenum);
#define wglDXObjectAccessNV _ptrc_wglDXObjectAccessNV
extern HANDLE (CODEGEN_FUNCPTR *_ptrc_wglDXOpenDeviceNV)(void *);
#define wglDXOpenDeviceNV _ptrc_wglDXOpenDeviceNV
extern HANDLE (CODEGEN_FUNCPTR *_ptrc_wglDXRegisterObjectNV)(HANDLE, void *, GLuint, GLenum, GLenum);
#define wglDXRegisterObjectNV _ptrc_wglDXRegisterObjectNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXSetResourceShareHandleNV)(void *, HANDLE);
#define wglDXSetResourceShareHandleNV _ptrc_wglDXSetResourceShareHandleNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXUnlockObjectsNV)(HANDLE, GLint, HANDLE *);
#define wglDXUnlockObjectsNV _ptrc_wglDXUnlockObjectsNV
extern BOOL (CODEGEN_FUNCPTR *_ptrc_wglDXUnregisterObjectNV)(HANDLE, HANDLE);
#define wglDXUnregisterObjectNV _ptrc_wglDXUnregisterObjectNV
#endif /*WGL_NV_DX_interop*/ 


enum wgl_LoadStatus
{
	wgl_LOAD_FAILED = 0,
	wgl_LOAD_SUCCEEDED = 1,
};

int wgl_LoadFunctions(HDC hdc);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //POINTER_C_GENERATED_HEADER_WINDOWSGL_H
