import sys
import os
import pathlib

p = str(pathlib.Path().absolute()) + "\SDL2.dll"
os.environ["PYSDL2_DLL_PATH"] = p

import sdl2.ext
import sdl2
from perlin_noise import PerlinNoise
import matplotlib.pyplot as plt
from PIL import Image

##################################
#   PERLIN NOISE INTERPOLATION   #
##################################

# This is applied precalculus as the perlin noise images are generated before the 

# octaves : number of sub rectangles in each [0, 1] range
# seed : specific seed with which you want to initialize random generator

p_octaves = 6 # octaves to use for perlin noise.
p_numnoises = 8 # number of individial noisemaps to generate.
xpix, ypix = 150, 150 # pixel size of maps (this option greatly affects performance, carefull when changing it to bigger values).
p_transition = 3 # transitional noise pictures.
create_new = False # if you want to create new noisemaps, choose False to use previously generated, do not change other configs if done so.

generated_img_total = p_numnoises*(p_transition+1)-p_transition
base_noises = []

def dec_to_rgb(dec):
    return int((1 + dec) * 127.5)

if create_new:
    # generate noise maps.
    for x in range (p_numnoises):
        noise = PerlinNoise(octaves = p_octaves)
        base_noises.append(noise)
        print("generated noisemap: " + str(x+1))


    img_count = 0
    for x in range(0, p_numnoises):
        # generate an image based on the noisemap.
        img = Image.new('RGB',(xpix,ypix), "black") # Create a new black image
        pixels = img.load()

        for i in range(ypix):
            for j in range(xpix):
                v = dec_to_rgb(base_noises[x]([i/xpix, j/ypix]))
                pixels[i,j] = (v,v,v)

        img_count += 1
        img.save("resources\demoscene2_images\img"+str(img_count)+".bmp")
        print("processed picture: " +str(img_count) +"/"+str(generated_img_total))
        
        # generate intermediate images.
        if(x < p_numnoises-1):
            for z in range(p_transition):
                img = Image.new('RGB',(xpix,ypix), "black") # Create a new black image
                pixels = img.load()

                for i in range(ypix):
                    for j in range(xpix):
                        noise_val = base_noises[x]([i/xpix, j/ypix])
                        noise_val += (z+1/(p_transition+1)) * base_noises[x+1]([i/xpix, j/ypix])
                        v = dec_to_rgb(noise_val)
                        pixels[i,j] = (v,v,v)

                img_count += 1
                img.save("resources\demoscene2_images\img"+str(img_count)+".bmp")
                print("processed picture: " +str(img_count) +"/"+str(generated_img_total))


######################
#   MAIN SDL2 LOOP   #  
######################

RESOURCES = sdl2.ext.Resources(__file__,"resources/demoscene2_images")

sdl2.ext.init()
window = sdl2.ext.Window("Perlin Interpolation", size=(640, 480))
window.show()

world = sdl2.ext.World()

factory = sdl2.ext.SpriteFactory(sdl2.ext.SOFTWARE)

sprites = []
for x in range(generated_img_total):
    sprite = factory.from_image(RESOURCES.get_path("img"+str(x+1)+".bmp"))
    sprite.position = 320-int(xpix/2), 240-int(ypix/2)
    sprites.append(sprite)

spriterenderer = factory.create_sprite_render_system(window)
running = True

count = 0

while running: # Main event loop for the SDL program.

    events = sdl2.ext.get_events()
    for event in events:
        if event.type == sdl2.SDL_QUIT:
            running = False
            break
    if count == generated_img_total-1:
        count = 0

    spriterenderer.render(sprites[count])

    count +=1

    sdl2.SDL_Delay(10)
    world.process()

sys.exit()