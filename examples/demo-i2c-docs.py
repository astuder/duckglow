import time

# reading memory of duck over I2C

# I2C device address of duck
DUCK_ADDR = 0x6c

# I2C registers of duckGLOW
DUCK_REG_DOCS       = 0x00
DUCK_REG_MAX_RGBA   = 0xec
DUCK_REG_MIN_RGBA   = 0xf0
DUCK_REG_SPEED_RGBA = 0xf4
DUCK_REG_PHASE_RGBA = 0xf8
DUCK_REG_MODE       = 0xfc
DUCK_REG_SAVE       = 0xfd

duck_bus = which_bus_has_device_id(DUCK_ADDR)
if len(duck_bus) == 0:
    # there's no duck found
    print("no duck :(")
else:
    # read docs over I2C
    docs = duck_bus[0].readfrom_mem(DUCK_ADDR, DUCK_REG_DOCS, DUCK_REG_MAX_RGBA-DUCK_REG_DOCS)
    print(docs.decode("ascii"))
