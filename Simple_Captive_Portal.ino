#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Pins laut deinem Board
#define LCD_CS   15
#define LCD_DC   2
#define LCD_RST  4
#define LCD_BLK  32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1); // "Google-ähnliche" IP für besseres Auto-Popup

String gespeicherteEingabe = "Noch keine Daten";

// Die HTML Seite, die der Nutzer sieht
String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif; background:#222; color:white; text-align:center; padding:20px;}";
  html += "input{padding:10px; width:80%; margin:10px 0;} button{padding:10px 20px; background:cyan; border:none;}</style></head>";
  html += "<body><h1>WIFI Login</h1><p>Gib etwas ein:</p>";
  html += "<form action='/save' method='POST'><input type='text' name='daten' placeholder='Deine Nachricht...'><br>";
  html += "<button type='submit'>Speichern</button></form></body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);

  // Display Setup
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  lcd.init(170, 320);
  lcd.setRotation(1);
  lcd.fillScreen(ST77XX_BLACK);
  updateDisplay("AP Startet...");

  // Access Point konfigurieren
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("ESP32-Konfigurator");

  // DNS Server: Leitet alle Anfragen (*) auf den ESP32 um
  dnsServer.start(DNS_PORT, "*", apIP);

  // Webserver Routen
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });

  // Wenn Daten gesendet werden
  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("daten")) {
      gespeicherteEingabe = server.arg("daten");
      updateDisplay("Eingabe erhalten:\n" + gespeicherteEingabe);
      server.send(200, "text/html", "<h1>Gespeichert!</h1><p>Danke. Du kannst das Fenster schliessen.</p>");
    }
  });

  // Falls das Handy eine Test-URL aufruft (Captive Portal Erkennung)
  server.onNotFound([]() {
    server.send(200, "text/html", getHTML());
  });

  server.begin();
  updateDisplay("WLAN: ESP32-Konfigurator\nBereit!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void updateDisplay(String msg) {
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setCursor(10, 30);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.setTextSize(2);
  lcd.println(msg);
}
