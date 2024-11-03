import time

# controlling duckGLOW LED over I2C

# I2C device address of duck
DUCK_ADDR = 0x6c

# I2C registers of duckGLOW
DUCK_REG_MAX_RGBA   = 0xec
DUCK_REG_MIN_RGBA   = 0xf0
DUCK_REG_SPEED_RGBA = 0xf4
DUCK_REG_PHASE_RGBA = 0xf8
DUCK_REG_MODE       = 0xfc
DUCK_REG_SAVE       = 0xfd


# max brightness (0-255)
brightness=128
# animation speed (lower=faster)
speed=20

def demo_i2c():
    # get I2c bus(es) holding the duck(s)
    duck_bus = which_bus_has_device_id(DUCK_ADDR)
    if len(duck_bus) == 0:
        # bail if there's no duck found
        print("no duck :(")
        return
    
    # set speed to 0, disables automatic fading of LEDs
    duck_bus[0].writeto_mem(DUCK_ADDR, DUCK_REG_SPEED_RGBA, bytes([0,0,0,0]))  
    
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
            # update max brightness of RGB and background
            duck_bus[0].writeto_mem(DUCK_ADDR, DUCK_REG_MAX_RGBA, bytes([v&r,v&g,v&b,v]))
            v += 1
            time.sleep_ms(speed)
        while (v > 0):
            duck_bus[0].writeto_mem(DUCK_ADDR, DUCK_REG_MAX_RGBA, bytes([v&r,v&g,v&b,v]))
            v -= 1
            time.sleep_ms(speed)
        c += 1
        if (c > 7):
            c = 1

demo_i2c()