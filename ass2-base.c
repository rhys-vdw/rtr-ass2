/* $Id: tute-vertex.c 18 2006-07-26 10:28:03Z aholkner $ */
/* Updated pknowles, gl 2010 */

#include <GL/glew.h>
#include <GL/glut.h> /* for screen text only */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "shaders.h"
#include "sdl-base.h"
#include "objects.h"

#define CAMERA_VELOCITY 0.005		 /* Units per millisecond */
#define CAMERA_ANGULAR_VELOCITY 0.05	 /* Degrees per millisecond */
#define CAMERA_MOUSE_X_VELOCITY 0.3	 /* Degrees per mouse unit */
#define CAMERA_MOUSE_Y_VELOCITY 0.3	 /* Degrees per mouse unit */

#ifndef min
#define min(a, b) ((a)>(b)?(b):(a))
#endif
#ifndef max
#define max(a, b) ((a)>(b)?(a):(b))
#endif
#ifndef clamp
#define clamp(x, a, b) min(max(x, a), b)
#endif

static float camera_zoom;
static float camera_heading;	/* Direction in degrees the camera faces */
static float camera_pitch;	/* Up/down degrees for camera */
static int mouse1_down;		/* Left mouse button Up/Down. Only move camera when down. */
static int mouse2_down;		/* Right mouse button Up/Down. Only zoom camera when down. */

/* Object data */
Object* object = NULL;
static int tessellation = 2; /* Tessellation level */
const int min_tess = 2;
const int max_tess = 10;

/* Store the state (1 = pressed, 0 = not pressed) of each key  we're interested in. */
static char key_state[1024];

/* The opengl handle to our shader */
GLuint shader = 0;

/* Store render state variables.  Can be toggled with function keys. */
static struct {
	int wireframe;
	int lighting;
	int shaders;
} renderstate;

/* Light and materials */
static float light0_position[] = {2.0, 2.0, 2.0, 0.0};
static float light0_ambient[] = {0.5, 0.5, 0.5, 1.0};
static float light0_diffuse[] = {1.0, 1.0, 1.0, 1.0};
static float light0_specular[] = {1.0, 1.0, 1.0, 1.0};
static float material_ambient[] = {0.5, 0.5, 0.5, 1.0};
static float material_diffuse[] = {1.0, 0.0, 0.0, 1.0};
static float material_specular[] = {0.5, 0.5, 0.5, 1.0};
static float material_shininess = 50.0;

void update_renderstate()
{
	if (renderstate.lighting)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	glPolygonMode(GL_FRONT_AND_BACK, renderstate.wireframe ? GL_LINE : GL_FILL);
}

void regenerate_geometry()
{
	int subdivs;
	subdivs = 1 << (tessellation);
	
	/* Free previous object */
	if (object) 
		freeObject(object);
	
	printf("Generating %ix%i... ", subdivs, subdivs);
	fflush(stdout);
	
	/* Generate the new object. NOTE: different equations require different arguments. see objects.h */
	object = createObject(parametricSphere, subdivs + 1, subdivs + 1, 1.0, 0.5, 0.4);
	
	printf("done.\n");
	fflush(stdout);
}

void init()
{
	int argc = 0;
	char** argv = NULL;
	glutInit(&argc, argv); /* NOTE: this hack will not work on windows */
	glewInit();

	/* Load the shader */
	shader = getShader("shader.vert", "shader.frag");

	/* Lighting and colours */
	glClearColor(0, 0, 0, 0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);

	/* Camera */
	camera_zoom = 5.0;
	camera_heading = 0.0;
	camera_pitch = 0.0;
	mouse1_down = 0;
	mouse2_down = 0;

	/* Turn off all keystate flags */
	memset(key_state, 0, 1024);

	/* Default render modes */
	renderstate.wireframe = 0;
	renderstate.lighting = 1;
	renderstate.shaders = 0;

	update_renderstate();
	
	regenerate_geometry();
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	/* Reset the projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, width / (double) height, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

void draw_framerate(SDL_Surface *surface)
{
	/* Write framerate to a string */
	char buffer[32];
	char *bufp;
	snprintf(buffer, sizeof buffer, "FR: %d", frame_rate);
	
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	/* Apply an orthographic projection temporarily */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, surface->w, 0, surface->h, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	/* Draw the string */
	glRasterPos2i(10, surface->h - 20);
	for (bufp = buffer; *bufp; bufp++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

	glPopMatrix();	/* Pop modelview */
	glMatrixMode(GL_PROJECTION);

	glPopMatrix();	/* Pop projection */
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}

void display(SDL_Surface *surface)
{
	/* Clear the colour and depth buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Camera transformation */
	glLoadIdentity();
	glTranslatef(0, 0, -camera_zoom);
	glRotatef(-camera_pitch, 1, 0, 0);
	glRotatef(-camera_heading, 0, 1, 0);

	/* Set the light position (gets multiplied by the modelview matrix) */
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

	/* Draw the scene */
	if (renderstate.shaders)
		glUseProgram(shader); /* Use our shader for future rendering */
	drawObject(object);
	glUseProgram(0);

	/* Draw framerate */
	draw_framerate(surface);

	CHECKERROR;
}

void update(int milliseconds)
{
}

void set_mousestate(unsigned char button, int state)
{
	switch (button)
	{
	case SDL_BUTTON_LEFT:
		mouse1_down = state;
		break;
	case SDL_BUTTON_RIGHT:
		mouse2_down = state;
		break;
	}
}

void event(SDL_Event *event)
{
	static int first_mousemotion = 1;

	switch (event->type)
	{
	case SDL_KEYDOWN:
		key_state[event->key.keysym.sym] = 1;

		/* Handle non-state keys */
		switch (event->key.keysym.sym)
		{
		case SDLK_ESCAPE:
			quit();
			break;
		case SDLK_s:
			renderstate.shaders = !renderstate.shaders;
			printf("Using Shaders %i\n", renderstate.shaders);
			break;
		case SDLK_l:
			renderstate.lighting = !renderstate.lighting;
			printf("Lighting %i\n", renderstate.lighting);
			update_renderstate();
			break;
		case SDLK_w:
			renderstate.wireframe = !renderstate.wireframe;
			printf("Wireframe %i\n", renderstate.wireframe);
			update_renderstate();
			break;
		case SDLK_t:
			if ((key_state[SDLK_LSHIFT] || key_state[SDLK_RSHIFT]))
			{
				if (tessellation < max_tess) 
				{
					++tessellation;
					regenerate_geometry();
				} 
			}
			else 
			{
				if (tessellation > min_tess)
				{
					--tessellation;
					regenerate_geometry();
				}
			}
			break;
		default:
			break;
		}
		break;
	case SDL_KEYUP:
		key_state[event->key.keysym.sym] = 0;
		break;

	case SDL_MOUSEBUTTONDOWN:
		set_mousestate(event->button.button, 1);
		break;

	case SDL_MOUSEBUTTONUP:
		set_mousestate(event->button.button, 0);
		break;

	case SDL_MOUSEMOTION:
		if (first_mousemotion)
		{
			/* The first mousemotion event will have bogus xrel and
			   yrel, so ignore it. */
			first_mousemotion = 0;
			break;
		}
		if (mouse1_down)
		{
			/* Only move the camera if the mouse is down*/
			camera_heading -= event->motion.xrel * CAMERA_MOUSE_X_VELOCITY;
			camera_pitch -= event->motion.yrel * CAMERA_MOUSE_Y_VELOCITY;
		}
		if (mouse2_down)
		{
			camera_zoom -= event->motion.yrel * CAMERA_MOUSE_Y_VELOCITY * 0.1;
		}
		break;
	default:
		break;
	}
}

void cleanup()
{
	/* Delete the shader */
	glDeleteProgram(shader);
	
	/* Free object data */
	if (object) 
		freeObject(object);
}
