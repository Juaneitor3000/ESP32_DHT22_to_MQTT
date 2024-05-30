#include <esp_sleep.h>     // we use this to activate ESP32 Deep Sleep
#include <WiFi.h>          // used by PubSubClient library
#include <PubSubClient.h>  //library that allow us to connect to a broker , publish and suscribe to a MQTT topic over the network
#include <DHT.h>           //Allow us to read data from Temperature/Humidity DHT 22  (AM2302), AM2321 sensor
#include <DHT_U.h>         //Allow us to read data from Temperature/Humidity DHT 22  (AM2302), AM2321 sensor
#include <iostream>
#include <string>
#include <ArduinoJson.h>   // help us to "serialize" variables in to a JSON format.
#include <cmath>           // i have used this to round float variables to 2 digits float
using namespace std;


// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN 16     //pin where you will connect data pin  DHT sensor
#define DHT_SUPPLY 17   //This pin will supply the dth 22. It will not be ON all time.

//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// Deep Sleep Time
long DEEP_SLEEP_TIME_SEC = 600;  //Deep sleep time [Seconds]

// WiFi
const char *ssid = "Marcano"; // Enter your WiFi name
const char *password = "neutrino";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "192.168.1.15";     // MQTT broker IP
const char *topic = "esp32";                  //this is the MQTT main topic, you can change it.
const char *mqtt_username = "";               // use your MQTT broker uername or leave "" if there is no one.
const char *mqtt_password = "";               // use your MQTT broker password or leave "" if there is no one.
const int mqtt_port = 1883;                   //MQTT broker port, usually 1883.

WiFiClient espClient;
PubSubClient client(espClient);

// 
//  This function callback is used when you suscribe to a topic, not the case here, we dont use it, i just publish data and the we put uC to sleep to save battery
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
  Serial.print("String lenght:\n");
  Serial.print(L);
  Serial.print("\nJson to send:\n");
  Serial.print(out);
  Serial.print("\nOk lets publish ..\n");
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
  
  build_publish_json(h,t,f,hic,hif);  //call to function that build JSON with our sensor data.

}

void setup() {
 //pin to power_supply dht 22 
 pinMode (DHT_SUPPLY, OUTPUT);    
//led blink
 pinMode (2, OUTPUT);
 // Set software serial baud to 115200;
 Serial.begin(115200);
 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
    digitalWrite (2, HIGH);;
    delay(250);
    digitalWrite (2, LOW);
    delay(250);
    Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");
 digitalWrite (2, LOW);
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
 digitalWrite (DHT_SUPPLY, HIGH);
 read_dht();                          // Call read_dht function.
 delay(50);                           // time added after dht reading, to complete uart comunication. 
 Serial.print(client.state());
 Serial.end();
 client.disconnect();
 WiFi.disconnect();
 digitalWrite (DHT_SUPPLY, LOW);       // Turn off dht22
 digitalWrite (2, HIGH);;
 delay(50);                            // time added before deep sleep, to complete uart comunication and led blink.
 digitalWrite (2, LOW);               
 esp_deep_sleep(1000000ULL * DEEP_SLEEP_TIME_SEC);   
}

void loop() 
{
//client.loop();
}










