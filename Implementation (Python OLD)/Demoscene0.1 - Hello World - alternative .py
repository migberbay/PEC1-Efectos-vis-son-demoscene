import sys
import ctypes

# import sdl2 as sdl

# def main():
#     sdl.SDL_Init(sdl.SDL_INIT_VIDEO)
#     window = sdl.SDL_CreateWindow(b"Hello World", sdl.SDL_WINDOWPOS_CENTERED, sdl.SDL_WINDOWPOS_CENTERED, 592, 460, sdl.SDL_WINDOW_SHOWN)
#     windowsurface = sdl.SDL_GetWindowSurface(window)

#     image = sdl.SDL_LoadBMP(b"resources/hello.bmp")
#     sdl.SDL_BlitSurface(image, None, windowsurface, None)

#     sdl.SDL_UpdateWindowSurface(window)
#     sdl.SDL_FreeSurface(image)

#     running = True
#     event = sdl.SDL_Event()
#     while running:
#         while sdl.SDL_PollEvent(ctypes.byref(event)) != 0:
#             if event.type == sdl.SDL_QUIT:
#                 running = False
#                 break

#     sdl.SDL_DestroyWindow(window)
#     sdl.SDL_Quit()
#     return 0

from sdl2 import *

def main():
    SDL_Init(SDL_INIT_VIDEO)
    window = SDL_CreateWindow(b"Hello World",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              592, 460, SDL_WINDOW_SHOWN)
    windowsurface = SDL_GetWindowSurface(window)

    image = SDL_LoadBMP(b"resources/hello.bmp")
    SDL_BlitSurface(image, None, windowsurface, None)

    SDL_UpdateWindowSurface(window)
    SDL_FreeSurface(image)

    running = True
    event = SDL_Event()
    while running:
        while SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == SDL_QUIT:
                running = False
                break

    SDL_DestroyWindow(window)
    SDL_Quit()
    return 0

if __name__ == "__main__":
    sys.exit(main())