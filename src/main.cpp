#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include "../src/settings.cpp"

// https://github.com/bogde/HX711

// zapojeni https://cdn.instructables.com/F36/2JAR/J822Y1NX/F362JARJ822Y1NX.LARGE.jpg?auto=webp&width=1024&height=1024&fit=bounds
// bacha na barevnost dratu, hlavni je prostredni ze senzoru

// TODO: zavislost na teplote
// zima "zvysuje vahu", teplo snizuje. Pri 20C vaha 14.1kg, pri 3C vaha 14.5kg
// https://community.hiveeyes.org/t/temperature-compensation-for-hx711-and-or-load-cell/1601/4

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D1;
const int LOADCELL_SCK_PIN = D2;

// about 5 minutes
const double DEEP_SLEEP_TIME = 293e6;

Settings settings;

HX711 scale;

// start OTA update process
bool startOTA = false;

// alarm when difference between last two measurements is 1 kg
#define WEIGHT_DECREASE_LIMIT 1

// Attach Blynk virtual serial terminal
WidgetTerminal terminal(V3);

// Synchronize settings from Blynk server with device when internet is connected
BLYNK_CONNECTED()
{
  Blynk.syncAll();
}

// device is enabled
bool isEnabled = true;

// Turn on/off
BLYNK_WRITE(V0)
{
  isEnabled = param.asInt();
}

// Terminal input
BLYNK_WRITE(V3)
{
  String valueFromTerminal = param.asStr();

  if (String("clear") == valueFromTerminal)
  {
    terminal.clear();
    terminal.println("CLEARED");
    terminal.flush();
  }
  if (String("restart") == valueFromTerminal)
  {
    terminal.clear();
    terminal.println("RESTART");
    terminal.flush();
    delay(500);
    ESP.restart();
  }
  else if (String("update") == valueFromTerminal)
  {
    terminal.clear();
    terminal.println("UPDATE");
    terminal.flush();
    startOTA = true;
  }
  else if (valueFromTerminal != "\n" || valueFromTerminal != "\r" || valueFromTerminal != "")
  {
    terminal.println(String("unknown command: ") + valueFromTerminal);
    terminal.flush();
  }
}

void checkForUpdates()
{
  String message = "";
  String fwVersionURL = settings.firmwareVersionUrl;

  Serial.println("Checking for firmware updates.");
  Serial.print("Firmware version URL: ");
  Serial.println(fwVersionURL);

  HTTPClient httpClient;
  WiFiClient client;

  httpClient.begin(client, fwVersionURL);

  int httpCode = httpClient.GET();

  if (httpCode == 200)
  {
    String newFwVersion = httpClient.getString();

    Serial.print("Current firmware version: ");
    Serial.println(settings.version);
    Serial.print("Available firmware version: ");
    Serial.println(newFwVersion);

    if (String(settings.version) != newFwVersion)
    {
      Serial.println("Preparing to update");

      String fwImageURL = settings.firmwareBinUrl;
      t_httpUpdate_return ret = ESPhttpUpdate.update(client, fwImageURL);

      switch (ret)
      {
      case HTTP_UPDATE_OK:
        message = "Successfuly updated!";
        Serial.println(message);
        break;
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        message = "Update failed: " + String(ESPhttpUpdate.getLastErrorString());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        message = "No updates";
        Serial.println(message);
        break;
      }
    }
    else
    {
      message = "Already on latest version";
      Serial.println(message);
    }
  }
  else
  {
    message = "Version check failed, http code: " + String(httpCode) + " ,message: " + httpClient.errorToString(httpCode);
    Serial.println(message);
  }
  httpClient.end();

  terminal.println(message);
  terminal.flush();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.power_up();

  delay(300);

  EEPROM.begin(512);

  if (scale.is_ready())
  {
    // Sencor SBS 113L

    // A
    scale.set_scale(22194.6);
    float value = scale.get_units(10) + 8.37;

    // B
    // scale.set_scale(22500);
    // float value = scale.get_units(10) + 18.45;

    // C
    // scale.set_scale(22500);
    // float value = scale.get_units(10) + 0.9;

    Serial.println("average:\t" + String(value) + " kg");
    Serial.println("Sending to Blynk");

    Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);

    if (isEnabled)
    {
      Blynk.virtualWrite(V1, value);

      float previousValue = 0.00f;
      EEPROM.get(0, previousValue);
      EEPROM.put(0, value);
      EEPROM.commit();

      float difference = value - previousValue;
      Blynk.virtualWrite(V6, difference);

      // notify when diffence from last measuring is bigger than limit
      if (difference < 0 && abs(difference) > WEIGHT_DECREASE_LIMIT)
      {
        Blynk.notify("C: náhlý váhový rozdíl " + String(difference) + "Kg");
      }
    }

    Blynk.virtualWrite(V2, WiFi.RSSI());
    Blynk.virtualWrite(V4, settings.version);
    Blynk.virtualWrite(V5, isEnabled ? "OK" : "Váha je vypnuta.");

    Serial.println("Sent OK");
  }
  else
  {
    Serial.println("HX711 doesn't work");
    Blynk.begin(settings.blynkAuth, settings.wifiSSID, settings.wifiPassword);

    Blynk.virtualWrite(V2, WiFi.RSSI());
    Blynk.virtualWrite(V4, settings.version);
    Blynk.virtualWrite(V5, "HX711 doesn't work");
  }

  scale.power_down();
  delay(300);

  if (startOTA)
  {
    startOTA = false;
    checkForUpdates();
  }

  Blynk.disconnect();

  EEPROM.end();

  Serial.println("Go sleep, BYE.");
  ESP.deepSleep(DEEP_SLEEP_TIME);
}

void loop()
{
}