#include <ESP8266WiFi.h>
#include <KiwisIoT.h>
#include <DHT.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* pass = "YOUR_WIFI_PASSWORD";

const char* topic = "YOUR_DASHBOARD_TOPIC_ID";

KiwisIoT kiwisiot(ssid, pass, topic);

#define DHT_PIN     D5
#define DHT_TYPE    DHT11
#define TRIG_PIN    D1
#define ECHO_PIN    D2
#define BUZZER_PIN  D6

DHT dht(DHT_PIN, DHT_TYPE);

const unsigned long SEND_INTERVAL = 5000;

const float TEMP_LIMIT = 35.0;
const float HUMIDITY_MIN = 30.0;
const float HUMIDITY_MAX = 80.0;
const float ANIMAL_DISTANCE_CM = 100.0;

unsigned long lastSend = 0;

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    return -1.0;
  }

  return duration * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();

  Serial.println();
  Serial.println("================================");
  Serial.println(" LIVESTOCK MONITORING SYSTEM");
  Serial.println("================================");

  kiwisiot.begin();

  Serial.println("System initialized.");
}

void loop() {
  kiwisiot.run();

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float distance = readDistanceCM();

    bool validDHT =
      !isnan(temperature) && !isnan(humidity);

    bool validDistance =
      distance >= 2.0 && distance <= 400.0;

    bool animalNearby =
      validDistance &&
      distance <= ANIMAL_DISTANCE_CM;

    // Buzzer sounds only when an animal is nearby.
    digitalWrite(
      BUZZER_PIN,
      animalNearby ? HIGH : LOW
    );

    bool environmentAlert = false;

    if (validDHT) {
      environmentAlert =
        temperature >= TEMP_LIMIT ||
        humidity < HUMIDITY_MIN ||
        humidity > HUMIDITY_MAX;
    }

    String proximityStatus;

    if (!validDistance) {
      proximityStatus = "SENSOR ERROR";
    } else if (animalNearby) {
      proximityStatus = "ANIMAL NEARBY";
    } else {
      proximityStatus = "NO ANIMAL NEARBY";
    }

    String farmStatus;

    if (!validDHT) {
      farmStatus = "DHT SENSOR ERROR";
    } else if (environmentAlert) {
      farmStatus = "CHECK ENVIRONMENT";
    } else {
      farmStatus = "NORMAL";
    }

    Serial.println();
    Serial.println("------- LIVESTOCK MONITOR -------");

    Serial.print("Temperature: ");
    if (validDHT) {
      Serial.print(temperature, 1);
      Serial.println(" C");
    } else {
      Serial.println("Sensor error");
    }

    Serial.print("Humidity: ");
    if (validDHT) {
      Serial.print(humidity, 1);
      Serial.println(" %");
    } else {
      Serial.println("Sensor error");
    }

    Serial.print("Animal distance: ");
    if (validDistance) {
      Serial.print(distance, 1);
      Serial.println(" cm");
    } else {
      Serial.println("No valid echo");
    }

    Serial.print("Proximity: ");
    Serial.println(proximityStatus);

    Serial.print("Farm status: ");
    Serial.println(farmStatus);

    Serial.print("Buzzer: ");
    Serial.println(animalNearby ? "ON" : "OFF");

    if (validDHT) {
      kiwisiot.send("0", String(temperature, 1));
      kiwisiot.send("1", String(humidity, 1));
    }

    if (validDistance) {
      kiwisiot.send("2", String(distance, 1));
    }

    kiwisiot.send("3", proximityStatus);
    kiwisiot.send("4", farmStatus);

    Serial.println("Data sent to KiwisIoT.");
    Serial.println("--------------------------------");
  }

  delay(10);
}