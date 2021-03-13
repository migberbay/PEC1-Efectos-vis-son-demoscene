#include <SDL.h>
#include <SDL_image.h>
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

// buffer containing the texture
SDL_Surface* texdata;
// Points from texture
int pointx1, pointy1,
	pointx2, pointy2,
	pointx3, pointy3;

bool initSDL();
void update();
void render();

void close();
void waitTime();

void initRotozoom();
void updateRotozoom();
void renderRotozoom();

void DoRotoZoom(float cx, float cy, float radius, float angle);
void TextureScreen();


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
		initRotozoom();

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
	updateRotozoom();
}

void render() {

	renderRotozoom();
}

void close() {
	SDL_FreeSurface(texdata);
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

void initRotozoom() {

	// load the texture
	SDL_Surface* temp = IMG_Load("texture_zoom.png");
	if (temp == NULL) {
		std::cout << "Image can be loaded! " << IMG_GetError();
		close();
		exit(1);
	}
	texdata = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
}

void updateRotozoom() {
	DoRotoZoom(
		1024 * sin((float)currentTime / 3000.0f),       // X centre coord
		2048 * cos((float)currentTime / 2700.0f),       // Y centre coord
		256.0f + 192.0f*cos((float)currentTime / 400.0f), // zoom coef
		(float)(currentTime) / 700.0f);                 // angle
}

void renderRotozoom() {
	TextureScreen();
}
/*
* wrapper for the TextureScreen procedure
* all parameters are 16.16 fixed point
*/
void DoRotoZoom(float cx, float cy, float radius, float angle)
{
	pointx1 = (int)(65536.0f * (cx + radius * cos(angle))),
	pointy1 = (int)(65536.0f * (cy + radius * sin(angle))),
	pointx2 = (int)(65536.0f * (cx + radius * cos(angle + 2.02458))),
	pointy2 = (int)(65536.0f * (cy + radius * sin(angle + 2.02458))),
	pointx3 = (int)(65536.0f * (cx + radius * cos(angle - 1.11701))),
	pointy3 = (int)(65536.0f * (cy + radius * sin(angle - 1.11701)));
}

/*
* render a textured screen with no blocks
* all parameters are 16.16 fixed point
*/
void TextureScreen()
{
	// setup the offsets in the buffers
	Uint8 *dst;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;
	Uint8 *imagebuffer = (Uint8 *)texdata->pixels;
	int bppImage = texdata->format->BytesPerPixel;
	// compute deltas
	int dxdx = (pointx2 - pointx1) / SCREEN_WIDTH,
		dydx = (pointy2 - pointy1) / SCREEN_WIDTH,
		dxdy = (pointx3 - pointx1) / SCREEN_HEIGHT,
		dydy = (pointy3 - pointy1) / SCREEN_HEIGHT;

	SDL_LockSurface(screenSurface);
	// loop for all lines
	for (int j = 0; j<SCREEN_HEIGHT; j++)
	{
		dst = initbuffer + j *screenSurface->pitch;
		pointx3 = pointx1;
		pointy3 = pointy1;
		// for each pixel
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// get texel and store pixel
			Uint8 *p = (Uint8 *)imagebuffer + ((pointy3 >> 16) & 0xff) * texdata->pitch + ((pointx3 >> 16) & 0xFF) * bppImage;
			// copy it to the screen
			*(Uint32 *)dst = *(Uint32 *)p;
			// interpolate to get next texel in texture space
			pointx3 += dxdx;
			pointy3 += dydx;
			// next pixel
			dst += bpp;
		}
		// interpolate to get start of next line in texture space
		pointx1 += dxdy;
		pointy1 += dydy;
	}
	SDL_UnlockSurface(screenSurface);
}
