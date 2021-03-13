#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <cmath>

#include "vector.h"
#include "matrix.h"

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//The window we'll be rendering to
SDL_Window* window = NULL;
//The surface contained by the window
SDL_Surface* screenSurface = NULL;

#define FPS 60
int lastTime = 0, currentTime, deltaTime;
float msFrame = 1 / (FPS / 1000.0f);

// number of points in the object
#define MAXPTS 2048
// matrix that describes the rotation of the object
MATRIX obj;
// how far it is from the viewer
float base_dist;
// points of our object
VECTOR *pts;
// store the precalculated scaling values
int scaleX[SCREEN_WIDTH];
int scaleY[SCREEN_HEIGHT];
// Second buffer for effects
SDL_Surface *secondScreen;


bool initSDL();
void update();
void render();

void close();
void waitTime();

void initParticles();
void updateParticles();
void renderParticles();

void Rescale(SDL_Surface *src, SDL_Surface *dst);
void Blur(SDL_Surface *src, SDL_Surface *dst);
void Draw(SDL_Surface *where, VECTOR v);

int main( int argc, char* args[] )
{
	//Start up SDL and create window
	if (!initSDL())
	{
		std::cout << "Failed to initialize!\n";
		return 1;
	}
	else
	{
		IMG_Init(IMG_INIT_PNG);
		initParticles();

		//Main loop flag
		bool quit = false;

		//Event handler
		SDL_Event e;

		//While application is running
		while (!quit)
		{
			//Handle events on queue
			while (SDL_PollEvent(&e) != 0)
			{
				if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
						quit = true;
					}
				}
				//User requests quit
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
			}

			// updates all
			update();

			//Render
			render();

			//Update the surface
			SDL_UpdateWindowSurface(window);
			waitTime();
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}


bool initSDL() {

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "SDL could not initialize! SDL_Error: %s\n" << SDL_GetError();
		return false;
	}
	//Create window
	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	if (window == NULL)
	{
		std::cout << "Window could not be created! SDL_Error: %s\n" << SDL_GetError();
		return false;
	}
	//Get window surface
	screenSurface = SDL_GetWindowSurface(window);
	return true;
}

void update(){
	updateParticles();
}

void render() {

	renderParticles();
}

void close() {
	SDL_FreeSurface(secondScreen);
	//Destroy window
	SDL_DestroyWindow(window);
	//Quit SDL subsystems
	SDL_Quit();
}

void waitTime() {
	currentTime = SDL_GetTicks();
	deltaTime = currentTime - lastTime;
	if (deltaTime < (int)msFrame) {
		SDL_Delay((int)msFrame - deltaTime);
	}
	lastTime = currentTime;

}

void initParticles() {
	// generate our points
	pts = new VECTOR[MAXPTS];
	for (int i = 0; i < MAXPTS; i++) {
		pts[i] = (rotX(2.0f*M_PI*sin((float)i / 203))
			* rotY(2.0f*M_PI*cos((float)i / 157))
			* rotZ(-2.0f*M_PI*cos((float)i / 181))) * VECTOR(64 + 16 * sin((float)i / 191), 0, 0);
	}
	// create the second buffer for effects
	secondScreen =SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
//	secondScreen = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, screenSurface->format->format);
}

void updateParticles() {
	// recompute parameters for image rescaling
	int sx = (int)((SCREEN_WIDTH / 2) - (SCREEN_WIDTH / 4) * sin((float)currentTime / 5590)),
		sy = (int)((SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4) * sin((float)currentTime / 6110));
	for (int i = 0; i < SCREEN_WIDTH; i++) scaleX[i] = (int)(sx + (i - sx)*0.85f);
	for (int i = 0; i < SCREEN_HEIGHT; i++) scaleY[i] = (int)(sy + (i - sy)*0.85f);
	// setup the position of the object
	base_dist = 128 + 64 * sin((float)currentTime / 3200);
	obj = rotX(2.0f*M_PI*sin((float)currentTime / 2800))
		* rotY(2.0f*M_PI*cos((float)currentTime / 3000))
		* rotZ(-2.0f*M_PI*sin((float)currentTime / 2500));
}

void renderParticles() {
	//SDL_LockSurface(screenSurface);

	// rescale the image
	Rescale(screenSurface, secondScreen);
	//SDL_BlitSurface(screenSurface, NULL, secondScreen, NULL);
	// blur it
	Blur(secondScreen, screenSurface);
	//SDL_BlitSurface(secondScreen, NULL, screenSurface, NULL);

	// draw the particles
	for (int i = 0; i < MAXPTS; i++) {
		Draw(screenSurface, obj * pts[i]);
	}

	//SDL_UnlockSurface(screenSurface);
}

/*
* scale an image away from a given point
*/
void Rescale(SDL_Surface *src, SDL_Surface *dst)
{
	Uint8 *dstPixel;
	Uint8 *initbuffer = (Uint8 *)src->pixels;
	Uint8 *finalbuffer = (Uint8 *)dst->pixels;
	int initbufferbpp = src->format->BytesPerPixel;
	int finalbufferbpp = dst->format->BytesPerPixel;
	for (int j = 0; j<SCREEN_HEIGHT; j++)
	{
		dstPixel = finalbuffer + j * dst->pitch;
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// get value from pixel in scaled image, and store
			SDL_Color resultColor;
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + scaleY[j] * src->pitch + scaleX[i] * initbufferbpp), src->format, &resultColor.r, &resultColor.g, &resultColor.b);
			*(Uint32 *)dstPixel = SDL_MapRGB(dst->format, resultColor.r, resultColor.g, resultColor.b);
			dstPixel += finalbufferbpp;
		}
	}
}

/*
* smooth a buffer
*/
void Blur(SDL_Surface *src, SDL_Surface *dst)
{
	Uint8 *dstPixel;
	Uint8 *initbuffer = (Uint8 *)src->pixels;
	Uint8 *finalbuffer = (Uint8 *)dst->pixels;
	int initbufferbpp = src->format->BytesPerPixel;
	int finalbufferbpp = dst->format->BytesPerPixel;

	//first line black
	memset(finalbuffer, 0, SCREEN_WIDTH*finalbufferbpp);

	for (int j = 1; j<(SCREEN_HEIGHT-1); j++)
	{
		dstPixel = finalbuffer + j * dst->pitch;
		// set first pixel of the line to 0
		*(Uint32 *)dstPixel = 0;
		dstPixel += finalbufferbpp;
		// calculate the filter for all the other pixels
		for (int i = 1; i<(SCREEN_WIDTH-1); i++)
		{
			// calculate the average
			SDL_Color resultColor[8];
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j - 1) * src->pitch + (i - 1) * initbufferbpp), src->format, &resultColor[0].r, &resultColor[0].g, &resultColor[0].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j - 1) * src->pitch + (i)     * initbufferbpp), src->format, &resultColor[1].r, &resultColor[1].g, &resultColor[1].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j - 1) * src->pitch + (i + 1) * initbufferbpp), src->format, &resultColor[2].r, &resultColor[2].g, &resultColor[2].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j)     * src->pitch + (i - 1) * initbufferbpp), src->format, &resultColor[3].r, &resultColor[3].g, &resultColor[3].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j)     * src->pitch + (i + 1) * initbufferbpp), src->format, &resultColor[4].r, &resultColor[4].g, &resultColor[4].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j + 1) * src->pitch + (i - 1) * initbufferbpp), src->format, &resultColor[5].r, &resultColor[5].g, &resultColor[5].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j + 1) * src->pitch + (i)     * initbufferbpp), src->format, &resultColor[6].r, &resultColor[6].g, &resultColor[6].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)src->pixels + (j + 1) * src->pitch + (i + 1) * initbufferbpp), src->format, &resultColor[7].r, &resultColor[7].g, &resultColor[7].b);
			SDL_Color medianColor = { 0,0,0 };
			for (int c=0; c < 8;c++){
				medianColor.r += resultColor[c].r;
				medianColor.g += resultColor[c].g;
				medianColor.b += resultColor[c].b;
			}
			medianColor.r = medianColor.r / 8;
			medianColor.g = medianColor.g / 8;
			medianColor.b = medianColor.b / 8;
			// store the pixel
			Uint32 finalColor = SDL_MapRGB(dst->format, medianColor.r, medianColor.g, medianColor.b);
			*(Uint32 *)dstPixel = finalColor;
			dstPixel += finalbufferbpp;
		}
		// set last pixel of the line to 0
		*(Uint32 *)dstPixel = 0;
		dstPixel += finalbufferbpp;
	}
}

/*
* draw one single particle
*/
void Draw(SDL_Surface *where, VECTOR v)
{
	// calculate the screen coordinates of the particle
	float iz = 1 / (v[2] + base_dist),
		x = (SCREEN_WIDTH / 2) + (SCREEN_WIDTH / 2) * v[0] * iz,
		y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 2) * v[1] * iz;
	// clipping
	if ((x<0) || (x>(SCREEN_WIDTH - 1)) || (y<0) || (y>(SCREEN_HEIGHT -1))) return;
	// convert to fixed point to help antialiasing
	int sx = (int)(x*8.0f),
		sy = (int)(y*8.0f);
	// commpute color with Z
	Uint32 colorZ = (int)(iz * 20000);
	if (colorZ > 255) colorZ = 255;
	Uint32 color = 0xFF000000 + (colorZ  << 16) + (colorZ << 8) + colorZ;
	// draw particle
	*((Uint32*)((Uint8*)where->pixels + (sy / 8) * where->pitch + (sx / 8) * where->format->BytesPerPixel)) = color;
}
