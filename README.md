# mokkamaster-autoff-iot

Simple project to demonstrate howto use Wifi and mqtt on a esp32 using the Arduino
framework. The project includes a custom sensor board that comunicates to host via
i2c. The sensor is ment to provide useful feedback from a MokkaMaster or similar type
of coffe machine.

## Installing

This project uses [Platformio](https://platformio.org/) as build platform. It installs from
terminal by typing in the following commands:
```
pip install platformio
cd ../coffemakerIot
platformio run
platformio run -t upload

```
### Prerequisites

You probably need pip to install platfomrio. you also need to add yourself to the dialout 
group.

```
python pip
sudo adduser $USER dialout
```

## Known bugs

I2c Bus dropouts, could be hw fault.
Wifi dropouts

# Investigate this
[E][ssl_client.cpp:32] handle_error(): UNKNOWN ERROR CODE (004E)
[E][ssl_client.cpp:34] handle_error(): MbedTLS message code: -78
[E][ssl_client.cpp:84] start_ssl_client(): Connect to Server failed!
[E][WiFiClientSecure.cpp:108] connect(): lwip_connect_r: 113
[E][ssl_client.cpp:84] start_ssl_client(): Connect to Server failed!
[E][WiFiClientSecure.cpp:108] connect(): lwip_connect_r: 113
[E][ssl_client.cpp:84] start_ssl_client(): Connect to Server failed!
[E][WiFiClientSecure.cpp:108] connect(): lwip_connect_r: 113


## Built With

* [Arduino](https://www.arduino.cc/) - Arduino enviroment
* [ArduinoJson](https://arduinojson.org/) - Json library
* [arduino-mqtt](https://github.com/256dpi/arduino-mqtt) - Mqtt framework
* [Adafruit-sensors](https://www.adafruit.com/) - libraries for ADS1X15 and MLX90614 sensors

## Authors

* **Armin Bårdseth** - *Initial work*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc

