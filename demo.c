#include "SDL.h"
#include "SDL_syswm.h"

extern "C" {
#include "opengl_3dv.h"
}

SDL_Window *window;
GLD3DBuffers gl_d3d_buffers = {0};
bool nvidia_3dv = false;

typedef enum {
	EYE_CENTER,
	EYE_LEFT,
	EYE_RIGHT
} EYE;

void render_scene(EYE eye) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float d = 0.0f;
	if (eye == EYE_LEFT) {
		d = 0.01f;
	}
	else if (eye == EYE_RIGHT) {
		d = -0.01f;
	}

	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(-0.5f+d, -0.5f);
	glVertex2f(-0.5f+d,  0.5f);

	glVertex2f(-0.5f+d,  0.5f);
	glVertex2f( 0.5f+d,  0.5f);

	glVertex2f( 0.5f+d,  0.5f);
	glVertex2f( 0.5f+d, -0.5f);

	glVertex2f( 0.5f+d, -0.5f);
	glVertex2f(-0.5f+d, -0.5f);
	glEnd();
}

void render_clear() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render() {
	if (nvidia_3dv) {
		GLD3DBuffers_activate_left(&gl_d3d_buffers);
		render_clear();
		render_scene(EYE_LEFT);
		GLD3DBuffers_activate_right(&gl_d3d_buffers);
		render_clear();
		render_scene(EYE_RIGHT);
		GLD3DBuffers_deactivate(&gl_d3d_buffers);
		GLD3DBuffers_flush(&gl_d3d_buffers);
	}
	else {
		render_clear();
		render_scene(EYE_CENTER);
		SDL_GL_SwapWindow(window);
	}
}

bool handle_input() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
		}
		else if (event.type == SDL_TEXTINPUT) {
			char key = event.text.text[0];
			if (key == 'q') {
				return false;
			}
			else if (key == 'n') {
				nvidia_3dv = !nvidia_3dv;
				if (nvidia_3dv && !gl_d3d_buffers.initialized) {
					printf("Cannot enable 3D Vision because could not initialize OpenGL / 3D Vision bridge\n");
					nvidia_3dv = false;
				}
				else {
					printf(nvidia_3dv ? "3D Vision is now on\n" : "3D Vision is now off\n");
				}
			}
		}
	}

	return true;
}

int main(int argc, char *argv[]) {
	printf("Welcome to the OpenGL / 3D Vision bridge demo!\n");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		printf("Could not initialize SDL video\n");
		return 1;
	}

	window = SDL_CreateWindow("OpenGL / 3D Vision Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!window) {
		printf("Could not create SDL window\n");
		return 1;
	}

	SDL_GL_SetSwapInterval(1); // enable vsync when not using 3D Vision

	SDL_GLContext gl = SDL_GL_CreateContext(window);
	if (!gl) {
		printf("Could not create SDL OpenGL context\n");
		return 1;
	}

	if (ogl_LoadFunctions() == ogl_LOAD_FAILED) {
		printf("Could not bind OpenGL API\n");
		return 1;
	}

	SDL_SysWMinfo wmInfo = {0};
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hWnd = wmInfo.info.win.window;

	GLD3DBuffers_create(&gl_d3d_buffers, hWnd, true, true);

	printf("Press Q to quit and N to toggle NVIDIA 3D Vision\n");

	SDL_StartTextInput();
	while (handle_input()) {
		render();
	}
	SDL_StopTextInput();

	GLD3DBuffers_destroy(&gl_d3d_buffers);
	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
