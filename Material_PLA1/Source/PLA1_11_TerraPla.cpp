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

SDL_Surface* texture;
// setup 3D data
VECTOR A, B, C;

bool initSDL();
void update();
void render();

void close();
void waitTime();

void initPlane();
void updatePlane();
void renderPlane();

void DrawPlane(VECTOR Bp, VECTOR Up, VECTOR Vp);

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
		initPlane();

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
	updatePlane();
}

void render() {

	renderPlane();
}

void close() {
	SDL_FreeSurface(texture);
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

void initPlane() {
	SDL_Surface *temp = IMG_Load("texture.png");
	if (temp == NULL) {
		std::cout << "Image can be loaded! " << IMG_GetError();
		close();
		exit(1);
	}
	texture = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);

}

void updatePlane() {
	// setup the 3 control points of our plane, could do something much
	// more dynamic (with rotX also) but this looks good enough
	A = VECTOR((float)(currentTime) / 50, 8, (float)(currentTime) / 10);
	B = rotY(0.32) * VECTOR(256, 0, 0);
	C = rotY(0.32) * VECTOR(0, 0, 256);
}

void renderPlane() {
	// draw plane
	DrawPlane(A, B, C);
}

/*
* draw a perspective correctly textured plane
*/
void DrawPlane(VECTOR Bp, VECTOR Up, VECTOR Vp)
{
	// compute the 9 magic constants
	float Cx = Up[1] * Vp[2] - Vp[1] * Up[2],
		Cy = Vp[0] * Up[2] - Up[0] * Vp[2],
		// the 500 represents the distance of the projection plane
		// change it to modify the field of view
		Cz = (Up[0] * Vp[1] - Vp[0] * Up[1]) * 500,
		Ax = Vp[1] * Bp[2] - Bp[1] * Vp[2],
		Ay = Bp[0] * Vp[2] - Vp[0] * Bp[2],
		Az = (Vp[0] * Bp[1] - Bp[0] * Vp[1]) * 500,
		Bx = Bp[1] * Up[2] - Up[1] * Bp[2],
		By = Up[0] * Bp[2] - Bp[0] * Up[2],
		Bz = (Bp[0] * Up[1] - Up[0] * Bp[1]) * 500;
	// only render the lower part of the plane, looks ugly above
	SDL_LockSurface(screenSurface);
	for (int j = (SCREEN_HEIGHT / 2); j<SCREEN_HEIGHT; j++)
	{
		// compute the (U,V) coordinates for the start of the line
		float a = Az + Ay * (j - (SCREEN_HEIGHT / 2)) + Ax * -((SCREEN_WIDTH / 2) + 1),
			b = Bz + By * (j - (SCREEN_HEIGHT / 2)) + Bx * -((SCREEN_WIDTH / 2) + 1),
			c = Cz + Cy * (j - (SCREEN_HEIGHT / 2)) + Cx * -((SCREEN_WIDTH / 2) + 1),
			ic;
		// quick distance check, if it's too far reduce it
		if (fabs(c)>65536) ic = 1 / c; else ic = 1 / 65536;
		// compute original (U,V)
		Uint32 u = (int)(a * 16777216 * ic),
			v = (int)(b * 16777216 * ic),
			// and the deltas we need to interpolate along this line
			du = (int)(16777216 * Ax * ic),
			dv = (int)(16777216 * Bx * ic);
		// setup the offsets in the buffers
		Uint8 *dst;
		Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
		int bpp = screenSurface->format->BytesPerPixel;
		Uint8 *imagebuffer = (Uint8 *)texture->pixels;
		int bppImage = texture->format->BytesPerPixel;
		dst = initbuffer + j *screenSurface->pitch;
		// start the loop
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// load texel and store pixel
			Uint8 *p = (Uint8 *)imagebuffer + ((v >> 16) & 0xff) * texture->pitch + ((u >> 16) & 0xff) * bppImage;
			// copy it to the screen
			*(Uint32 *)dst = *(Uint32 *)p;
			// next pixel
			dst += bppImage;
			// interpolate
			u += du;
			v += dv;
		}
	}
	SDL_UnlockSurface(screenSurface);
}
