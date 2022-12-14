# Bee hive weight monitoring with ESP8266 or ESP32
Bee hive weight monitoring. 
Based on [Bodge HX711 library](https://github.com/bogde/HX711).
Data are sent to the Blynk.

## 🚀 12.2022 - UPDATE TO BLYNK 2.0 🚀
Project was updated to the new version of [Blynk 2.0](https://docs.blynk.io/en/)

> To build a project, you need to download all the necessary libraries and create the *settings.cpp* file in the *src* folder:
```c++
// Project settings
struct Settings
{
    const char *wifiSSID = "wifi";
    const char *wifiPassword = "pswd";
    const char *blynkAuth = "blynkAuth"; // for each weight
    const char *version = "1.0.0";
    const double scale = 22500; // for each weight
    const double offset = 0.9; // for each weight
};
```
### Features
* measuring the weight of the beehive
* Sends push notification, when weight decrease between measuring is lower than 1Kg (detection of bee swarm)

### Currents list:

* *ESP32 WROOM-32* or *WEMOS D1 Mini*
* [HX711](https://www.aliexpress.com/wholesale?SearchText=hx711)
* *Sencor SBS 113SL* or some other load cells

### In development 🚧
You should consider getting into the details of strain-gauge load cell sensors when expecting reasonable results. The range of topics is from sufficient and stable power supply, using the proper excitation voltage to the Seebeck effect and temperature compensation. See also [Bodge HX711 library](https://github.com/bogde/HX711).

### Schema:
![Schema](https://github.com/vitzaoral/beehive_weight/blob/master/schema/schema.png)

### Blynk:
![Schema](https://github.com/vitzaoral/beehive_weight/blob/master/schema/blynk.jpeg)

### Outside:
![Schema](https://github.com/vitzaoral/beehive_weight/blob/master/schema/1.jpeg)
![Schema](https://github.com/vitzaoral/beehive_weight/blob/master/schema/2.jpeg)
![Schema](https://github.com/vitzaoral/beehive_weight/blob/master/schema/3.jpeg)