// Simple Demonstration of a coffe level and temperature sensor. Sends
// mqtt data to broker via Wifi.
//
//
// based on a example by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

#include <Arduino.h>
#include "EEPROM.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Adafruit_ADS1015.h>
#include <Adafruit_MLX90614.h>


// i2c sensors initialisation. Ads0-2 is light sensors
// and mlx is ir temperature sensor.
Adafruit_ADS1115 ads0(0x48); // address 0b1001000
Adafruit_ADS1115 ads1(0x49); // address 0b1001001
Adafruit_ADS1115 ads2(0x4a); // address 0b1001010
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // address 5A 

// Constants for wifi connection.
const char ssid[] = SSID_NAME;
const char pass[] = SSID_PASWORD;

WiFiClientSecure net;
MQTTClient client;
unsigned long lastMillis = 0;

// Json buffer constants
StaticJsonBuffer<256> jb;
JsonObject &obj = jb.createObject();
char output[128];

// Defines Button and led pins
const int buttonPin = 23;
const int buttonDrivePin = 12;
const int ledPin = 13;


// Includes all data from the sensor stick
struct SensorLevelArray{
    unsigned int fillLevel = 0;
    double tempObjectC = 0;
    double tempAmbientC = 0;
    const static int thresholdNum = 12;
    unsigned int sensor[thresholdNum];
    unsigned int sensorThresholdValues[thresholdNum];
    unsigned int currentSense = 0;
};
SensorLevelArray fillArray;

// EEPROM/Flash store size. Determine number of bytes needed to
// store reference values.
#define EEPROM_SIZE fillArray.thresholdNum * sizeof(unsigned int) // <- fix constant
// pin for current reading with ADC conversion.
#define ANALOG_PIN_0 36 


// Used for debugging.
#define DEBUG 
#ifdef DEBUG
    #define DEBUG_PRINT(x)      Serial.print (x)
    #define DEBUG_PRINTln(x)    Serial.println (x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTln(x)
#endif

// Checks if we have a valid connection for Wifi and
// mqtt broker.
void connect()
{
    // Checks if we got a wifi connection,
    // reboots system if we fails 15 times.
    uint8_t i = 0;
    DEBUG_PRINT("checking wifi...");
    while (WiFi.status() != WL_CONNECTED) {
	DEBUG_PRINT(".");
	delay(1000);
	i++;
	if (i > 15){
	    // restart when wifi hangs
	    ESP.restart();
	}
    }

    // Connects to mqtt broker, hangs if none is
    // found.
    DEBUG_PRINT("\n\rconnecting...");
    while (!client.connect("mokkamaster", "try", "try")) {
	DEBUG_PRINT(".");
	delay(1000);
    }
    DEBUG_PRINTln("\n\rconnected!");
    client.subscribe("coffeMaker1");
}


// Message received callback. Does nothing
// except printing topics and payload.
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
    for (int i = 0; i < fillArray.thresholdNum ; i++)
    {
	Serial.print("eeprom :" + String(i) + " : ");
	Serial.println(EEPROM.readUInt(i*4));
	
    }
    // initialize i2c bus and sensors.
    ads0.begin();
    ads1.begin();
    ads2.begin();
    mlx.begin();

    // starts Wifi connection.
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

// Loops trough i2c adc, and updates values
// in struct sensorArray
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

// Updates threshold values stored in EEPROM/FLASH. Theese
// Are used when we determine fill level. ButtonPin is used when we
// store settings.
void configMode(SensorLevelArray *sensorArray){
    // <=TODO give some visulal feedback when user isn't connected via terminal.
    // <=TODO fix fix hang forever, if user dosent complete setup.
    int serialByte = 0;
    Serial.println("This is for configuring you brand new coffee machine iot sensor.");
    Serial.println("Please place a empty coffe container on the hotplate and press");
    Serial.println("button.");
    delay(1000);
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


// main program loop
void loop() {

    delay(200);
    if(digitalRead(buttonPin) == HIGH){
	configMode(&fillArray);
	for( int i = 0; i != 12; ++i){
	    EEPROM.writeUInt(i * sizeof(fillArray.sensorThresholdValues[i]),  fillArray.sensorThresholdValues[i]);
	    EEPROM.commit();
	}
    }

    client.loop();
    delay(1000);  // <- should fix some issues with WiFi stability
    if (!client.connected()){
	connect();
    }
    //
    //Only runs when millis overflows.
    //This is our main loop.
    if (millis() - lastMillis > 2000) {
	delay(10);
	for( int i = 0; i < fillArray.thresholdNum ; ++i){
	    DEBUG_PRINT("eeprom :" + String(i) + " : ");
	    DEBUG_PRINTln(EEPROM.readUInt(i* sizeof(fillArray.sensorThresholdValues[i])));
	}
	// Checks if coffemaker draws current
	fillArray.currentSense = analogRead(ANALOG_PIN_0); // this needs a filter or something
	// DEBUG_PRINTln(currentSense);

	getSensorData(&fillArray);
	getCoffeLevel(&fillArray);
	fillArray.tempObjectC =  mlx.readObjectTempC();
	fillArray.tempAmbientC = mlx.readAmbientTempC();
	//
	// Compares sensorvalue with reference value and sets coffeLevel count accordingly.
	int coffeLevel = 0;
	int i = 0;
	for( sensorValue : fillArray.sensor){
	    if (sensorValue > fillArray.sensorThresholdValues[i]){ coffeLevel++ ;}
	    i++;
	}
	fillArray.fillLevel = coffeLevel;

	//Updates Jsonbuffer with current values
	//and sends mqtt message
	obj["currentSense"] = fillArray.currentSense;
	obj["CoffeLevel"] = fillArray.fillLevel;
	obj["PerculatorOnTimer"].set(42);
	obj["coffeTemp"].set(fillArray.tempObjectC);
	obj["ambientTemp"].set(fillArray.tempAmbientC);
	obj.printTo(output);
	Serial.println(output);
	client.publish("coffeMaker1", output);

	lastMillis = millis();
    }
}
