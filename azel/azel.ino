
#include <Servo.h>
#include <math.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define CLIENT_ID "azel1"

Servo azimuth;
Servo elevation;

int azimuthVal = -1;
int elevationVal = -1;
bool positionReset = false;

const int AZIMUTH = D4;
const int ELEVATION = D3;

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "192.168.0.9";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); //We don't want the ESP to act as an AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected

  digitalWrite(LED_BUILTIN, HIGH);

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe("sun/position");
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  char azStr[length];
  char elStr[length];

  int j = 0;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];

    if (c == ':') {
      j = -1;
      azStr[i] = '\0';
    } else {
      if (j == i) {
        azStr[i] = c;
      } else {
        elStr[j] = c;
      }
    }

    j++;
  }
  elStr[j] = '\0';

  int az = atoi(azStr);
  int el = atoi(elStr);

  Serial.print("azimuth: ");
  Serial.print(az);
  Serial.print(", elevation: ");
  Serial.println(el);

  az = 180 - az;

  if (el > 0) {
    positionReset = false;
    Serial.println("Writing az/el values...");
    azW(az < 0 ? 0 : az > 180 ? 180 : az);
    elW(el);
  } else {
    Serial.println("Elevation below horizon.");
    
    if (!positionReset) {
      Serial.println("Resetting azimuth...");
      int resetAzVal = 180 - az;

      for (int i = az; i >= resetAzVal; i--) {
        azW(i);
        delay(50);
      }
      elW(0);
      positionReset = true;
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void azW(int az) {
  if (az != azimuthVal) {
    Serial.print("Writing azimuth: ");
    Serial.println(az);
    azimuth.write(az);
    azimuthVal = az;
  } else {
    Serial.println("Azimuth not changed, ignoring.");
  }
}

void elW(int el) {
  el = el < 0 ? 0 : el;
  if (el != elevationVal) {
    Serial.print("Writing elevation: ");
    Serial.println(el);
    azimuth.write(el);
    elevationVal = el;
  } else {
    Serial.println("Elevation not changed, ignoring.");
  }
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(AZIMUTH, OUTPUT);
  pinMode(ELEVATION, OUTPUT);

  Serial.begin(115200);

  azimuth.attach(AZIMUTH);
  azimuth.write(180);
  elevation.attach(ELEVATION);
  elevation.write(0);

  delay(2000);

  Serial.println("Sweeping azimuth...");
  for (int i = 180; i >= 0; i--) {
    azimuth.write(i);
    delay(10);
  }
  for (int i = 0; i <= 180; i++) {
    azimuth.write(i);
    delay(10);
  }

  Serial.println("Sweeping elevation...");
  for (int i = 0; i <= 180; i++) {
    elevation.write(i);
    delay(10);
  }
  for (int i = 180; i >= 0; i--) {
    elevation.write(i);
    delay(10);
  }

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
