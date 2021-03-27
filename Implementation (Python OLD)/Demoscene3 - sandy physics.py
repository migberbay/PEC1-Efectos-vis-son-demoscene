import sys
import sdl2.ext
import sdl2
import random
import time

#################
#   HOURGLASS   #
#################

num_grains = 300
TARGET_FPS = 20
GRAVITY = 1 # pixel acceleration suffered per 5 frames
BOUNCE_FACTOR = .5 # how much velocity is conserved in bounces.
COLLIDABLE_COUNT = 5 # how many collidables to create in the scene.


WHITE = sdl2.ext.Color(255, 255, 255)
SAND1 = sdl2.ext.Color(76, 70, 50)
SAND2 = sdl2.ext.Color(88, 66, 37)
SAND3 = sdl2.ext.Color(93, 80, 62)
sands = [SAND1,SAND2,SAND3]

class SoftwareRenderer(sdl2.ext.SoftwareSpriteRenderSystem):
    def __init__(self, window):
        super(SoftwareRenderer, self).__init__(window)

    def render(self, components):
        sdl2.ext.fill(self.surface, sdl2.ext.Color(0, 0, 0)) ## we paint the whole window black on every frame
        super(SoftwareRenderer, self).render(components)


class TimerSystem():
    def __init__(self, fps):
        self.ms_per_frame = int(1000/fps)
        self.curr_t = time.time() * 1000
        self.laststep = time.time() * 1000
    
    def step(self):
        self.curr_t = time.time() * 1000
        if((self.curr_t - self.laststep) > self.ms_per_frame):
            self.laststep = self.curr_t
            return True
        return False



class MovementSystem(sdl2.ext.Applicator):
    def __init__(self, minx, miny, maxx, maxy):
        super(MovementSystem, self).__init__()
        self.componenttypes = Velocity, sdl2.ext.Sprite
        self.minx = minx
        self.miny = miny
        self.maxx = maxx
        self.maxy = maxy
        self.gravityStep = 0

    def process(self, world, componentsets): # movement systems processes every components that has a sprite and a velocity.
        self.gravityStep += 1
        
        for velocity, sprite in componentsets:
            if(self.gravityStep >= 5):
                velocity.vy += GRAVITY 
            
            swidth, sheight = sprite.size
            sprite.x += velocity.vx
            sprite.y += velocity.vy

            sprite.x = max(self.minx, sprite.x)
            sprite.y = max(self.miny, sprite.y)

            pmaxx = sprite.x + swidth
            pmaxy = sprite.y + sheight

            if pmaxx > self.maxx:
                sprite.x = self.maxx - swidth
            if pmaxy > self.maxy:
                sprite.y = self.maxy - sheight
        
        if(self.gravityStep >= 5):        
            self.gravityStep = 0


class CollisionSystem(sdl2.ext.Applicator): 
    def __init__(self, minx, miny, maxx, maxy):
        super(CollisionSystem, self).__init__()
        self.componenttypes = Velocity, sdl2.ext.Sprite
        self.grains = []
        self.collidables = []
        self.minx = minx
        self.miny = miny
        self.maxx = maxx
        self.maxy = maxy

    def _overlap(self, grain):
        g_vel, g_sprite = grain
        g_left, g_top, g_right, g_bottom = g_sprite.area

        for col in self.collidables:
            c_sprite = col.sprite
            c_left, c_top, c_right, c_bottom = c_sprite.area

            cond1 = g_left < c_right
            cond2 = g_right > c_left
            cond3 = g_top < c_bottom
            cond4 = g_bottom > c_top

            if(cond1 and cond2 and cond3 and cond4):
                if(g_sprite.position[0] < c_sprite.position[0]): #grain is to the left. 
                    g_vel.vx = random.randint(-5,-2)
                else:
                    g_vel.vx = random.randint(2,5)

                g_vel.vy = int(-g_vel.vy * BOUNCE_FACTOR)

    def process(self, world, componentsets):
        for grain in componentsets:
            # comp is a sprite and a velocity from an object with collision.
            self._overlap(grain) 
            

class Velocity(object):
    def __init__(self):
        super(Velocity, self).__init__()
        self.vx = 0
        self.vy = 0


class Obstacle(sdl2.ext.Entity):
    def __init__(self, world, sprite, posx=0, posy=0, ai=False):
        self.sprite = sprite
        self.sprite.position = posx, posy


class Grain(sdl2.ext.Entity):
    def __init__(self, world, sprite, posx=0, posy=0):
        self.sprite = sprite
        self.sprite.position = posx, posy
        self.velocity = Velocity()

def generate_collidable(factory, world, collision_system, width, height, posx, posy):
    sp_obs = factory.from_color(WHITE, size=(width, height))
    obstacle = Obstacle(world, sp_obs, posx, posy)
    collision_system.collidables.append(obstacle)
    

def run():
    sdl2.ext.init()
    window = sdl2.ext.Window("Physics", size=(640, 480))
    window.show()
    
    world = sdl2.ext.World()

    timer = TimerSystem(TARGET_FPS)
    movement = MovementSystem(0, 0, 640, 480)
    collision = CollisionSystem(0, 0, 640, 480)
    spriterenderer = SoftwareRenderer(window)

    world.add_system(movement)
    world.add_system(collision)
    world.add_system(spriterenderer)

    factory = sdl2.ext.SpriteFactory(sdl2.ext.SOFTWARE)

    for x in range(num_grains):
        sp_grain = factory.from_color(sands[random.randint(0,2)], size=(random.randint(2,4), random.randint(2,4)))
        grain = Grain(world, sp_grain, x*6%640, random.randint(1,15))
        grain.velocity.vy = 2
        collision.grains.append(grain)

    for x in range(5):
        generate_collidable(factory, world, collision, random.randint(20,40), random.randint(20,40), random.randint(120,520), random.randint(150,300))
    
    running = True
    while running: # Main event loop for the SDL program.
        events = sdl2.ext.get_events()

        for event in events:
            if event.type == sdl2.SDL_QUIT:
                running = False
                break
        
        if(timer.step()):
            timer.laststep = timer.curr_t
            world.process()

    return 0

if __name__ == "__main__":
    sys.exit(run())
