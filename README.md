# duckGLOW SAO

duckGLOW is an SAO with a glowing rubber duck, lit up by an RGB or UV LED,
controllable over I2C, or like an addressable LED following the WS2812
'standard'.

See [hackaday.io](https://hackaday.io/project/198918-duckglow-sao) for more
information.

## I2C Interface

The LED of the duckGLOW SAO can be controlled over I2C. 

The SAO has four channels: red, green and blue for the duck LED, and on the
duckJAWS variant, red background lightning.

The SAO implements smooth fading configurable per channel. The LED brightness
fades between **max** and **min** value, at the chosen **speed**. If speed is 0,
the LED will steadily glow at the configured maximum. The **phase** parameter
sets the offset in the fading waveform.

The **mode** register can be used to select the interface for LED control
(auto-detect, I2C or WS2812), and to check what type of LED was detected
(RGB or UV). 

The current settings can be stored as power-on preset by writing decimal *42*
to the **save** register.

### I2C Address

The default I2C address of the duckGLOW SAO is *`0x6C`*. The address can be 
changed by closing solder jumpers on the back of the circuit board. 
|SJ1|SJ2|Address|
|-|-|-|
|O|O|0x6C|
|C|O|0x6D|
|O|C|0x6E|
|C|C|0x6F|

### I2C Registers

Register are accessed by first writing the 8-bit register address, and then
reading or writing register data. The register address will auto-increment
and multiple registers can be read or written within a single I2C transaction. 

For example to read the max RGB colors with an Arduino:
```
Wire.beginTransmission(0x6C);
Wire.write((uint8_t) 0xEC);
Wire.endTransmission(false);
Wire.requestFrom(0x6C, 3);
while (Wire.available()) {
  char c = Wire.read();
  Serial.println(c, HEX);
}
```

And to set the red LED to maximum brightness:

```
Wire.beginTransmission(0x6C);
Wire.write((uint8_t) 0xEC); // red led max brightness
Wire.write(0xff);
Wire.endTransmission();
Wire.beginTransmission(0x6C);
Wire.write((uint8_t) 0xF4); // animation speed
Wire.write(0x0);
Wire.endTransmission();
```

|addr|register|description|
|-|-|-|
|0x00| docs | some info about the add-on |
|0xEC-EF | max | max for red, green, blue and background channels |
|0xF0-F3| min | min for red, green, blue and background  channels |
|0xF4-F7| speed | fading speed per channel, 0 no fading |
|0xF8-FB| phase | offset within fading animation per channel |
|0xFC| mode | bits 0..1: LED ctrl 0=auto, 1=I2C, 2=WS2812<br>bit 4: LED type 0=RGB, 1=UV |
|0xFD| save | write 42 to save current register values as new default | 
