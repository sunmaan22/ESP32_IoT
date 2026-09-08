#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define PIR_PIN      2   // D0
#define BUZZER_PIN  15   // D1
#define DHT_PIN     26   // D2
#define LED_PIN     32   // D3

#define MIC_PIN     36   // A0
#define GAS_PIN     39   // A1
#define LIGHT_PIN   34   // A2

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

const int LEDC_CHANNEL    = 0;
const int LEDC_FREQ       = 5000;  // 5kHz
const int LEDC_RESOLUTION = 8;     // 0~255

const int MODE_HOME = 0;
const int MODE_AWAY = 1;
int currentMode = MODE_HOME;

const int MIC_THRESHOLD = 1000;
const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";

const char* mqttServer = "YOUR_MQTT_BROKER_IP";
const int   mqttPort   = 1883;

const char* topicSensors = "home/sensors";
const char* topicAlerts  = "home/alert";
const char* topicMode    = "home/mode";

WiFiClient espClient;
PubSubClient client(espClient);

String stMac;
char mac[50];
char clientId[50];


void wifiConnect();
void mqttReconnect();
void publishSensorData(float temperature, float humidity,
                       int lightValue, int gasValue,
                       int pirValue, bool loudSound);


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("[MQTT] Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(" | payload: ");

  String payload;
  for (unsigned int i = 0; i < length; i++) {
    char c = (char)message[i];
    Serial.print(c);
    payload += c;
  }
  Serial.println();

  if (String(topic) == String(topicMode)) {
    if (payload == "HOME") {
      currentMode = MODE_HOME;
      Serial.println("[MODE] HOME 모드로 변경 (MQTT)");
    } else if (payload == "AWAY") {
      currentMode = MODE_AWAY;
      Serial.println("[MODE] AWAY 모드로 변경 (MQTT)");
    } else {
      Serial.println("[MODE] 알 수 없는 모드 명령");
    }
  }
}


void wifiConnect() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("[WiFi] Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  stMac = WiFi.macAddress();
  stMac.replace(":", "_");
}

void mqttReconnect() {
  while (!client.connected()) {
    Serial.print("[MQTT] Attempting connection... ");

    long r = random(1000);
    sprintf(clientId, "ESP32-%ld", r);

    if (client.connect(clientId)) {
      Serial.print(clientId);
      Serial.println(" connected");

      client.subscribe(topicMode);
      Serial.print("[MQTT] Subscribed to: ");
      Serial.println(topicMode);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> 5초 후 재시도");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  randomSeed(analogRead(0));

  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);


  ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL);

  wifiConnect();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  mqttReconnect();
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();

  int lightValue = analogRead(LIGHT_PIN);
  int micValue   = analogRead(MIC_PIN);
  int pirValue   = digitalRead(PIR_PIN);
  int gasValue   = analogRead(GAS_PIN);

  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();


  int duty = map(lightValue, 0, 4095, 255, 0);
  duty = constrain(duty, 255, 0);
  ledcWrite(LEDC_CHANNEL, duty);


  bool loudSound = (micValue >= MIC_THRESHOLD);
  if (loudSound) {
    Serial.println("[ALERT] 큰 소리 감지!");
    client.publish(topicAlerts, "LOUD");
  }


  if (currentMode == MODE_HOME) {
    if (pirValue == HIGH) {
      Serial.println("[HOME] PIR: 사람 감지 (경보 없음)");
    }
    digitalWrite(BUZZER_PIN, LOW);
  } else if (currentMode == MODE_AWAY) {
    if (pirValue == HIGH) {
      Serial.println("[AWAY] 침입 감지! 부저 ON");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(300);
      digitalWrite(BUZZER_PIN, LOW);


      client.publish(topicAlerts, "INTRUSION");
    }
  }


  publishSensorData(temperature, humidity,
                    lightValue, gasValue,
                    pirValue, loudSound);


  Serial.print("Mode: ");
  Serial.print(currentMode == MODE_HOME ? "HOME" : "AWAY");
  Serial.print(" | Light: "); Serial.print(lightValue);
  Serial.print(" (duty "); Serial.print(duty); Serial.print(")");
  Serial.print(" | MIC: "); Serial.print(micValue);
  Serial.print(" | PIR: "); Serial.print(pirValue);
  Serial.print(" | Gas: "); Serial.print(gasValue);
  Serial.print(" | T: "); Serial.print(temperature);
  Serial.print(" | H: "); Serial.println(humidity);

  delay(500);
}

//JSON
void publishSensorData(float temperature, float humidity,
                       int lightValue, int gasValue,
                       int pirValue, bool loudSound) {

  String modeStr = (currentMode == MODE_HOME) ? "HOME" : "AWAY";

  String payload = "{";
  payload += "\"temp\":";   payload += isnan(temperature) ? 0 : temperature; payload += ",";
  payload += "\"hum\":";    payload += isnan(humidity) ? 0 : humidity;      payload += ",";
  payload += "\"light\":";  payload += lightValue;                          payload += ",";
  payload += "\"gas\":";    payload += gasValue;                            payload += ",";
  payload += "\"pir\":";    payload += pirValue;                            payload += ",";
  payload += "\"loud\":";   payload += (loudSound ? 1 : 0);                 payload += ",";
  payload += "\"mode\":\""; payload += modeStr;                             payload += "\"";
  payload += "}";

  client.publish(topicSensors, payload.c_str());
}
