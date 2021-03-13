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

// our precalculated mandelbrot fractals
unsigned char *frac1, *frac2, *fractmp;
// define the point in the complex plane to which we will zoom into
#define or    -0.577816-9.31323E-10-1.16415E-10
#define oi    -0.631121-2.38419E-07+1.49012E-08
// set original zooming settings
//    double zx = 2.91038E-11, zy = 2.91038E-11;
//    bool zoom_in = false;
double zx = 4.0, zy = 4.0;
bool zoom_in = true;
#define ZOOM_IN_FACTOR 0.5
#define ZOOM_OUT_FACTOR 2

/*
* global variables used for calculating our fractal
*/
double dr, di, pr, pi, sr, si;
long offs;
// setup the palette
int i, j = 0, k = 0;

struct RGBColor { unsigned char R, G, B; };
RGBColor palette[256];

bool initSDL();
void update();
void render();

void close();
void waitTime();
void buildPalette();

void initFractal();
void updateFractal();
void renderFractal();

void Start_Frac(double _sr, double _si, double er, double ei);
void Compute_Frac();
void Done_Frac();
void Zoom(double z);

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
		initFractal();

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

	updateFractal();
}

void render() {

	renderFractal();
}

void close() {
	free(frac1);
	free(frac2);
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

void buildPalette() {
	for (int i = 0; i<256; i++)
	{
		palette[i].R = (unsigned char)(128 + 127 * cos(i * M_PI / 128 + (double)currentTime / 740));
		palette[i].G = (unsigned char)(128 + 127 * sin(i * M_PI / 128 + (double)currentTime / 630));
		palette[i].B = (unsigned char)(128 - 127 * cos(i * M_PI / 128 + (double)currentTime / 810));
	}

}

void initFractal() {
	// allocate memory for our fractal
	frac1 = (unsigned char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
	frac2 = (unsigned char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
	// calculate the first fractal
	Start_Frac(or -zx, oi - zy, or +zx, oi + zy);
	for (j = 0; j<(SCREEN_HEIGHT / 2); j++) Compute_Frac();
	Done_Frac();
	// adjust zooming coefficient for next view
	if (zoom_in)
	{
		zx *= ZOOM_IN_FACTOR;
		zy *= ZOOM_IN_FACTOR;
	}
	else {
		zx *= ZOOM_OUT_FACTOR;
		zy *= ZOOM_OUT_FACTOR;
	}
	// start calculating the next fractal
	Start_Frac(or -zx, oi - zy, or +zx, oi + zy);
	j = 0;
}

void updateFractal() {
	buildPalette();
	if (j < (SCREEN_HEIGHT /2)) {
		j++;
		// calc another few lines
		Compute_Frac();
	}
	else {
		// adjust zooming coefficient for next view
		if (zoom_in)
		{
			zx *= ZOOM_IN_FACTOR;
			zy *= ZOOM_IN_FACTOR;
		}
		else {
			zx *= ZOOM_OUT_FACTOR;
			zy *= ZOOM_OUT_FACTOR;
		}
		j = 0;
		// start calculating the next fractal
		Start_Frac(or -zx, oi - zy, or +zx, oi + zy);
		// one more image displayed
		k++;
		// check if we've gone far enough
		if (k % 38 == 0)
		{
			// if so, reverse direction
			zoom_in = !zoom_in;
			if (zoom_in) {
				zx *= ZOOM_IN_FACTOR;
				zy *= ZOOM_IN_FACTOR;
			}
			else {
				zx *= ZOOM_OUT_FACTOR;
				zy *= ZOOM_OUT_FACTOR;
			}
			// and make sure we use the same fractal again, in the other direction
			fractmp = frac1;
			frac1 = frac2;
			frac2 = fractmp;
		}
		Done_Frac();
	}
}

void renderFractal() {
	// display the old fractal, zooming in or out
	if (zoom_in) Zoom((double)j / (SCREEN_HEIGHT / 2));
	else Zoom(1.0f - (double)j / (SCREEN_HEIGHT / 2));
	// select some new colours
	//updatePalette(k * 100 + j);
}

void Start_Frac(double _sr, double _si, double er, double ei)
{
	// compute deltas for interpolation in complex plane
	dr = (er - _sr) / (SCREEN_WIDTH * 2);
	di = (ei - _si) / (SCREEN_HEIGHT * 2);
	// remember start values
	pr = _sr;
	pi = _si;
	sr = _sr;
	si = _si;
	offs = 0;
}

/*
* compute 4 lines of fracal
*/
void Compute_Frac()
{
	for (int j = 0; j<4; j++)
	{
		pr = sr;
		for (int i = 0; i<(SCREEN_WIDTH * 2); i++)
		{
			unsigned char c = 0;
			double vi = pi, vr = pr, nvi, nvr;
			// loop until distance is above 2, or counter hits limit
			while ((vr*vr + vi*vi<4) && (c<255))
			{
				// compute Z(n+1) given Z(n)
				nvr = vr*vr - vi*vi + pr;
				nvi = 2 * vi * vr + pi;

				// that becomes Z(n)
				vi = nvi;
				vr = nvr;

				// increment counter
				c++;
			}
			// store colour
			frac1[offs] = c;
			offs++;
			// interpolate X
			pr += dr;
		}
		// interpolate Y
		pi += di;
	}
}

/*
* finished the computation, swap buffers
*/
void Done_Frac()
{
	fractmp = frac1;
	frac1 = frac2;
	frac2 = fractmp;
}

/*
* the zooming procedure
* takes a double screen bitmap, and scales it to screen size given the zooming coef
*/
void Zoom(double z)
{
	// setup the offsets in the buffers
	Uint8 *dst;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;

	// what's the size of rectangle in the source image we want to display
	int width = (int)(((SCREEN_WIDTH * 2) << 16) / (256.0f*(1 + z))) << 8,
		height = (int)(((SCREEN_HEIGHT *2) << 16) / (256.0f*(1 + z))) << 8,
		// where do we start our interpolation
		startx = (((SCREEN_WIDTH * 2) << 16) - width) >> 1,
		starty = (((SCREEN_HEIGHT * 2) << 16) - height) >> 1,
		// get our deltas
		deltax = width / SCREEN_WIDTH,
		deltay = height / SCREEN_HEIGHT,
		px, py = starty;

	for (int j = 0; j<SCREEN_HEIGHT; j++)
	{
		// set start value
		px = startx;
		dst = initbuffer + j *screenSurface->pitch;
		for (int i = 0; i<SCREEN_WIDTH; i++)
		{
			// load the bilinear filtered texel
			unsigned int Color = 0;
			// Uncomment for bilinear filter
			/*
			Color = 
				(frac2[(py >> 16) * (SCREEN_WIDTH * 2) + (px >> 16)] * (0x100 - ((py >> 8) & 0xff)) * (0x100 - ((px >> 8) & 0xff))
					+ frac2[(py >> 16) * (SCREEN_WIDTH * 2) + ((px >> 16) + 1)] * (0x100 - ((py >> 8) & 0xff)) * ((px >> 8) & 0xff)
					+ frac2[((py >> 16) + 1) * (SCREEN_WIDTH * 2) + (px >> 16)] * ((py >> 8) & 0xff) * (0x100 - ((px >> 8) & 0xff))
					+ frac2[((py >> 16) + 1) * (SCREEN_WIDTH * 2) + ((px >> 16) + 1)] * ((py >> 8) & 0xff) * ((px >> 8) & 0xff)) >> 16;
			*/
			int indexColor = frac2[(py >> 16) * (SCREEN_WIDTH * 2) + (px >> 16)]; // Direct Pixel color
			Color = 0xFF000000 + (palette[indexColor].R << 16) + (palette[indexColor].G << 8) + palette[indexColor].B;
			*(Uint32 *)dst = Color;
			// interpolate X
			px += deltax;
			dst += bpp;
		}
		// interpolate Y
		py += deltay;
	}
}