// based on a example by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <Adafruit_MLX90614.h>

// #include <PubSubClient.h>
#define MQTT_MAX_PACKET_SIZE 512


Adafruit_ADS1115 ads0(0x48); // address 0b1001000
Adafruit_ADS1115 ads1(0x49); // address 0b1001001
Adafruit_ADS1115 ads2(0x4a); // address 0b1001010
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // address 5A 
/* const char ssid[] = "dang"; */
/* const char pass[] = "passdegforaliens"; */
const char ssid[] = "origo";
const char pass[] = "langepassorderviktig";
/* const char ssid[] = "farmin"; */
/* const char pass[] = "farmin85"; */

WiFiClientSecure net;
MQTTClient client;
unsigned long lastMillis = 0;

void connect() {
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED) {
	Serial.print(".");
	delay(1000);
    }

    Serial.print("\n\rconnecting...");
    while (!client.connect("mokkamaster", "try", "try")) {
	Serial.print(".");
	delay(1000);
    }

    Serial.println("\n\rconnected!");
    client.subscribe("coffeMaker1");
    // client.unsubscribe("coffeMaker1");
}

void messageReceived(String &topic, String &payload) {
    Serial.println("incoming: " + topic + " - " + payload);
}

void setup() {
    Serial.begin(115200);
    ads0.begin();
    ads1.begin();
    ads2.begin();
    mlx.begin();

    WiFi.begin(ssid, pass);
    // MQTT brokers usually use port 8883 for secure connections.
    //client.begin("broker.shiftr.io", 8883, net);
    client.begin("192.168.188.235", 8883, net);
    //client.begin("mqtt.googleapis.com", 8883, net);
    client.onMessage(messageReceived);
    connect();
}

int getCoffeLevel(unsigned int *sensors, int arrayLength){
    for (uint8_t i = 0; i != 11 ; i++ ){
	Serial.println (sensors[i]);
    }
}

int getSensorData(unsigned int *sensors, int arrayLength){
    /* for (uint8_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]) + 1 ; i++ ){ */
    for (uint8_t i = 0; i != arrayLength ; ++i ){
	if (i >= 0 && i <= 3){
	    sensors[i] = ads0.readADC_SingleEnded(i);
	    Serial.print(sensors[i]);
	    Serial.print(" first ");
	    Serial.println(i);
	}
	else if( i >= 3  && i <= 7 ){
	    sensors[i] = ads1.readADC_SingleEnded(i - 4);
	    Serial.print(sensors[i]);
	    Serial.print(" second ");
	    Serial.println(i);
	}
	else if (i >= 7 ){
	    sensors[i] = ads2.readADC_SingleEnded(i - 8);
	    Serial.print(sensors[i]);
	    Serial.print(" third ");
	    Serial.println(i);
	}
	else{
	    Serial.print("error");
	}
    }
}


StaticJsonBuffer<256> jb;
JsonObject &obj = jb.createObject();
char output[128];


void loop() {
    client.loop();
    delay(1000);  // <- fixes some issues with WiFi stability
    if (!client.connected()) {
	connect();
    }
    if (millis() - lastMillis > 2000) {
	ads0.begin();
	ads1.begin();
	ads2.begin();
	mlx.begin();
	delay(10);
	
	unsigned int lightSensorArray[11];
	getSensorData(lightSensorArray, 11);
	//getCoffeLevel(lightSensorArray, 11);
	char toc[8];
	char tac[8];
	double tempObjectC = mlx.readObjectTempC();
	double tempAmbientC = mlx.readAmbientTempC();
	dtostrf(tempObjectC, 3, 1, toc);
	dtostrf(tempAmbientC, 3, 1, tac);
		
	Serial.print("object temp: ");
	Serial.println(toc);
	Serial.print("ambient temp: ");
	Serial.println(tac);

	obj["CoffeLevel"] = 1;
	obj["PerculatorOnTimer"].set(42);
	obj["coffeTemp"].set(tempObjectC);
	obj["ambientTemp"].set(tempAmbientC);

	
	obj.printTo(output);
	//Serial.println(output);

	client.publish("coffeMaker1", output);

	lastMillis = millis();
    }
}
