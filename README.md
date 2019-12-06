# Bee hive weight monitoring with ESP8266 or ESP32
Bee hive weight monitoring. 
Based on [Bodge HX711 library](https://github.com/bogde/HX711).
In development.. 🚧 

> To build a project, you need to download all the necessary libraries and create the *settings.cpp* file in the *src* folder:
```c++
// Project settings
struct Settings
{
    const char *wifiSSID = "YYY";
    const char *wifiPassword = "ZZZ";
    const char *blynkAuth = "WWW";
};
```
### Features
* measuring the weight of the beehive

### Currents list:

* *ESP32 WROOM-32* or *WEMOS D1 Mini*
* [HX711](https://www.aliexpress.com/wholesale?SearchText=hx711)
* *Sencor SBS 113L* or some other load cells

### In development 🚧
You should consider getting into the details of strain-gauge load cell sensors when expecting reasonable results. The range of topics is from sufficient and stable power supply, using the proper excitation voltage to the Seebeck effect and temperature compensation. See also [Bodge HX711 library](https://github.com/bogde/HX711).