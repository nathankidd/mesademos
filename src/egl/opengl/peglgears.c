/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the infamous "glxgears" demo to straight EGL
 * Port by Dane Rushton 10 July 2005
 * 
 * No command line options.
 * Program runs for 5 seconds then exits, outputing framerate to console
 */

//#define EGL_EGLEXT_PROTOTYPES

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "gl_wrap.h"

#define MAX_CONFIGS 10
#define MAX_MODES 100

#define BENCHMARK

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static double
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

#else /*BENCHMARK*/

/* dummy */
static double
current_time(void)
{
   /* update this function for other platforms! */
   static double t = 0.0;
   static int warn = 1;
   if (warn) {
      fprintf(stderr, "Warning: current_time() not implemented!!\n");
      warn = 0;
   }
   return t += 1.0;
}

#endif /*BENCHMARK*/


#ifndef M_PI
#define M_PI 3.14159265
#endif


static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

#if 0
static GLfloat eyesep = 5.0;		/* Eye separation. */
static GLfloat fix_point = 40.0;	/* Fixation point distance.  */
static GLfloat left, right, asp;	/* Stereo frustum params.  */
#endif


/*
 *
 *  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat angle, da;
   GLfloat u, v, len;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   glShadeModel(GL_FLAT);

   glNormal3f(0.0, 0.0, 1.0);

   /* draw front face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      if (i < teeth) {
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    width * 0.5);
      }
   }
   glEnd();

   /* draw front sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
   }
   glEnd();

   glNormal3f(0.0, 0.0, -1.0);

   /* draw back face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      if (i < teeth) {
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    -width * 0.5);
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      }
   }
   glEnd();

   /* draw back sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
   }
   glEnd();

   /* draw outward faces of teeth */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      u = r2 * cos(angle + da) - r1 * cos(angle);
      v = r2 * sin(angle + da) - r1 * sin(angle);
      len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      glNormal3f(v, -u, 0.0);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
   }

   glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
   glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

   glEnd();

   glShadeModel(GL_SMOOTH);

   /* draw inside radius cylinder */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glNormal3f(-cos(angle), -sin(angle), 0.0);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
   }
   glEnd();
}


static void
draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glCallList(gear1);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
   glCallList(gear2);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
   glCallList(gear3);
   glPopMatrix();

   glPopMatrix();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   GLfloat h = (GLfloat) height / (GLfloat) width;

   glViewport(0, 0, (GLint) width, (GLint) height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -40.0);
}
   


static void
init(void)
{
   static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
   static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   /* make the gears */
   gear1 = glGenLists(1);
   glNewList(gear1, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
   gear(1.0, 4.0, 1.0, 20, 0.7);
   glEndList();

   gear2 = glGenLists(1);
   glNewList(gear2, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
   gear(0.5, 2.0, 2.0, 10, 0.7);
   glEndList();

   gear3 = glGenLists(1);
   glNewList(gear3, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
   gear(1.3, 2.0, 0.5, 10, 0.7);
   glEndList();

   glEnable(GL_NORMALIZE);
}




static void run_gears(EGLDisplay dpy, EGLSurface surf, int ttr)
{
	double st = current_time();
	double ct = st;
	int frames = 0;
	GLfloat seconds, fps;

	while (ct - st < ttr)
	{
		double tt = current_time();
		double dt = tt - ct;
		ct = tt;
		
		/* advance rotation for next frame */
		angle += 70.0 * dt;  /* 70 degrees per second */
		if (angle > 3600.0)
			angle -= 3600.0;
		
		draw();
		
		eglSwapBuffers(dpy, surf);
	
		
		frames++;
	}
	
	seconds = ct - st;
	fps = frames / seconds;
	printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
	fflush(stdout);
	
}
#if 1 // https://www.khronos.org/registry/EGL/extensions/MESA/EGL_MESA_platform_gbm.txt
// This example program creates an EGL surface from a GBM surface.
//
// If the macro EGL_MESA_platform_gbm is defined, then the program
// creates the surfaces using the methods defined in this specification.
// Otherwise, it uses the methods defined by the EGL 1.4 specification.
//
// Compile with `cc -std=c99 example.c -lgbm -lEGL`.

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gbm.h>

struct my_display {
	struct gbm_device *gbm;
	EGLDisplay egl;
};

struct my_config {
	struct my_display dpy;
	EGLConfig egl;
};

struct my_window {
	struct my_config config;
	struct gbm_surface *gbm;
	EGLSurface egl;
};

static void
check_extensions(void)
{
#ifdef EGL_MESA_platform_gbm
	const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	if (!client_extensions) {
	    // EGL_EXT_client_extensions is unsupported.
	    abort();
	}
	if (!strstr(client_extensions, "EGL_MESA_platform_gbm")) {
	    abort();
	}
#endif
}

static struct my_display
get_display(void)
{
	struct my_display dpy;

	int fd = open("/dev/dri/card0", O_RDWR | FD_CLOEXEC);
	if (fd < 0) {
	    abort();
	}

	dpy.gbm = gbm_create_device(fd);
	if (!dpy.gbm) {
	    abort();
	}


	#ifdef EGL_MESA_platform_gbm
//	dpy.egl = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, dpy.gbm, NULL);
	dpy.egl = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, dpy.gbm, NULL);
	#else
	dpy.egl = eglGetDisplay(dpy.gbm);
	#endif

	if (dpy.egl == EGL_NO_DISPLAY) {
	    abort();
	}

	EGLint major, minor;
	if (!eglInitialize(dpy.egl, &major, &minor)) {
	    abort();
	}

	return dpy;
}

static struct my_config
get_config(struct my_display dpy)
{
	struct my_config config = {
	    .dpy = dpy,
	};

	EGLint egl_config_attribs[] = {
	    EGL_BUFFER_SIZE,        32,
	    EGL_DEPTH_SIZE,         EGL_DONT_CARE,
	    EGL_STENCIL_SIZE,       EGL_DONT_CARE,
	    EGL_RENDERABLE_TYPE,    EGL_OPENGL_BIT, //EGL_OPENGL_ES2_BIT,
	    EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
	    EGL_NONE,
	};

	EGLint num_configs;
	if (!eglGetConfigs(dpy.egl, NULL, 0, &num_configs)) {
	    abort();
	}

	EGLConfig *configs = malloc(num_configs * sizeof(EGLConfig));
	if (!eglChooseConfig(dpy.egl, egl_config_attribs,
			     configs, num_configs, &num_configs)) {
	    abort();
	}
	if (num_configs == 0) {
	    abort();
	}

	// Find a config whose native visual ID is the desired GBM format.
	for (int i = 0; i < num_configs; ++i) {
	    EGLint gbm_format;

	    if (!eglGetConfigAttrib(dpy.egl, configs[i],
				    EGL_NATIVE_VISUAL_ID, &gbm_format)) {
		abort();
	    }

	    if (gbm_format == GBM_FORMAT_XRGB8888) {
		config.egl = configs[i];
		free(configs);
		return config;
	    }
	}

	// Failed to find a config with matching GBM format.
	abort();
}

static struct my_window
get_window(struct my_config config)
{
	struct my_window window = {
	    .config = config,
	};

	window.gbm = gbm_surface_create(config.dpy.gbm,
					256, 256,
					GBM_FORMAT_XRGB8888,
					GBM_BO_USE_RENDERING);
	if (!window.gbm) {
	    abort();
	}

	#ifdef EGL_MESA_platform_gbm
	//window.egl = eglCreatePlatformWindowSurfaceEXT(config.dpy.egl,
	window.egl = eglCreatePlatformWindowSurface(config.dpy.egl,
						       config.egl,
						       window.gbm,
						       NULL);
	#else
	window.egl = eglCreateWindowSurface(config.dpy.egl,
					    config.egl,
					    window.gbm,
					    NULL);
	#endif

	if (window.egl == EGL_NO_SURFACE) {
	    abort();
	}

	return window;
}

#endif


int
main(int argc, char *argv[])
{
	int major, minor;
	EGLContext ctx;
	EGLSurface surface;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs, i;
	EGLBoolean b;
	EGLDisplay d;
	EGLint configAttribs[10];
	EGLint screenAttribs[10];
	GLboolean printInfo = GL_FALSE;
	EGLint width = 300, height = 300;
	
        /* parse cmd line args */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-info") == 0)
		{
			printInfo = GL_TRUE;
		}
		else
			printf("Warning: unknown parameter: %s\n", argv[i]);
	}
	
#if 0
	/* DBR : Create EGL context/surface etc */
	d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(d);

	if (!eglInitialize(d, &major, &minor)) {
		printf("peglgears: eglInitialize failed\n");
		return 0;
	}
#else
	check_extensions();

	struct my_display dpy = get_display();
	d = dpy.egl;
#endif

	printf("peglgears: EGL version = %d.%d\n", major, minor);
	printf("peglgears: EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

#if 0
	i = 0;
	configAttribs[i++] = EGL_RENDERABLE_TYPE;
	configAttribs[i++] = EGL_OPENGL_BIT;
	configAttribs[i++] = EGL_SURFACE_TYPE;
	configAttribs[i++] = EGL_PBUFFER_BIT;
	configAttribs[i++] = EGL_NONE;

	numConfigs = 0;
	if (!eglChooseConfig(d, configAttribs, configs, MAX_CONFIGS, &numConfigs) ||
	    !numConfigs) {
		printf("peglgears: failed to choose a config\n");
		return 0;
	}
#else
	struct my_config config = get_config(dpy);
	configs[0] = config.egl;
#endif

	eglBindAPI(EGL_OPENGL_API);

	ctx = eglCreateContext(d, configs[0], EGL_NO_CONTEXT, NULL);
	if (ctx == EGL_NO_CONTEXT) {
		printf("peglgears: failed to create context\n");
		return 0;
	}
	
#if 0
	/* build up screenAttribs array */
	i = 0;
	screenAttribs[i++] = EGL_WIDTH;
	screenAttribs[i++] = width;
	screenAttribs[i++] = EGL_HEIGHT;
	screenAttribs[i++] = height;
	screenAttribs[i++] = EGL_NONE;

	surface = eglCreatePbufferSurface(d, configs[0], screenAttribs);
	if (surface == EGL_NO_SURFACE) {
		printf("peglgears: failed to create pbuffer surface\n");
		return 0;
	}
#else
	struct my_window window = get_window(config);
	surface = window.egl;
#endif
	
	b = eglMakeCurrent(d, surface, surface, ctx);
	if (!b) {
		printf("peglgears: make current failed\n");
		return 0;
	}
	
	if (printInfo)
	{
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
	}
	
	init();
	reshape(width, height);

	glDrawBuffer( GL_BACK );

	run_gears(d, surface, 5.0);
	
	eglDestroySurface(d, surface);
	eglDestroyContext(d, ctx);
	eglTerminate(d);
	
	return 0;
}
