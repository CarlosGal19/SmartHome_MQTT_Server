#include <WiFi.h>
#include <PubSubClient.h>

//const char* ssid = "IZZI-A8A4";
const char* ssid = "DESKTOP-KALDOS7 3748";
//const char* password = "nPTyfhpEmALRLF9tcc";
const char* password = "9=4p72L3";

const char* mqtt_server = "89.116.191.188";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ===================================================
// VARIABLES DE ESTADO
// ===================================================
bool garage_status = false;
bool living_room_status = false;
bool room_1_status = false;
bool room_2_status = false;
bool hallway_status = false;
bool bathroom_status = false;
bool kitchen_status = false;
bool patio_status = false;

bool door_status = false;
bool window_status = false;
bool blind_status = false;

// ===================================================
// ESTRUCTURAS DE SALIDAS
// ===================================================
struct outputItem {
  const char* name;
  int pin;
  bool* stateVar;
};

outputItem leds[] = {
  {"garage", 5, &garage_status},
  {"living_room", 18, &living_room_status},
  {"room_1", 19, &room_1_status},
  {"room_2", 21, &room_2_status},
  {"hallway", 22, &hallway_status},
  {"bathroom", 23, &bathroom_status},
  {"kitchen", 25, &kitchen_status},
  {"patio", 26, &patio_status}
};

outputItem motors[] = {
  {"door", 27, &door_status},
  {"window", 32, &window_status},
  {"blind", 33, &blind_status}
};

// ===================================================
// WIFI
// ===================================================
void setup_wifi() {
  Serial.print("Connecting to WiFi ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

// ===================================================
// MQTT CALLBACK — SOLO LEE smarthome/.../set
// ===================================================
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("MQTT => ");
  Serial.print(topic);
  Serial.print(" => ");
  Serial.println(message);

  int ledsCount = sizeof(leds) / sizeof(leds[0]);
  for (int i = 0; i < ledsCount; i++) {
    String expected = String("smarthome/") + leds[i].name + "/set";

    if (String(topic) == expected) {
      bool newState = (message == "1");

      *(leds[i].stateVar) = newState;
      digitalWrite(leds[i].pin, newState ? HIGH : LOW);

      String statusTopic = String("smarthome/") + leds[i].name + "/status";
      mqttClient.publish(statusTopic.c_str(), newState ? "1" : "0");

      Serial.printf("LED %s SET => %d\n", leds[i].name, newState);
      return;
    }
  }

  int motorsCount = sizeof(motors) / sizeof(motors[0]);
  for (int i = 0; i < motorsCount; i++) {
    String expected = String("smarthome/") + motors[i].name + "/set";

    if (String(topic) == expected) {
      bool newState = (message == "1");

      *(motors[i].stateVar) = newState;
      digitalWrite(motors[i].pin, newState ? HIGH : LOW);

      String statusTopic = String("smarthome/") + motors[i].name + "/status";
      mqttClient.publish(statusTopic.c_str(), newState ? "1" : "0");

      Serial.printf("MOTOR %s SET => %d\n", motors[i].name, newState);
      return;
    }
  }
}

// ===================================================
// MQTT SETUP
// ===================================================
void setup_mqtt() {
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqtt_callback);

  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT... ");

    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected!");

      int ledsCount = sizeof(leds) / sizeof(leds[0]);
      for (int i = 0; i < ledsCount; i++) {
        String topic = String("smarthome/") + leds[i].name + "/set";
        mqttClient.subscribe(topic.c_str());
        Serial.print("Subscribed: ");
        Serial.println(topic);
      }

      int motorsCount = sizeof(motors) / sizeof(motors[0]);
      for (int i = 0; i < motorsCount; i++) {
        String topic = String("smarthome/") + motors[i].name + "/set";
        mqttClient.subscribe(topic.c_str());
        Serial.print("Subscribed: ");
        Serial.println(topic);
      }

    } else {
      Serial.print("FAILED. rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying...");
      delay(2000);
    }
  }
}

// ===================================================
// SETUP
// ===================================================
void setup() {
  Serial.begin(115200);

  int ledsCount = sizeof(leds) / sizeof(leds[0]);
  int motorsCount = sizeof(motors) / sizeof(motors[0]);

  for (int i = 0; i < ledsCount; i++) {
    pinMode(leds[i].pin, OUTPUT);
    digitalWrite(leds[i].pin, LOW);
  }

  for (int i = 0; i < motorsCount; i++) {
    pinMode(motors[i].pin, OUTPUT);
    digitalWrite(motors[i].pin, LOW);
  }

  setup_wifi();
  setup_mqtt();
}

// ===================================================
// LOOP
// ===================================================
void loop() {
  if (!mqttClient.connected()) {
    setup_mqtt();
  }
  mqttClient.loop();

  static unsigned long lastToggle = 0;
  if (millis() - lastToggle >= 5000) {

    patio_status = !patio_status;
    digitalWrite(26, patio_status ? HIGH : LOW);

    mqttClient.publish("smarthome/patio/status", patio_status ? "1" : "0");

    Serial.print("Toggled patio => ");
    Serial.println(patio_status ? "1" : "0");

    lastToggle = millis();
  }
}

