#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS   15
#define LCD_DC   2
#define LCD_RST  4
#define LCD_BLK  32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

void setup() {
  lcd.init(170, 320);
  lcd.fillScreen(ST77XX_BLACK);
}

void loop() {
  lcd.setTextSize(3);
  lcd.print("Hello, ideaspark");
  delay(100000);
}

