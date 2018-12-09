#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define CLIENT_ID "mor1"

#define RELAY_PIN D1
#define PIR_PIN D4

// Update these with values suitable for your network.
const char* ssid = "";
const char* password` = "";
const char* mqtt_server = "192.168.0.9";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
float temp = 0;

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
    digitalWrite(RELAY_PIN, HIGH);
    client.publish("node/"CLIENT_ID"/relay", "1", true);
  } else {
    digitalWrite(RELAY_PIN, LOW);
    client.publish("node/"CLIENT_ID"/relay", "0", true);
  }

}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite(LED_BUILTIN, LOW);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe("node/"CLIENT_ID"/relay/set");
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
 
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
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
  long val = digitalRead(PIR_PIN);
  if (val != lastMsg) {
    lastMsg = val;
    if (val == LOW) {
      client.publish("node/"CLIENT_ID"/motion", "0", true);  
      Serial.println("motion stopped");
    } else {
      client.publish("node/"CLIENT_ID"/motion", "1", true);  
      Serial.println("Motion started");
    }
  }
}
