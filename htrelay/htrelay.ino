#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define CLIENT_ID "dhtr1"

#define RELAY_PIN D1
#define DHT_PIN D4

#define DHTTYPE DHT22   // DHT 22  (AM2302)

// Update these with values suitable for your network.
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "192.168.0.9";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMillis = 0;
unsigned long lastSent = 0;

const long maxMillis = 15 * 60 * 1000; //15 minutes;

float temp = 0;
float humid = 0;
DHT dht(DHT_PIN, DHTTYPE);

const int ARR_SZ = 4;

float temps[ARR_SZ] = {-1, -1, -1, -1};
float humids[ARR_SZ] = {-1, -1, -1, -1};
int tempIx = 0;
int humidIx = 0;

float average (float * array) {
  float sum = 0;
  float count = ARR_SZ;
  for (int i = 0; i < ARR_SZ; i++) {
    if (array[i] > -1) {
      sum += array[i];
    } else {
      count --;
    }
  }
  return  sum / count; 
}

float stdDev(float * array) {
  float avg = average(array);
  float devs[4];
  for (int i = 0; i < ARR_SZ; i++) {
    float dev = array[i] - avg;
    devs[i] = sq(dev);
  }
  float avgDev = average(devs);
  float stdDev = (float)sqrt(avgDev);
  return stdDev;
}

void insertHumidity(float humidity) {
  humidIx++;
  if (humidIx >= ARR_SZ) {
    humidIx = 0;
  }
  humids[humidIx] = humidity;
}

void insertTemp(float temp) {
  tempIx++;
  if (tempIx >= ARR_SZ) {
    tempIx = 0;
  }
  temps[tempIx] = temp;
}

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
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.begin();
}

// function called to publish the temperature and the humidity
void publishData(float p_temperature, float p_humidity) {
 
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  String data = "{\n";
  data += "  \"temperature\":  \"";
  data += (String)p_temperature;
  data += "\",\n";
  data += "  \"humidity\":  \"";
  data += p_humidity;
  data += "\"\n";
  data += "}";
  Serial.print(data);
  Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70"
     }
  */
  char buf[200];
  data.toCharArray(buf, data.length() + 1);
  client.publish("node/"CLIENT_ID"/sensor", buf, true);
  //yield();
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  long sleepDelay = 30000;
  
  if (now - lastMillis > 30000) {
    lastMillis = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!(isnan(h) || isnan(t))) {
      sleepDelay = 30000;
      Serial.println("measurements ok");
      float hic = dht.computeHeatIndex(t, h, false);
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(" *C ");
      Serial.print("Heat index: ");
      Serial.print(hic);
      Serial.println(" *C ");
      
      insertHumidity(h);
      insertTemp(t);

      float avgT = average(temps);
      float avgH = average(humids);
      float sdT = stdDev(temps);
      float sdH = stdDev(humids);

      bool updateTime = now - lastSent > maxMillis;

      if (updateTime || // it's 15 mins since last Tx
      abs(avgT - t) > sdT || abs(avgH - h) > sdH || // latest temp is out of range
      abs(avgT - temp) > sdT || abs(avgH - humid) > sdH) { // last sent temp is out of range
        Serial.println("data different or time expired, publishing");
        publishData(t, h); 
        lastSent = now;
        temp = t;
        humid = h; 
      } else {
        Serial.println("data not changed enough to warrant publishing");
      }
      
    } else {
      sleepDelay = 5000;
      Serial.print("H val ");
      Serial.print(h);
      Serial.print("T val ");
      Serial.println(t);
    }
  }
}
