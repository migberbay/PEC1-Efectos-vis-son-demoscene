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

// buffer containing the (u,v) pairs at each pixel
unsigned char *texcoord;
// buffer containing the texture
SDL_Surface* texdata;

bool initSDL();
void update();
void render();

void close();
void waitTime();

void initTunel();
void renderTunel();
float get_x_pos(float f);
float get_y_pos(float f);
float get_radius(float f);
void Draw_Hole(int du, int dv);

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
		initTunel();

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

}

void render() {

	renderTunel();
}

void close() {
	free(texcoord);
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

void initTunel() {

	// alloc memory to store SCREEEN SIZE times u, v
	texcoord =  (unsigned char*) malloc(SCREEN_WIDTH * SCREEN_HEIGHT *2);
	long offs = 0;
	// precalc the (u,v) coordinates
	for (int j = -(SCREEN_HEIGHT /2); j<(SCREEN_HEIGHT/2); j++) {
		for (int i = -(SCREEN_WIDTH / 2); i<(SCREEN_WIDTH / 2); i++)
		{
			// get coordinates of ray that projects through this pixel
			float dx = (float)i / SCREEN_HEIGHT;
			float dy = (float)-j / SCREEN_HEIGHT;
			float dz = 1;
			// normalize them
			float d = 20 / sqrt(dx*dx + dy*dy + 1);
			dx *= d;
			dy *= d;
			dz *= d;
			// start interpolation at origin
			float x = 0;
			float y = 0;
			float z = 0;
			// set original precision
			d = 16;
			// interpolate along ray
			while (d>0)
			{
				// continue until we hit a wall
				while (((x - get_x_pos(z))*(x - get_x_pos(z)) + (y - get_y_pos(z))*(y - get_y_pos(z)) < get_radius(z)) && (z<1024))
				{
					x += dx;
					y += dy;
					z += dz;
				};
				// reduce precision and reverse direction
				x -= dx;  dx /= 2;
				y -= dy;  dy /= 2;
				z -= dz;  dz /= 2;
				d -= 1;
			}
			// calculate the texture coordinates
			x -= get_x_pos(z);
			y -= get_y_pos(z);
			float ang = atan2(y, x) * 256 / M_PI;
			unsigned char u = (unsigned char)ang;
			unsigned char v = (unsigned char)z;
			// store texture coordinates
			texcoord[offs] = u;
			texcoord[offs + 1] = v;
			offs += 2;
		}
	}

	// load the texture
	texdata = IMG_Load("texture.png");
}


void renderTunel() {
	Draw_Hole( currentTime / 16, currentTime / 32 );
}

/*
* position of the centre of the hole along the X axis
*/
float get_x_pos(float f)
{
	return -16 * sin(f * M_PI / 256);
};

/*
* position of the centre of the hole along the Y axis
*/
float get_y_pos(float f)
{
	return -16 * sin(f * M_PI / 256);
};

/*
* size of the hole
*/
float get_radius(float f)
{
	return 128;
};

void Draw_Hole(int du, int dv)
{
	Uint8 *dst;
	Uint8 *initbuffer = (Uint8 *)screenSurface->pixels;
	int bpp = screenSurface->format->BytesPerPixel;
	Uint8 *initbufferTexture = (Uint8 *)texdata->pixels;
	int bppTexture = texdata->format->BytesPerPixel;

	long doffs = 0, soffs = 0;
	SDL_LockSurface(screenSurface);
	for (int j = 0; j<SCREEN_HEIGHT; j++) {
		dst = initbuffer + j *screenSurface->pitch;
		for (int i = 0; i<SCREEN_WIDTH; i++) {
			// load (u,v) and add displacement
			unsigned char u = texcoord[soffs] + du;
			unsigned char v = texcoord[soffs + 1] + dv;

			SDL_Color Color;
			SDL_GetRGB(*(Uint32*)((Uint8*)texdata->pixels + v * texdata->pitch + u * bppTexture), texdata->format, &Color.r, &Color.g, &Color.b);
			Uint32 resultColor = SDL_MapRGB(screenSurface->format, Color.r, Color.g, Color.b);
			*(Uint32 *)dst = resultColor;
			dst += bpp;

			soffs += 2;
		}
	}
	SDL_UnlockSurface(screenSurface);
}