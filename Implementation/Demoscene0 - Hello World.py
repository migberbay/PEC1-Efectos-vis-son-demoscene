import sys

import os
import pathlib

p = str(pathlib.Path().absolute()) + "\SDL2.dll"
os.environ["PYSDL2_DLL_PATH"] = p

import sdl2.ext
import sdl2

###################
#   HELLO WORLD   #
###################

RESOURCES = sdl2.ext.Resources(__file__,"resources")

sdl2.ext.init()

window = sdl2.ext.Window("Hello World!", size=(640, 480))
window.show()

factory = sdl2.ext.SpriteFactory(sdl2.ext.SOFTWARE)
sprite = factory.from_image(RESOURCES.get_path("hello.bmp"))

# will cause the renderer to draw the sprite 10px to the right and
# 20 px to the bottom
# sprite.position = 10, 20

spriterenderer = factory.create_sprite_render_system(window)
spriterenderer.render(sprite)

processor = sdl2.ext.TestEventProcessor()
processor.run(window)

sdl2.ext.quit()