#!/bin/bash

#
# This is an example script for building the demo using the MinGW
# cross compiler.
#
# It was tested on Ubuntu 13.10 with the "mingw32" package installed.
#

DIRECTX_SDK_DIR=directx
NVAPI_DIR=R331-developer
SDL2_DIR=sdl

i586-mingw32msvc-g++ -mthreads \
	-o wow.exe \
	opengl_3dv.c demo.c gl_custom.c wgl_custom.c \
	-lopengl32 \
	-I$DIRECTX_SDK_DIR/include -L$DIRECTX_SDK_DIR/lib -ld3d9 -DCINTERFACE \
	-I$NVAPI_DIR -L$NVAPI_DIR/x86 -lnvapi \
	-I$SDL2_DIR/include/SDL2 -L$SDL2_DIR/lib -lmingw32 -lSDL2main -lSDL2 \
	-Wno-write-strings
