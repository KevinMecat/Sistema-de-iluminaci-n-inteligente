#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <math.h>  // <- Para pow()

// Configuración WiFi
const char* ssid = "FAMILIA-BLANCO";
const char* password = "2010mary35kk6915";

WebServer server(80);

String getContentType(const String& path); // <- Prototipo necesario
// Implementaciones de las funciones declaradas
bool serveFile(const char* path) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
}

void listAllFiles() {
  Serial.println("Listing all files in SPIFFS:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while(file) {
    Serial.print("File: ");
    Serial.print(file.name());
    Serial.print(" Size: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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


// Estructura para bombillos
struct Bulb {
  uint8_t pin;
  uint8_t pwmChannel;
  bool hasAutoMode;
  bool hasBrightness;
  bool hasPIRMode;  // Nueva propiedad para modo PIR
  int8_t sensorPin;
  int8_t pirPin;    // Nuevo pin para sensor PIR
  uint16_t currentValue;
  bool isOn;
  bool autoMode;
  bool pirMode;     // Nuevo modo PIR
  int ldrMin;
  int ldrThreshold;
  int ldrMax;
  unsigned long lastPIRDetection; // Tiempo de última detección PIR
};

// Configuración de los LEDs (ahora son 6)
Bulb bulbs[6] = {
  // pin, pwmChan, auto, bright, PIR, sensor, PIRpin, currVal, isOn, auto, PIRmode, ldrMin, ldrThresh, ldrMax, lastPIR
  {2, 0, true, true, false, 34, -1, 0, false, false, false, 500, 1600, 1700, 0},  // Config LDR
  {4, 1, false, true, false, -1, -1, 0, false, false, false, 0, 0, 0, 0},
  {5, 2, false, true, false, -1, -1, 0, false, false, false, 0, 0, 0, 0},
  {18, 3, false, true, false, 35, -1, 0, false, false, false, 0, 0, 0, 0},
  {19, 4, false, true, false, -1, -1, 0, false, false, false, 0, 0, 0, 0},
  {21, 5, false, true, true, -1, 13, 0, false, false, false, 0, 0, 0, 0} // Nuevo LED con PIR (pin 13 para PIR)
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
  for (int i = 0; i < 6; i++) { // Ahora son 6 LEDs
    ledcSetup(bulbs[i].pwmChannel, 5000, 8);
    ledcAttachPin(bulbs[i].pin, bulbs[i].pwmChannel);
    ledcWrite(bulbs[i].pwmChannel, 0);
    if (bulbs[i].sensorPin != -1)
      pinMode(bulbs[i].sensorPin, INPUT);
    if (bulbs[i].pirPin != -1)
      pinMode(bulbs[i].pirPin, INPUT);
  }
}

String getBulbStatus(int id) {
  String json;
  json.reserve(150); // Aumentado para incluir el nuevo campo
  json = "{\"id\":";
  json += id;
  json += ",\"isOn\":";
  json += bulbs[id].isOn ? "true" : "false";
  json += ",\"value\":";
  json += bulbs[id].currentValue;
  json += ",\"autoMode\":";
  json += bulbs[id].autoMode ? "true" : "false";
  json += ",\"pirMode\":";
  json += bulbs[id].pirMode ? "true" : "false";
  json += ",\"hasPIRMode\":";
  json += bulbs[id].hasPIRMode ? "true" : "false";
  json += "}";
  return json;
}

// ... (las funciones serveFile, getContentType, listAllFiles, connectWiFi, handleNotFound permanecen igual)

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
    if (id < 0 || id >= 6) { // Ahora son 6 LEDs
      server.send(400, "text/plain", "Invalid ID");
      return;
    }

    String action = server.arg("action");

    if (action == "toggle") {
      bulbs[id].isOn = !bulbs[id].isOn;
      bulbs[id].autoMode = false;
      bulbs[id].pirMode = false;
      ledcWrite(bulbs[id].pwmChannel, bulbs[id].isOn ? bulbs[id].currentValue : 0);
    } 
    else if (action == "setBrightness" && bulbs[id].hasBrightness) {
      bulbs[id].currentValue = server.arg("value").toInt();
      bulbs[id].currentValue = constrain(bulbs[id].currentValue, 0, 255);
      bulbs[id].isOn = (bulbs[id].currentValue > 0);
      if (bulbs[id].isOn) {
        bulbs[id].autoMode = false;
        bulbs[id].pirMode = false;
      }
      ledcWrite(bulbs[id].pwmChannel, bulbs[id].currentValue);
    } 
    else if (action == "setMode" && bulbs[id].hasAutoMode) {
      bulbs[id].autoMode = (server.arg("mode") == "auto");
      if (bulbs[id].autoMode) bulbs[id].pirMode = false;
    }
    else if (action == "setPIRMode" && bulbs[id].hasPIRMode) {
      bulbs[id].pirMode = (server.arg("mode") == "pir");
      if (bulbs[id].pirMode) bulbs[id].autoMode = false;
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

    for (int i = 0; i < 6; i++) { // Ahora son 6 LEDs
      // Modo automático con LDR (como antes)
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
      
      // Modo automático con PIR (nuevo)
      if (bulbs[i].pirMode && bulbs[i].pirPin != -1) {
        bool pirDetected = digitalRead(bulbs[i].pirPin) == HIGH;
        
        if (pirDetected) {
          bulbs[i].lastPIRDetection = millis();
          if (!bulbs[i].isOn) {
            bulbs[i].isOn = true;
            bulbs[i].currentValue = 255; // Encender al máximo cuando se detecta movimiento
            ledcWrite(bulbs[i].pwmChannel, bulbs[i].currentValue);
            Serial.print("PIR (ID ");
            Serial.print(i);
            Serial.println("): Movimiento detectado - LED encendido");
          }
        } else if (bulbs[i].isOn && (millis() - bulbs[i].lastPIRDetection > 30000)) {
          // Apagar después de 30 segundos sin detección
          bulbs[i].isOn = false;
          bulbs[i].currentValue = 0;
          ledcWrite(bulbs[i].pwmChannel, bulbs[i].currentValue);
          Serial.print("PIR (ID ");
          Serial.print(i);
          Serial.println("): Tiempo sin movimiento - LED apagado");
        }
      }
    }
  }
}