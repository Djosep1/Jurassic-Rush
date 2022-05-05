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

typedef double Flt;
struct Vector {
    float x, y, z;
};

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
  sprite("sprites/boy/run.png"),
  intro("pics/Dungeon.png");

enum {
	STATE_INTRO,
	STATE_INSTRUCTIONS,
	STATE_PLAYER_SELECT,
	STATE_PAUSE,
	STATE_PLAY,
	STATE_GAME_OVER
};

class Global {
public:
	int xres, yres;
	double sxres, syres;
	char keys[65536];
	float pos[2];
    float w;
	float r; //x value box
	float u; // y value box
	float dir;
    int inside;
	unsigned int texid;
	unsigned int spriteid;
	unsigned int psid;
	unsigned int introid;
	Flt gravity;
	int frameno;
	Global() {
		memset(keys, 0, sizeof(keys));
		xres = 400;
		yres = 200;
		sxres = (double)xres;
		syres = (double)yres;
		gravity = 0.5;
		// Box
		w = 20.0f;
		u = 40.0f;
		r = 10.0f;
		pos[0] = 0.0f + w;	
		pos[1] = yres/2.0f;	
		dir = 5.0f;
		inside = 0;
		gravity = 20.0;
		frameno = 0;
	};
} gl;

class Player {
public:
    Flt pos[3];
    Flt vel[3];
    float w, h;
    unsigned int color;
    bool alive_or_dead;
	Flt mass;
	Player() {
		w = h = 10.0;
		pos[0] = gl.xres/2;
		pos[1] = gl.yres/2;
		vel[0] = 4.0;
		vel[1] = 1.0;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.05;
		h = w;
	}
};

class Box {
public:
	Flt pos[3];
	Flt vel[3];
	float w, h;
	float dir;
	Box() {
		w = 40.0;
		h = 5.0;
		pos[0] = 0.0f + w;	
		pos[1] = gl.yres/4;
		dir = 0.2f;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.05;
		h = w;
	}
} b;

class Game {
public:
	Player players[2];
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
	void move_right() {
		players[0].pos[0] += 8.0;
		// Collision with right of screen
		if (players[0].pos[0] >= gl.xres-players[0].w) {
			players[0].pos[0] = gl.xres-players[0].w;
			players[0].vel[0] = 0.0;
		}
	}
	void move_left() {
		players[0].pos[0] -= 8.0;
		// Collision with left of screen
		if (players[0].pos[0] <= players[0].w) {
			players[0].pos[0] = players[0].w;
			players[0].vel[0] = 0.0;
		}
	}
	void move_up() {
		players[0].pos[1] += 8.0;
		// Collision with top of screen
		if (players[0].pos[1] >= gl.yres-players[0].h) {
			players[0].pos[1] = gl.yres-players[0].h;
			players[0].vel[1] = 0.0;
		}
	}
	void move_down() {
		players[0].pos[1] -= 8.0;
		// Collision with bottom of screen
		if (players[0].pos[1] <= players[0].h) {
			lives -= 1;
			players[0].pos[1] = players[0].h;
			players[0].vel[1] = 0.0;
		}
	}
	void jump () {
		players[0].pos[1] += 1.0;
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
	// struct timespec start, end;
	// extern double timeDiff(struct timespec *start, struct timespec *end);
	// extern void timeCopy(struct timespec *dest, struct timespec *source);
	//-----------------------------------------------------------------------------
	// clock_gettime(CLOCK_REALTIME, &start);
	// double diff;
	// while (true) {
	// 	//If an amount of time has passed, change the frame number.
	// 	clock_gettime(CLOCK_REALTIME, &end);
	// 	diff = timeDiff(&start, &end);

	// 	// How fast the frame changes
	// 	if (diff >= 0.0625) {
	// 		// Enough time has passed
	// 		++gl.frameno;
	// 		if (gl.frameno > 10) {
	// 			//If frame number is 10 go back to 1
	// 			gl.frameno = 1;
	// 		}
	// 		timeCopy(&start, &end);
	// 	}
	// }
	return (void*)0;
}

int main()
{
	//Start the thread
	pthread_t sThread;
	pthread_create(&sThread, NULL, spriteThread, NULL);

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
	XStoreName(dpy, win, "Jurassic Rush");
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
	g.players[0].set_dimensions(gl.xres, gl.yres);
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
						if (g.lives == 0) {
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
	if (e->type != KeyPress && e->type != KeyRelease) {
		return 0;
	}
	int key = XLookupKeysym(&e->xkey, 0);

	// Key Press for Global
	if (e->type == KeyPress) { gl.keys[key] = 1; }
	if (e->type == KeyRelease) { gl.keys[key] = 0; }

	if (e->type == KeyPress) {
		switch (key) {
			// Controls for game
			case XK_Return:
				if (g.state == STATE_INTRO) {
					g.state = STATE_PLAYER_SELECT;
				}
				break;
			case XK_1:
			case XK_2:
				//Key 2 was pressed
				if (g.state == STATE_PLAYER_SELECT) {
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

			// Controls for Player 1
			case XK_d:
			case XK_Right:
				if (g.state == STATE_PLAY) {
					//Move sprite to the right
					g.move_right();
				}
				break;
			case XK_a:
			case XK_Left:
				if (g.state == STATE_PLAY) {
					//Move sprite to the left
					g.move_left();
				}
				break;
			case XK_s:
			case XK_Down:
				if (g.state == STATE_PLAY) {
					//Move sprite down
					g.move_down();
				}
				break;
			case XK_w:
			case XK_Up:
				if (g.state == STATE_PLAY) {
					//Move sprite up
					g.move_up();
				}
				break;

			// Controls to quit game
			case XK_q:
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

unsigned char *buildAlphaData(Image *img)
{
	//Add 4th component to an RGB stream...
	//RGBA
	//When you do this, OpenGL is able to use the A component to determine
	//transparency information.
	//It is used in this application to erase parts of a texture-map from view.
	int i;
	int a,b,c;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	newdata = (unsigned char *)malloc(img->width * img->height * 4);
	ptr = newdata;
	for (i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		//-----------------------------------------------
		//get largest color component...
		//*(ptr+3) = (unsigned char)((
		//		(int)*(ptr+0) +
		//		(int)*(ptr+1) +
		//		(int)*(ptr+2)) / 3);
		//d = a;
		//if (b >= a && b >= c) d = b;
		//if (c >= a && c >= b) d = c;
		//*(ptr+3) = d;
		//-----------------------------------------------
		//this code optimizes the commented code above.
		//code contributed by student: Chris Smith
		//
		*(ptr+3) = (a != 255 && b != 255 && c );
		//-----------------------------------------------
		ptr += 4;
		data += 3;
	}
	return newdata;
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

	//Intro Image
	glGenTextures(1, &gl.introid);
	glBindTexture(GL_TEXTURE_2D, gl.introid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, intro.width, intro.height, 0,
							GL_RGB, GL_UNSIGNED_BYTE, intro.data);

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

	// Sprite Image
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
	g.players[0].set_dimensions(gl.xres, gl.yres);
}

void physics()
{
	// Gravity
	//g.players[0].vel[1] += gl.gravity;

    // Check the Players Bounderies
	if (g.players[0].pos[0] >= gl.xres) {
		g.players[0].pos[0] = gl.xres;
		g.players[0].vel[0] = 0.0;
	}
	if (g.players[0].pos[0] <= 0) {
		g.players[0].pos[0] = 0;
		g.players[0].vel[0] = 0.0;
	}
	if (g.players[0].pos[1] >= gl.yres) {
		g.players[0].pos[1] = gl.yres;
		g.players[0].vel[1] = 0.0;
	}
	if (g.players[0].pos[1] <= 0) {
		g.players[0].pos[1] = 0;
		g.players[0].vel[1] = 0.0;
	}

	// Check if player is colliding with a box

	// Collision Detection
	b.pos[0] += b.dir;
	if (b.pos[0] >= (gl.xres-b.w)) {
		b.pos[0] = (gl.xres-b.w);
		b.dir = -b.dir;
	}
	if (b.pos[0] <= b.w) {
		b.pos[0] = b.w;
		b.dir = -b.dir;
	}
}

void render()
{	
    glClear(GL_COLOR_BUFFER_BIT);
	Rect r;

	if (g.state == STATE_INTRO) {
		// Show the Intro screen
		// r.bot = gl.yres / 2;
		// r.left = gl.xres / 2;
		// r.center = 1;
		// ggprint8b(&r, 30, 0x00ffffff, "Welcome to Fossil Frenzy!");
		// ggprint8b(&r, 20, 0x00ff0000, "Press enter to start");
		glColor3ub(255, 255, 255); //Make it brighter
		glBindTexture(GL_TEXTURE_2D, gl.introid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,       0);
			glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		return;
	}

	if (g.state == STATE_PLAYER_SELECT) {
		// Show the Player Selection screen
		glColor3ub(255, 255, 255); //Make it brighter

		glBindTexture(GL_TEXTURE_2D, gl.psid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,       0);
			glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
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

		// Draw Player
		glPushMatrix();
		glTranslatef(g.players[0].pos[0], g.players[0].pos[1], 0.0f);

		//Set Alpha Test
		//https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glAlphaFunc.xml
		glEnable(GL_ALPHA_TEST);

		//Transparent if alpha value is greater than 0.0
		glAlphaFunc(GL_GREATER, 0.0f);
		
		//Set 4-channels of color intensity
		glColor4ub(255, 255, 255, 255);

		glBindTexture(GL_TEXTURE_2D, gl.spriteid);

		//Make texture coordinates based on frame number
		float tx1 = 0.0f + (float)((gl.frameno-1) % 5) * 0.2f; // Column
		float tx2 = tx1 + 0.2f;
		float ty1 = 0.0f + (float)((gl.frameno-1) / 2) * 0.5f; // Row
		float ty2 = ty1 + 0.5f;

		//Change x-coords so that the bee flips when he turns
		// if (g.players[0].pos[0] < 0.0) {
		// 	float tmp = tx2;
		// 	tx2 = tx1;
		// 	tx1 = tmp;
		// }

		glBegin(GL_QUADS);
			glTexCoord2f(tx1, ty2); glVertex2f(-g.players[0].w, -g.players[0].h);
			glTexCoord2f(tx1, ty1); glVertex2f(-g.players[0].w,  g.players[0].h);
			glTexCoord2f(tx2, ty1); glVertex2f( g.players[0].w,  g.players[0].h);
			glTexCoord2f(tx2, ty2); glVertex2f( g.players[0].w, -g.players[0].h);
		glEnd();

		//Turn off Alpha Test
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_ALPHA_TEST);
		glPopMatrix();

		// Draw Box
		glPushMatrix();
		glColor3ub(225, 173, 1);
		glTranslatef(b.pos[0], b.pos[1], 0.0f);
		glBegin(GL_QUADS);
			glVertex2f(-b.w, -b.h);
			glVertex2f(-b.w,  b.h);
			glVertex2f( b.w,  b.h);
			glVertex2f( b.w, -b.h);
		glEnd();
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






