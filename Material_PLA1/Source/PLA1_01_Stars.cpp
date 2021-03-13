#include <SDL.h>
#include <iostream>

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

// Stars
// change this to adjust the number of stars
#define MAXSTARS 256

// this record contains the information for one star
struct TStar
{
	float x, y;             // position of the star
	unsigned char plane;    // remember which plane it belongs to
};

// this is a pointer to an array of stars
TStar *stars;


bool initSDL();
void update();
void render();

void close();
void waitTime();
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void initStars();
void updateStars();
void renderStars();

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
		initStars();

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
	updateStars();
}

void render() {
	//Fill with black
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));

	renderStars();
}

void close() {
	// free memory
	delete[](stars);

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

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	// Clipping
	if ((x < 0) || (x >= SCREEN_WIDTH) || (y < 0) || (y >= SCREEN_HEIGHT)) 
		return;

	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		}
		else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

void initStars() {
	// allocate memory for all our stars
	stars = new TStar[MAXSTARS];
	// randomly generate some stars
	for (int i = 0; i<MAXSTARS; i++)
	{
		stars[i].x = (float)(rand() % SCREEN_WIDTH);
		stars[i].y = (float)(rand() % SCREEN_HEIGHT);
		stars[i].plane = rand() % 3;     // star colour between 0 and 2
	}

}

void updateStars() {
	// update all stars
	for (int i = 0; i < MAXSTARS; i++)
	{
		// move this star right, determine how fast depending on which
		// plane it belongs to
		stars[i].x += (deltaTime + (float)stars[i].plane)*0.15f;
		// check if it's gone out of the right of the screen
		if (stars[i].x > SCREEN_WIDTH)
		{
			// if so, make it return to the left
			stars[i].x = -(float)(rand() % SCREEN_WIDTH);
			// and randomly change the y position
			stars[i].y = (float)(rand() % SCREEN_HEIGHT);
		}
	}
}

void renderStars() {
	// update all stars
	for (int i = 0; i<MAXSTARS; i++)
	{
		// draw this star, with a colour depending on the plane
		unsigned int color = 0;
		switch (1+stars[i].plane) {
		case 1:
			color = 0xFF606060; // dark grey
			break;
		case 2:
			color = 0xFFC2C2C2; // light grey
			break;
		case 3:
			color = 0xFFFFFFFF; // white
			break;
		}
		putpixel(screenSurface, (int)stars[i].x, (int)stars[i].y, color);
	}
}