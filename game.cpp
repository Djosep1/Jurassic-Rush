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
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include "fonts.h"
#include "timers.h"

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
			fin.open(newfile, ios::in|ios::out);
		} else {
			fin.open(fname);
		}
		char p6[10];
		fin >> p6;
		fin >> width >> height;
		fin >> max;
		//width *  height * the number of colors - r g b
		data = new char [width * height * 3];
		fin.read(data, 1); // Add data by 1 so that it can read max fully
		fin.read(data, width * height * 3); 
		fin.close();
		if (!isPPM) {
			unlink(newfile);
		}
	}
} 	img("pics/background.png"),
	screen("pics/Resolution_Screen.png"),
	ps("pics/Player_Screen.png"),
	sprite_idle("sprites/boy/idle.png"),
	sprite_run("sprites/boy/run.png"),
	sprite_jump("sprites/boy/jump.png"),
	intro("pics/Dungeon.png");

// Choose between a girl or boy player.
Image player[2] = {"sprites/boy/run.png", "sprites/girl/run.png"};
int nplayer = 0;

enum {
	STATE_INTRO,
	STATE_INSTRUCTIONS,
	STATE_PLAYER_SELECT,
	STATE_RESOLUTION,
	STATE_PAUSE,
	STATE_PLAY,
	STATE_GAME_OVER
};

class Global {
public:
	int xres, yres;
	double sxres, syres;
	char keys[65536];
    float w;
	float dir;
	Flt gravity;
	int frameno;
	int show;
	// Image ID's
	unsigned int texid;
	unsigned int psid;
	unsigned int introid;
	unsigned int screenid;
	unsigned int sprite_idleID;
	unsigned int sprite_runID;
	unsigned int sprite_jumpID;
	Global() {
		memset(keys, 0, sizeof(keys));
		// Odin
		//xres = 640;
		//yres = 480;
		xres = 1200;
		yres = 720;
		sxres = (double)xres;
		syres = (double)yres;
		gravity = 0.05f;
		frameno = 0;
		show = 0;
	};
} gl;


class Box {
public:
	Flt pos[2];
	Flt vel[2];
	float w, h;
	float dir;
	Box() {
		pos[0] = 0.0f;
		pos[1] = gl.yres/4;
		dir = 2.0f;
	}
	void reset() {
		pos[0] = 0.0f; 
		pos[1] = gl.yres/4;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.10f;
		h = (float)y * 0.025f;
	}
} b;

class Box2 {
public:
	Flt pos[2];
	Flt vel[2];
	float w, h;
	float dir;
	Box2() {
		pos[0] = gl.xres/1.1f;
		pos[1] = gl.yres/2;
		dir = 1.0f;
	}
	void reset() {
		pos[0] = gl.xres/1.1f;
		pos[1] = gl.yres/2;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.10f;
		h = (float)y * 0.025f;
	}
} b2;

class Player {
public:
    Flt pos[2];
    Flt vel[2];
    float w, h;
    unsigned int color;
    bool alive_or_dead;
	Flt mass;
	Player() {
		pos[0] = gl.xres/10;
		pos[1] = b.pos[1] + b.h;
		vel[0] = 0.0f;
		vel[1] = 0.0f;
	}
	void reset() {
		pos[0] = gl.xres/10;
		pos[1] = b.pos[1] + b.h;
		vel[0] = 0.0f;
		vel[1] = 0.0f;
	}
	void set_dimensions(int x, int y) {
		w = (float)x * 0.05;
		h = w;
	}
};

class Game {
public:
	Player players[2];
	Box boxes[5];
	int state;
	float score;
	int lives;
	int playtime;
	int countdown;
	int starttime;
	float position;
	bool isJumping;
	bool onPlatform;
	Game() {
		// Initialize game state
		state = STATE_INTRO;
		score = 0;
		lives = 1;
		position = 0.0f;
		isJumping = false;
		onPlatform = false;
	}
	void movement_controls() {
		// Move Left
		Flt speedMult = 1.5;
		if (gl.keys[XK_Left] || gl.keys[XK_a]) {
			position = -1.0f;
			players[0].pos[0] -= 1.5f * speedMult;
		}
		// Move Right
		if (gl.keys[XK_Right] || gl.keys[XK_d]) {
			position = 0.0f;
			players[0].pos[0] += 1.5f * speedMult;
		}
		// Fixed Height Jump
		if (gl.keys[XK_space] == 1) {
			if (!isJumping && onPlatform) {
				isJumping = true;
				players[0].vel[1] += 6.0f;
				if (players[0].vel[1] > 4.5f) {
					players[0].vel[1] = 4.5f;
				}
			}
		}
		players[0].vel[1] -= gl.gravity;
        players[0].pos[1] += players[0].vel[1];
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
void restartGame(void);

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
	while (true) {
		//If an amount of time has passed, change the frame number.
		clock_gettime(CLOCK_REALTIME, &end);
		diff = timeDiff(&start, &end);
		if ((gl.keys[XK_Left] || gl.keys[XK_a] || gl.keys[XK_Right] || gl.keys[XK_d]) && !g.isJumping) {
			// How fast the frame changes
			if (diff >= 0.2225) {
				// Enough time has passed
				++gl.frameno;
				if (gl.frameno > 10) {
					//If frame number is 10 go back to 1
					gl.frameno = 1;
				}
				timeCopy(&start, &end);
			}
		}
		if (gl.keys[XK_space]) {
			if (diff >= 0.2625) {
				++gl.frameno;
				if (gl.frameno > 1) {
					gl.frameno = 1;
				}
				timeCopy(&start, &end);
			}
		}
	}
	return (void*)0;
}

double physicsRate = 1.0 / 120.0;
const double applyphysicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
struct timespec ptimeStart, ptimeCurrent;
double applyPhysicsCountdown = 0.0;
double phystimeSpan = 0.0;

void applyPhysics()
{
	//-----------------------------------------------------------------------------
	//Setup timers
	//const double physicsRate = 1.0 / 60.0;
	//const double oobillion = 1.0 / 1e9;
	//struct timespec timeStart, timeCurrent;
	//struct timespec physStart, curr;
	//double applyPhysicsCountdown = 0.0;
	//static double physicsCountdown = 0.0;
	//double timeSpan;
	// extern double timeDiff(struct timespec *start, struct timespec *end);
	// extern void timeCopy(struct timespec *dest, struct timespec *source);
	//-----------------------------------------------------------------------------
	static int getInitStart = 1;
	if (getInitStart) {
		clock_gettime(CLOCK_REALTIME, &ptimeStart);
		getInitStart = 0;
	}

    clock_gettime(CLOCK_REALTIME, &ptimeCurrent);
    phystimeSpan = timeDiff(&ptimeStart, &ptimeCurrent);
    timeCopy(&ptimeStart, &ptimeCurrent);
    applyPhysicsCountdown += phystimeSpan;
	//printf("%f\n", applyPhysicsCountdown);
    while(applyPhysicsCountdown >= physicsRate) {
        physics();
		//printf("%f\n", applyPhysicsCountdown);
        applyPhysicsCountdown -= physicsRate;
    }

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
		if (g.state == STATE_PLAY) {
			// Check Countdown Timer
			g.countdown = time(NULL) - g.starttime;
			if (g.countdown > g.playtime) {
				g.state = STATE_GAME_OVER;
			}
			if (g.players[0].pos[1] <= g.players[0].h) {
				g.lives -= 1;
			}
		}
		//You can call physics a number of times to smooth out the process
		applyPhysics();           //move things
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
	b.set_dimensions(gl.xres, gl.yres);
	b2.set_dimensions(gl.xres, gl.yres);
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
			//int y = gl.yres - e->xbutton.y;
            //int x = e->xbutton.x;
			if (g.state == STATE_RESOLUTION) {
				
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
				if (g.state == STATE_INSTRUCTIONS) {
					g.state = STATE_PLAY;
					g.starttime = time(NULL);
					g.playtime = 60;
				}
				if (g.state == STATE_INTRO) {
					g.state = STATE_INSTRUCTIONS;
				}
				break;
			case XK_1:
			case XK_2:
				//Key 2 was pressed
				if (g.state == STATE_PLAYER_SELECT) {
					g.state = STATE_PLAY;
					g.starttime = time(NULL);
					g.playtime = 30;
				}
				break;
			case XK_r:
				if (g.state == STATE_GAME_OVER) {
					restartGame();
				}
				break;
			case XK_b:
				// Show Bounding Boxes
				gl.show ^= 1;
				break;
			case XK_l:
				// Show Resolution Screen
				if (g.state == STATE_INTRO) {
					g.state = STATE_RESOLUTION;
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

void sprite_image(Image sprite, unsigned int sprite_action, unsigned char *data2) {
	// Sprite Image
	data2 = new unsigned char[sprite.width*sprite.height*4];
	for (int i = 0; i < sprite.height; i++) {
		for (int j = 0; j < sprite.width; j++) {
			int offset  = i*sprite.width*3 + j*3;
			int offset2 = i*sprite.width*4 + j*4;

			//data2 is using 4 elements of RGB + Alpha
			data2[offset2+0] = sprite.data[offset+0];
			data2[offset2+1] = sprite.data[offset+1];
			data2[offset2+2] = sprite.data[offset+2];
			data2[offset2+3] = ((unsigned char)sprite.data[offset+0] != 153 && 
								(unsigned char)sprite.data[offset+1] != 153 && 
								(unsigned char)sprite.data[offset+2] != 153);
		}
	}

	glGenTextures(1, &sprite_action);
	glBindTexture(GL_TEXTURE_2D, sprite_action);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite.width, sprite.height, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, data2);
	delete [] data2;
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

	// Resolution Image
	glGenTextures(1, &gl.screenid);
	glBindTexture(GL_TEXTURE_2D, gl.screenid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, screen.width, screen.height, 0,
							GL_RGB, GL_UNSIGNED_BYTE, screen.data);

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

	// Sprite Idle Image 
	unsigned char *data2 = new unsigned char[sprite_idle.width*sprite_idle.height*4];
	for (int i = 0; i < sprite_idle.height; i++) {
		for (int j = 0; j < sprite_idle.width; j++) {
			int offset  = i*sprite_idle.width*3 + j*3;
			int offset2 = i*sprite_idle.width*4 + j*4;

			//data2 is using 4 elements of RGB + Alpha
			data2[offset2+0] = sprite_idle.data[offset+0];
			data2[offset2+1] = sprite_idle.data[offset+1];
			data2[offset2+2] = sprite_idle.data[offset+2];
			data2[offset2+3] = ((unsigned char)sprite_idle.data[offset+0] != 153 && 
								(unsigned char)sprite_idle.data[offset+1] != 153 && 
								(unsigned char)sprite_idle.data[offset+2] != 153);
		}
	}

	glGenTextures(1, &gl.sprite_idleID);
	glBindTexture(GL_TEXTURE_2D, gl.sprite_idleID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite_idle.width, sprite_idle.height, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, data2);
	delete [] data2;

	// Sprite Run Image 
	unsigned char *data3 = new unsigned char[sprite_run.width*sprite_run.height*4];
	for (int i = 0; i < sprite_run.height; i++) {
		for (int j = 0; j < sprite_run.width; j++) {
			int offset  = i*sprite_run.width*3 + j*3;
			int offset2 = i*sprite_run.width*4 + j*4;

			//data2 is using 4 elements of RGB + Alpha
			data3[offset2+0] = sprite_run.data[offset+0];
			data3[offset2+1] = sprite_run.data[offset+1];
			data3[offset2+2] = sprite_run.data[offset+2];
			data3[offset2+3] = ((unsigned char)sprite_run.data[offset+0] != 153 && 
								(unsigned char)sprite_run.data[offset+1] != 153 && 
								(unsigned char)sprite_run.data[offset+2] != 153);
		}
	}

	glGenTextures(1, &gl.sprite_runID);
	glBindTexture(GL_TEXTURE_2D, gl.sprite_runID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite_run.width, sprite_run.height, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, data3);
	delete [] data3;

	// Sprite Run Image 
	unsigned char *data4 = new unsigned char[sprite_jump.width*sprite_jump.height*4];
	for (int i = 0; i < sprite_jump.height; i++) {
		for (int j = 0; j < sprite_jump.width; j++) {
			int offset  = i*sprite_jump.width*3 + j*3;
			int offset2 = i*sprite_jump.width*4 + j*4;

			//data2 is using 4 elements of RGB + Alpha
			data4[offset2+0] = sprite_jump.data[offset+0];
			data4[offset2+1] = sprite_jump.data[offset+1];
			data4[offset2+2] = sprite_jump.data[offset+2];
			data4[offset2+3] = ((unsigned char)sprite_jump.data[offset+0] != 153 && 
								(unsigned char)sprite_jump.data[offset+1] != 153 && 
								(unsigned char)sprite_jump.data[offset+2] != 153);
		}
	}

	glGenTextures(1, &gl.sprite_jumpID);
	glBindTexture(GL_TEXTURE_2D, gl.sprite_jumpID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite_jump.width, sprite_jump.height, 0,
									GL_RGBA, GL_UNSIGNED_BYTE, data4);
	delete [] data4;

	//Set the dimensions of the sprite and box
	g.players[0].set_dimensions(gl.xres, gl.yres);
	b.set_dimensions(gl.xres, gl.yres);
	b2.set_dimensions(gl.xres, gl.yres);
}

void restartGame()
{
	//Reset the game
	g.score = 0;
	g.lives = 1;
	g.state = STATE_INTRO;
	g.players[0].reset();
	b.reset();
	b2.reset();
}

void physics()
{
	if (g.state == STATE_PLAY) {
		// Movement Controls
		g.movement_controls();
		b.pos[0] += b.dir;
		b2.pos[0] -= b2.dir;
	}

	g.isJumping = false;
	g.onPlatform = false;

    // Check the Players Boundaries
	// Collision with right of screen
	if (g.players[0].pos[0] >= gl.xres-g.players[0].w) {
		g.players[0].pos[0] = gl.xres-g.players[0].w;
		g.players[0].vel[0] = 0.0;
	}
	// Collision with left of screen
	if (g.players[0].pos[0] <= g.players[0].w) {
		g.players[0].pos[0] = g.players[0].w;
		g.players[0].vel[0] = 0.0;
	}
	// Collision with top of screen
	if (g.players[0].pos[1] >= gl.yres-g.players[0].h) {
		g.players[0].pos[1] = gl.yres-g.players[0].h;
		g.players[0].vel[1] = 0.0;
	}
	// Collision with bottom of screen
	if (g.players[0].pos[1] <= g.players[0].h) {
		g.players[0].pos[1] = g.players[0].h;
		g.players[0].vel[1] = 0.0;
		//g.lives -= 1;
	}
	
	// Collision with right side of screen
	if (b.pos[0] >= (gl.xres+b.w)) {
		b.pos[0] = (gl.xres+b.w);
		b.dir = -b.dir;
	}
	// Collision with left side of screen
	if (b.pos[0] <= 0-b.w) {
		b.pos[0] = 0-b.w;
		b.dir = -b.dir;
	}

	// Collision with right side of screen
	if (b2.pos[0] >= (gl.xres+b2.w)) {
		b2.pos[0] = (gl.xres+b2.w);
		b2.dir = -b2.dir;
	}
	// Collision with left side of screen
	if (b2.pos[0] <= 0-b2.w) {
		b2.pos[0] = 0-b2.w;
		b2.dir = -b2.dir;
	}

	// Get the hitbox sizes of the player and the box platform
    Flt boxLeft   = b.pos[0] - b.w;
    Flt boxRight  = b.pos[0] + b.w;
    Flt boxTop    = b.pos[1] + b.h;
    Flt boxBottom = b.pos[1] - b.h;

	Flt boxLeft2   = b2.pos[0] - b2.w;
    Flt boxRight2  = b2.pos[0] + b2.w;
    Flt boxTop2    = b2.pos[1] + b2.h;
    Flt boxBottom2 = b2.pos[1] - b2.h;

    // Flt playerLeft   = g.players[0].pos[0] - g.players[0].w;
    // Flt playerRight  = g.players[0].pos[0] + g.players[0].w;
    Flt playerTop    = g.players[0].pos[1] + g.players[0].h;
    Flt playerBottom = g.players[0].pos[1] - g.players[0].h;

    // Check if the player is on the box platform
	if (g.players[0].pos[0] >= boxLeft && g.players[0].pos[0] <= boxRight && playerBottom <= boxTop && playerTop >= boxBottom) {
		// if ((playerLeft <= boxLeft && playerRight >= boxLeft && playerTop >= boxTop) || 
		// (playerLeft >= boxRight && playerRight <= boxRight && playerTop >= boxTop)) {
		// if (playerLeft <= boxLeft && playerRight <= boxRight && playerTop >= boxTop) {
		// g.players[0].pos[0] = boxLeft;

		// Centers the player on the platform and moves with it
		// g.players[0].pos[0] -= g.players[0].pos[0] - b.pos[0];
		g.onPlatform = true;
		g.players[0].pos[1] = boxTop + g.players[0].h;
        g.players[0].vel[1] = 0.0;
		g.score += 0.01;
    }

	if (g.players[0].pos[0] >= boxLeft2 && g.players[0].pos[0] <= boxRight2 && playerBottom <= boxTop2 && playerTop >= boxBottom2) {
		// if ((playerLeft <= boxLeft && playerRight >= boxLeft && playerTop >= boxTop) || 
		// (playerLeft >= boxRight && playerRight <= boxRight && playerTop >= boxTop)) {
		// if (playerLeft <= boxLeft && playerRight <= boxRight && playerTop >= boxTop) {
		// g.players[0].pos[0] = boxLeft;

		// Centers the player on the platform and moves with it
		// g.players[0].pos[0] -= g.players[0].pos[0] - b.pos[0];
		g.onPlatform = true;
		g.players[0].pos[1] = boxTop2 + g.players[0].h;
        g.players[0].vel[1] = 0.0;
		g.score += 0.01;
    }
}

void sprite_id(unsigned int);

void render()
{	
    glClear(GL_COLOR_BUFFER_BIT);
	Rect r;

	if (g.state == STATE_INTRO) {
		// Show the Intro screen

		glColor3ub(255, 255, 255); //Make it brighter
		glBindTexture(GL_TEXTURE_2D, gl.introid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,       0);
			glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		// r.bot = gl.yres - 20;
		// r.left = 10;
		// r.center = 0;
		// ggprint8b(&r, 20, 0x00ffffff, "Press AD or arrow keys to move");
		// ggprint8b(&r, 20, 0x00ffffff, "Press space to jump");
		// ggprint8b(&r, 20, 0x00ffffff, "Goal is to stay on the platforms as long as possible");
		return;
	}

	if (g.state == STATE_INSTRUCTIONS) {
		// Show the Player Selection screen
		// glColor3ub(255, 255, 255); //Make it brighter

		// glBindTexture(GL_TEXTURE_2D, gl.psid);
		// glBegin(GL_QUADS);
		// 	glTexCoord2f(0, 1); glVertex2i(0,       0);
		// 	glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
		// 	glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
		// 	glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		// glEnd();
		// glBindTexture(GL_TEXTURE_2D, 0);

		glColor3ub(0, 0, 0); 

		r.bot = gl.yres / 2;
		r.left = gl.xres / 2;
		r.center = 1;
		ggprint8b(&r, 20, 0x00ffffff, "Goal is to stay on the platforms as long as possible");
		ggprint8b(&r, 40, 0x00ffffff, "Press AD keys or arrow keys to move and space to jump");
		ggprint8b(&r, 20, 0x00ffffff, "Press ENTER to start");
		return;
	}

	if (g.state == STATE_RESOLUTION) {
		// Show the Resolution Selection screen
		
		glColor3ub(255, 255, 255); // Make it White

		glBindTexture(GL_TEXTURE_2D, gl.screenid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,       0);
			glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glPushMatrix();
		glTranslatef(gl.xres/6, gl.yres/2, 0.0f);
		
		glColor3ub(255, 255, 0); // Make it Pink
		glBegin(GL_LINE_LOOP);
			glVertex2f(-gl.xres/12, -gl.yres/18);
			glVertex2f(-gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12, -gl.yres/18);
		glEnd();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(gl.xres/2, gl.yres/2, 0.0f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(-gl.xres/12, -gl.yres/18);
			glVertex2f(-gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12, -gl.yres/18);
		glEnd();
		glPopMatrix();

		glPushMatrix();
		glTranslatef(gl.xres/1.2, gl.yres/2, 0.0f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(-gl.xres/12, -gl.yres/18);
			glVertex2f(-gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12,  gl.yres/18);
			glVertex2f( gl.xres/12, -gl.yres/18);
		glEnd();

		glPopMatrix();
		return;
	}

	if (g.state == STATE_PLAY) {
		// Show the Play screen	

		//Initialize Texture Map
		glColor3ub(255, 255, 255); //Make it brighter

		glBindTexture(GL_TEXTURE_2D, gl.texid);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex2i(0,       0);
			glTexCoord2f(0, 0); glVertex2i(0,       gl.yres);
			glTexCoord2f(1, 0); glVertex2i(gl.xres, gl.yres);
			glTexCoord2f(1, 1); glVertex2i(gl.xres, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		// Show the Score Box
		glColor4ub(0, 0, 0, 120); //Make it brighter
		glPushMatrix();
		glTranslatef(gl.xres/99, gl.yres * 0.99, 0.0f);
		glBegin(GL_QUADS);
			glVertex2f(-60, -50);
			glVertex2f(-60,  50);
			glVertex2f( 60,  50);
			glVertex2f( 60, -50);
		glEnd();
		glPopMatrix();

		r.bot = gl.yres - 20;
		r.left = 10;
		r.center = 0;
		ggprint8b(&r, 20, 0x00ffffff, "Score: %0.0f", g.score);
		ggprint8b(&r, 0, 0x00ffff00, "Time: %i", g.playtime - g.countdown);   

		// Draw Player
		if (gl.keys[XK_Left] || gl.keys[XK_a] || gl.keys[XK_Right] || gl.keys[XK_d]) {
			sprite_id(gl.sprite_runID);
		} else if (gl.keys[XK_space] == 1) {
			sprite_id(gl.sprite_jumpID);
		} else {
			sprite_id(gl.sprite_idleID);
		}

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

		if (gl.show) {
			// Check the bounding box for the box
			glColor3ub(255, 255, 0); // Make it Yellow
			glBegin(GL_LINE_LOOP);
				glVertex2f(-b.w, -b.h);
				glVertex2f(-b.w,  b.h);
				glVertex2f( b.w,  b.h);
				glVertex2f( b.w, -b.h);
			glEnd();
		}
		glPopMatrix();

		// Draw Box 2
		glPushMatrix();
		glColor3ub(225, 173, 1);
		glTranslatef(b2.pos[0], b2.pos[1], 0.0f);
		glBegin(GL_QUADS);
			glVertex2f(-b2.w, -b2.h);
			glVertex2f(-b2.w,  b2.h);
			glVertex2f( b2.w,  b2.h);
			glVertex2f( b2.w, -b2.h);
		glEnd();

		if (gl.show) {
			// Check the bounding box for the box
			glColor3ub(255, 255, 0); // Make it Yellow
			glBegin(GL_LINE_LOOP);
				glVertex2f(-b2.w, -b2.h);
				glVertex2f(-b2.w,  b2.h);
				glVertex2f( b2.w,  b2.h);
				glVertex2f( b2.w, -b2.h);
			glEnd();
		}
		glPopMatrix();

		//Check for Game Over
		if (g.lives == 0) {
			g.state = STATE_GAME_OVER;
		}

		return;
	}

	if (g.state == STATE_GAME_OVER) {
		// Show the Game Over screen
		r.bot = gl.yres / 2;
		r.left = gl.xres / 2;
		r.center = 1;
		ggprint8b(&r, 20, 0x00ffffff, "GAME OVER");
		ggprint8b(&r, 30, 0x00ff0000, "Your score: %0.0f", g.score);
		ggprint8b(&r, 0, 0x00fff000, "Press r to restart");		
		return;
	}
}

void sprite_id(unsigned int id) {
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

		glBindTexture(GL_TEXTURE_2D, id);

		//Make texture coordinates based on frame number
		float tx1 = 0.0f + (float)((gl.frameno-1) % 5) * 0.2f; // Column
		float tx2 = tx1 + 0.2f;
		float ty1 = 0.0f + (float)((gl.frameno-1) / 2) * 0.5f; // Row
		float ty2 = ty1 + 0.5f;

		//Change x-coords so that the bee flips when he turns
		if (g.position < 0.0) {
			float tmp = tx2;
			tx2 = tx1;
			tx1 = tmp;
		}

		glBegin(GL_QUADS);
			glTexCoord2f(tx1, ty2); glVertex2f(-g.players[0].w, -g.players[0].h);
			glTexCoord2f(tx1, ty1); glVertex2f(-g.players[0].w,  g.players[0].h);
			glTexCoord2f(tx2, ty1); glVertex2f( g.players[0].w,  g.players[0].h);
			glTexCoord2f(tx2, ty2); glVertex2f( g.players[0].w, -g.players[0].h);
		glEnd();

		//Turn off Alpha Test
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_ALPHA_TEST);

		if (gl.show) {
			// Check the bounding box for the player
			glColor3ub(255, 255, 0);
			glBegin(GL_LINE_LOOP);
				glVertex2f(-g.players[0].w, -g.players[0].h);
				glVertex2f(-g.players[0].w,  g.players[0].h);
				glVertex2f( g.players[0].w,  g.players[0].h);
				glVertex2f( g.players[0].w, -g.players[0].h);
			glEnd();
		}

		glPopMatrix();
}





