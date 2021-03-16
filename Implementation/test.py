import time
import math

a = time.time()

for _ in range (10000):
    c = 1+1
    d = math.sqrt(c)

b = time.time()

print((b-a)*1000)