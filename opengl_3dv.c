#include <stdio.h>

#if defined(_WIN32)

extern "C" {
#include "opengl_3dv.h"
}

#include "initguid.h" // this is necessary in order to use IID_IDirect3D9, see: http://support.microsoft.com/kb/130869
#include "d3d9.h"
#include "nvapi.h"
#include "wgl_custom.h"

typedef HRESULT (WINAPI *DIRECT3DCREATE9EXFUNCTION)(UINT SDKVersion, IDirect3D9Ex**);

#define D3D_EXCEPTION(M) printf("Error: %s\n", M); fflush(stdout); return;
#define D3D_EXCEPTIONF(F, ARGS...) printf("Error: " F " %s\n", ARGS); fflush(stdout); return;

/*
 * NVIDIA stereo
 * 
 * (From nvstereo.h in NVIDIA Direct3D SDK)
 */

typedef struct _Nv_Stereo_Image_Header {
	unsigned int dwSignature;
	unsigned int dwWidth;
	unsigned int dwHeight;
	unsigned int dwBPP;
	unsigned int dwFlags;
} NVSTEREOIMAGEHEADER, *LPNVSTEREOIMAGEHEADER;

// dwSignature value
#define NVSTEREO_IMAGE_SIGNATURE 0x4433564e

// dwFlags values
#define SIH_SWAP_EYES 0x00000001
#define SIH_SCALE_TO_FIT 0x00000002
#define SIH_SCALE_TO_FIT2 0x00000004 // Stretch to fullscreen? (undocumented?!)

/*
 * Constants
 */

#define DEPTH_STENCIL_FORMAT D3DFMT_D24S8

/*
 * Complex stuff!
 * 
 * We need to:
 * 
 * 1. Get the window handle from SDL.
 * 2. Link the WGL API via the window's device context.
 * 3. Make sure we support the WGL NV_DX_interop extension.
 * 4. Enable the Direct3D API.
 * 5. Create a Direct3D device for our window.
 * 5. Enable OpenGL interop for the device.
 * 6. Create the Direct3D textures and buffers.
 * 7. Create OpenGL textures.
 * 8. Bind them to the Direct3D buffers.
 * 9. Create a regular OpenGL frame buffer object using the textures.
 *
 * See my own post here: http://www.mtbs3d.com/phpbb/viewtopic.php?f=105&t=16310&p=97553
 *
 * Other useful information:
 *  http://developer.download.nvidia.com/opengl/specs/WGL_NV_DX_interop.txt
 *  http://sourceforge.net/projects/mingw-w64/forums/forum/723797/topic/5293852
 *  http://www.panda3d.org/forums/viewtopic.php?t=11583
 */
void GLD3DBuffers_create(GLD3DBuffers *gl_d3d_buffers, void *window_handle, bool vsync, bool stereo) {
	printf("Creating OpenGL/Direct3D bridge...\n");

	ZeroMemory(gl_d3d_buffers, sizeof(GLD3DBuffers));
	gl_d3d_buffers->stereo = stereo;

	HWND hWnd = (HWND) window_handle;
	HRESULT result;

	// Fullscreen?
	WINDOWINFO windowInfo;
	result = GetWindowInfo(hWnd, &windowInfo);
	if (FAILED(result)) {
		D3D_EXCEPTION("Could not get window info");
	}
	BOOL fullscreen = (windowInfo.dwExStyle & WS_EX_TOPMOST) != 0;
	if (fullscreen) {
		printf(" We are in fullscreen mode: %u\n", (int) hWnd);

		// Doesn't work well :/
		fullscreen = FALSE;
	}
	else {
		printf(" We are in windowed mode\n");
	}

	// Window size
	RECT windowRect;
	result = GetClientRect(hWnd, &windowRect);
	if (FAILED(result)) {
		D3D_EXCEPTION("Could not get window size");
	}
	unsigned int width = windowRect.right - windowRect.left;
	unsigned int height = windowRect.bottom - windowRect.top;
	gl_d3d_buffers->width = width;
	gl_d3d_buffers->height = height;
	printf(" Window size: %u, %u\n", width, height);

	/*if (fullscreen) {
		DestroyWindow(hWnd);
		hWnd = GetDesktopWindow();

		//HDC hDc = GetDC(hWnd);
		//HGLRC hGl = wglCreateContext(hDc);
		//wglMakeCurrent(hDc, hGl);
		printf(" Destroyed SDL window\n");
	}
	*/

	// OpenGL context (needed fullscreen only)
	/*if (!sdl) {
		HGLRC glRc = wglCreateContext(dc);
		if (!glRc) {
			printf(" Failed to create OpenGL context for fullscreen\n");
			return FALSE;
		}
		if (!wglMakeCurrent(dc, glRc)) {
			printf(" Failed to make current OpenGL context for fullscreen\n");
			return FALSE;
		}
		hWnd = NULL;
		printf(" Created OpenGL context for fullscreen\n");
	}*/

	// WGL
	//
	// (Note: the window must have an OpenGL context for this to work;
	// SDL took care of that for us; also see wglGetCurrentDC)
	HDC dc = GetDC(hWnd);
	if (wgl_LoadFunctions(dc) == wgl_LOAD_FAILED) {
		D3D_EXCEPTION("Failed to link to WGL API");
	}
	printf(" Linked to WGL API\n");

	/*PIXELFORMATDESCRIPTOR px;
	int formats = DescribePixelFormat(dc, 1, sizeof(px), &px);
	while (formats) {
		DescribePixelFormat(dc, formats, sizeof(px), &px);
		if (px.dwFlags & PDF_STEREO) {
			printf(" Found supported stereo pixel format\n");
		}
		formats--;
	}*/

	if (wgl_ext_NV_DX_interop == wgl_LOAD_FAILED) {
		D3D_EXCEPTION("WGL NV_DX_interop extension not supported on this platform");
	}
	printf(" WGL NV_DX_interop supported on this platform\n");

	// Are we a WDDM OS?
	OSVERSIONINFO osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	if (!GetVersionEx(&osVersionInfo)) {
		D3D_EXCEPTION("Could not get Windows version");
	}
	BOOL wddm = osVersionInfo.dwMajorVersion == 6;
	if (wddm) {
		printf(" This operating system uses WDDM (it's Windows Vista or later)\n");
	}
	else {
		printf(" This operating system does not use WDDM (it's prior to Windows Vista)\n");
	}

	// Direct3D API
	//
	// (Note: Direct3D/OpenGL interop *requires* the use of 9Ex for WDDM-enabled operating systems)
	IDirect3D9 *d3d;
	if (wddm) {
		// We are dynamically linking to the Direct3DCreate9Ex function, because we
		// don't want to add a run-time dependency for it in our executable, which
		// would make it not run in Windows XP.
		//
		// See: http://msdn.microsoft.com/en-us/library/cc656710.aspx

		HMODULE d3dLibrary = LoadLibrary("d3d9.dll");
		if (!d3dLibrary) {
			D3D_EXCEPTION("Failed to link to d3d9.dll");
		}
		gl_d3d_buffers->d3dLibrary = d3dLibrary;

		DIRECT3DCREATE9EXFUNCTION pfnDirect3DCreate9Ex = (DIRECT3DCREATE9EXFUNCTION) GetProcAddress(d3dLibrary, "Direct3DCreate9Ex");
		if (!pfnDirect3DCreate9Ex) {
			D3D_EXCEPTION("Failed to link to Direct3D 9Ex (WDDM)");
		}

		IDirect3D9 *d3dEx;
		result = (*pfnDirect3DCreate9Ex)(D3D_SDK_VERSION, (IDirect3D9Ex**) &d3dEx);
		if (result != D3D_OK) {
			D3D_EXCEPTION("Failed to activate Direct3D 9Ex (WDDM)");
		}

		if (d3dEx->lpVtbl->QueryInterface(d3dEx, &IID_IDirect3D9, (LPVOID*) &d3d) != S_OK) {
			D3D_EXCEPTION("Failed to cast Direct3D 9Ex to Direct3D 9");
		}

		printf(" Activated Direct3D 9Ex (WDDM)\n");
	}
	else {
		d3d = Direct3DCreate9(D3D_SDK_VERSION);
		if (!d3d) {
			D3D_EXCEPTION("Failed to activate Direct3D 9 (no WDDM)");
		}
		printf(" Activated Direct3D 9 (no WDDM)\n");
	}

	// Find display mode format
	//
	// (Our buffers will be the same to avoid conversion overhead)
	D3DDISPLAYMODE d3dDisplayMode;
	result = d3d->lpVtbl->GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &d3dDisplayMode);
	if (result != D3D_OK) {
		D3D_EXCEPTION("Failed to retrieve adapter display mode");
	}
	D3DFORMAT d3dBufferFormat = d3dDisplayMode.Format;
	printf(" Retrieved adapter display mode, format: %u\n", d3dBufferFormat);

	// Make sure devices can support the required formats
	result = d3d->lpVtbl->CheckDeviceFormat(d3d,
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dBufferFormat,
		D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DEPTH_STENCIL_FORMAT);
	if (result != D3D_OK) {
		D3D_EXCEPTION("Our required formats are not supported");
	}
	printf(" Our required formats are supported\n");

	// Get the device capabilities
	D3DCAPS9 d3dCaps;
	result = d3d->lpVtbl->GetDeviceCaps(d3d,
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps);
	if (result != D3D_OK) {
		D3D_EXCEPTION("Failed to retrieve device capabilities");
	}
	printf(" Retrieved device capabilities\n");

	// Verify that we can do hardware vertex processing
	if (d3dCaps.VertexProcessingCaps == 0) {
		D3D_EXCEPTION("Hardware vertex processing is not supported");
	}
	printf(" Hardware vertex processing is supported\n");

	// Register stereo (must happen *before* device creation)
	if (gl_d3d_buffers->stereo) {
		if (NvAPI_Initialize() != NVAPI_OK) {
			D3D_EXCEPTION("Failed to initialize NV API");
		}
		printf(" Initialized NV API:\n");

		if (NvAPI_Stereo_CreateConfigurationProfileRegistryKey(NVAPI_STEREO_DX9_REGISTRY_PROFILE) != NVAPI_OK) {
			D3D_EXCEPTION("Failed to register stereo profile");
		}
		printf("  Registered stereo profile\n");

		if (NvAPI_Stereo_Enable() != NVAPI_OK) {
			D3D_EXCEPTION("Could not enable NV stereo");
		}
		printf("  Enabled stereo\n");
	}

	D3DPRESENT_PARAMETERS d3dPresent;
	ZeroMemory(&d3dPresent, sizeof(d3dPresent));
	d3dPresent.BackBufferCount = 1;
	d3dPresent.BackBufferFormat = d3dBufferFormat;
	d3dPresent.BackBufferWidth = width;
	d3dPresent.BackBufferHeight = height;
	d3dPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresent.MultiSampleQuality = 0;
	d3dPresent.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dPresent.EnableAutoDepthStencil = TRUE;
	d3dPresent.AutoDepthStencilFormat = DEPTH_STENCIL_FORMAT;
	d3dPresent.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	d3dPresent.PresentationInterval = vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	if (fullscreen) {
		//d3dPresent.FullScreen_RefreshRateInHz = 60;
		d3dPresent.Windowed = FALSE;
	}
	else {
		d3dPresent.Windowed = TRUE;
		d3dPresent.hDeviceWindow = hWnd; // can be NULL in windowed mode, in which case it will use the arg in CreateDevice
	}

	// Create Direct3D device
	//
	// Stereo normally only works in fullscreen mode... except for a few special applications
	// recognized by the driver. So, if we rename our exectuable to "googleearth.exe", it will
	// do stereo in a window!
	//
	// (Note: Direct3D/OpenGL interop *requires* D3DCREATE_MULTITHREADED)
	IDirect3DDevice9 *d3dDevice;
	result = d3d->lpVtbl->CreateDevice(d3d,
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd, // used for ALT+TAB; must be top-level window in fullscreen mode; can be NULL in windowed mode
		//D3DCREATE_SOFTWARE_VERTEXPROCESSING,// | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE | D3DCREATE_PUREDEVICE,
		&d3dPresent,
		&d3dDevice);
	if (result != D3D_OK) {
		switch (result) {
		case D3DERR_DEVICELOST:
			D3D_EXCEPTION("Failed to create device: device lost");
		case D3DERR_INVALIDCALL:
			D3D_EXCEPTION("Failed to create device: invalid call");
		case D3DERR_NOTAVAILABLE:
			D3D_EXCEPTION("Failed to create device: not available");
		case D3DERR_OUTOFVIDEOMEMORY:
			D3D_EXCEPTION("Failed to create device: out of video memory");
		default:
			D3D_EXCEPTIONF("Failed to create device: unknown error %u", (int) result);
		}
	}
	gl_d3d_buffers->d3dDevice = d3dDevice;
	printf(" Created device\n");

	// Enable Direct3D/OpenGL interop on device
	HANDLE d3dDeviceInterop = wglDXOpenDeviceNV(d3dDevice);
	if (!d3dDeviceInterop) {
		DWORD e = GetLastError();
		GLD3DBuffers_destroy(gl_d3d_buffers);
		switch (e) {
		case ERROR_OPEN_FAILED:
			D3D_EXCEPTION("Failed to enable OpenGL interop on device");
		case ERROR_NOT_SUPPORTED:
			D3D_EXCEPTION("Failed to enable OpenGL interop on device: device not supported");
		default:
			D3D_EXCEPTIONF("Failed to enable OpenGL interop on device: unknown error %u", (int) e);
		}
	}
	gl_d3d_buffers->d3dDeviceInterop = d3dDeviceInterop;
	printf(" Enabled OpenGL interop on device\n");

	// Enable stereo on device (WDDM only)
	if (gl_d3d_buffers->stereo && wddm) {
		StereoHandle nvStereo;
		if (NvAPI_Stereo_CreateHandleFromIUnknown((IUnknown *) d3dDevice, &nvStereo) != NVAPI_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to create NV stereo handle");
		}
		gl_d3d_buffers->nvStereo = nvStereo;
		printf(" Created NV stereo handle on device:\n");

		if (NvAPI_Stereo_Activate(nvStereo) != NVAPI_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to activate stereo");
		}
		printf("  Activated stereo\n");

		float separation, convergence, eyeSeparation;
		NV_FRUSTUM_ADJUST_MODE frustum;

		NvAPI_Stereo_GetSeparation(nvStereo, &separation);
		NvAPI_Stereo_GetConvergence(nvStereo, &convergence);
		NvAPI_Stereo_GetFrustumAdjustMode(nvStereo, &frustum);
		NvAPI_Stereo_GetEyeSeparation(nvStereo, &eyeSeparation);

		printf("  Separation: %f\n", separation);
		printf("  Convergence: %f\n", convergence);
		switch (frustum) {
		case NVAPI_NO_FRUSTUM_ADJUST:
			printf("  Frustum: no adjust\n");
			break;
		case NVAPI_FRUSTUM_STRETCH:
			printf("  Frustum: stretch\n");
			break;
		case NVAPI_FRUSTUM_CLEAR_EDGES:
			printf("  Frustum: clear edges\n");
			break;
		default:
			printf("  Frustum: unknown\n");
		}
		printf("  Eye separation: %f\n", eyeSeparation);
	}

	// Get device back buffer
	IDirect3DSurface9 *d3dBackBuffer;
	result = d3dDevice->lpVtbl->GetBackBuffer(d3dDevice,
		0, 0, D3DBACKBUFFER_TYPE_MONO, &d3dBackBuffer);
	if (result != D3D_OK) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to retrieve device back buffer");
	}
	gl_d3d_buffers->d3dBackBuffer = d3dBackBuffer;
	printf(" Retrieved device back buffer\n");

	// Direct3D textures
	//
	// (Note: we *must* use textures for stereo to show both eyes;
	// a CreateRenderTarget will only work for one eye)

	// Left texture
	IDirect3DTexture9 *d3dLeftColorTexture;
	result = d3dDevice->lpVtbl->CreateTexture(d3dDevice,
		width, height,
		0, // "levels" (mipmaps)
		0, // usage
		d3dBufferFormat,
		D3DPOOL_DEFAULT,
		&d3dLeftColorTexture,
		NULL);
	if (result != D3D_OK) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to create color texture (left)");
	}
	gl_d3d_buffers->d3dLeftColorTexture = d3dLeftColorTexture;
	printf(" Created color texture (left)\n");

	IDirect3DTexture9 *d3dRightColorTexture;
	if (gl_d3d_buffers->stereo) {
		// Right texture
		result = d3dDevice->lpVtbl->CreateTexture(d3dDevice,
			width, height,
			0, // "levels" (mipmaps)
			0, // usage
			d3dBufferFormat,
			D3DPOOL_DEFAULT,
			&d3dRightColorTexture,
			NULL);
		if (result != D3D_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to create color texture (right)");
		}
		gl_d3d_buffers->d3dRightColorTexture = d3dRightColorTexture;
		printf(" Created color texture (right)\n");
	}

	// Get Direct3D buffers from textures

	IDirect3DSurface9 *d3dLeftColorBuffer;
	result = d3dLeftColorTexture->lpVtbl->GetSurfaceLevel(d3dLeftColorTexture, 0, &d3dLeftColorBuffer);
	if (FAILED(result)) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to retrieve color buffer from color texture (left)");
	}
	gl_d3d_buffers->d3dLeftColorBuffer = d3dLeftColorBuffer;
	printf(" Retrieved color buffer from color texture (left)\n");

	if (gl_d3d_buffers->stereo) {
		IDirect3DSurface9 *d3dRightColorBuffer;
		result = d3dRightColorTexture->lpVtbl->GetSurfaceLevel(d3dRightColorTexture, 0, &d3dRightColorBuffer);
		if (FAILED(result)) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to retrieve color buffer from color texture (right)");
		}
		gl_d3d_buffers->d3dRightColorBuffer = d3dRightColorBuffer;
		printf(" Retrieved color buffer from color texture (right)\n");
	}

	// Left render target surface
	/*IDirect3DSurface9 *d3dLeftColorBuffer;
	result = d3dDevice->lpVtbl->CreateRenderTarget(d3dDevice,
		width, height,
		d3dBufferFormat,
		D3DMULTISAMPLE_NONE, 0,
		FALSE, // must not be lockable for interop
		&d3dLeftColorBuffer,
		NULL);*/
	/*result = d3dDevice->lpVtbl->CreateOffscreenPlainSurface(d3dDevice,
		width, height,
		d3dBufferFormat,
		D3DPOOL_DEFAULT,
		&d3dLeftColorBuffer,
		NULL);*/
	/*if (result != D3D_OK) {
		printf(" Failed to create render target surface (left)\n");
		GLD3DBuffers_destroy(gl_d3d_buffers);
		return FALSE;
	}
	gl_d3d_buffers->d3dLeftColorBuffer = d3dLeftColorBuffer;
	printf(" Created render target surface (left)\n");*/

	if (gl_d3d_buffers->stereo) {
		// Stereo render target surface
		// (Note: must be at least 20 bytes wide to handle stereo header row!)
		IDirect3DSurface9 *d3dStereoColorBuffer;
		result = d3dDevice->lpVtbl->CreateRenderTarget(d3dDevice,
			width * 2, height + 1, // the extra row is for the stereo header
			d3dBufferFormat,
			D3DMULTISAMPLE_NONE, 0,
			TRUE, // must be lockable!
			&d3dStereoColorBuffer,
			NULL);
		/*result = d3dDevice->lpVtbl->CreateOffscreenPlainSurface(d3dDevice,
			width * 2, height + 1, // the extra row is for the stereo header
			d3dBufferFormat,
			D3DPOOL_DEFAULT,
			&d3dStereoColorBuffer,
			NULL);*/
		if (result != D3D_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to create render target surface (stereo)");
		}
		gl_d3d_buffers->d3dStereoColorBuffer = d3dStereoColorBuffer;
		printf(" Created render target surface (stereo)\n");

		D3DLOCKED_RECT d3dLockedRect;
		result = d3dStereoColorBuffer->lpVtbl->LockRect(d3dStereoColorBuffer, &d3dLockedRect, NULL, 0);
		if (result != D3D_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to lock surface rect (stereo)");
		}

		// Insert stereo header into last row (the "+1") of stereo render target surface
		LPNVSTEREOIMAGEHEADER nvStereoHeader = (LPNVSTEREOIMAGEHEADER) (((unsigned char *) d3dLockedRect.pBits) + (d3dLockedRect.Pitch * gl_d3d_buffers->height));
		nvStereoHeader->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		nvStereoHeader->dwFlags = 0;
		//nvStereoHeader->dwFlags = SIH_SWAP_EYES;
		//nvStereoHeader->dwFlags = SIH_SWAP_EYES | SIH_SCALE_TO_FIT | SIH_SCALE_TO_FIT2;

		// Note: the following all seem to be ignored
		nvStereoHeader->dwWidth = gl_d3d_buffers->width * 2;
		nvStereoHeader->dwHeight = gl_d3d_buffers->height;
		nvStereoHeader->dwBPP = 32; // hmm, get this from the format? my netbook declared D3DFMT_X8R8G8B8, which is indeed 32 bits

		result = d3dStereoColorBuffer->lpVtbl->UnlockRect(d3dStereoColorBuffer);
		if (result != D3D_OK) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to unlock surface rect (stereo)");
		}
		printf(" Inserted stereo header into render target surface (stereo)\n");
	}

	// Depth/stencil surface
	IDirect3DSurface9 *d3dDepthBuffer;
	result = d3dDevice->lpVtbl->CreateDepthStencilSurface(d3dDevice,
		width, height,
		DEPTH_STENCIL_FORMAT,
		D3DMULTISAMPLE_NONE, 0,
		FALSE, // Z-buffer discarding
		&d3dDepthBuffer,
		NULL);
	if (result != D3D_OK) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to create depth/stencil surface)");
	}
	gl_d3d_buffers->d3dDepthBuffer = d3dDepthBuffer;
	printf(" Created depth/stencil surface\n");

	// OpenGL textures

	glGenTextures(1, &gl_d3d_buffers->texture_left);
	GLenum error = glGetError();
	if (error) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to create OpenGL color texture (left)");
	}
	printf(" Created OpenGL color texture (left)\n");

	if (gl_d3d_buffers->stereo) {
		glGenTextures(1, &gl_d3d_buffers->texture_right);
		GLenum error = glGetError();
		if (error) {
			GLD3DBuffers_destroy(gl_d3d_buffers);
			D3D_EXCEPTION("Failed to create OpenGL color texture (right)");
		}
		printf(" Created OpenGL color texture (right)\n");
	}

	// OpenGL render buffer (one should be enough for stereo?)
	glGenTextures(1, &gl_d3d_buffers->render_buffer);
	error = glGetError();
	if (error) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		D3D_EXCEPTION("Failed to create OpenGL depth/stencil texture");
	}
	printf(" Created OpenGL depth/stencil texture\n");
	/*glGenRenderbuffersEXT(1, &gl_d3d_buffers->render_buffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, gl_d3d_buffers->render_buffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	error = glGetError();
	if (error) {
		printf(" Failed to create OpenGL render buffer\n");
		GLD3DBuffers_destroy(gl_d3d_buffers);
		return FALSE;
	}
	printf(" Created OpenGL render buffer\n");*/

	// Bind OpenGL textures
	//
	// CreateOffscreenPlainSurface did not work for us.
	//
	// (Note: if we are using a texture, we *must* use the IDirect3DTexture9 instance, not a surface)

	HANDLE d3dLeftColorInterop = wglDXRegisterObjectNV(d3dDeviceInterop, d3dLeftColorTexture, gl_d3d_buffers->texture_left, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
	if (!d3dLeftColorInterop) {
		DWORD e = GetLastError();
		GLD3DBuffers_destroy(gl_d3d_buffers);
		switch (e) {
		case ERROR_OPEN_FAILED:
			D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (left)");
		case ERROR_INVALID_DATA:
			D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (left): invalid data");
		case ERROR_INVALID_HANDLE:
			D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (left): invalid handle");
		default:
			D3D_EXCEPTIONF("Failed to bind render target surface to OpenGL texture (left): unknown error %u", (int) e);
		}
	}
	gl_d3d_buffers->d3dLeftColorInterop = d3dLeftColorInterop;
	printf(" Bound render target surface to OpenGL texture (left)\n");

	HANDLE d3dRightColorInterop;
	if (gl_d3d_buffers->stereo) {
		d3dRightColorInterop = wglDXRegisterObjectNV(d3dDeviceInterop, d3dRightColorTexture, gl_d3d_buffers->texture_right, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
		if (!d3dRightColorInterop) {
			DWORD e = GetLastError();
			GLD3DBuffers_destroy(gl_d3d_buffers);
			switch (e) {
			case ERROR_OPEN_FAILED:
				D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (right)");
			case ERROR_INVALID_DATA:
				D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (right): invalid data");
			case ERROR_INVALID_HANDLE:
				D3D_EXCEPTION("Failed to bind render target surface to OpenGL texture (right): invalid handle");
			default:
				D3D_EXCEPTIONF("Failed to bind render target surface to OpenGL texture (right): unknown error %u", (int) e);
			}
		}
		gl_d3d_buffers->d3dRightColorInterop = d3dRightColorInterop;
		printf(" Bound render target surface to OpenGL texture (right)\n");
	}

	// Bind OpenGL render buffer
	//HANDLE d3dDepthInterop = wglDXRegisterObjectNV(d3dDeviceInterop, d3dDepthBuffer, gl_d3d_buffers->render_buffer, GL_RENDERBUFFER_EXT, WGL_ACCESS_READ_WRITE_NV);
	HANDLE d3dDepthInterop = wglDXRegisterObjectNV(d3dDeviceInterop, d3dDepthBuffer, gl_d3d_buffers->render_buffer, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
	if (!d3dDepthInterop) {
		DWORD e = GetLastError();
		GLD3DBuffers_destroy(gl_d3d_buffers);
		switch (e) {
		case ERROR_OPEN_FAILED:
			D3D_EXCEPTION("Failed to bind depth/stencil surface to OpenGL render buffer");
		case ERROR_INVALID_DATA:
			D3D_EXCEPTION("Failed to bind depth/stencil surface to OpenGL render buffer: invalid data");
		case ERROR_INVALID_HANDLE:
			D3D_EXCEPTION("Failed to bind depth/stencil surface to OpenGL render buffer: invalid handle");
		default:
			D3D_EXCEPTIONF("Failed to bind depth/stencil surface to OpenGL render buffer: unknown error %u", (int) e);
		}
	}
	gl_d3d_buffers->d3dDepthInterop = d3dDepthInterop;
	printf(" Bound depth/stencil surface to OpenGL render buffer\n");

	// Note: if we don't lock the interops first, glCheckFramebufferStatusEXT will fail with an unknown error
	wglDXLockObjectsNV(d3dDeviceInterop, 1, &d3dLeftColorInterop);
	if (gl_d3d_buffers->stereo) {
		wglDXLockObjectsNV(d3dDeviceInterop, 1, &d3dRightColorInterop);
	}
	wglDXLockObjectsNV(d3dDeviceInterop, 1, &d3dDepthInterop);

	// OpenGL frame buffer objects
	//
	// Note: why not glFramebufferRenderbufferEXT? Because it's apparently buggy in stereo :/
	// glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, gl_d3d_buffers->render_buffer);

	glGenFramebuffersEXT(1, &gl_d3d_buffers->fbo_left);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gl_d3d_buffers->fbo_left);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, gl_d3d_buffers->texture_left, 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, gl_d3d_buffers->render_buffer, 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, gl_d3d_buffers->render_buffer, 0);
	GLenum status_left = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	GLenum status_right;
	if (gl_d3d_buffers->stereo) {
		glGenFramebuffersEXT(1, &gl_d3d_buffers->fbo_right);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gl_d3d_buffers->fbo_right);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, gl_d3d_buffers->texture_right, 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, gl_d3d_buffers->render_buffer, 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, gl_d3d_buffers->render_buffer, 0);
		status_right = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}

	wglDXUnlockObjectsNV(d3dDeviceInterop, 1, &d3dLeftColorInterop);
	if (gl_d3d_buffers->stereo) {
		wglDXUnlockObjectsNV(d3dDeviceInterop, 1, &d3dRightColorInterop);
	}
	wglDXUnlockObjectsNV(d3dDeviceInterop, 1, &d3dDepthInterop);

	// Check for completeness

	if (status_left != GL_FRAMEBUFFER_COMPLETE_EXT) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		switch (status_left) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): attachment");
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): missing attachment");
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): dimensions");
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): formats");
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): draw buffer");
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): read buffer");
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (left): unsupported");
		default:
			D3D_EXCEPTIONF("Failed to create OpenGL frame buffer object (left): unknown error %u", (int) status_left);
		}
	}

	if (status_right != GL_FRAMEBUFFER_COMPLETE_EXT) {
		GLD3DBuffers_destroy(gl_d3d_buffers);
		switch (status_right) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): attachment");
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): missing attachment");
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): dimensions");
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): formats");
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): draw buffer");
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): read buffer");
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			D3D_EXCEPTION("Failed to create OpenGL frame buffer object (right): unsupported");
		default:
			D3D_EXCEPTIONF("Failed to create OpenGL frame buffer object (right): unknown error %u", (int) status_right);
		}
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	printf(" Created OpenGL frame buffer objects\n");

	gl_d3d_buffers->initialized = true;
}

/*
 * Before using the OpenGL frame buffer object, we need to make sure to lock
 * the Direct3D buffers.
 */
void GLD3DBuffers_activate_left(GLD3DBuffers *gl_d3d_buffers) {
	wglDXLockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dLeftColorInterop);
	wglDXLockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dDepthInterop);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gl_d3d_buffers->fbo_left);
}

void GLD3DBuffers_activate_right(GLD3DBuffers *gl_d3d_buffers) {
	wglDXLockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dRightColorInterop);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gl_d3d_buffers->fbo_right);
}

void GLD3DBuffers_deactivate(GLD3DBuffers *gl_d3d_buffers) {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	wglDXUnlockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dLeftColorInterop);
	if (gl_d3d_buffers->stereo) {
		wglDXUnlockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dRightColorInterop);
	}
	wglDXUnlockObjectsNV(gl_d3d_buffers->d3dDeviceInterop, 1, &gl_d3d_buffers->d3dDepthInterop);
}

/*
 * Flushing:
 * 
 * 1. Copy our color buffer to the device's back buffer.
 * 2. Present the device back buffer.
 */
void GLD3DBuffers_flush(GLD3DBuffers *gl_d3d_buffers) {
	IDirect3DDevice9 *d3dDevice = (IDirect3DDevice9 *) gl_d3d_buffers->d3dDevice;
	IDirect3DSurface9 *d3dBackBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dBackBuffer;
	IDirect3DSurface9 *d3dLeftColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dLeftColorBuffer;
	IDirect3DSurface9 *d3dRightColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dRightColorBuffer;
	IDirect3DSurface9 *d3dStereoColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dStereoColorBuffer;

	HRESULT result;

	if (gl_d3d_buffers->stereo /*&& d3dRightColorBuffer*/ && d3dStereoColorBuffer) {
		// Stretch left color buffer to left side
		RECT dest;
		dest.top = 0;
		dest.bottom = gl_d3d_buffers->height;
		dest.left = 0;
		dest.right = gl_d3d_buffers->width;
		result = d3dDevice->lpVtbl->StretchRect(d3dDevice,
			d3dLeftColorBuffer, NULL, d3dStereoColorBuffer, &dest, D3DTEXF_POINT);
		if (result != D3D_OK) {
			D3D_EXCEPTION("Failed to stretch left color buffer to stereo color buffer");
		}

		// Stretch right color buffer to right side
		dest.left = gl_d3d_buffers->width;
		dest.right = gl_d3d_buffers->width * 2;
		result = d3dDevice->lpVtbl->StretchRect(d3dDevice,
			d3dRightColorBuffer, NULL, d3dStereoColorBuffer, &dest, D3DTEXF_POINT);
		if (result != D3D_OK) {
			D3D_EXCEPTION("Failed to stretch right color buffer to stereo color buffer");
		}

		// Stretch stereo color buffer to back buffer
		//
		// This is where the stereo magic happens! The driver discovers the header and
		// does the right thing with our left/right images. In fact, it ignores the
		// source and dest rects in this case. The StretchRect call seems to trigger
		// an entirely different function in the driver.
		//
		// Note: we are currently getting an "Out of Memory" error for this in fullscreen
		// mode.

		result = d3dDevice->lpVtbl->StretchRect(d3dDevice,
			d3dStereoColorBuffer, NULL, d3dBackBuffer, NULL, D3DTEXF_POINT);
		if (result != D3D_OK) {
			D3D_EXCEPTION("Failed to stretch stereo color buffer to back buffer");
		}
	}
	else {
		// Stretch left color buffer to back buffer
		result = d3dDevice->lpVtbl->StretchRect(d3dDevice,
			d3dLeftColorBuffer, NULL, d3dBackBuffer, NULL, D3DTEXF_POINT);
		if (result != D3D_OK) {
			D3D_EXCEPTION("Failed to stretch color buffer to back buffer");
		}
	}

	// Present back buffer
	result = d3dDevice->lpVtbl->Present(d3dDevice, NULL, NULL, NULL, NULL);
	if (result != D3D_OK) {
		D3D_EXCEPTION("Failed to present back buffer");
	}
}

void GLD3DBuffers_destroy(GLD3DBuffers *gl_d3d_buffers) {
	// Destroy NV stereo handle
	if (gl_d3d_buffers->nvStereo) {
		NvAPI_Stereo_DestroyHandle(gl_d3d_buffers->nvStereo);
		gl_d3d_buffers->nvStereo = NULL;
	}

	// Unbind OpenGL textures from Direct3D buffers
	if (gl_d3d_buffers->d3dDeviceInterop && gl_d3d_buffers->d3dLeftColorInterop) {
		wglDXUnregisterObjectNV(gl_d3d_buffers->d3dDeviceInterop, gl_d3d_buffers->d3dLeftColorInterop);
		gl_d3d_buffers->d3dLeftColorInterop = NULL;
	}
	if (gl_d3d_buffers->d3dDeviceInterop && gl_d3d_buffers->d3dRightColorInterop) {
		wglDXUnregisterObjectNV(gl_d3d_buffers->d3dDeviceInterop, gl_d3d_buffers->d3dRightColorInterop);
		gl_d3d_buffers->d3dRightColorInterop = NULL;
	}
	if (gl_d3d_buffers->d3dDeviceInterop && gl_d3d_buffers->d3dDepthInterop) {
		wglDXUnregisterObjectNV(gl_d3d_buffers->d3dDeviceInterop, gl_d3d_buffers->d3dDepthInterop);
		gl_d3d_buffers->d3dDepthInterop = NULL;
	}

	// Disable Direct3D/OpenGL interop
	if (gl_d3d_buffers->d3dDeviceInterop) {
		wglDXCloseDeviceNV(gl_d3d_buffers->d3dDeviceInterop);
		gl_d3d_buffers->d3dDeviceInterop = NULL;
	}

	// Release Direct3D objects
	if (gl_d3d_buffers->d3dLeftColorTexture) {
		IDirect3DSurface9 *d3dLeftColorTexture = (IDirect3DSurface9 *) gl_d3d_buffers->d3dLeftColorTexture;
		d3dLeftColorTexture->lpVtbl->Release(d3dLeftColorTexture);
		gl_d3d_buffers->d3dLeftColorTexture = NULL;
	}
	if (gl_d3d_buffers->d3dRightColorTexture) {
		IDirect3DSurface9 *d3dRightColorTexture = (IDirect3DSurface9 *) gl_d3d_buffers->d3dRightColorTexture;
		d3dRightColorTexture->lpVtbl->Release(d3dRightColorTexture);
		gl_d3d_buffers->d3dRightColorTexture = NULL;
	}
	if (gl_d3d_buffers->d3dLeftColorBuffer) {
		IDirect3DSurface9 *d3dLeftColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dLeftColorBuffer;
		d3dLeftColorBuffer->lpVtbl->Release(d3dLeftColorBuffer);
		gl_d3d_buffers->d3dLeftColorBuffer = NULL;
	}
	if (gl_d3d_buffers->d3dRightColorBuffer) {
		IDirect3DSurface9 *d3dRightColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dRightColorBuffer;
		d3dRightColorBuffer->lpVtbl->Release(d3dRightColorBuffer);
		gl_d3d_buffers->d3dRightColorBuffer = NULL;
	}
	if (gl_d3d_buffers->d3dStereoColorBuffer) {
		IDirect3DSurface9 *d3dStereoColorBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dStereoColorBuffer;
		d3dStereoColorBuffer->lpVtbl->Release(d3dStereoColorBuffer);
		gl_d3d_buffers->d3dStereoColorBuffer = NULL;
	}
	if (gl_d3d_buffers->d3dDepthBuffer) {
		IDirect3DSurface9 *d3dDepthBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dDepthBuffer;
		d3dDepthBuffer->lpVtbl->Release(d3dDepthBuffer);
		gl_d3d_buffers->d3dDepthBuffer = NULL;
	}
	if (gl_d3d_buffers->d3dBackBuffer) {
		IDirect3DSurface9 *d3dBackBuffer = (IDirect3DSurface9 *) gl_d3d_buffers->d3dBackBuffer;
		d3dBackBuffer->lpVtbl->Release(d3dBackBuffer);
		gl_d3d_buffers->d3dBackBuffer = NULL;
	}
	if (gl_d3d_buffers->d3dDevice) {
		IDirect3DDevice9 *d3dDevice = (IDirect3DDevice9 *) gl_d3d_buffers->d3dDevice;
		d3dDevice->lpVtbl->Release(d3dDevice);
		gl_d3d_buffers->d3dDevice = NULL;
	}
	if (gl_d3d_buffers->d3dLibrary) {
		FreeLibrary((HINSTANCE) gl_d3d_buffers->d3dLibrary);
		gl_d3d_buffers->d3dLibrary = NULL;
	}

	// Delete OpenGL frame buffer object
	if (gl_d3d_buffers->fbo_left) {
		glDeleteFramebuffers(1, &gl_d3d_buffers->fbo_left);
		gl_d3d_buffers->fbo_left = 0;
	}

	// Delete OpenGL render buffer
	if (gl_d3d_buffers->render_buffer) {
		//glDeleteTextures(1, &gl_d3d_buffers->render_buffer);
		glDeleteRenderbuffersEXT(1, &gl_d3d_buffers->render_buffer);
		gl_d3d_buffers->render_buffer = 0;
	}

	// Delete OpenGL textures
	if (gl_d3d_buffers->texture_left) {
		glDeleteTextures(1, &gl_d3d_buffers->texture_left);
		gl_d3d_buffers->texture_left = 0;
	}
	if (gl_d3d_buffers->texture_right) {
		glDeleteTextures(1, &gl_d3d_buffers->texture_right);
		gl_d3d_buffers->texture_right = 0;
	}
}

#else

void GLD3DBuffers_create(GLD3DBuffers *gl_d3d_buffers, void *window_handle, bool vsync, bool stereo) {
	printf("Direct3D not available on this platform\n");
}

void GLD3DBuffers_activate_left(GLD3DBuffers *gl_d3d_buffers) {
}

void GLD3DBuffers_activate_right(GLD3DBuffers *gl_d3d_buffers) {
}

void GLD3DBuffers_deactivate(GLD3DBuffers *gl_d3d_buffers) {
}

void GLD3DBuffers_flush(GLD3DBuffers *gl_d3d_buffers) {
}

void GLD3DBuffers_destroy(GLD3DBuffers *gl_d3d_buffers) {
}

#endif
