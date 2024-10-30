#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define DUCK_ADDR 0x6c
#define DUCK_REG_MAX_RGBA 0xec
#define DUCK_REG_MIN_RGBA 0xf0
#define DUCK_REG_SPEED_RGBA 0xf4
#define DUCK_REG_PHASE_RGBA 0xf8
#define DUCK_REG_MODE 0xfc
#define DUCK_REG_SAVE 0xfd

#define NEOPIXEL_PIN 6
#define NUMPIXELS 1

uint8_t blue = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_WGRB);

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pixels.begin();
  delay(5000);
  show_menu();
}

void show_menu() {
  Serial.println();
  Serial.println("1) show documentation");
  Serial.println("2) show registers");
  Serial.println("3) edit registers *TODO*");
  Serial.println("4) demo i2c color control");
  Serial.println("5) demo ws2812 color control");  
  Serial.println("8) save settings");
  Serial.println("9) factory reset");
  Serial.println("0) configure I2C address *TODO*");
  Serial.print("> ");
}

void show_docs() {
  Serial.println("---");
  Wire.beginTransmission(DUCK_ADDR);
  Wire.write((uint8_t) 0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(DUCK_ADDR, DUCK_REG_MAX_RGBA);
  while (Wire.available()) {
    char c = Wire.read();
    if (c == 0) break;
    Serial.print(c);
  }
  while (Wire.available()) {
    // ignore remaining bytes
    char c = Wire.read();
  }
  Serial.println("---");
}

void show_regs() {
  Serial.println("---");
  Serial.println(      "      R  G  B  BG");
  char buffer[30];
  Wire.beginTransmission(DUCK_ADDR);
  Wire.write((uint8_t) DUCK_REG_MAX_RGBA);
  Wire.endTransmission(false);
  Wire.requestFrom(DUCK_ADDR, 19);
  snprintf(buffer, 30, "Max   %02x %02x %02x %02x", Wire.read(), Wire.read(), Wire.read(), Wire.read());
  Serial.println(buffer);
  snprintf(buffer, 30, "Min   %02x %02x %02x %02x", Wire.read(), Wire.read(), Wire.read(), Wire.read());
  Serial.println(buffer);
  snprintf(buffer, 30, "Speed %02x %02x %02x %02x", Wire.read(), Wire.read(), Wire.read(), Wire.read());
  Serial.println(buffer);
  snprintf(buffer, 30, "Phase %02x %02x %02x %02x", Wire.read(), Wire.read(), Wire.read(), Wire.read());
  Serial.println(buffer);
  snprintf(buffer, 30, "Mode  %02x", Wire.read());
  Serial.println(buffer);
  Wire.read(); // dummy read to skip save
  snprintf(buffer, 30, "Error %02x", Wire.read());
  Serial.println(buffer);
  Serial.println("---");
}

uint8_t state_backup[16];

void save_state() {
  // TODO: save current I2C register state
}

void restore_state() {
  // TODO: restire I2C register state
}

void led_demo_i2c() {
  Serial.println("LEDs controlled over I2C. Press any key to exit.");
  save_state();
  // stop fading
  Wire.endTransmission();
  Wire.beginTransmission(DUCK_ADDR);
  Wire.write((uint8_t) DUCK_REG_SPEED_RGBA);
  Wire.write(0x0);
  Wire.write(0x0);
  Wire.write(0x0);
  Wire.endTransmission();

  uint8_t c = 1;
  uint8_t v = 0;
  uint8_t exit = 0;
  uint8_t r, g, b;
  
  while (!exit) {
    r = (c & 1) ? 0xff : 0;
    g = (c & 2) ? 0xff : 0;
    b = (c & 4) ? 0xff : 0;

    v = 0;
    while (!exit && v < 255) {
      Wire.beginTransmission(DUCK_ADDR);
      Wire.write((uint8_t) DUCK_REG_MAX_RGBA);
      Wire.write(v & r);
      Wire.write(v & g);
      Wire.write(v & b);
      Wire.write(v);    // background LEDs
      Wire.endTransmission();
      if (Serial.available()) {
        exit = 1;
      }
      v++;
      delay(10);
    }
    while (!exit && v > 0) {
      Wire.beginTransmission(DUCK_ADDR);
      Wire.write((uint8_t) DUCK_REG_MAX_RGBA);
      Wire.write(v & r);
      Wire.write(v & g);
      Wire.write(v & b);
      Wire.write(v);  // background LEDs
      Wire.endTransmission();
      if (Serial.available()) {
        exit = 1;
      }
      v--;
      delay(10);
    }
    c++;
    if (c > 7) {
      c = 1;
    }
  }
  restore_state();

  // flush any keys pressed
  while (Serial.available()) {
    Serial.read();
  }
}

void led_demo_ws2812() {
  Serial.println("LEDs controlled over GPIO2. Press any key to exit.");
  save_state();
  uint8_t c = 1;
  uint8_t v = 0;
  uint8_t exit = 0;
  uint8_t r, g, b;
  
  while (!exit) {
    r = (c & 1) ? 0xff : 0;
    g = (c & 2) ? 0xff : 0;
    b = (c & 4) ? 0xff : 0;

    v = 0;
    while (!exit && v < 255) {
      pixels.setPixelColor(0, pixels.Color(v & r, v & g, v & b, v));
      pixels.show(); 
      if (Serial.available()) {
        exit = 1;
      }
      v++;
      delay(10);
    }
    while (!exit && v > 0) {
      pixels.setPixelColor(0, pixels.Color(v & r, v & g, v & b, v));
      pixels.show(); 
      if (Serial.available()) {
        exit = 1;
      }
      v--;
      delay(10);
    }
    c++;
    if (c > 7) {
      c = 1;
    }
  }  
  restore_state();

  // flush any keys pressed
  while (Serial.available()) {
    Serial.read();
  }
}

void edit_regs() {
  // TODO: interactively set I2C register values
}

void save_presets() {
  Serial.print("saving presets...");
  Wire.beginTransmission(DUCK_ADDR);
  Wire.write((uint8_t) DUCK_REG_SAVE);
  Wire.write((uint8_t) 42);
  Wire.endTransmission();
  delay(500);
  Serial.println(" done!");
}

void factory_reset() {
  Serial.print("factory reset...");
  Wire.beginTransmission(DUCK_ADDR);
  Wire.write((uint8_t) DUCK_REG_SAVE);
  Wire.write((uint8_t) 24);
  Wire.endTransmission();
  delay(500);
  Serial.println(" done!");
}

void loop() {
  
  // put your main code here, to run repeatedly:  
  if (Serial.available()) {
    char c = Serial.read();
    Serial.println(c);
    switch (c) {
      case '1':
        show_docs();
        break;
      case '2':
        show_regs();
        break;
      case '3':
        edit_regs();
      case '4':
        led_demo_i2c();
        break;
      case '5':
        led_demo_ws2812();
        break;
      case '8':
        save_presets();
        break;
      case '9':
        factory_reset();
        break;
      default:
        Serial.println("Error: unknown command");
        break;
    }
    show_menu();
    while (Serial.available()) {
      // flush extra characters (like line-feed)
      Serial.read();
    }
  }
}

