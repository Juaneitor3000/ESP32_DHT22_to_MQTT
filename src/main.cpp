#include <esp_sleep.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <iostream>
#include <string>
#include <ArduinoJson.h>
#include <cmath>
using namespace std;


// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN 16     //pin where you will connect data pin  DHT sensor

//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);


// WiFi
const char *ssid = "Marcano"; // Enter your WiFi name
const char *password = "neutrino";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "192.168.1.15";
char *topic = "esp32";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;
// led blink


WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}

void build_publish_json(float h,float t,float f,float hic,float hif){
  
  JsonDocument doc;

  char out[256];
  doc["hum"]=round(h * 100.0) / 100.0;
  doc["temp"]=round(t * 100.0) / 100.0;
  doc["tempf"]=round(f * 100.0) / 100.0;
  doc["temphic"]=round(hic * 100.0) / 100.0;
  doc["temphif"]=round(hif * 100.0) / 100.0;

  int L=serializeJson(doc,out);
  Serial.print("Longitud de la serialización:\n");
  Serial.print(L);
  Serial.print("\nJson a enviar:\n");
  Serial.print(out);
  Serial.print("\nPublicando ..\n");
  client.publish("esp32", out,L);
}

void read_dht(){
// Wait a few seconds between measurements.
  delay(2000);
  digitalWrite (2, HIGH);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  digitalWrite (2, LOW);
  
  build_publish_json(h,t,f,hic,hif);


}

void setup() {

 
 //led blink
 pinMode (2, OUTPUT);
 // Set software serial baud to 115200;
 Serial.begin(115200);
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");
 
 //connecting to a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
 //client.publish(topic, "Hi EMQX I'm ESP32 ^^");     //
 //client.subscribe(topic);     //allow to subcribe to a topic, not the case here we are using deep sleep
 
 
 
 dht.begin();
 read_dht();
 delay(10000);
 Serial.print(client.state());
 Serial.end();
 client.disconnect();
 WiFi.disconnect();
 delay(5000);
 esp_deep_sleep(600 * 1000000); 
}



void loop() {
//client.loop();

}










