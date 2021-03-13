#include <SDL.h>
#include <iostream>
#include <cmath>

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

// the two function buffers sized SCREEN_WIDTH * SCREEN_HEIGHT * 4
unsigned char *plasma1;
unsigned char *plasma2;
// define the plasma movement
int Windowx1, Windowy1, Windowx2, Windowy2;
long src1, src2;

struct RGBColor { unsigned char R, G, B; };
RGBColor palette[256];

bool initSDL();
void update();
void render();

void close();
void waitTime();
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void initPlasma();
void updatePlasma();
void renderPlasma();

void buildPalette();

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
		initPlasma();

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

	updatePlasma();
}

void render() {

	renderPlasma();
}

void close() {
	free(plasma1);
	free(plasma2);
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

void initPlasma() {

	plasma1 = (unsigned char*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT * 4);
	plasma2 = (unsigned char*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT * 4);

	int i, j, dst = 0;
	for (j = 0; j<(SCREEN_HEIGHT*2); j++)
	{
		for (i = 0; i<(SCREEN_WIDTH*2); i++)
		{
			plasma1[dst] = (unsigned char)(64 + 63 * (sin((double)hypot(SCREEN_HEIGHT - j, SCREEN_WIDTH - i) / 16)));
			plasma2[dst] = (unsigned char)(64 + 63 * sin((float)i / (37 + 15 * cos((float)j / 74)))
				* cos((float)j / (31 + 11 * sin((float)i / 57))));
			dst++;
		}
	}
}

void updatePlasma() {
	// setup some nice colours, different every frame
	// this is a palette that wraps around itself, with different period sine
	// functions to prevent monotonous colours
	buildPalette();

	// move plasma with more sine functions :)
	Windowx1 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2)-1) * cos((double)currentTime / 970));
	Windowx2 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2) - 1) * sin((double)-currentTime / 1140));
	Windowy1 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * sin((double)currentTime / 1230));
	Windowy2 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * cos((double)-currentTime / 750));
	// we only select the part of the precalculated buffer that we need
	src1 = Windowy1 * (SCREEN_WIDTH * 2) + Windowx1;
	src2 = Windowy2 * (SCREEN_WIDTH * 2) + Windowx2;
}

void renderPlasma() {
	// draw the plasma... this is where most of the time is spent.

	Uint8 *dst;
	long i, j;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;

	SDL_LockSurface(screenSurface);

	dst = initbuffer;
	for (j = 0; j<SCREEN_HEIGHT; j++)
	{
		dst = initbuffer + j *screenSurface->pitch;
		for (i = 0; i<SCREEN_WIDTH; i++)
		{
			// plot the pixel as a sum of all our plasma functions
			unsigned int Color = 0;
			int indexColor = (plasma1[src1] + plasma2[src2]) % 256;
			Color = 0xFF000000 + (palette[indexColor].R << 16) + (palette[indexColor].G << 8) + palette[indexColor].B;
			*(Uint32 *)dst = Color;

			dst+= bpp; 
			src1++; src2++; 
		}
		// get the next line in the precalculated buffers
		src1 += SCREEN_WIDTH; src2 += SCREEN_WIDTH;
	}
	SDL_UnlockSurface(screenSurface);

}

void buildPalette() {
	for (int i = 0; i<256; i++)
	{
		palette[i].R = (unsigned char)(128 + 127 * cos(i * M_PI / 128 + (double)currentTime / 740));
		palette[i].G = (unsigned char)(128 + 127 * sin(i * M_PI / 128 + (double)currentTime / 630));
		palette[i].B = (unsigned char)(128 - 127 * cos(i * M_PI / 128 + (double)currentTime / 810));
	}

}