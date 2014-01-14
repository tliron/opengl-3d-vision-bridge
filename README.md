OpenGL / 3D Vision Bridge
=========================

This tiny yet sophisticated library allows you to write graphical applications using OpenGL that can make
use of the consumer version of [NVIDIA 3D Vision](http://www.nvidia.com/object/3d-vision-main.html) technology, which normally requires the Microsoft-only
Direct3D API.

It relies on a native NVIDIA bridging feature, and thus performance is equal to that of using Direct3D
directly.

Why would you want this?
------------------------

You can use it to easily create cross-platform applications that can use 3D Vision when running on Windows.

OpenGL is a great choice for cross-platform applications, as it's supported on Linux, Unix, Mac OS X, Windows
and many mobile and console platforms. However, without this library, you would need to write your graphics
code in Direct3D in order to make use of 3D Vision. Writing the same application in both OpenGL and Direct3D
is a prohibitively enormous task for most developers, and so they're left with the difficult choice between
3D Vision + Microsoft-only vs. OpenGL + cross-platform but no 3D Vision. With this library you can have your
cake and eat it!

What this library does *not* do
-------------------------------

*Not* a "wrapper":

This library relies on you, the *developer*, to render separate images for the left and right eyes. It is not
a magical wrapper that allows existing OpenGL games to use 3D Vision. That nice automatic support for 3D Vision
that Direct3D games enjoy relies on heuristics baked into NVIDIA's driver. (Indeed, these heuristics
are imperfect and many games do not run well with 3D Vision if they were not designed and tested for using it.)
These heuristics are not supported in OpenGL. Indeed, the usual 3D Vision configuration in the NVIDIA
Control Panel will not have any affect on this library, neither will the depth/convergence hot keys work.

*Not* OpenGL quad buffers:

Stereoscopy support is included in the OpenGL standard, as "quad buffers." Unfortunately, that feature is
currently *not* supported in the consumer version of 3D Vision (though it is supported in [3D Vision Pro](http://www.nvidia.com/object/quadro_stereo_technology.html),
available only for the NVIDIA Quadro range of pro video cards). If you want your application to support both
OpenGL quad buffers and this library, you will have to code specifically for both cases. As you'll see, though,
it shouldn't be hard: this library has only a handful of easy-to-use APIs.

License and "cost"
------------------

There's not a lot of code here, but it's *extremely* delicate, and I worked very hard on getting it working.
I've decided to provide it to you free of charge (and without warranty), with a permissive MIT-style distribution
license, because I love stereoscopy and want it used as widely as possible. All I ask in return (but do not require)
is that you credit me somewhere in your final product, at the very least with something like this:

"This product makes use of code written by Tal Liron."

You may also provide a link to this site, and of course provide more details about my contribution to your
product. 

If you are feeling especially grateful, I am graciously accepting donations: ;)

[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=NQXC28JYKUKH2)

I am also available for hire (via [Three Crickets](http://threecrickets.com/)) to help you integrate this library into your product. 

How does this sorcery work?
---------------------------

This library relies on a combination of two NVIDIA features, which are each officially supported, but
poorly documented and lacking of examples:

[OpenGL/Direct3D interop](http://developer.download.nvidia.com/opengl/specs/WGL_NV_DX_interop.txt):
This NVIDIA extension to OpenGL lets you use a Direct3D surface as an OpenGL texture. There are many constraints
to this feature, and it's very tricky to get right. I am grateful to Snippets and Driblits for sharing [example code](https://sites.google.com/site/snippetsanddriblits/OpenglDxInterop)
for this feature. Note that an annoyance of this interop is that OpenGL and Direct3D use vertically opposite
coordinate systems. This means that you will have to draw "upside down" in OpenGL in order for the rendered image
to appear correctly. 

Magic 3D Vision surfaces: You can turn on 3D Vision by [inserting a magic code](http://developer.download.nvidia.com/presentations/2009/GDC/GDC09-3DVision-The_In_and_Out.pdf)
into a Direct3D surface.  Unfortunately this is supported only for certain .exe filenames, the list of which is
hardcoded into the driver. (This was likely done in order for NVIDIA to be able to control the list of
applications that can make use of this feature.) And so, a bizarre and annoying requirement for using this
library is that your .exe has to have one of those names. I suggest "wow.exe". I wonder what WoW stands for? ;)

You can read full details about my long journey to get this working in [my post on MTBS](http://www.mtbs3d.com/phpbb/viewtopic.php?f=105&t=16310&p=97553).

Building the library and demo
-----------------------------

To build the library, you will need:

* A C compiler: Tested using gcc, specifically the MinGW cross-compiler version (not tested with Visual C, but
should work).
* [NVAPI](https://developer.nvidia.com/nvapi): Tested with version R331.
* [Microsoft DirectX SDK](http://www.microsoft.com/en-us/download/details.aspx?id=6812): Tested with the June
2010 version. Note that you will need to unpack the enormous SDK in Windows. You definitely don't need the whole
SDK: you only need the Direct3D headers files and .lib files.
* OpenGL bindings: I'm including cross-platform bindings I've created using the excellent [glLoadGen tool](https://bitbucket.org/alfonse/glloadgen/wiki/Home).
However, bindings are highly customizable and specific to your usage. You can build your own using glLoadGen
or modify the code to use whatever binding tool you prefer (e.g., [GLEW](http://glew.sourceforge.net/)).

To build the demo, you will also need:

* SDL 2

Note: The DirectX SDK headers are not supported on recent versions of gcc's C compiler, but work fine when using
its C++ compiler. Despite using the C++ compiler, our code is written in C and the resulting library has a C ABI.

An example build script is included, named "build.sh".

Obviously, you will have to run "wow.exe" on Windows with an NVIDIA GPU that supports 3D Vision. You also need to
make sure that SDL2.dll is are in the same directory as wow.exe.

The demo is intentionally simple: it will show a simple red rectangle on a white background. Press "N" to toggle
3D Vision. When enabled, the rectangle will appear to "pop out" of the screen if you're wearing the 3D Vision
glasses.

How to use this library
-----------------------

There are only a few APIs, and they are very easy to use:

Use GLD3DBuffers_create to enable the library and GLD3DBuffers_destroy to disable it.

GLD3DBuffers_activate_left and GLD3DBuffers_activate_right each active the FBO (Frame Buffer Object) for
each eye. While activated, all OpenGL output will be sent to that FBO. (Unless, of course, you're using
FBOs for other purposes: in which case only your final rendering will be sent to the eye's FBO.)

Finally, to render both eyes in stereo call GLD3DBuffers_deactivate and GLD3DBuffers_flush.  

Important: The library's flush API *replaces* your OpenGL swapping mechanism, so make sure *not* to invoke
swapping. If you're using SDL, this means *not* calling SDL_GL_SwapWindow. Note that vsync, too, is handled
by the library.

See demo.c for a complete example.

Now, you must also take into account that Direct3D's Y-axis is reversed to that of OpenGL. This means that
*all* your drawing routines have to support "flipping" the Y-axis when 3D Vision is enabled. I can't teach you
how to do that, but here are a few suggestions:

1. If you're using shaders (the recommended and best-performing drawing mechanism for OpenGL) then you can
bind a uniform float, let's call it "y\_factor". When not flipping, it should be 1.0. When flipping, it should
be -1.0. In your vertex shader, simply multiply your y coordinate by the y_factor. Here's a partial example:

		uniform float y_factor;
		attribute vec2 vertex;
		void main() {
			gl_Position.x = vertex.x;
			gl_Position.y = vertex.y * y_factor;
		}

2. If you're using the immediate rendering API, you can reverse the Y axis via the projection matrix.

3. If you really can't be bothered with modifying your drawing routines to support "flipping," as a last
resort you can render your whole scene to an an FBO (Frame Buffer Object) and then "flip it" when rendering
it. However, this brute-force method is not recommended, because it will add a performance hit.
