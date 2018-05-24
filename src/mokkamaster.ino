// based on a example by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt
#include <Arduino.h>
#include "EEPROM.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Adafruit_ADS1015.h>
#include <Adafruit_MLX90614.h>
// #include <Button.h>

#define MQTT_MAX_PACKET_SIZE 512
#define ANALOG_PIN_0 36
int currentSense = 0;

Adafruit_ADS1115 ads0(0x48); // address 0b1001000
Adafruit_ADS1115 ads1(0x49); // address 0b1001001
Adafruit_ADS1115 ads2(0x4a); // address 0b1001010
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // address 5A 

const char ssid[] = SSID_NAME;
const char pass[] = SSID_PASWORD;

int addr = 0;
//#define EEPROM_SIZE 4 * 11
#define SENSOR_ARRAY_SIZE 11
#define EEPROM_SIZE SENSOR_ARRAY_SIZE * sizeof(unsigned int) 

WiFiClientSecure net;
MQTTClient client;
unsigned long lastMillis = 0;

const int buttonPin = 23;
const int buttonDrivePin = 12;
const int ledPin = 13;


#define DEBUG 
#ifdef DEBUG
#define DEBUG_PRINT(x)      Serial.print (x)
#define DEBUG_PRINTln(x)    Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTln(x)
#endif


struct SensorLevelArray{
    unsigned int fillLevel = 0;
    double tempObjectC = 0;
    double tempAmbientC = 0;
    const static int thresholdNum = 11;
    unsigned int sensor[thresholdNum];
    unsigned int sensorThresholdValues[thresholdNum];
};
SensorLevelArray fillArray;

//unsigned int  sensorArrayThreshold[11];

void connect()
{
    DEBUG_PRINT("checking wifi...");
    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED) {
	DEBUG_PRINT(".");
	delay(1000);
	i++;
	if (i > 15){
	    // restart when wifi hangs
	    ESP.restart();
	}
    }
    
    DEBUG_PRINT("\n\rconnecting...");
    while (!client.connect("mokkamaster", "try", "try")) {
	DEBUG_PRINT(".");
	delay(1000);
    }

    DEBUG_PRINTln("\n\rconnected!");
    client.subscribe("coffeMaker1");
}

void messageReceived(String &topic, String &payload) {
    Serial.println("incoming: " + topic + " - " + payload);
    // client.unsubscribe("coffeMaker1");
}

void setup()
{
    Serial.begin(115200);
    pinMode(buttonPin, INPUT);
    pinMode(buttonDrivePin, OUTPUT);
    digitalWrite(buttonDrivePin, HIGH);
    
    if (!EEPROM.begin(EEPROM_SIZE))
    {
	Serial.println("failed to initialise EEPROM");
	delay(1000000);
    }
     Serial.println(" bytes read from Flash . Values are:");
     //    for (int i = 0; i < EEPROM_SIZE; ++i)
     for (int i = 0; i < SENSOR_ARRAY_SIZE ; i++)
    {
	Serial.print("eeprom :" + String(i) + " : ");
	Serial.println(EEPROM.readUInt(i*4));
	
    }
    
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

int getCoffeLevel(SensorLevelArray *sensorArray ){
    for (uint8_t i = 0; i < sensorArray->thresholdNum ; i++ ){
	Serial.printf("sensor %1d has value %5d\r\n", i, sensorArray->sensor[i]);
	//Serial.println (sensors[i]);
    }
}

// int getCoffeLevel(unsigned int *sensors, int arrayLength){
//     for (uint8_t i = 0; i < SENSOR_ARRAY_SIZE ; i++ ){
// 	Serial.printf("sensor %1d has value %5d\r\n", i, sensors[i]);
// 	//Serial.println (sensors[i]);
//     }
// }

int getSensorData(SensorLevelArray *sensorArray){
    for (uint8_t i = 0; i != sensorArray->thresholdNum ; i++ ){
	if (i >= 0 && i <= 3){
	    sensorArray->sensor[i] = ads0.readADC_SingleEnded(i);
	}
	else if( i >= 3  && i <= 7 ){
	    sensorArray->sensor[i] = ads1.readADC_SingleEnded(i - 4);
	}
	else if (i >= 7 ){
	    sensorArray->sensor[i] = ads2.readADC_SingleEnded(i - 8);
	}
	else{
	    Serial.print("error");
	}
    }
    
}

// int getSensorData(unsigned int *sensors, int arrayLength){
//     /* for (uint8_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]) + 1 ; i++ ){ */
//     for (uint8_t i = 0; i != arrayLength ; i++ ){
// 	if (i >= 0 && i <= 3){
// 	    sensors[i] = ads0.readADC_SingleEnded(i);
// 	}
// 	else if( i >= 3  && i <= 7 ){
// 	    sensors[i] = ads1.readADC_SingleEnded(i - 4);
// 	}
// 	else if (i >= 7 ){
// 	    sensors[i] = ads2.readADC_SingleEnded(i - 8);
// 	}
// 	else{
// 	    Serial.print("error");
// 	}
//     }
// }

void configMode(SensorLevelArray *sensorArray){
    //  TODO ==> get some eeprom and stor values permanently, this have to do for now.
    int serialByte = 0;
    Serial.println("This is for configuring you brand new coffee machine iot sensor.");
    Serial.println("Please place a empty coffe container on the hotplate and press");
    Serial.println("button.");
    delay(3000);

    while( digitalRead(buttonPin) != HIGH)
    {
	Serial.print('.');
	delay(250);
    }
    SensorLevelArray sensorsEmptyContainer;

    getSensorData(&sensorsEmptyContainer);
    Serial.println("Now fill the container with coffe and press the button again");
    delay(1000);
    while( digitalRead(buttonPin) != HIGH)
    {
	Serial.print('.');
	delay(250);
    }
    unsigned int cf = 0;
    SensorLevelArray sensorsWhenCoffe;
    getSensorData(&sensorsWhenCoffe);
    for(int i = 0; i < sensorArray->thresholdNum ; i++ )
    {
	// dLow + dHigh / 2
	cf = ((sensorsEmptyContainer.sensor[i] + sensorsWhenCoffe.sensor[i])) ;
	cf = cf / 2 ;
	sensorArray->sensorThresholdValues[i] = cf;
    }
}


// void configMode(unsigned int *sensorThresholdValues, int arrayLength){
//     //  TODO ==> get some eeprom and stor values permanently, this have to do for now.
//     int serialByte = 0;
//     Serial.println("This is for configuring you brand new coffee machine iot sensor.");
//     Serial.println("Please place a empty coffe container on the hotplate and press");
//     Serial.println("button.");
//     delay(3000);

//     while( digitalRead(buttonPin) != HIGH)
//     {
// 	Serial.print('.');
// 	delay(250);
//     }
//     unsigned int sensorsWhenLit[11];
//     getSensorData(sensorsWhenLit, 12);
//     Serial.println("Now fill the container with coffe and press the button again");
//     delay(1000);
//     while( digitalRead(buttonPin) != HIGH)
//     {
// 	Serial.print('.');
// 	delay(250);
//     }
//     unsigned int sensorsWhenCoffe[11];
//     //float cf = 0;
//     unsigned int cf = 0;
//     getSensorData(sensorsWhenCoffe, 12);
//     for(int i = 0; i < arrayLength; i++ )
//     {
// 	// dLow + dHigh / 2
// 	cf = ((sensorsWhenLit[i] + sensorsWhenCoffe[i])) ;
// 	cf = cf / 2 ;
// 	sensorThresholdValues[i] = cf;
//     }
// }


StaticJsonBuffer<256> jb;
JsonObject &obj = jb.createObject();
char output[128];


void loop() {

    delay(200);
    if(digitalRead(buttonPin) == HIGH){
	//configMode(sensorArrayThreshold, 12);
	configMode(&fillArray);
	for( int i = 0; i != 12; ++i){
	    EEPROM.writeUInt(i * sizeof(fillArray.sensorThresholdValues[i]),
			     fillArray.sensorThresholdValues[i]);
	    EEPROM.commit();
	}
    }

    client.loop();
    delay(1000);  // <- fixes some issues with WiFi stability
    if (!client.connected()){
	connect();
    }
    if (millis() - lastMillis > 2000) {
	delay(10);

	for( int i = 0; i < SENSOR_ARRAY_SIZE ; ++i){
	    DEBUG_PRINT("eeprom :" + String(i) + " : ");
	    DEBUG_PRINTln(EEPROM.readUInt(i* sizeof(sensorArrayThreshold[i])));
	}

	currentSense = analogRead(ANALOG_PIN_0); // this needs a filter or something
	// DEBUG_PRINTln(currentSense);
		
	// unsigned int lightSensorArray[11];
	// getSensorData(lightSensorArray, 12);
	//getCoffeLevel(lightSensorArray, 12);

	getSensorData(&fillArray);
	getCoffeLevel(&fillArray);
	fillArray.tempObjectC =  mlx.readObjectTempC();
	fillArray.tempAmbientC = mlx.readAmbientTempC();
	
	// char toc[8];
	// char tac[8];
	// double tempObjectC = mlx.readObjectTempC();
	// double tempAmbientC = mlx.readAmbientTempC();
	// dtostrf(tempObjectC, 3, 1, toc);
	// dtostrf(tempAmbientC, 3, 1, tac);
	// DEBUG_PRINT("object temp: ");
	// DEBUG_PRINTln(toc);
	// DEBUG_PRINT("ambient temp: ");
	// DEBUG_PRINTln(tac);
	obj["currentSense"] = currentSense;
	obj["CoffeLevel"] = 1;
	obj["PerculatorOnTimer"].set(42);
	obj["coffeTemp"].set(fillArray.tempObjectC);
	obj["ambientTemp"].set(fillArray.tempAmbientC);
	
	obj.printTo(output);
	Serial.println(output);
	client.publish("coffeMaker1", output);

	lastMillis = millis();
    }
}
