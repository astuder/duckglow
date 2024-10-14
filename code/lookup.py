# helper script to create lookup table for LED fading

import math

steps = 1024
zeroes = 128

with open('lookup.h', 'w', encoding='ascii') as lookup:
    lookup.write("uint16_t sine[] = {\n\t")
    i = 0
    for n in range (0, steps, 1):
        f = (((2*math.pi) / float(steps)) * n) + (1.5*math.pi)
        s = int((math.sin(f)+1.0) * 65536.0 / 2.0)
        if s > 65535:
            s = 65535
        lookup.write("{},".format(s))
        if ((i & 7) == 7):
            lookup.write("\n\t");
        i += 1
    for y in range(0, zeroes >> 3):
        lookup.write("0, 0, 0, 0, 0, 0, 0, 0,\n\t")
        i += 8
    lookup.write("};\n")
    lookup.write("uint16_t sine_size = {};\n".format(i));
