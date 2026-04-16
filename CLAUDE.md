# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Beehive weight monitoring system using ESP8266 (WEMOS D1 Mini). Reads weight from an HX711 load cell amplifier, sends data to Blynk 2.0 IoT platform, then enters deep sleep to conserve battery. The entire workflow runs in `setup()` — `loop()` is empty by design.

## Build & Upload

This is a PlatformIO project targeting `espressif8266` / `d1_mini` with the Arduino framework.

```bash
pio run                  # compile
pio run -t upload        # compile and upload to device
pio device monitor       # serial monitor (115200 baud)
```

## Required Setup

Before building, create `src/settings.cpp` (gitignored) with WiFi credentials, Blynk auth token, and per-device scale/offset calibration values. See README.md for the struct template.

## Architecture

- **src/main.cpp** — single-file firmware. Wake → init HX711 → read weight → connect WiFi → send to Blynk → deep sleep.
- **Blynk virtual pins**: V0 (enable/disable), V1 (weight), V2 (RSSI), V3 (terminal), V4 (version), V5 (status), V6 (weight diff), V7 (sleep interval display), V8 (sleep interval input).
- **OTA updates** via Blynk's `InternalPinOTA` mechanism.
- **EEPROM** stores previous weight reading for calculating change between cycles (swarm detection: weight drop > 1kg triggers notification).
- **Deep sleep interval** defaults to 285 seconds (~5 min), adjustable remotely via Blynk V8.

## Language

Code comments and Blynk status strings are in Czech. Maintain this convention.
