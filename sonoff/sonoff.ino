#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define CLIENT_ID "sr1"

#define RELAY_PIN 12
#define LED_PIN 13
#define EXT_PIN 14
#define BTN_PIN 0

// Update these with values suitable for your network.
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "192.168.0.9";
bool relayState = LOW;

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
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void doSwitch() {
  if (relayState == HIGH) {
    Serial.println("Setting State ON");
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    client.publish("node/"CLIENT_ID"/relay", "1", true);
  } else {
    Serial.println("Setting State OFF");
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
    client.publish("node/"CLIENT_ID"/relay", "0", true);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character

  if ((char)payload[0] == '1') {
    relayState = HIGH;
  } else {
    relayState = LOW;
  }
  doSwitch();
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite(LED_PIN, HIGH);
  delay(250);
  while (!client.connected()) {
    digitalWrite(LED_PIN, LOW);
    delay(250);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe("node/"CLIENT_ID"/relay/set");
      for (int i = 0; i < 10; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(150);
        digitalWrite(LED_PIN, LOW);
        delay(50);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 2.5 seconds before retrying
      delay(2500);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(150);
    digitalWrite(LED_PIN, HIGH);
    delay(150);
  }
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // unreliable!
  //extButton();
}
