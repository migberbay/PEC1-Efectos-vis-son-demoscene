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

// two fire buffers
unsigned char *fire1;
unsigned char *fire2;
// a temporary variable to swap the two previous buffers
unsigned char *tmp;

struct RGBColor { unsigned char R, G, B; };
RGBColor palette[256];

bool initSDL();
void update();
void render();

void close();
void waitTime();
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void initFire();
void updateFire();
void renderFire();

void buildPalette();
void Shade_Pal(int s, int e, int r1, int g1, int b1, int r2, int g2, int b2);

void Heat(unsigned char *dst);
void Blur_Up(unsigned char *src, unsigned char *dst);


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
		initFire();

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

	updateFire();
}

void render() {

	renderFire();
}

void close() {
	free(fire1);
	free(fire2);
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

void initFire() {
	buildPalette();
	// two fire buffers
	fire1 = (unsigned char*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT);
	fire2 = (unsigned char*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT);
	// clear the buffers
	memset(fire1, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
	memset(fire2, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
}

void updateFire() {
	// swap our two fire buffers
	tmp = fire1;
	fire1 = fire2;
	fire2 = tmp;

	// heat the fire
	Heat(fire1);
	// apply the filter
	Blur_Up(fire1, fire2);
}

void renderFire() {
	Uint8 *dst;
	int src = 0;
	long i, j;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;

	SDL_LockSurface(screenSurface);
	dst = initbuffer;
	for (j = 0; j<(SCREEN_HEIGHT-3); j++) // Menos las 3 ultimas lineas
	{
		dst = initbuffer + j *screenSurface->pitch;
		for (i = 0; i<SCREEN_WIDTH; i++)
		{
			// plot the pixel from fire2
			unsigned int Color = 0;
			int indexColor = fire2[src];
			Color = 0xFF000000 + (palette[indexColor].R << 16) + (palette[indexColor].G << 8) + palette[indexColor].B;
			*(Uint32 *)dst = Color;
			dst += bpp;
			src++;
		}
	}
	SDL_UnlockSurface(screenSurface);

}

void buildPalette() {
	// setup some fire-like colours
	Shade_Pal(0, 23,	  0,   0,   0,   32,   0, 64);
	Shade_Pal(24, 47,	  32,   0, 64, 255,   0, 0);
	Shade_Pal(48, 63,	255,   0,   0, 255, 255, 0);
	Shade_Pal(64, 127,	255, 255,   0, 255, 255, 255);
	Shade_Pal(128, 255, 255, 255, 255, 255, 255, 255);
}

/*
* create a shade of colours in the palette from s to e
*/
void Shade_Pal(int s, int e, int r1, int g1, int b1, int r2, int g2, int b2)
{
	int i;
	float k;
	for (i = 0; i <= e - s; i++)
	{
		k = (float)i / (float)(e - s);
		palette[s + i].R = (int)(r1 + (r2 - r1)*k);
		palette[s + i].G = (int)(g1 + (g2 - g1)*k);
		palette[s + i].B = (int)(b1 + (b2 - b1)*k);
	}
}

/*
* adds some hot pixels to a buffer
*/
void Heat(unsigned char *dst)
{
	int i, j;
	
	j = (rand() % 512);
	// add some random hot spots at the bottom of the buffer
	for (i = 0; i<j; i++)
	{
		dst[(SCREEN_WIDTH*(SCREEN_HEIGHT - 3)) + (rand() % (SCREEN_WIDTH * 3))] = 255;
	}
}

/*
* smooth a buffer upwards, make sure not to read pixels that are outside of
* the buffer!
*/
void Blur_Up(unsigned char *src, unsigned char *dst)
{
	int offs = 0;
	unsigned char b;
	for (int j = 0; j<(SCREEN_HEIGHT-2); j++)
	{
		// set first pixel of the line to 0
		dst[offs] = 0; offs++;
		// calculate the filter for all the other pixels
		for (int i = 1; i<(SCREEN_WIDTH-1); i++)
		{
			// calculate the average
			b = (int)(src[offs - 1] +  src[offs + 1]
				+ src[offs + (SCREEN_WIDTH - 1)] + src[offs + (SCREEN_WIDTH)] + src[offs + (SCREEN_WIDTH + 1)]
				+ src[offs + ((SCREEN_WIDTH*2) - 1)] + src[offs + (SCREEN_WIDTH * 2)] + src[offs + ((SCREEN_WIDTH*2) + 1)]) / 8 ;
			// store the pixel
			dst[offs] = b;
			offs++;
		}
		// set last pixel of the line to 0
		dst[offs] = 0; offs++;
	}
	// clear the last 2 lines
	memset((void *)((long)(dst)+offs), 0, SCREEN_WIDTH * 2);
}
