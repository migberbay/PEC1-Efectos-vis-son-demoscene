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

// The window we'll be rendering to
SDL_Window* window = NULL;

// The surface contained by the window
SDL_Surface* screenSurface = NULL;

// Frame Logic
#define FPS 60
int lastTime = 0, currentTime, deltaTime;
float msFrame = 1 / (FPS / 1000.0f);

// TRANSITION & DEMO HANDLER VARIABLES
const int ALLOCATED_DEMO_TIME = 2500;
const int ALLOCATED_TRANSITION_TIME = 1500;

// 0->transition, 1->stars, 2->plasma
int current_demo = 1; 
int prev_demo = 0;

// milliseconds left until swap.
int current_time_left = ALLOCATED_DEMO_TIME;

// number of effects in demoscene.
int numDemos = 2;

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

// THREE DIMENSIONS
 SDL_Surface* texture;
//256 x 256 light patter buffer.
unsigned char *light;

// 16 bit buffer
unsigned char *zbuffer;

//Lookup table
unsigned char *lut;

// SpaceShip properties.
const float SPC_BASE_WIDTH = 60;
const float SPC_BASE_HEIGHT = 10;
const float SPC_LENGTH = 90;


bool spaceFirstInit = true;

// we need two structures, one that holds the position of all vertices
// in object space,  and the other in screen space. the coords in world
// space doesn't need to be stored
struct
{
    VECTOR* vertices, * normals;
} org, cur;

// this structure contains all the relevant data for each poly
typedef struct
{
    int p[4];  // pointer to the vertices
    int tx[4]; // static X texture index
    int ty[4]; // static Y texture index
    VECTOR normal, centre;
} POLY;

POLY* polies;

// count values
int num_polies;
int num_vertices;

// one entry of the edge table
typedef struct {
    int x, px, py, tx, ty, z;
} edge_data;

// store two edges per horizontal line
edge_data edge_table[SCREEN_HEIGHT][2];

// remember the highest and the lowest point of the polygon
int poly_minY, poly_maxY;

// object position and orientation
MATRIX objrot;
VECTOR objpos;

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

// 3D
void init3D();
void update3D();
void render3D();

void InitEdgeTable();
void ScanEdge(VECTOR p1, int tx1, int ty1, int px1, int py1, VECTOR p2, int tx2, int ty2, int px2, int py2);
void DrawSpan(int y, edge_data* p1, edge_data* p2);
void DrawPolies();
void init_object();
void TransformPts();


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
        update3D();
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
        render3D();
        break;
    }
}

void initCorrespondingModule() {
    // 0->transition, 1->stars, 2->plasma
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
        init3D();
        break;
    }
}

void close() {
    // free memory
    delete[](stars);

    free(plasma1);
    free(plasma2);

    free(transBuffer);

    free(zbuffer);
    free(org.vertices);
    free(org.normals);
    free(cur.vertices);
    free(cur.normals);
    free(polies);

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
                std::cout << "Last demo, ";
                current_demo = 1;
            }
            else {
                std::cout << "Not last demo, ";
                current_demo = prev_demo + 1;
            }
            prev_demo = 0;
            current_time_left = ALLOCATED_DEMO_TIME;
        }
        else { // we change from effect to transition.
            prev_demo = current_demo;
            std::cout << "From effect to Transition,  ";
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

// 3D spaceships
void init3D() {

    if (spaceFirstInit) {
        // Load Texture
        SDL_Surface* temp = IMG_Load("spacecship.png");
        if (temp == NULL) {
            std::cout << "Image can be loaded! " << IMG_GetError();
            close();
            exit(1);
        }
        texture = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);

        // prepare the lighting
        light = new unsigned char[256 * 256];
        for (int j = 0; j < 256; j++)
        {
            for (int i = 0; i < 256; i++)
            {
                // calculate distance from the centre
                int c = ((128 - i) * (128 - i) + (128 - j) * (128 - j)) / 35;
                // check for overflow
                if (c > 255) c = 255;
                // store lumel
                light[(j << 8) + i] = 255 - c;
            }
        }
        // prepare 3D data
        zbuffer = (unsigned short*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(unsigned short));
        init_object(); // we generate the 3d object
        spaceFirstInit = false;
    }
}

void update3D() {
    // clear the zbuffer
    memset(zbuffer, 255, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(unsigned short));

    // set the spaceship' rotation 
    objrot = rotX(rand()%45) * rotY(0) * rotZ(45);

    // and it's position --> depending on the beat we are on we will go faster or slower.
    objpos = VECTOR(
        48 * cos((float)currentTime / 1266.0f),
        48 * sin((float)currentTime / 1424.0f),
        200 + 80 * sin((float)currentTime / 1912.0f));
    // rotate and project our points
    TransformPts();
}

void render3D() {

    // clear the background
    SDL_FillRect(screenSurface, NULL, 0);
    // and draw the polygons
    DrawPolies();
}

//clears all entries in the edge table
void InitEdgeTable()
{
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        edge_table[i][0].x = -1;
        edge_table[i][1].x = -1;
    }
    poly_minY = SCREEN_HEIGHT;
    poly_maxY = -1;
}

/*
* scan along one edge of the poly, i.e. interpolate all values and store
* in the edge table
*/
void ScanEdge(VECTOR p1, int tx1, int ty1, int px1, int py1,
    VECTOR p2, int tx2, int ty2, int px2, int py2)
{
    // we can't handle this case, so we recall the proc with reversed params
    // saves having to swap all the vars, but it's not good practice
    if (p2[1] < p1[1]) {
        ScanEdge(p2, tx2, ty2, px2, py2, p1, tx1, ty1, px1, py1);
        return;
    }
    // convert to fixed point
    int x1 = (int)(p1[0] * 65536),
        y1 = (int)(p1[1]),
        z1 = (int)(p1[2] * 16),
        x2 = (int)(p2[0] * 65536),
        y2 = (int)(p2[1]),
        z2 = (int)(p2[2] * 16);
    // update the min and max of the current polygon
    if (y1 < poly_minY) poly_minY = y1;
    if (y2 > poly_maxY) poly_maxY = y2;
    // compute deltas for interpolation
    int dy = y2 - y1;
    if (dy == 0) return;
    int dx = (x2 - x1) / dy,                // assume 16.16 fixed point
        dtx = (tx2 - tx1) / dy,
        dty = (ty2 - ty1) / dy,
        dpx = (px2 - px1) / dy,
        dpy = (py2 - py1) / dy,
        dz = (z2 - z1) / dy;              // probably 12.4, but doesn't matter
                                          // interpolate along the edge
    for (int y = y1; y < y2; y++)
    {
        // don't go out of the screen
        if (y > (SCREEN_HEIGHT - 1)) return;
        // only store if inside the screen, we should really clip
        if (y >= 0)
        {
            // is first slot free?
            if (edge_table[y][0].x == -1)
            { // if so, use that
                edge_table[y][0].x = x1;
                edge_table[y][0].tx = tx1;
                edge_table[y][0].ty = ty1;
                edge_table[y][0].px = px1;
                edge_table[y][0].py = py1;
                edge_table[y][0].z = z1;
            }
            else { // otherwise use the other
                edge_table[y][1].x = x1;
                edge_table[y][1].tx = tx1;
                edge_table[y][1].ty = ty1;
                edge_table[y][1].px = px1;
                edge_table[y][1].py = py1;
                edge_table[y][1].z = z1;
            }
        }
        // interpolate our values
        x1 += dx;
        px1 += dpx;
        py1 += dpy;
        tx1 += dtx;
        ty1 += dty;
        z1 += dz;
    }
}

/*
* draw a horizontal double textured span
*/
void DrawSpan(int y, edge_data* p1, edge_data* p2)
{
    // quick check, if facing back then draw span in the other direction,
    // avoids having to swap all the vars... not a very elegant
    if (p1->x > p2->x)
    {
        DrawSpan(y, p2, p1);
        return;
    };
    // load starting points
    int z1 = p1->z,
        px1 = p1->px,
        py1 = p1->py,
        tx1 = p1->tx,
        ty1 = p1->ty,
        x1 = p1->x >> 16,
        x2 = p2->x >> 16;
    // check if it's inside the screen
    if ((x1 > (SCREEN_WIDTH - 1)) || (x2 < 0)) return;
    // compute deltas for interpolation
    int dx = x2 - x1;
    if (dx == 0) return;
    int dtx = (p2->tx - p1->tx) / dx,  // assume 16.16 fixed point
        dty = (p2->ty - p1->ty) / dx,
        dpx = (p2->px - p1->px) / dx,
        dpy = (p2->py - p1->py) / dx,
        dz = (p2->z - p1->z) / dx;

    // setup the offsets in the buffers
    Uint8* dst;
    Uint8* initbuffer = (Uint8*)screenSurface->pixels;
    int bpp = screenSurface->format->BytesPerPixel;
    Uint8* imagebuffer = (Uint8*)texture->pixels;
    int bppImage = texture->format->BytesPerPixel;

    // get destination offset in buffer
    long offs = y * SCREEN_WIDTH + x1;
    // loop for all pixels concerned
    for (int i = x1; i < x2; i++)
    {
        if (i > (SCREEN_WIDTH - 1)) return;
        // check z buffer
        if (i >= 0) if (z1 < zbuffer[offs])
        {
            // if visible load the texel from the translated texture
            Uint8* p = (Uint8*)imagebuffer + ((ty1 >> 16) & 0xff) * texture->pitch + ((tx1 >> 16) & 0xFF) * bppImage;
            SDL_Color ColorTexture;
            SDL_GetRGB(*(Uint32*)(p), texture->format, &ColorTexture.r, &ColorTexture.g, &ColorTexture.b);
            // and the texel from the light map
            unsigned char LightFactor = light[((py1 >> 8) & 0xff00) + ((px1 >> 16) & 0xff)];
            // mix them together, and store
            int ColorR = (ColorTexture.r + LightFactor);
            if (ColorR > 255)
                ColorR = 255;
            int ColorG = (ColorTexture.g + LightFactor);
            if (ColorG > 255)
                ColorG = 255;
            int ColorB = (ColorTexture.b + LightFactor);
            if (ColorB > 255)
                ColorB = 255;
            Uint32 resultColor = 0xFF000000 | (ColorR << 16) | (ColorG << 8) | ColorB;
            dst = initbuffer + y * screenSurface->pitch + i * bpp;
            *(Uint32*)dst = resultColor;
            // and update the zbuffer
            zbuffer[offs] = z1;
        }
        // interpolate our values
        px1 += dpx;
        py1 += dpy;
        tx1 += dtx;
        ty1 += dty;
        z1 += dz;
        // and find next pixel
        offs++;
    }
}

/*
* cull and draw the visible polies
*/
void DrawPolies()
{
    int i;
    for (int n = 0; n < num_polies; n++)
    {
        // rotate the centre and normal of the poly to check if it is actually visible.
        VECTOR ncent = objrot * polies[n].centre,
            nnorm = objrot * polies[n].normal;

        // calculate the dot product, and check it's sign
        if ((ncent[0] + objpos[0]) * nnorm[0]
            + (ncent[1] + objpos[1]) * nnorm[1]
            + (ncent[2] + objpos[2]) * nnorm[2] < 0)
        {
            // the polygon is visible, so setup the edge table
            InitEdgeTable();
            // process all our edges
            for (i = 0; i < 4; i++)
            {
                ScanEdge(
                    // the vertex in screen space
                    cur.vertices[polies[n].p[i]],
                    // the static texture coordinates
                    polies[n].tx[i], polies[n].ty[i],
                    // the dynamic text coords computed with the normals
                    (int)(65536 * (128 + 127 * cur.normals[polies[n].p[i]][0])),
                    (int)(65536 * (128 + 127 * cur.normals[polies[n].p[i]][1])),
                    // second vertex in screen space
                    cur.vertices[polies[n].p[(i + 1) & 3]],
                    // static text coords
                    polies[n].tx[(i + 1) & 3], polies[n].ty[(i + 1) & 3],
                    // dynamic texture coords
                    (int)(65536 * (128 + 127 * cur.normals[polies[n].p[(i + 1) & 3]][0])),
                    (int)(65536 * (128 + 127 * cur.normals[polies[n].p[(i + 1) & 3]][1]))
                );
            }
            // quick clipping
            if (poly_minY < 0) poly_minY = 0;
            if (poly_maxY > SCREEN_HEIGHT) poly_maxY = SCREEN_HEIGHT;
            // do we have to draw anything?
            if ((poly_minY < poly_maxY) && (poly_maxY > 0) && (poly_minY < SCREEN_HEIGHT))
            {
                // if so just draw relevant lines
                for (i = poly_minY; i < poly_maxY; i++)
                {
                    DrawSpan(i, &edge_table[i][0], &edge_table[i][1]);
                }
            }
        }
    }
}

/*
* generate a Spaceship object
*/
void init_object()
{
    // allocate necessary memory for points and their normals
    num_vertices = 5;
    org.vertices = new VECTOR[num_vertices];
    cur.vertices = new VECTOR[num_vertices];
    org.normals = new VECTOR[num_vertices];
    cur.normals = new VECTOR[num_vertices];
    int i, j, k = 0;
    //starting at the top of the "pyramid" we move LEGNTH to the back and half BASE_WIDTH to each side then half BASE_HEIGHT up and down
    
    // top of the pyramid
    org.vertices[0] = VECTOR(0, 0, 0);

    // base points left
    org.vertices[1] = VECTOR(-SPC_BASE_WIDTH / 2, SPC_LENGTH, -SPC_BASE_HEIGHT / 2);
    org.vertices[2] = VECTOR(-SPC_BASE_WIDTH / 2, SPC_LENGTH, SPC_BASE_HEIGHT / 2);
   
    // base points right
    org.vertices[3] = VECTOR(SPC_BASE_WIDTH / 2, SPC_LENGTH, -SPC_BASE_HEIGHT / 2);
    org.vertices[4] = VECTOR(SPC_BASE_WIDTH / 2, SPC_LENGTH, SPC_BASE_HEIGHT / 2);
    
    VECTOR center = VECTOR(0, SPC_LENGTH / 2, 0);
    // normals
    for (i = 0; i < 5; i++) {
        org.normals[i] = normalize(org.vertices[i] - center);
    }

    // now initialize the polygons
    num_polies = 5;
    polies = new POLY[num_polies];
    for (i = 0; i < num_polies; i++) {
        POLY& P = polies[i];
        
        // setup the pointers to the 4 concerned vertices
        switch (i) {
        case 0:
            // BASE
            P.p[0] = 1;
            P.p[1] = 2;
            P.p[2] = 3;
            P.p[3] = 4;

            break;

        case 1:
            // LEFT Tri
            P.p[0] = 4;
            P.p[1] = 3;
            P.p[2] = 0;
            P.p[3] = NULL;
            break;

        case 2:
            // LEFT Tri
            P.p[0] = 2;
            P.p[1] = 1;
            P.p[2] = 0;
            P.p[3] = NULL;
            break;

        case 3:
            P.p[0] = 2;
            P.p[1] = 4;
            P.p[2] = 0;
            P.p[3] = NULL;
            break;

        case 4:
            P.p[0] = 1;
            P.p[1] = 3;
            P.p[2] = 0;
            P.p[3] = NULL;
            break;
        }

        int SLICES = 4;
        int SPANS = 2;

        // now compute the static texture refs (X)
        P.tx[0] = (i * 512 / SLICES) << 16;
        P.tx[1] = (i * 512 / SLICES) << 16;
        P.tx[3] = ((i + 1) * 512 / SLICES) << 16;
        P.tx[2] = ((i + 1) * 512 / SLICES) << 16;

        // now compute the static texture refs (Y)
        P.ty[0] = (j * 512 / SPANS) << 16;
        P.ty[1] = ((j + 1) * 512 / SPANS) << 16;
        P.ty[3] = (j * 512 / SPANS) << 16;
        P.ty[2] = ((j + 1) * 512 / SPANS) << 16;


        // get the normalized diagonals
        if (i == 0) {
            VECTOR d1 = normalize(org.vertices[P.p[2]] - org.vertices[P.p[0]]);
            VECTOR d2 = normalize(org.vertices[P.p[3]] - org.vertices[P.p[1]]);

            // and their dot product
            VECTOR temp = VECTOR(d1[1] * d2[2] - d1[2] * d2[1],
                d1[2] * d2[0] - d1[0] * d2[2],
                d1[0] * d2[1] - d1[1] * d2[0]);
            // normalize that and we get the face's normal
            P.normal = normalize(temp);
        }
        else {
            VECTOR face_center = (org.vertices[P.p[0]] + org.vertices[P.p[1]] + org.vertices[P.p[2]]);
            face_center[0] = face_center[0] / 3;
            face_center[1] = face_center[1] / 3;
            face_center[2] = face_center[2] / 3;
            P.normal = normalize(face_center - center);
        }
        

        // the centre of the face is just the average of the 3/4 corners
        // we could use this for depth sorting
        VECTOR temp = org.vertices[P.p[0]] + org.vertices[P.p[1]] + org.vertices[P.p[2]];
        if (i == 0) {
            temp = temp + org.vertices[P.p[3]];
            P.centre = VECTOR(temp[0] * 0.25, temp[1] * 0.25, temp[2] * 0.25);
        }
        else {
            P.centre = VECTOR(temp[0] * 0.33, temp[1] * 0.33, temp[2] * 0.33);
        }
    }
}

/*
* rotate and project all vertices, and just rotate point normals
*/
void TransformPts()
{
    for (int i = 0; i < num_vertices; i++)
    {
        // perform rotation
        cur.normals[i] = objrot * org.normals[i];
        cur.vertices[i] = objrot * org.vertices[i];
        // now project onto the screen
        cur.vertices[i][2] += objpos[2];
        cur.vertices[i][0] = SCREEN_HEIGHT * (cur.vertices[i][0] + objpos[0]) / cur.vertices[i][2] + (SCREEN_WIDTH / 2);
        cur.vertices[i][1] = SCREEN_HEIGHT * (cur.vertices[i][1] + objpos[1]) / cur.vertices[i][2] + (SCREEN_HEIGHT / 2);
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
        IMG_Init(IMG_INIT_PNG);

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