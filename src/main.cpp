#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLkxFvg8yn"
#define BLYNK_TEMPLATE_NAME "Váhy včel"
#define BLYNK_FIRMWARE_VERSION "2.1.0"

#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
extern "C" {
  #include <user_interface.h>
}
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

// about 5 minutes 293e6
// about 20 minutes 1193e6
// const double DEEP_SLEEP_TIME = 293e6;

// Deep sleep interval in seconds
int deep_sleep_interval = 285;

Settings settings;
HX711 scale;

BLYNK_WRITE(InternalPinOTA)
{
  Serial.println("OTA Started");
  String otaUrl = param.asString();
  Serial.print("overTheAirURL = ");
  Serial.println(otaUrl);

  WiFiClient otaClient;
  t_httpUpdate_return ret = ESPhttpUpdate.update(otaClient, otaUrl);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.println("[update] Update failed.");
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not be called since we reboot the ESP
    break;
  }
}

// deep sleep interval in seconds
BLYNK_WRITE(V8)
{
  if (param.asInt())
  {
    deep_sleep_interval = param.asInt();
    Serial.println("Deep sleep interval was set to: " + String(deep_sleep_interval));
    Blynk.virtualWrite(V7, String(deep_sleep_interval));
  }
}

// Synchronize settings from Blynk server with device when internet is connected
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0, V8);
}

// device is enabled
bool isEnabled = true;

// Turn on/off
BLYNK_WRITE(V0)
{
  isEnabled = param.asInt();
}


void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing the scale");

  EEPROM.begin(512);

  // Rucne zkontrolovat jestli HX711 je pripojene pred volanim begin(),
  // protoze begin() vola set_gain() -> read() -> wait_ready() coz je
  // nekonecny loop a zpusobi WDT reset kdyz HX711 neni zapojene
  pinMode(LOADCELL_DOUT_PIN, INPUT);
  pinMode(LOADCELL_SCK_PIN, OUTPUT);
  digitalWrite(LOADCELL_SCK_PIN, LOW);
  delay(500);

  bool hx711Present = false;
  unsigned long startMs = millis();
  while (millis() - startMs < 3000)
  {
    if (digitalRead(LOADCELL_DOUT_PIN) == LOW)
    {
      hx711Present = true;
      break;
    }
    delay(10);
  }

  bool secondChance = false;
  bool scaleReady = false;

  if (hx711Present)
  {
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    delay(300);
    scale.power_up();
    delay(1000);
    scaleReady = scale.wait_ready_timeout(3000);

    if (!scaleReady)
    {
      secondChance = true;
      scale.power_down();
      delay(1000);
      scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
      delay(300);
      scale.power_up();
      delay(1000);
      yield();
      scaleReady = scale.wait_ready_timeout(3000);
    }
  }
  else
  {
    Serial.println("HX711 neni pripojene");
  }

  if (scaleReady)
  {
    // Sencor SBS 113L
    scale.set_scale(settings.scale);
    delay(1000);
    yield();

    // formula A. B. C
    // cte po jednom s yield() aby WDT neresetoval ESP
    float sum = 0;
    int validReadings = 0;
    for (int i = 0; i < 10; i++)
    {
      if (scale.wait_ready_timeout(1000))
      {
        sum += scale.get_units(1);
        validReadings++;
      }
      yield();
    }

    float average = (validReadings > 0) ? (sum / validReadings) : 0;
    if (average < 0)
    {
      average = average * -1;
    }

    float value = average + settings.offset;

    // // formula E, F
    // float value = scale.get_units(10) + settings.offset;
    // if (value < 0)
    // {
    //   value = value * -1;
    // }

    Serial.println("average:\t" + String(value) + " kg");

    int connAttempts = 0;
    Serial.println("\r\nTry connecting to: " + String(settings.wifiSSID));

    WiFi.begin(settings.wifiSSID, settings.wifiPassword);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      if (connAttempts > 30)
      {
        Serial.println("Error - couldn't connect to WIFI");
        break;
      }

      connAttempts++;
    }

    Blynk.config(settings.blynkAuth);
    Blynk.connect(2000);

    if (Blynk.connected())
    {
      Serial.println("WiFI connected OK");
    }
    else
    {
      Serial.println("No WIFI !");
    }

    // wait for blynk sync
    delay(1000);

    Blynk.virtualWrite(V2, WiFi.RSSI());
    Blynk.virtualWrite(V4, settings.version);
    Blynk.virtualWrite(V5, isEnabled ? (secondChance ? "OK 2 pokusy" : "OK") : "Váha je vypnuta.");

    if (isEnabled)
    {
      Serial.println("ENABLED");
      Blynk.virtualWrite(V1, value);

      float previousValue = 0.00f;
      EEPROM.get(0, previousValue);
      EEPROM.put(0, value);
      EEPROM.commit();

      // validace EEPROM - pri prvnim spusteni nebo poskozenych datech
      bool validPrevious = !isnan(previousValue) && !isinf(previousValue) && previousValue > 0 && previousValue < 200;

      if (validPrevious)
      {
        float difference = value - previousValue;
        Blynk.virtualWrite(V6, difference);

        // detekce rojeni - pokles vahy o vice nez 1 kg
        if (difference < -1.0)
        {
          Blynk.logEvent("swarm_detected", String("Pokles váhy o ") + String(difference, 1) + " kg");
        }
      }
      else
      {
        Blynk.virtualWrite(V6, 0);
      }
    } else {
      Serial.println("DISABLED");
    }

    if (Blynk.connected())
    {
      Serial.println("Sent OK");
    }
  }
  else
  {
    Serial.println("HX711 doesn't work");

    // Pouzit lehci WiFi.begin + Blynk.config misto tezkeho Blynk.begin
    WiFi.begin(settings.wifiSSID, settings.wifiPassword);
    int connAttempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      if (connAttempts > 30) break;
      connAttempts++;
    }
    Blynk.config(settings.blynkAuth);
    Blynk.connect(2000);

    Serial.println("Connect blynk");

    int rssi = WiFi.RSSI();
    Blynk.virtualWrite(V2, rssi);
    Blynk.virtualWrite(V4, settings.version);

    if (secondChance)
    {
      Blynk.virtualWrite(V5, "HX711 nefunguje. 2. pokus");
    }
    else
    {
      Blynk.virtualWrite(V5, "HX711 nefunguje.");
    }

    Serial.println("Blynk send");
  }

  scale.power_down();
  delay(300);

  if (WiFi.isConnected())
  {
    Blynk.disconnect();
    WiFi.disconnect(true);

    Serial.println("Disconnected OK");
  }
  else
  {
    Serial.println("Already disconnected");
  }

  EEPROM.end();

  Serial.println("Go to sleep for " + String(deep_sleep_interval) + " seconds.");
  system_deep_sleep_instant(deep_sleep_interval * 1000000UL);
}

void loop()
{
}