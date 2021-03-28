//Using SDL and standard IO
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <string>

#include "vector.h"
#include "matrix.h"

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Helper classes

class LTexture
{
public:
    //Initializes variables
    LTexture();

    //Deallocates memory
    ~LTexture();

    //Loads image at specified path
    bool loadFromFile(std::string path);

    //Deallocates texture
    void free();

    //Set color modulation
    void setColor(Uint8 red, Uint8 green, Uint8 blue);

    //Set blending
    void setBlendMode(SDL_BlendMode blending);

    //Set alpha modulation
    void setAlpha(Uint8 alpha);

    //Renders texture at given point
    void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

    //Gets image dimensions
    int getWidth();
    int getHeight();

private:
    //The actual hardware texture
    SDL_Texture* mTexture;

    //Image dimensions
    int mWidth;
    int mHeight;

};

// The window we'll be rendering to
SDL_Window* window = NULL;

// The surface contained by the window
SDL_Surface* screenSurface = NULL;

// Frame Logic
#define FPS 60
int lastTime = 0, currentTime, deltaTime;
float msFrame = 1 / (FPS / 1000.0f);

// TRANSITION & DEMO HANDLER VARIABLES
const int ALLOCATED_DEMO_TIMES[] = { 500, 500, 20000 };
const int ALLOCATED_TRANSITION_TIME = 500;

// 0->transition, 1->stars, 2->plasma, 3 -> spaceships
int current_demo = 1; 
int prev_demo = 0;

// milliseconds left until swap.
int current_time_left = 2500;

// number of effects in demoscene.
int numDemos = 3;

// transition buffers
unsigned char *transBuffer;
bool transFirstInit = true;

// number of initial lines in transition
const int numTransLines = 5;
// array containing which lines are to be filled next.
int height_lines[numTransLines * 2];


// DEMO VARIABLES 
// GENERAL

// color palette
struct RGBColor { unsigned char R, G, B; };
RGBColor palette[256];

// STARS
// change this to adjust the number of stars
#define MAXSTARS 256

// this record contains the information for one star
struct TStar {
    float x, y;             // position of the star
    unsigned char plane;    // remember which plane it belongs to
};

// this is a pointer to an array of stars
TStar* stars;
bool starsFirstInit = true;

// PLASMA
// the two function buffers sized SCREEN_WIDTH * SCREEN_HEIGHT * 4
unsigned char *plasma1;
unsigned char *plasma2;

bool plasmaFirstInit = true;

// plasma movement
int Windowx1, Windowy1, Windowx2, Windowy2;
long src1, src2;

// SPACESHIPS WITH SOUND.
Mix_Music *imperial;
const int BPM_MUSIC = 103;
const float MSEG_BPM = (60000 / BPM_MUSIC);
const int SPACESHIP_TTL = 6000;
const int MAX_SPACESHIPS = 11; // The maximum numberof spaceships that will be active at any given moment

// the window renderer
SDL_Renderer* spaceshipRenderer = NULL;

//Currently displayed texture
LTexture spaceshipTexture;

struct TSpaceship {
    int start_x, start_y; // initial position of spaceship
    int x, y;  // position of spaceship
    int rotation; // rotation of the ship.
    int TTL; // remaining Time to live. set to inactive on <= 0
    bool active;
};

TSpaceship *spaceships;

//Flip type
SDL_RendererFlip flipType = SDL_FLIP_NONE;

int MusicCurrentTime;
int MusicCurrentTimeBeat;
int MusicCurrentBeat;
int MusicPreviousBeat;

bool firstInitMusic = true;
bool firstInitSpaceship = true;
// FUNTION DECLARATIONS

// General functions
bool initSDL();
void update();
void render();
void close();
void waitTime();
void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel);


// Demo control
void demoControlTime(int deltaTime);
void initTransition();
void updateTransition();
void renderTransition();
void initCorrespondingModule();

// Demo functions
// Stars
 void initStars();
 void updateStars();
 void renderStars();

// Plasma
void initPlasma();
void updatePlasma();
void renderPlasma();
void buildPalettePlasma();

// Spaceships
void initMusic();
void updateMusic();

void initSpaceships();
void updateSpaceships();
void renderSpaceships();


// FUNCTION METHODS
/* 
* Initialize the window and screen surface variables.
*/
bool initSDL() {
    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! SDL_Error: %s\n" << SDL_GetError();
        return false;
    }

    //Set texture filtering to linear
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
    {
        printf("Warning: Linear texture filtering not enabled!");
    }

    //Create window
    window = SDL_CreateWindow("Demoscene", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: %s\n" << SDL_GetError();
        return false;
    }
    //Get window surface
    screenSurface = SDL_GetWindowSurface(window);

    return true;
}

void update() {
    // Handle update functions here.
    // 0->transition, 1->stars, 2->plasma
    switch (current_demo) {
    case 0:
        updateTransition();
        break;
    case 1:
        updateStars();
        break;
    case 2:
        updatePlasma();
        break;
    case 3: 
        updateMusic();
        updateSpaceships();
        break;
    }
}

void render() {
    //Fill with black
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));

    // Handle render functions here.
    switch (current_demo) {
    case 0:
        renderTransition();
        break;
    case 1:
        renderStars();
        break;
    case 2:
        renderPlasma();
        break;
    case 3:
        renderSpaceships();
        break;
    }
}

void initCorrespondingModule() {
    // 0->transition, 1->stars, 2->plasma, 3-> spaceships
    switch (current_demo) {
    case 0:
        initTransition();
        break;
    case 1:
        initStars();
        break;
    case 2:
        initPlasma();
        break;
    case 3:
        initSpaceships();
        initMusic();
        break;
    }
}

void close() {
    // free memory
    delete[](stars);

    free(plasma1);
    free(plasma2);

    free(transBuffer);

    delete[](spaceships);

    //Free loaded image
    spaceshipTexture.free();

    //Destroy window    
    SDL_DestroyRenderer(spaceshipRenderer);
    SDL_DestroyWindow(window);

    window = NULL;
    spaceshipRenderer = NULL;
    
    //Quit SDL subsystems
    SDL_Quit();
    IMG_Quit();

}

void waitTime() {
    currentTime = SDL_GetTicks();
    deltaTime = currentTime - lastTime;
    if (deltaTime < (int)msFrame) {
        SDL_Delay((int)msFrame - deltaTime);
    }
    lastTime = currentTime;
    demoControlTime(deltaTime);
}

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
* This is a slow function to paint the whole screen.
*/
void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
    // Clipping
    if ((x < 0) || (x >= SCREEN_WIDTH) || (y < 0) || (y >= SCREEN_HEIGHT))
        return;

    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16*)p = pixel;
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
        *(Uint32*)p = pixel;
        break;
    }
}


// TRANSITION
void demoControlTime(int deltaTime) {
    current_time_left -= deltaTime;
    if (current_time_left <= 0) { // Time to change demo.
        std::cout << "Changing to new module, ";
        if (current_demo == 0) { // we change from transition to new effect.
            std::cout << "From Transition to effect,  ";
            if (prev_demo == numDemos) {// we are in the last demo, so we reset.
                std::cout << "Last demo.";
                current_demo = 1;
            }
            else {
                std::cout << "Not last demo. \n";
                current_demo = prev_demo + 1;
            }
            prev_demo = 0;
            current_time_left = ALLOCATED_DEMO_TIMES[current_demo-1];
        }
        else { // we change from effect to transition.
            prev_demo = current_demo;
            std::cout << "From effect to Transition. \n";
            current_demo = 0;
            current_time_left = ALLOCATED_TRANSITION_TIME;
        }
        initCorrespondingModule();
    }
}

void initTransition() {
    std::cout << "Initializing Transition Module \n";

    int tot = SCREEN_HEIGHT * SCREEN_WIDTH;
    //try and deallocate memory for buffer.
   
    // asignamos memoria para el buffer.
    if (transFirstInit) {
        transBuffer = (unsigned char*)malloc(tot);
        transFirstInit = false;
    }
    //limpiamos lo que haya
    memset(transBuffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH);

    std::cout << "memory allocated and cleaned. \n";

    // draw n horizontal lines randomly.
    int n, j;
    for (n = 0; n < numTransLines*2; n += 2) {
        int initial_line = rand() % SCREEN_HEIGHT;
        height_lines[n] = initial_line;
        height_lines[n + 1] = initial_line;
        for (j = 0; j < SCREEN_WIDTH; j++) {
            transBuffer[initial_line * SCREEN_WIDTH + j] = 0xFFFFFFFF;
        }
    }

}

// We add the new lines 
void updateTransition(){
    int n, j;
    for (n = 0; n < numTransLines * 2; n += 2) {
        if (height_lines[n] - 1 >= 0) { height_lines[n] --;}
        if (height_lines[n+1] + 1 <= SCREEN_HEIGHT) { height_lines[n+1] ++; }
        
        if (transBuffer[height_lines[n] + 1] != 0xFFFFFFFF) {
            for (j = 0; j < SCREEN_WIDTH; j++) {
                transBuffer[height_lines[n] * SCREEN_WIDTH + j] = 0xFFFFFFFF;
            }
        }

        if (transBuffer[height_lines[n + 1] + 1] != 0xFFFFFFFF) {
            for (j = 0; j < SCREEN_WIDTH; j++) {
                transBuffer[height_lines[n + 1]*SCREEN_WIDTH + j] = 0xFFFFFFFF;
            }
        }
    }
}

void renderTransition() {
    int i, j;
    for (j = 0; j < SCREEN_HEIGHT; j++)
    {
        for (i = 0; i < SCREEN_WIDTH; i++)
        {
            // plot the pixel as the value from the transition buffer
            putpixel(screenSurface, i, j, transBuffer[i * j]);
        }
    }
}


// STARS
void initStars() {
    std::cout << "Initializing Stars Module \n";

    // allocate memory for all our stars
    if (starsFirstInit) {
        stars = new TStar[MAXSTARS];
        starsFirstInit = false;
    }
    
    // randomly generate some stars
    for (int i = 0; i < MAXSTARS; i++)
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
        stars[i].x += (deltaTime + (float)stars[i].plane) * 0.15f;
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
    for (int i = 0; i < MAXSTARS; i++)
    {
        // draw this star, with a colour depending on the plane
        unsigned int color = 0;
        switch (1 + stars[i].plane) {
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


// PLASMA
void initPlasma() {
    std::cout << "Initializing Plasma Module \n";

    if (plasmaFirstInit) {
        plasma1 = (unsigned char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
        plasma2 = (unsigned char*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
        plasmaFirstInit = false;
    }

    int i, j, dst = 0;
    for (j = 0; j < (SCREEN_HEIGHT * 2); j++) {
        for (i = 0; i < (SCREEN_WIDTH * 2); i++)
        {
            plasma1[dst] = (unsigned char)(64 + 63 * (sin((double)hypot(SCREEN_HEIGHT - j, SCREEN_WIDTH - i) / 16)));
            plasma2[dst] = (unsigned char)(64 + 63 * sin((float)i / (37 + 15 * cos((float)j / 74))) * cos((float)j / (31 + 11 * sin((float)i / 57))));
            dst++;
        }
    }
}

void updatePlasma() {
    // setup some nice colours, different every frame
    // this is a palette that wraps around itself, with different period sine
    // functions to prevent monotonous colours
    buildPalettePlasma();

    // move plasma with more sine functions :)
    Windowx1 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2) - 1) * cos((double)currentTime / 970));
    Windowx2 = (SCREEN_WIDTH / 2) + (int)(((SCREEN_WIDTH / 2) - 1) * sin((double)-currentTime / 1140));
    Windowy1 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * sin((double)currentTime / 1230));
    Windowy2 = (SCREEN_HEIGHT / 2) + (int)(((SCREEN_HEIGHT / 2) - 1) * cos((double)-currentTime / 750));
    // we only select the part of the precalculated buffer that we need
    src1 = Windowy1 * (SCREEN_WIDTH * 2) + Windowx1;
    src2 = Windowy2 * (SCREEN_WIDTH * 2) + Windowx2;
}

void renderPlasma() {
    // draw the plasma... this is where most of the time is spent.

    Uint8* dst;
    long i, j;
    Uint8* initbuffer = (Uint8*)screenSurface->pixels;
    int bpp = screenSurface->format->BytesPerPixel;

    SDL_LockSurface(screenSurface);

    dst = initbuffer;
    for (j = 0; j < SCREEN_HEIGHT; j++)
    {
        dst = initbuffer + j * screenSurface->pitch;
        for (i = 0; i < SCREEN_WIDTH; i++)
        {
            // plot the pixel as a sum of all our plasma functions
            unsigned int Color = 0;
            int indexColor = (plasma1[src1] + plasma2[src2]) % 256;
            Color = 0xFF000000 + (palette[indexColor].R << 16) + (palette[indexColor].G << 8) + palette[indexColor].B;
            *(Uint32*)dst = Color;

            dst += bpp;
            src1++; src2++;
        }
        // get the next line in the precalculated buffers
        src1 += SCREEN_WIDTH; src2 += SCREEN_WIDTH;
    }
    SDL_UnlockSurface(screenSurface);
}

void buildPalettePlasma() {
    for (int i = 0; i < 256; i++)
    {
        palette[i].R = (unsigned char)(128 + 127 * cos(i * M_PI / 128 + (double)currentTime / 740));
        palette[i].G = (unsigned char)(128 + 127 * sin(i * M_PI / 128 + (double)currentTime / 630));
        palette[i].B = (unsigned char)(128 - 127 * cos(i * M_PI / 128 + (double)currentTime / 810));
    }

}


// SPACESHIPS CLASS FUNCTIONS

LTexture::LTexture()
{
    //Initialize
    mTexture = NULL;
    mWidth = 40;
    mHeight = 40;
}

LTexture::~LTexture()
{
    //Deallocate
    free();
}

bool LTexture::loadFromFile(std::string path)
{
    //Get rid of preexisting texture
    free();

    //The final texture
    SDL_Texture* newTexture = NULL;

    //Load image at specified path
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    }
    else
    {
        //Color key image
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

        //Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface(spaceshipRenderer, loadedSurface);
        if (newTexture == NULL)
        {
            printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
        }
        else
        {
            //Get image dimensions
            mWidth = loadedSurface->w/5;
            mHeight = loadedSurface->h/5;
        }

        //Get rid of old loaded surface
        SDL_FreeSurface(loadedSurface);
    }

    //Return success
    mTexture = newTexture;
    return mTexture != NULL;
}

void LTexture::free()
{
    //Free texture if it exists
    if (mTexture != NULL)
    {
        SDL_DestroyTexture(mTexture);
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
    //Modulate texture rgb
    SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
    //Set blending function
    SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
    //Modulate texture alpha
    SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };

    //Set clip rendering dimensions
    if (clip != NULL)
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }

    //Render to screen
    SDL_RenderCopyEx(spaceshipRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
    return mWidth;
}

int LTexture::getHeight()
{
    return mHeight;
}

bool loadMedia()
{
    //Loading success flag
    bool success = true;

    //Load arrow
    if (!spaceshipTexture.loadFromFile("../ship.png"))
    {
        printf("Failed to load arrow texture!\n");
        success = false;
    }

    return success;
}


// SPACESHIP LOGIC

void initSpaceships() {
    std::cout << "Initializing Spaceship Module \n";
    if (firstInitSpaceship) {
        spaceships = new TSpaceship[MAX_SPACESHIPS];
        //create renderer for window.
        spaceshipRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (spaceshipRenderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
            close();
            exit(1);
        }
        else {
            // initialize renderer as white.
            std::cout << "assigned renderer successfully. \n";
            SDL_SetRenderDrawColor(spaceshipRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
            int imgFlags = IMG_INIT_PNG;

            if (!(IMG_Init(imgFlags) & imgFlags)) {
                printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
                close();
                exit(1);
            }
        }

        if (!loadMedia()) {
            printf("Failed to load media!\n");
            close();
            exit(1);
        }

        std::cout << "loaded media. \n";

        int i = 0;
        int heights[2] = { 20, SCREEN_HEIGHT - 20 };
        int widths[2] = { 20, SCREEN_WIDTH - 20 };

        for (i = 0; i < MAX_SPACESHIPS; i++) {
            spaceships[i].active = false;
            spaceships[i].TTL = SPACESHIP_TTL;
            int a = widths[rand() % 2];
            int b = heights[rand() % 2];
            spaceships[i].start_x = a;
            spaceships[i].start_y = b;
            spaceships[i].x = a;
            spaceships[i].y = b;

            if (spaceships[i].x == 20) {
                if (spaceships[i].y == 20) { // top left
                    spaceships[i].rotation = 115;
                }
                else { // bottom left
                    spaceships[i].rotation = 45;
                }
            }
            else {
                if (spaceships[i].y == 20) { // top right
                    spaceships[i].rotation = 225;
                }
                else { // bottom right
                    spaceships[i].rotation = 315;
                }
            }
        }
        firstInitSpaceship = false;
    }
}

void updateSpaceships() {
    for (int i = 0; i < MAX_SPACESHIPS; i++) {
        if (spaceships[i].active) {
            spaceships[i].TTL -= deltaTime;
            if (spaceships[i].TTL <= 0) {
                std::cout << "reset spaceship ";
                spaceships[i].active = false;
                spaceships[i].x = spaceships[i].start_x;
                spaceships[i].y = spaceships[i].start_y;
            }
            else {
                switch (spaceships[i].rotation) {
                case 45:
                    spaceships[i].x++;
                    spaceships[i].y--;
                    break;
                case 115:
                    spaceships[i].x++;
                    spaceships[i].y++;
                    break;
                case 225:
                    spaceships[i].x--;
                    spaceships[i].y--;
                    break;
                case 315:
                    spaceships[i].x--;
                    spaceships[i].y++;
                    break;
                }
            }
        }
    }
}

void renderSpaceships() {
    //Clear screen
    SDL_SetRenderDrawColor(spaceshipRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(spaceshipRenderer);

    for (int i = 0; i < MAX_SPACESHIPS; i++) {
        TSpaceship s = spaceships[i];
        if (spaceships[i].active) {
            spaceshipTexture.render(spaceships[i].x, spaceships[i].y, NULL, spaceships[i].rotation, NULL, SDL_FLIP_HORIZONTAL);
        }
    }

    SDL_RenderPresent(spaceshipRenderer);
}


void initMusic() {
    std::cout << "Initializing Music Module \n";
    if (firstInitMusic) {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
        Mix_Init(MIX_INIT_OGG);
        imperial = Mix_LoadMUS("../imperial.ogg");
        if (!imperial) {
            std::cout << "Error loading Music: " << Mix_GetError() << std::endl;
            close();
            exit(1);
        }
        Mix_PlayMusic(imperial, 0);
        MusicCurrentTime = 0;
        MusicCurrentTimeBeat = 0;
        MusicCurrentBeat = 0;
        MusicPreviousBeat = -1;
        firstInitMusic = false;
    }
}

void updateMusic(){
    MusicCurrentTime += deltaTime;
    MusicCurrentTimeBeat += deltaTime;
    MusicPreviousBeat = MusicCurrentBeat;
    if (MusicCurrentTimeBeat >= MSEG_BPM) {
        std::cout << "New beat \n";
        MusicCurrentTimeBeat = 0;
        MusicCurrentBeat++;
        int i;
        for (i= 0; i < MAX_SPACESHIPS; i++){
            if (!spaceships[i].active) {
                std::cout << "activated new spaceship \n ";
                spaceships[i].active = true;
                break;
            }
        }
    }
}

int main(int argc, char* args[])
{
    //Initialize SDL
    if (!initSDL())
    {
        std::cout << "Failed to initialize!\n";
        return 1;
    }
    else
    {
        //Modules initialization
        initCorrespondingModule();

        //Main loop flag
        bool quit = false;

        //Event handler
        SDL_Event e;

        while (!quit) {
            // Handle events on queue
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