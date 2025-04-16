#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Configuración WiFi
const char* ssid = "FAMILIA-BLANCO";
const char* password = "2010mary35kk6915";

WebServer server(80);

// Declaraciones
bool serveFile(const char* path);
String getContentType(const String& path);
void listAllFiles();
void connectWiFi();
void handleNotFound();

// Estructura para bombillos
struct Bulb {
  uint8_t pin;
  uint8_t pwmChannel;
  bool hasAutoMode;
  bool hasBrightness;
  int8_t sensorPin;
  uint16_t currentValue;
  bool isOn;
  bool autoMode;
  int ldrMin;
  int ldrThreshold;
  int ldrMax;
};

Bulb bulbs[5] = {
  // pin, pwmChan, auto, bright, sensor, currVal, isOn, auto, ldrMin, ldrThresh, ldrMax
  {2, 0, true, true, 34, 0, false, false, 500, 1600, 1700},  // Config LDR
  {4, 1, false, true, -1, 0, false, false, 0, 0, 0},
  {5, 2, false, true, -1, 0, false, false, 0, 0, 0},
  {18, 3, false, true, 35, 0, false, false, 0, 0, 0},
  {19, 4, false, true, -1, 0, false, false, 0, 0, 0}
};

uint16_t calcularBrilloLED(int valorLDR, const Bulb &bulb) {
  if (!bulb.hasAutoMode || bulb.sensorPin == -1) return bulb.currentValue;

  const int LDR_APAGADO = 1700;
  const int LDR_REGULACION = 1600;
  const int LDR_MAX_LUZ = 500;

  valorLDR = constrain(valorLDR, LDR_MAX_LUZ, LDR_APAGADO);

  // Apagado completo
  if (valorLDR >= LDR_APAGADO) return 0;

  // Calcular porcentaje de luz (invertido)
  float porcentaje = 1.0 - (float)(valorLDR - LDR_MAX_LUZ) / (LDR_REGULACION - LDR_MAX_LUZ);
  porcentaje = constrain(porcentaje, 0.0, 1.0);

  // Aplicar curva suave (puedes ajustar el factor)
  float brillo = pow(porcentaje, 1.5) * 255;

  return (uint16_t)brillo;
}

void setupPWM() {
  for (int i = 0; i < 5; i++) {
    ledcSetup(bulbs[i].pwmChannel, 5000, 8);
    ledcAttachPin(bulbs[i].pin, bulbs[i].pwmChannel);
    ledcWrite(bulbs[i].pwmChannel, 0);
    if (bulbs[i].sensorPin != -1)
      pinMode(bulbs[i].sensorPin, INPUT);
  }
}

String getBulbStatus(int id) {
  String json;
  json.reserve(128);
  json = "{\"id\":";
  json += id;
  json += ",\"isOn\":";
  json += bulbs[id].isOn ? "true" : "false";
  json += ",\"value\":";
  json += bulbs[id].currentValue;
  json += ",\"autoMode\":";
  json += bulbs[id].autoMode ? "true" : "false";
  json += "}";
  return json;
}

String getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg")) return "image/jpeg";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool serveFile(const char* path) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
}

void listAllFiles() {
  Serial.println("📂 Archivos en SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print(" - ");
    Serial.print(file.name());
    Serial.print("\t");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

void connectWiFi() {
  Serial.print("🔌 Conectando a WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi conectado");
    Serial.print("🌐 Dirección IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ No se pudo conectar a WiFi");
    ESP.restart();
  }
}

void handleNotFound() {
  String message = "404 - Página no encontrada\n\n";
  message += "URI: ";
  message += server.uri();
  server.send(404, "text/plain", message);
}

void handleRoot() {
  if (!serveFile("/Index.html")) {
    server.send(404, "text/plain", "Página principal no encontrada");
  }
}

void handleControl() {
  if (!serveFile("/Control/Control.html")) {
    server.send(404, "text/plain", "Página de control no encontrada");
  }
}

void handleConsumo() {
  if (!serveFile("/Consumo/Consumo.html")) {
    server.send(404, "text/plain", "Página de consumo no encontrada");
  }
}

void handleControlAPI() {
  if (server.hasArg("id") && server.hasArg("action")) {
    int id = server.arg("id").toInt();
    if (id < 0 || id >= 5) {
      server.send(400, "text/plain", "Invalid ID");
      return;
    }

    String action = server.arg("action");

    if (action == "toggle") {
      bulbs[id].isOn = !bulbs[id].isOn;
      ledcWrite(bulbs[id].pwmChannel, bulbs[id].isOn ? bulbs[id].currentValue : 0);
    } 
    else if (action == "setBrightness" && bulbs[id].hasBrightness) {
      bulbs[id].currentValue = server.arg("value").toInt();
      bulbs[id].currentValue = constrain(bulbs[id].currentValue, 0, 255);
      bulbs[id].isOn = (bulbs[id].currentValue > 0);
      ledcWrite(bulbs[id].pwmChannel, bulbs[id].currentValue);
    } 
    else if (action == "setMode" && bulbs[id].hasAutoMode) {
      bulbs[id].autoMode = (server.arg("mode") == "auto");
    }
    else if (action == "status") {
      server.send(200, "application/json", getBulbStatus(id));
      return;
    }

    server.send(200, "application/json", getBulbStatus(id));
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Error al montar SPIFFS");
    delay(1000);
    ESP.restart();
  }

  listAllFiles();

  const char* essentialFiles[] = {
    "/Index.html",
    "/Control/Control.html",
    "/Consumo/Consumo.html"
  };

  bool allFilesExist = true;
  for (const auto& filePath : essentialFiles) {
    if (!SPIFFS.exists(filePath)) {
      Serial.print("❌ Archivo faltante: ");
      Serial.println(filePath);
      allFilesExist = false;
    }
  }

  if (!allFilesExist) {
    Serial.println("❌ Faltan archivos HTML esenciales");
    delay(1000);
    ESP.restart();
  }

  setupPWM();
  connectWiFi();

  server.on("/", handleRoot);
  server.on("/Control", handleControl);
  server.on("/Consumo", handleConsumo);
  server.on("/api/control", handleControlAPI);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("🌐 Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

  static unsigned long lastAutoUpdate = 0;
  if (millis() - lastAutoUpdate > 1000) {
    lastAutoUpdate = millis();

    for (int i = 0; i < 5; i++) {
      if (bulbs[i].autoMode && bulbs[i].sensorPin != -1) {
        int ldrValue = analogRead(bulbs[i].sensorPin);
        bulbs[i].currentValue = calcularBrilloLED(ldrValue, bulbs[i]);
        bulbs[i].isOn = (bulbs[i].currentValue > 0);
        ledcWrite(bulbs[i].pwmChannel, bulbs[i].currentValue);

        Serial.print("LDR (ID ");
        Serial.print(i);
        Serial.print("): ");
        Serial.print(ldrValue);
        Serial.print(" → Brillo: ");
        Serial.println(bulbs[i].currentValue);
      }
    }
  }
}
