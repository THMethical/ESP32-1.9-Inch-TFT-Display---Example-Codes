#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "WiFi.h"

// Deine bewährte Pin-Belegung
#define LCD_CS   15
#define LCD_DC   2
#define LCD_RST  4
#define LCD_BLK  32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

void setup() {
  Serial.begin(115200);

  // Hintergrundbeleuchtung an
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);

  // Display-Start
  lcd.init(170, 320);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);
  
  // WiFi in den Station-Modus versetzen
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  scanAndShowWiFi();
}

void scanAndShowWiFi() {
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setCursor(10, 10);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.setTextSize(2);
  lcd.println("Suche Netzwerke...");

  // Scan starten
  int n = WiFi.scanNetworks();
  
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setCursor(10, 10);
  
  if (n == 0) {
    lcd.println("Keine Netzwerke");
  } else {
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.print(n);
    lcd.println(" gefunden:");
    lcd.println("--------------------");
    
    lcd.setTextSize(1); // Kleinerer Text, damit mehr drauf passt
    lcd.setTextColor(ST77XX_WHITE);
    
    // Die ersten 8 Netzwerke anzeigen (damit es auf den Screen passt)
    for (int i = 0; i < n && i < 8; ++i) {
      lcd.print(i + 1);
      lcd.print(": ");
      lcd.print(WiFi.SSID(i));
      lcd.print(" (");
      lcd.print(WiFi.RSSI(i)); // Signalstärke
      lcd.println("dBm)");
      delay(10);
    }
  }
}

void loop() {
  // Alle 30 Sekunden neu scannen
  delay(30000);
  scanAndShowWiFi();
}
