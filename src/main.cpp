#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "../src/settings.cpp"

// https://github.com/bogde/HX711

// zapojeni https://cdn.instructables.com/F36/2JAR/J822Y1NX/F362JARJ822Y1NX.LARGE.jpg?auto=webp&width=1024&height=1024&fit=bounds
// bacha na barevnost dratu, hlavni je prostredni ze senzoru

// TODO: zavislost na teplote
// zima "zvysuje vahu", teplo snizuje. Pri 20C vaha 14.1kg, pri 3C vaha 14.5kg
// https://community.hiveeyes.org/t/temperature-compensation-for-hx711-and-or-load-cell/1601/4

// HX711 circuit wiring
// TODO: jine piny at neblika LED dioda
const int LOADCELL_DOUT_PIN = D3;
const int LOADCELL_SCK_PIN = D4;

const double DEEP_SLEEP_TIME = 300e6;

Settings settings;

HX711 scale;

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.power_up();

  delay(100);

  if (scale.is_ready())
  {
    // Sencor SBS 113L
    scale.set_scale(22336.f);
    float value = scale.get_units(10) + 0.62;

    Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);
    Blynk.virtualWrite(V1, value);
    Blynk.virtualWrite(V2, WiFi.RSSI());

    Serial.println("average:\t" + String(value) + " kg");
  }
  else
  {
    Serial.println("HX711 doesn't work");
  }

  scale.power_down();
  delay(300);
  ESP.deepSleep(DEEP_SLEEP_TIME);
}

void loop()
{
}