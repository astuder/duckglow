import neopixel
import time

# controlling duckGLOW LED like a NeoPixel
# note that the duck will switch/glitch to default animation when
# an I2C write is detected

# GPIO2 of duck (gpio_2) for WS2812 control
duckgp2=gpio42
# max brightness (0-255)
brightness=128
# animation speed (lower=faster)
speed=20

def demo_ws2812():
    np = neopixel.NeoPixel(duckgp2, 2)
    c=1
    while True:
        if c&1:
            r = 0xff
        else:
            r = 0
        if c&2:
            g = 0xff
        else:
            g = 0
        if c&4:
            b = 0xff
        else:
            b = 0
        v = 0
        while (v < brightness):
            # background LED, only B-channel is connected
            np[0] = (0, 0, v)
            # RGB LED
            np[1] = (v&r, v&g, v&b)
            np.write()
            v += 1
            time.sleep_ms(speed)
        while (v > 0):
            np[0] = (0, 0, v)
            np[1] = (v&r, v&g, v&b)
            np.write()
            v -= 1
            time.sleep_ms(speed)
        c += 1
        if (c > 7):
            c = 1

demo_ws2812()