#include <driver/ledc.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "IZZI-A8A4";
// const char* ssid = "DESKTOP-KALDOS7 3748";
const char* password = "nPTyfhpEmALRLF9tcc";
// const char* password = "9=4p72L3";

// const char* mqtt_server = "89.116.191.188";
const char* mqtt_server = "3.83.35.123";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ===================================================
// STATE VARIABLES
// ===================================================
bool garage_status = false;
bool living_room_status = false;
bool room_1_status = false;
bool room_2_status = false;
bool hallway_status = false;
bool bathroom_status = false;
bool kitchen_status = false;
bool patio_status = false;

bool door_1_status = false;
bool door_2_status = false;
bool window_1_status = false;
bool window_2_status = false;

bool buzzer_status = false;

// ===================================================
// OUTPUT/INPUT STRUCTURES
// ===================================================
struct outputItem {
  const char* name;
  int pin;
  bool* stateVar;
};

struct inputButton {
  const char* name;
  int pin;
  bool* stateVar;
};

outputItem leds[] = {
  { "living_room", 18, &living_room_status },
  { "room_1", 19, &room_1_status },
  { "room_2", 21, &room_2_status },
  { "bathroom", 23, &bathroom_status },
  { "patio", 26, &patio_status }
};

inputButton ledButtons[] = {
  { "living_room", 13, &living_room_status },
  { "room_1", 14, &room_1_status },
  { "room_2", 32, &room_2_status },
  { "bathroom", 33, &bathroom_status },
  { "patio", 15, &patio_status }
};

outputItem motors[] = {
  { "door_1", 4, &door_1_status },
  { "door_2", 5, &door_2_status },
  { "window_1", 12, &window_1_status },
  { "window_2", 16, &window_2_status },
};

outputItem buzzer = { "buzzer", 25, &buzzer_status };

int ledButtonsCount = sizeof(ledButtons) / sizeof(ledButtons[0]);
int ledsCount = sizeof(leds) / sizeof(leds[0]);
int motorsCount = sizeof(motors) / sizeof(motors[0]);

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
// MQTT CALLBACK — READ smarthome/${NAME}/set
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

  for (int i = 0; i < motorsCount; i++) {
    String expected = String("smarthome/") + motors[i].name + "/set";

    if (String(topic) == expected) {
      bool newState = (message == "1");

      *(motors[i].stateVar) = newState;

      String statusTopic = String("smarthome/") + motors[i].name + "/status";
      mqttClient.publish(statusTopic.c_str(), newState ? "1" : "0");

      Serial.printf("MOTOR %s SET => %d\n", motors[i].name, newState);
      return;
    }
  }
  
  if (String(topic) == "smarthome/buzzer/set") {
    bool newState = false;

    *(buzzer.stateVar) = newState;

    String statusTopic = "smarthome/buzzer/status";
    mqttClient.publish(statusTopic.c_str(), "0");
    noTone(buzzer.pin);
    Serial.printf("BUZZER %s SET => %d\n", buzzer.name, newState);
    return;
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

      for (int i = 0; i < ledsCount; i++) {
        String topic = String("smarthome/") + leds[i].name + "/set";
        mqttClient.subscribe(topic.c_str());
        Serial.print("Subscribed: ");
        Serial.println(topic);
      }

      for (int i = 0; i < motorsCount; i++) {
        String topic = String("smarthome/") + motors[i].name + "/set";
        mqttClient.subscribe(topic.c_str());
        Serial.print("Subscribed: ");
        Serial.println(topic);
      }

      String topic = "smarthome/buzzer/set";
      mqttClient.subscribe(topic.c_str());
      Serial.print("Subscribed: ");
      Serial.println(topic);

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

  for (int i = 0; i < ledsCount; i++) {
    pinMode(leds[i].pin, OUTPUT);
    digitalWrite(leds[i].pin, LOW);
  }

  for (int i = 0; i < motorsCount; i++) {
    pinMode(motors[i].pin, INPUT);
  }

  for (int i = 0; i < ledButtonsCount; i++) {
    pinMode(ledButtons[i].pin, INPUT);
  }

  pinMode(buzzer.pin, OUTPUT);

  setup_wifi();
  setup_mqtt();
}

// ===================================================
// LOOP
// ===================================================

unsigned long lastPressButtons[5] = { 0 };
unsigned long lastPressMotors[4] = { 0 };
const unsigned long debounceDelay = 500;

void loop() {
  if (!mqttClient.connected()) {
    setup_mqtt();
  }
  mqttClient.loop();

  for (int i = 0; i < ledButtonsCount; i++) {
    if (digitalRead(ledButtons[i].pin) == HIGH) {

      if (millis() - lastPressButtons[i] > debounceDelay) {

        *ledButtons[i].stateVar = !(*ledButtons[i].stateVar);

        digitalWrite(leds[i].pin, (*ledButtons[i].stateVar) ? HIGH : LOW);

        String topic = String("smarthome/") + ledButtons[i].name + "/status";

        String payload = (*ledButtons[i].stateVar) ? "1" : "0";

        Serial.println(topic);
        Serial.println(digitalRead(ledButtons[i].pin));
        Serial.println(leds[i].pin);

        mqttClient.publish(topic.c_str(), payload.c_str());
        lastPressButtons[i] = millis();
      }
    }
  }

  for (int i = 0; i < motorsCount; i++) {
    Serial.println(digitalRead(motors[3].pin));
    if ((digitalRead(motors[i].pin) == HIGH) && (*motors[i].stateVar) && (!(*buzzer.stateVar))) {

      if (millis() - lastPressMotors[i] > debounceDelay) {
        *buzzer.stateVar = true;
        tone(buzzer.pin, 440);

        String topic = String("smarthome/buzzer/status");

        String payload = "1";

        Serial.println(topic);
        Serial.println(digitalRead(buzzer.pin));

        mqttClient.publish(topic.c_str(), payload.c_str());
        lastPressButtons[i] = millis();
      }
    }
  }
}