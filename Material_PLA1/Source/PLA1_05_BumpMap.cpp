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

// size of the spot light
#define LIGHTSIZE 2.4f
#define LIGHT_PIXEL_RES 256
// contains the precalculated spotlight
unsigned char *light; 
// image color
SDL_Surface *image;
// imagen bump
SDL_Surface *bump;

// define the distortion buffer movement
int windowx1, windowy1, windowx2, windowy2, windowZ;

bool initSDL();
void update();
void render();

void close();
void waitTime();

void initBumpMap();
void updateBumpMap();
void renderBumpMap();

void Compute_Light();
void Bump();

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
		initBumpMap();

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

	updateBumpMap();
}

void render() {

	renderBumpMap();
}

void close() {
	free(light);
	SDL_FreeSurface(image);
	SDL_FreeSurface(bump);
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

void initBumpMap() {
	// contains the image of the spotlight
	light = (unsigned char*)malloc(LIGHT_PIXEL_RES * LIGHT_PIXEL_RES);
	// generate the light pattern
	Compute_Light();
	// load the color image
	SDL_Surface *temp = IMG_Load("wall.png");
	if (temp == NULL) {
		std::cout << "Image can be loaded! " << IMG_GetError();
		close();
		exit(1);
	}
	image = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
	// load the bump image
	temp = IMG_Load("bump.png");
	if (temp == NULL) {
		std::cout << "Image can be loaded! " << IMG_GetError();
		close();
		exit(1);
	}
	bump = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
}

void updateBumpMap() {
	// move the light.... more sines :)
	windowx1 = (int)((LIGHT_PIXEL_RES / 2) * cos((double)currentTime / 640)) - 20;
	windowy1 = (int)((LIGHT_PIXEL_RES / 2) * sin((double)-currentTime / 450)) + 20;
	windowx2 = (int)((LIGHT_PIXEL_RES / 2) * cos((double)-currentTime / 510)) - 20;
	windowy2 = (int)((LIGHT_PIXEL_RES / 2) * sin((double)currentTime / 710)) + 20;
	windowZ = 192 + (int)(((LIGHT_PIXEL_RES / 2) - 1) * sin((double)currentTime / 1120));
}

void renderBumpMap() {
	// draw the bumped image
	Bump();
}

/*
* generate a "spot light" pattern
*/
void Compute_Light()
{
	for (int j = 0; j<LIGHT_PIXEL_RES; j++)
		for (int i = 0; i<LIGHT_PIXEL_RES; i++)
		{
			// get the distance from the centre
			float dist = (float)((LIGHT_PIXEL_RES / 2) - i)*((LIGHT_PIXEL_RES / 2) - i) + ((LIGHT_PIXEL_RES / 2) - j)*((LIGHT_PIXEL_RES / 2) - j);
			if (fabs(dist)>1) dist = sqrt(dist);
			// then fade if according to the distance, and a random coefficient
			int c = (int)(LIGHTSIZE*dist) + (rand() & 7) - 3;
			// clip it
			if (c<0) c = 0;
			if (c>255) c = 255;
			// and store it
			light[(j * LIGHT_PIXEL_RES) + i] = 255 - c;
		}
}

/*
* this needs a bump map and a colour map, and 2 light coordinates
* it computes the output colour with the look up table
*/
void Bump()
{
	int i, j, px, py, x, y, c;
	// setup the offsets in the buffers
	Uint8 *dst;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;
	Uint32 *imagebuffer = (Uint32 *)image->pixels;
	int bppImage = image->format->BytesPerPixel;
	Uint32 *bumpbuffer = (Uint32 *)bump->pixels;
	int bppBump = bump->format->BytesPerPixel;

	// we skip the first line since there are no pixels above
	// to calculate the slope with
	// loop for all the other lines
	SDL_LockSurface(screenSurface);
	for (j = 1; j<SCREEN_HEIGHT; j++)
	{
		// likewise, skip first pixel since there are no pixels on the left
		dst = initbuffer + j *screenSurface->pitch;
		*(Uint32 *)dst = 0;
		for (i = 1; i<SCREEN_WIDTH; i++)
		{
			// calculate coordinates of the pixel we need in light map
			// given the slope at this point, and the zoom coefficient
			SDL_Color Colors[3]; // 0=left pixel 1=center pixel 2=up pixel
			SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + j      * image->pitch + (i - 1) * bppImage), image->format, &Colors[0].r, &Colors[0].g, &Colors[0].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + j      * image->pitch + i       * bppImage), image->format, &Colors[1].r, &Colors[1].g, &Colors[1].b);
			SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + (j -1) * image->pitch + i       * bppImage), image->format, &Colors[2].r, &Colors[2].g, &Colors[2].b);
			px = (i*windowZ >> 8) + Colors[0].r - Colors[1].r;
			py = (j*windowZ >> 8) + Colors[2].r - Colors[1].r;
			// add the movement of the first light
			x = px + windowx1;
			y = py + windowy1;
			// check if the coordinates are inside the light buffer
			if ((y >= 0) && (y<LIGHT_PIXEL_RES) && (x >= 0) && (x<LIGHT_PIXEL_RES))
				// if so get the pixel
				c = light[(y * LIGHT_PIXEL_RES) + x];
			// otherwise assume intensity 0
			else c = 0;
			// now do the same for the second light
			x = px + windowx2;
			y = py + windowy2;
			// this time we add the light's intensity to the first value
			if ((y >= 0) && (y<LIGHT_PIXEL_RES) && (x >= 0) && (x<LIGHT_PIXEL_RES))
				c += light[(y * LIGHT_PIXEL_RES) + x];
			// make sure it's not too big
			if (c>255) c = 255;
			// look up the colour multiplied by the light coeficient
			SDL_Color ColorBump; // 0=left pixel 1=center pixel 2=up pixel
			SDL_GetRGB(*(Uint32*)((Uint8*)bump->pixels + j * bump->pitch + i * bppBump), bump->format, &ColorBump.r, &ColorBump.g, &ColorBump.b);

			Uint32 Color[3]; // 0=R  1=G  2=B
			Color[0] = (Uint32)((((Colors[0].r * ColorBump.r) / 255))*(c / 255.0f));
			Color[1] = (Uint32)((((Colors[0].g * ColorBump.g) / 255))*(c / 255.0f));
			Color[2] = (Uint32)((((Colors[0].b * ColorBump.b) / 255))*(c / 255.0f));
			if (Color[0] > 255) Color[0] = 255;
			if (Color[1] > 255) Color[1] = 255;
			if (Color[2] > 255) Color[2] = 255;
			Uint32 resultColor = SDL_MapRGB(image->format, Color[0], Color[1], Color[2]);
			*(Uint32 *)dst = resultColor;
			dst += bpp;
		}
	}
	SDL_UnlockSurface(screenSurface);
}
