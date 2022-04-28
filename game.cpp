/* * * * * * * * * * * * * * * * * * 
 * Name:  Daniel Josep             *
 * Name:  Alyssa Diaz              *
 * Name:  Carlos Hernandez         *
 * Class: 4490 - Game Development  *
 * Date:  04/11/22                 *
 * Game Name: Fossil Frenzy		   *
 * * * * * * * * * * * * * * * * * */

//author: Gordon Griesel
//date: Spring 2022
//purpose: get Image working
//
#include <iostream>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"
#include <pthread.h>

class Image {
public:
	int width, height, max;
	char *data;
	Image() { }
	Image (const char *fname) {
		bool isPPM = true;
		char str[1200];
		char newfile[200];
		ifstream fin;
		char *p = strstr((char *)fname, ".ppm");
		if (!p) {
			// Not a PPM file
			isPPM = false;
			strcpy(newfile, fname);
			newfile[strlen(newfile)-4] = '\0';
			strcat(newfile, ".ppm");
			sprintf(str, "convert %s %s", fname, newfile);
			system(str);
			fin.open(newfile);
		} else {
			fin.open(fname);
		}
		char p6[10];
		fin >> p6;
		fin >> width >> height;
		fin >> max;
		//width *  height * the number of colors - r g b
		data = new char [width * height * 3];
		fin.read(data, 1); //Add data by 1 so that it can read max fully
		fin.read(data, width * height * 3); 
		fin.close();
		if (!isPPM) {
			unlink(newfile);
		}
	}
} img("pics/background.png"),
  ps("pics/Player_Screen.png"),
  sprite("pics/bees.png");

struct Vector {
    float x, y, z;
};

typedef double Flt;
class Bee {
public:
    Flt pos[3];
    Flt vel[3];
    float w, h;
    unsigned int color;
    bool alive_or_dead;
	Flt mass;
	Bee() {
		w = h = 4.0;
		pos[0] = 1.0;
		pos[1] = 200.0;
		vel[0] = 4.0;
		vel[1] = 0.0;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.05;
		h = w;
	}
};

enum {
	STATE_INTRO,
	STATE_INSTRUCTIONS,
	STATE_PLAYER_SELECT,
	STATE_SHOW_OBJECTIVES,
	STATE_PLAY,
	STATE_GAME_OVER
};

class Global {
public:
	int xres, yres;
    float pos[2];
    float w;
	float dir;
    int inside;
	unsigned int texid;
	unsigned int spriteid;
	unsigned int psid;
	Bee bees[2];
	Flt gravity;
	int frameno;
	Global() {
		xres = 400;
		yres = 200;
		// Box
		w = 20.0f;
		pos[0] = 0.0f + w;	
		pos[1] = yres/2.0f;	
		dir = 25.0f;
		inside = 0;
		gravity = 20.0;
		frameno = 0;
	};
} gl;

class Game {
public:
	int state;
	int score;
	int lives;
	int playtime;
	int countdown;
	int starttime;
	Game() {
		// Initialize game state
		state = STATE_INTRO;
		score = 0;
		lives = 3;
	}
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);

void *spriteThread(void *arg)
{
	//-----------------------------------------------------------------------------
	//Setup timers
	struct timespec start, end;
	extern double timeDiff(struct timespec *start, struct timespec *end);
	extern void timeCopy(struct timespec *dest, struct timespec *source);
	//-----------------------------------------------------------------------------
	clock_gettime(CLOCK_REALTIME, &start);
	double diff;
	while (1) {
		//If an amount of time has passed, change the frame number.
		clock_gettime(CLOCK_REALTIME, &end);
		diff = timeDiff(&start, &end);
		if (diff >= 0.05) {
			//Enough time has passed
			++gl.frameno;
			if (gl.frameno > 20) {
				//If frame number is 22 go back to 0
				gl.frameno = 1;
			}
			timeCopy(&start, &end);
		}
	}
	return (void*)0;
}

int main()
{
	//Start the thread
	//pthread_t sThread;
	//pthread_create(&sThread, NULL, spriteThread, NULL);

	init_opengl();
	//main game loop
	int done = 0;
	while (!done) {
		//process events...
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
		if (g.state == STATE_PLAY) {
			// Check Countdown Timer
			g.countdown = time(NULL) - g.starttime;
			if (g.countdown > g.playtime) {
				g.state = STATE_GAME_OVER;
			}
		}
		//You can call physics a number of times to smooth
		//out the process
		physics();           //move things
		render();            //draw things
		x11.swapBuffers();   //make video memory visible
		usleep(1000);        //pause to let X11 work better
	}
    cleanup_fonts();
	return 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = gl.xres, h = gl.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "Fossil Frenzy");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//window has been resized.
	gl.xres = width;
	gl.yres = height;
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	gl.bees[0].set_dimensions(gl.xres, gl.yres);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != gl.xres || xce.height != gl.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			int y = gl.yres - e->xbutton.y;
            int x = e->xbutton.x;
			if (g.state == STATE_INTRO) {

			}
			if (g.state == STATE_PLAY) {
				if (x >= gl.pos[0] - gl.w && x <= gl.pos[0] + gl.w) {
					if (y >= gl.pos[1] - gl.w && y <= gl.pos[1] + gl.w) {
						g.score += 1;

						//Check for Game Over
						if (g.score == 5) {
							g.state = STATE_GAME_OVER;
						}
                	}
            	}
			}       
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.
		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_s:
				//Key s was pressed
				if (g.state == STATE_INTRO) {
					g.state = STATE_PLAY;
					g.starttime = time(NULL);
					g.playtime = 10;
				}
				break;
			case XK_r:
				if (g.state == STATE_GAME_OVER) {
					//restart_game();
					g.score = 0;
					g.state = STATE_INTRO;
				}
				break;
			case XK_p:
				if (g.state == STATE_INTRO) {
					g.state = STATE_PLAYER_SELECT;
				}
				break;
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, gl.xres, gl.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
    // Allow 2D texture maps
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();

	//Background Image
	glGenTextures(1, &gl.texid);
	glBindTexture(GL_TEXTURE_2D, gl.texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, img.width, img.height, 0,
							GL_RGB, GL_UNSIGNED_BYTE, img.data);

	//Player Screen Image
	glGenTextures(1, &gl.psid);
	glBindTexture(GL_TEXTURE_2D, gl.psid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, ps.width, ps.height, 0,
							GL_RGB, GL_UNSIGNED_BYTE, ps.data);

	unsigned char *data2 = new unsigned char[sprite.width*sprite.height*4];
	for (int i = 0; i < sprite.height; i++) {
		for (int j = 0; j < sprite.width; j++) {
			int offset  = i*sprite.width*3 + j*3;
			int offset2 = i*sprite.width*4 + j*4;

			//data2 is using 4 elements of RGB + Alpha
			data2[offset2+0] = sprite.data[offset+0];
			data2[offset2+1] = sprite.data[offset+1];
			data2[offset2+2] = sprite.data[offset+2];
			data2[offset2+3] = ((unsigned char)sprite.data[offset+0] != 255 && 
								(unsigned char)sprite.data[offset+1] != 255 && 
								(unsigned char)sprite.data[offset+2] != 255);
		}
	}

	glGenTextures(1, &gl.spriteid);
	glBindTexture(GL_TEXTURE_2D, gl.spriteid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite.width, sprite.height, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, data2);
	delete [] data2;

	//Set the dimensions of the Bee
	gl.bees[0].set_dimensions(gl.xres, gl.yres);
}

void physics()
{
	// Movement of the Bee
	gl.bees[0].pos[0] += gl.bees[0].vel[0];
	gl.bees[0].pos[1] += gl.bees[0].vel[1];

    // Check the Bounderies
	if (gl.bees[0].pos[0] >= gl.xres) {
		gl.bees[0].pos[0] = gl.xres;
		gl.bees[0].vel[0] = 0.0;
	}
	if (gl.bees[0].pos[0] <= 0) {
		gl.bees[0].pos[0] = 0;
		gl.bees[0].vel[0] = 0.0;
	}
	if (gl.bees[0].pos[1] >= gl.yres) {
		gl.bees[0].pos[1] = gl.yres;
		gl.bees[0].vel[1] = 0.0;
	}
	if (gl.bees[0].pos[1] <= 0) {
		gl.bees[0].pos[1] = 0;
		gl.bees[0].vel[1] = 0.0;
	}
	
	//Move Bee towards flower
	Flt cx = gl.xres/2.0;
	Flt cy = gl.yres/2.0;

	//Points toward the second flower on flower.jpg
	//cx = gl.xres * (218.0 / 300.0);
	//dx = gl.yres * (86.0 / 169.0);

	Flt dx = cx - gl.bees[0].pos[0];
	Flt dy = cy - gl.bees[0].pos[1];
	Flt distance = (dx*dx + dy*dy);

	//If it goes near the center, it will
	//get a burst of velocity
	if (distance < 0.01) {
		distance = 0.01; // Clamp
	}

	//Change in velocity based on a force (gravity)
	//Multiply by an integer to make the Bee go faster
	gl.bees[0].vel[0] += (dx / distance) * gl.gravity;
	gl.bees[0].vel[1] += (dy / distance) * gl.gravity;

	//Creates random interferences
	//No repeat pattern
	gl.bees[0].vel[0] += ((Flt)rand() / (Flt)RAND_MAX) * 0.50 - 0.25;
	gl.bees[0].vel[1] += ((Flt)rand() / (Flt)RAND_MAX) * 0.50 - 0.25;
}

void render()
{	
    glClear(GL_COLOR_BUFFER_BIT);
	Rect r;

	if (g.state == STATE_INTRO) {
		// Show the Intro screen
		r.bot = gl.yres / 2;
		r.left = gl.xres / 2;
		r.center = 1;
		ggprint8b(&r, 30, 0x00ffffff, "Welcome to Fossil Frenzy!");
		ggprint8b(&r, 20, 0x00ff0000, "Press s to start");
		ggprint8b(&r, 0, 0x00ff0000, "Press p to go to player screen");
		return;
	}

	if (g.state == STATE_PLAYER_SELECT) {
		// Show the Player Selection screen
		glColor3ub(255, 255, 255); //Make it brighter

		glBindTexture(GL_TEXTURE_2D, gl.psid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,      0);
			glTexCoord2f(0, 0); glVertex2i(0,      gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		return;
	}

	if (g.state == STATE_PLAY) {
		// Show the Play screen
		r.bot = gl.yres - 20;
		r.left = 10;
		r.center = 0;
		ggprint8b(&r, 30, 0x00ffffff, "Score: %i", g.score);
		ggprint8b(&r, 0, 0x00ffff00, "Time: %i", g.playtime - g.countdown);		
		
		//Initialize Texture Map
		glColor3ub(255, 255, 255); //Make it brighter

		glBindTexture(GL_TEXTURE_2D, gl.texid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,      0);
			glTexCoord2f(0, 0); glVertex2i(0,      gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		// Bee
		glPushMatrix();
		glTranslatef(gl.bees[0].pos[0], gl.bees[0].pos[1], 0.0f);

		//Set Alpha Test
		//https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glAlphaFunc.xml
		glEnable(GL_ALPHA_TEST);

		//Transparent if alpha value is greater than 0.0
		glAlphaFunc(GL_GREATER, 0.0f);
		
		//Set 4-channels of color intensity
		glColor4ub(255, 255, 255, 255);

		glBindTexture(GL_TEXTURE_2D, gl.spriteid);

		//Make texture coordinates based on frame number
		float tx1 = 0.0f + (float)((gl.frameno-1) % 5) * 0.2f;
		float tx2 = tx1 + 0.2f;
		float ty1 = 0.0f + (float)((gl.frameno-1) / 5) *0.2f;
		float ty2 = ty1 + 0.2;

		//Change x-coords so that the bee flips when he turns
		if(gl.bees[0].vel[0] > 0.0) {
			float tmp = tx1;
			tx1 = tx2;
			tx2 = tmp;
		}

		glBegin(GL_QUADS);
			glTexCoord2f(tx1, ty2); glVertex2f(-gl.bees[0].w, -gl.bees[0].h);
			glTexCoord2f(tx1, ty1); glVertex2f(-gl.bees[0].w,  gl.bees[0].h);
			glTexCoord2f(tx2, ty1); glVertex2f( gl.bees[0].w,  gl.bees[0].h);
			glTexCoord2f(tx2, ty2); glVertex2f( gl.bees[0].w, -gl.bees[0].h);
		glEnd();

		//Turn off Alpha Test
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_ALPHA_TEST);
		glPopMatrix();
		return;
	}

	if (g.state == STATE_GAME_OVER) {
		// Show the Game Over screen
		r.bot = gl.yres / 2;
		r.left = gl.xres / 2;
		r.center = 1;
		ggprint8b(&r, 20, 0x00ffffff, "GAME OVER");
		ggprint8b(&r, 30, 0x00ff0000, "Your score: %i", g.score);
		ggprint8b(&r, 0, 0x00fff000, "Press r to restart");		
		return;
	}
}






