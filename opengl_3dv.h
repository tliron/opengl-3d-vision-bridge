#include "gl_custom.h"
#include "windows.h"

typedef struct struct_GLD3DBuffers {
    GLuint fbo_left, fbo_right, texture_left, texture_right, render_buffer;
    int width, height;
    bool initialized, stereo;

    // Internal
    void *d3dLibrary;
    void *d3dDevice;
    void *d3dLeftColorTexture;
    void *d3dRightColorTexture;
    void *d3dLeftColorBuffer;
    void *d3dRightColorBuffer;
    void *d3dStereoColorBuffer;
    void *d3dDepthBuffer;
    void *d3dBackBuffer;
    void *d3dDeviceInterop;
    void *d3dLeftColorInterop;
    void *d3dRightColorInterop;
    void *d3dDepthInterop;
    void *nvStereo;
} GLD3DBuffers;

void GLD3DBuffers_create(GLD3DBuffers *gl_d3d_buffers, void *window_handle, bool vsync, bool stereo);
void GLD3DBuffers_destroy(GLD3DBuffers *gl_d3d_buffers);
void GLD3DBuffers_activate_left(GLD3DBuffers *gl_d3d_buffers);
void GLD3DBuffers_activate_right(GLD3DBuffers *gl_d3d_buffers);
void GLD3DBuffers_deactivate(GLD3DBuffers *gl_d3d_buffers);
void GLD3DBuffers_flush(GLD3DBuffers *gl_d3d_buffers);
