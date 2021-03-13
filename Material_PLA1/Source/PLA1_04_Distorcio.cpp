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

// displacement buffers
char *dispX, *dispY;
// image background
SDL_Surface *image;
// define the distortion buffer movement
int windowx1, windowy1, windowx2, windowy2;

bool initSDL();
void update();
void render();

void close();
void waitTime();
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void initDistortion();
void updateDistortion();
void renderDistortion();

void precalculate();
void Distort();
void Distort_Bili();

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
		initDistortion();

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

	updateDistortion();
}

void render() {

	renderDistortion();
}

void close() {
	free(dispX);
	free(dispY);
	SDL_FreeSurface(image);
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

void initDistortion() {
	// two buffers
	dispX = (char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
	dispY = (char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
	// create two distortion functions
	precalculate();
	// load the background image
	SDL_Surface *temp = IMG_Load("uoc.png");
	if (temp == NULL) {
		std::cout << "Image can be loaded! " << IMG_GetError();
		close();
		exit(1);
	}
	image = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
}

void updateDistortion() {
	// move distortion buffer
	windowx1 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2)-1) * cos((double)currentTime / 2050));
	windowx2 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2) - 1) * sin((double)-currentTime / 1970));
	windowy1 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * sin((double)currentTime / 2310));
	windowy2 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * cos((double)-currentTime / 2240));
}

void renderDistortion() {
	// draw the effect showing without filter and with filter each 2 seconds
	if ((currentTime & 2048) < 1024) {
		Distort();
	}
	else {
		Distort_Bili();
	}
}

/*
* calculate a distorion function for X and Y in 5.3 fixed point
*/
void precalculate()
{
	int i, j, dst;
	dst = 0;
	for (j = 0; j<(SCREEN_HEIGHT * 2); j++)
	{
		for (i = 0; i<(SCREEN_WIDTH * 2); i++)
		{
			float x = (float)i;
			float y = (float)j;
			// notice the values contained in the buffers are signed
			// i.e. can be both positive and negative
			// also notice we multiply by 8 to get 5.3 fixed point distortion
			// coefficients for our bilinear filtering
			dispX[dst] = (signed char)(8 * (2 * sin(x / 20) + sin(x*y/2000)));
			dispY[dst] = (signed char)(8 * (cos(x / 31) + cos(x*y / 1783)));

			// Uncomment this to take another beautiful distorsion
			/*
			dispX[dst] = (signed char)(8 * (2 * (sin(x / 20) + sin(x*y / 2000)
				+ sin((x + y) / 100) + sin((y - x) / 70) + sin((x + 4 * y) / 70)
				+ 2 * sin(hypot(256 - x, (150 - y / 8)) / 40))));
			dispY[dst] = (signed char)(8 * ((cos(x / 31) + cos(x*y / 1783) +
				+2 * cos((x + y) / 137) + cos((y - x) / 55) + 2 * cos((x + 8 * y) / 57)
				+ cos(hypot(384 - x, (274 - y / 9)) / 51))));
			*/
			dst++;
		}
	}
}

/*
*   copy an image to the screen with added distortion.
*   no bilinear filtering.
*/
void Distort()
{
	// setup the offsets in the buffers
	Uint8 *dst;
	int	src1 = windowy1 * (SCREEN_WIDTH * 2) + windowx1,
		src2 = windowy2 * (SCREEN_WIDTH * 2) + windowx2;
	int dX, dY;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;
	Uint8 *imagebuffer = (Uint8 *)image->pixels;
	int bppImage = image->format->BytesPerPixel;

	SDL_LockSurface(screenSurface);
	// loop for all lines
	for (int j = 0; j<SCREEN_HEIGHT; j++)
	{
		dst = initbuffer + j *screenSurface->pitch;
		// for all pixels
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// get distorted coordinates, use the integer part of the distortion
			// buffers and truncate to closest texel
			dY = j + (dispY[src1] >> 3);
			dX = i + (dispX[src2] >> 3);
			// check the texel is valid
			if ((dY >= 0) && (dY<(SCREEN_HEIGHT - 1)) && (dX >= 0) && (dX<(SCREEN_WIDTH - 1)))
			{
				// copy it to the screen
				Uint8 *p = (Uint8 *)imagebuffer + dY * image->pitch + dX * bppImage;
				*(Uint32 *)dst = *(Uint32 *)p;
			}
			// otherwise, just set it to black
			else *(Uint32 *)dst = 0;
			// next pixel
			dst += bpp; 
			src1++; src2++;
		}
		// next line
		src1 += SCREEN_WIDTH;
		src2 += SCREEN_WIDTH;
	}
	SDL_UnlockSurface(screenSurface);
}

/*
*   copy an image to the screen with added distortion.
*   with bilinear filtering.
*/
void Distort_Bili()
{
	// setup the offsets in the buffers
	Uint8 *dst;
	int src1 = windowy1 * (SCREEN_WIDTH * 2) + windowx1,
		src2 = windowy2 * (SCREEN_WIDTH * 2) + windowx2;
	int dX, dY, cX, cY;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;
	Uint32 *imagebuffer = (Uint32 *)image->pixels;
	int bppImage = image->format->BytesPerPixel;

	SDL_LockSurface(screenSurface);
	// loop for all lines
	for (int j = 0; j<SCREEN_HEIGHT; j++)
	{
		dst = initbuffer + j *screenSurface->pitch;
		// for all pixels
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// get distorted coordinates, by using the truncated integer part
			// of the distortion coefficients
			dY = j + (dispY[src1] >> 3);
			dX = i + (dispX[src2] >> 3);
			// get the linear interpolation coefficiants by using the fractionnal
			// part of the distortion coefficients
			cY = dispY[src1] & 0x7;
			cX = dispX[src2] & 0x7;
			// check if the texel is valid
			if ((dY >= 0) && (dY<(SCREEN_HEIGHT-1)) && (dX >= 0) && (dX<(SCREEN_WIDTH-1)))
			{
				// load the 4 surrounding texels and multiply them by the
				// right bilinear coefficients, then get rid of the fractionnal
				// part by shifting right by 6
				SDL_Color Colorvalues[4];
				SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + dY     * image->pitch + dX     * bppImage), image->format, &Colorvalues[0].r, &Colorvalues[0].g, &Colorvalues[0].b);
				SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + dY     * image->pitch + (dX+1) * bppImage), image->format, &Colorvalues[1].r, &Colorvalues[1].g, &Colorvalues[1].b);
				SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + (dY+1) * image->pitch + dX     * bppImage), image->format, &Colorvalues[2].r, &Colorvalues[2].g, &Colorvalues[2].b);
				SDL_GetRGB(*(Uint32*)((Uint8*)image->pixels + (dY+1) * image->pitch + (dX+1) * bppImage), image->format, &Colorvalues[3].r, &Colorvalues[3].g, &Colorvalues[3].b);
				Uint32 Color[3]; // 0=R  1=G  2=B
				Color[0] = (Colorvalues[0].r * (0x8 - cX) * (0x8 - cY) +
					Colorvalues[1].r * cX * (0x8 - cY) +
					Colorvalues[2].r * (0x8 - cX) * cY +
					Colorvalues[3].r * cX * cY) >> 6;
				Color[1] = (Colorvalues[0].g * (0x8 - cX) * (0x8 - cY) +
					Colorvalues[1].g * cX * (0x8 - cY) +
					Colorvalues[2].g * (0x8 - cX) * cY +
					Colorvalues[3].g * cX * cY) >> 6;
				Color[2] = (Colorvalues[0].b * (0x8 - cX) * (0x8 - cY) +
					Colorvalues[1].b * cX * (0x8 - cY) +
					Colorvalues[2].b * (0x8 - cX) * cY +
					Colorvalues[3].b * cX * cY) >> 6;
				Uint32 resultColor = SDL_MapRGB(image->format, Color[0], Color[1], Color[2]);
				*(Uint32 *)dst = resultColor;
			}
			// otherwise, just make it black
			else *(Uint32 *)dst = 0;
			dst += bpp;
			src1++; src2++;
		}
		// next line
		src1 += SCREEN_WIDTH;
		src2 += SCREEN_WIDTH;
	}
	SDL_UnlockSurface(screenSurface);
}
