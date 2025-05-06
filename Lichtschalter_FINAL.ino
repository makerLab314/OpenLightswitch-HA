#include <SPI.h>          // Wird oft von WiFi-Bibliotheken benötigt
#include <WiFiNINA.h>     // <- Beibehalten für dein Board!
#include <ArduinoMqttClient.h> // <- Neue MQTT-Bibliothek aus dem Beispiel
#include <Servo.h>

// --- Konfiguration ---
// WLAN - BITTE ANPASSEN!
const char* ssid = "QuantumDATA";
const char* password = "81047475";

// MQTT Broker - BITTE ANPASSEN!
const char* mqtt_server = "192.168.1.117"; // z.B. "192.168.1.100"
const int mqtt_port = 1883;
const char* client_id = "arduinoLightSwitchOffice"; // Eindeutige Client-ID

// MQTT Zugangsdaten (falls benötigt, sonst leer lassen oder Zeilen auskommentieren)
const char* mqtt_user = "superuser";
const char* mqtt_pass = "homeassistant3Dffm";

// MQTT Topics
const char* command_topic = "home/lightswitch/office/set";
const char* state_topic = "home/lightswitch/office/state";

// Pins
const int servoPin = 9;
const int touchPin = 2;

// Servo-Parameter - BITTE ANPASSEN!
// *** WICHTIGE ÄNDERUNG HIER ***
const int servoCenterPosition = 90; // Mittelposition auf 90 Grad gesetzt
// *****************************
const int servoTapAngle = 50;       // Ausschlag beim Tippen in jede Richtung (Grad) - Ggf. anpassen!
const int servoTapHoldDelay = 150;  // Kurze Pause in der Tipp-Position (ms) - für Sichtbarkeit
const int servoReturnDelay = 50;   // Kurze Pause nach Rückkehr zur Mitte (ms) - zum Absetzen

// --- Globale Variablen ---
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient); // MqttClient aus ArduinoMqttClient
Servo myServo;

// Zustandmanagement
bool lastTurnRight = false; // Speichert den *logischen* Zustand (ON=true, OFF=false) für MQTT
bool nextTapRight = true;   // Steuert die Richtung des *nächsten manuellen Tipps* (true=rechts, false=links)

// Touch Sensor Debounce
long lastTouchTime = 0;
const long debounceDelay = 200; // Entprellzeit für den Touchsensor (ms)

// MQTT Reconnect Timing
unsigned long lastReconnectAttempt = 0;

// --- Prototypen ---
void connectWiFi();
void connectMQTT();
void publishState();
void performTap(bool goRight); // Funktion für die Tipp-Bewegung
void checkTouchSensor();
void onMqttMessage(int messageSize);

// --- Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Starting Smart Light Switch (Tap Control)...");

  // Pin für den Touchsensor als Eingang konfigurieren
  pinMode(touchPin, INPUT);
  Serial.println("Touch Pin initialized.");

  // Servo initialisieren
  myServo.attach(servoPin);
  myServo.write(servoCenterPosition); // Start in der neuen Mittelposition (90 Grad)
  lastTurnRight = false; // ANNAHME: Startzustand ist logisch "OFF"
  nextTapRight = true;   // Erster manueller Tipp geht nach rechts (ON)
  Serial.print("Servo initialized to center position: ");
  Serial.println(servoCenterPosition);
  delay(500); // Kurz warten, bis der Servo die Position erreicht hat

  // WLAN verbinden
  connectWiFi();

  // MQTT Client konfigurieren
  mqttClient.setId(client_id);
  // Falls Zugangsdaten benötigt werden:
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  // MQTT Message Callback setzen
  mqttClient.onMessage(onMqttMessage);

  Serial.println("Setup finished.");
  // Die erste MQTT-Verbindung wird im Loop versucht
}

// --- Hauptschleife ---
void loop() {
  // 1. Prüfe WLAN Verbindung
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting reconnect...");
    connectWiFi();
    // Nach erfolgreicher WLAN-Verbindung wird im nächsten Loop-Durchlauf
    // versucht, die MQTT-Verbindung aufzubauen (oder wiederherzustellen).
  } else {
    // Nur fortfahren, wenn WLAN verbunden ist
    // 2. Prüfe MQTT Verbindung und verbinde ggf. neu
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      // Versuche nur alle 5 Sekunden einen Reconnect, um nicht zu spammen
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        connectMQTT();
      }
    } else {
      // 3. MQTT-Nachrichten verarbeiten, wenn verbunden
      mqttClient.poll();
    }
  }

  // 4. Prüfe den Touchsensor (immer prüfen, damit lokale Steuerung funktioniert)
  // Der Status wird nur gesendet, wenn MQTT verbunden ist (innerhalb publishState geprüft)
  checkTouchSensor();

  // Keine zusätzlichen Delays im Hauptloop, um reaktionsfähig zu bleiben
}

// --- Funktionen ---

void connectWiFi() {
  Serial.print("Checking WiFi module...");
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(" Communication with WiFi module failed!");
    while (true); // Dauerhaft anhalten bei Hardwarefehler
  }
  Serial.println(" OK.");

  String fv = WiFi.firmwareVersion();
  Serial.print("WiFiNINA Firmware Version: ");
  Serial.println(fv);

  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // Start des Verbindungsversuchs

  unsigned long connectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Timeout nach z.B. 30 Sekunden, um nicht ewig zu blockieren
    if (millis() - connectStart > 30000) {
        Serial.println("\nConnection attempt timed out!");
        // Optional: Hier könnte man z.B. das Board neu starten
        // oder in einen Fehlerzustand gehen. Vorerst nur Meldung.
        return; // Verhindert Endlosschleife, wenn Verbindung nicht klappt
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  // Versuche nur, wenn WLAN verbunden ist
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("MQTT connect skipped: WiFi not connected.");
      return;
  }

  Serial.print("Attempting MQTT connection to ");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(mqtt_port);

  if (!mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // Nicht blockieren, nächster Versuch im Loop
  } else {
    Serial.println("MQTT connected!");
    Serial.print("Subscribing to command topic: ");
    Serial.println(command_topic);
    if (mqttClient.subscribe(command_topic)) {
        Serial.println("Subscription successful!");
        // Den aktuellen *logischen* Status sofort nach erfolgreicher Verbindung senden
        publishState();
    } else {
        Serial.println("Subscription failed!");
        // Optional: Verbindung trennen oder erneut versuchen?
        // mqttClient.disconnect(); // Oder einfach auf nächsten Poll warten
    }
  }
}

void publishState() {
  // Sende nur, wenn MQTT verbunden ist
  if (!mqttClient.connected()) {
    // Serial.println("Cannot publish state: MQTT not connected."); // Weniger Output im Normalbetrieb
    return;
  }

  const char* currentState = lastTurnRight ? "ON" : "OFF"; // Verwende den logischen Zustand
  Serial.print("Publishing logical state [");
  Serial.print(state_topic);
  Serial.print("]: ");
  Serial.println(currentState);

  // Sende mit Retain-Flag und QoS 1
  mqttClient.beginMessage(state_topic, strlen(currentState), true, 1);
  mqttClient.print(currentState);
  mqttClient.endMessage();
}

// Funktion für die schnelle Tipp-Bewegung
void performTap(bool goRight) {
  int targetPosition;
  int startPosition = servoCenterPosition; // Aktuelle Position ist immer die Mitte

  if (goRight) {
    Serial.println("Performing tap RIGHT (-> ON logic)");
    // Berechne Zielposition, begrenze auf 0-180
    targetPosition = constrain(startPosition + servoTapAngle, 0, 180);
  } else {
    Serial.println("Performing tap LEFT (-> OFF logic)");
    // Berechne Zielposition, begrenze auf 0-180
    targetPosition = constrain(startPosition - servoTapAngle, 0, 180);
  }

  Serial.print("Moving servo from ");
  Serial.print(startPosition);
  Serial.print(" to ");
  Serial.println(targetPosition);

  // Schnell zur Zielposition bewegen
  myServo.write(targetPosition);
  delay(servoTapHoldDelay); // Kurz halten für den "Tipp"-Effekt

  // Schnell zurück zur Mittelposition
  Serial.print("Returning servo to ");
  Serial.println(startPosition);
  myServo.write(startPosition);
  delay(servoReturnDelay); // Kurze Pause zum Absetzen
  Serial.println("Returned to center.");
}

void checkTouchSensor() {
  // Nur prüfen, wenn Debounce-Zeit abgelaufen ist
  long currentTime = millis();
  if (currentTime - lastTouchTime < debounceDelay) {
      return; // Zu früh für eine neue Berührung
  }

  int touchState = digitalRead(touchPin);

  // Prüfe auf HIGH Flanke
  if (touchState == HIGH) {
    Serial.println("Touch detected!");
    lastTouchTime = currentTime; // Debounce-Zeit neu starten

    // Führe den Tap in die Richtung aus, die 'nextTapRight' anzeigt
    performTap(nextTapRight);

    // Wechsle den *logischen* Zustand für MQTT
    lastTurnRight = !lastTurnRight;
    publishState(); // Sende den neuen logischen Zustand (prüft intern auf MQTT-Verbindung)

    // Wechsle die Richtung für den *nächsten manuellen* Tipp
    nextTapRight = !nextTapRight;

    // WICHTIG: Eine zusätzliche Wartezeit nach der Aktion ist hier nicht mehr nötig,
    // da der Debounce-Timer (lastTouchTime) direkt nach der Erkennung gesetzt wird
    // und die Servo-Bewegungszeit innerhalb des Debounce-Intervalls liegen sollte.
    // Der alte Delay wurde entfernt, um die Reaktionsfähigkeit zu erhöhen.
    // Stelle sicher, dass debounceDelay > (servoTapHoldDelay + servoReturnDelay) ist.
  }
}

// --- MQTT Callback Funktion ---
void onMqttMessage(int messageSize) {
  String topic = mqttClient.messageTopic();
  String payload = ""; // Umbenannt von message zu payload für Klarheit
  while (mqttClient.available()) {
    payload += (char)mqttClient.read();
  }

  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);

  if (topic == command_topic) {
    bool commandOn = (payload == "ON"); // Prüfe auf ON
    bool commandOff = (payload == "OFF"); // Prüfe auf OFF

    if (commandOn) {
      Serial.println("Received ON command");
      if (!lastTurnRight) { // Nur ausführen, wenn logischer Zustand OFF war
        Serial.println("Current state is OFF, performing tap RIGHT.");
        performTap(true);  // Tippe nach rechts
        lastTurnRight = true; // Setze logischen Zustand auf ON
        publishState();      // Veröffentliche neuen Zustand
        nextTapRight = false; // Nächster *manueller* Tipp soll nach links gehen (OFF)
      } else {
        Serial.println("Already ON (logically), no action needed.");
      }
    } else if (commandOff) {
      Serial.println("Received OFF command");
      if (lastTurnRight) { // Nur ausführen, wenn logischer Zustand ON war
        Serial.println("Current state is ON, performing tap LEFT.");
        performTap(false); // Tippe nach links
        lastTurnRight = false; // Setze logischen Zustand auf OFF
        publishState();       // Veröffentliche neuen Zustand
        nextTapRight = true;  // Nächster *manueller* Tipp soll nach rechts gehen (ON)
      } else {
          Serial.println("Already OFF (logically), no action needed.");
      }
    } else {
      Serial.print("Unknown command payload received: ");
      Serial.println(payload);
    }
  } else {
      Serial.print("Received message on unexpected topic: ");
      Serial.println(topic);
  }
}