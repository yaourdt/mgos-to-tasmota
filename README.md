# mg2x

A minimal firmware for OTA (over the air) flashing various target firmwares
starting from Mongoose OS.

## Overview

Mg2x is an intermediate firmware that can be used to install [Tasmota](https://github.com/arendst/Tasmota)
or [Home Accessory Architect](https://github.com/RavenSystem/haa) on various
Shelly models. For Tasmota, it will install the same version as [Tuya Convert](https://github.com/ct-Open-Source/tuya-convert/),
and you can continue from there to your favourite target release.

## Install

**Warning:** _This application should generally be safe to use for all supported
devices. Still, overwriting a device's boot loader via OTA update is a risky
operation. If something unexpected fails, your device may be bricked, unless you
know how to flash a new firmware over a wired connection._

**Warning:** _As of now, once you convert to Tasmota, there is no way back via
an OTA update to Mongoose OS! You'll need a wired connection to get your device
back to stock firmware._

Before flashing this firmware, connect your device to a WIFI network with
internet access. From your browser, open the update URL for your device from the
table below. Replace `shellyip` with the IP address of your Shelly 1. The device
will restart one or two times and attempt to download Tasmota. If this download
succeeds, the device will restart again, and you will see a new WIFI network
labeled _tasmota-????_. This process should take no longer than 4 - 5 minutes,
depending on your network connection.

If you replace `mg2tasmota` by `mg2haa` in the update URLs, your device will
install the Home Accessory Architect firmware instead of Tasmota.

There is a [video tutorial](https://youtu.be/_oRr8FZyyQ0) on how to flash this
firmware. Thank you, [digiblur](https://github.com/digiblur)!

If the download fails, or your internet connection is disrupted, simply turn the
device off and on again, the intermediate firmware will retry until it succeeds.

In the unlikely event that the WIFI credentials are wrong, the device will try
to connect to a backup WIFI with SSID _mgos-recover_ and password _RJoPuKC3u5_,
which you can use for recovery.

Device | Update URL | Tasmota Template
--- | --- | ---
Shelly 1        | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly1.zip`       | `{"NAME":"Shelly 1","GPIO":[0,0,0,0,21,82,0,0,0,0,0,0,0],"FLAG":0,"BASE":46}`
Shelly 1PM      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly1PM.zip`     | `{"NAME":"Shelly 1PM","GPIO":[56,0,0,0,82,134,0,0,0,0,0,21,0],"FLAG":2,"BASE":18}`
Shelly Plug S   | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyPlugS.zip`   | `{"NAME":"Shelly Plug S","GPIO":[57,255,56,255,0,134,0,0,131,17,132,21,0],"FLAG":2,"BASE":45}`
Shelly 2        | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly2.zip`       | `{"NAME":"Shelly 2","GPIO":[0,135,0,136,21,22,0,0,9,0,10,137,0],"FLAG":0,"BASE":47}`
Shelly 2.5      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly25.zip`      | `{"NAME":"Shelly 2.5","GPIO":[56,0,17,0,21,83,0,0,6,82,5,22,156],"FLAG":2,"BASE":18}`
Shelly RGBW2    | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyRGBW2.zip`   | `{"NAME":"Shelly RGBW2","GPIO":[0,0,52,0,40,255,0,0,37,17,39,38,0],"FLAG":0,"BASE":18}`
Shelly Dimmer 1 | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDimmer1.zip` | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly Dimmer 2 | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDimmer2.zip` | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly EM       | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyEM.zip`      | `{"NAME":"Shelly EM","GPIO":[0,0,0,0,0,0,0,0,6,156,5,21,0],"FLAG":15,"BASE":18}`
Shelly Bulb     | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyBulb.zip`    | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly Vintage  | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyVintage.zip` | **not yet available, only flash if you a perfectly certain about what you are doing**

For your convenience, the table above also lists the matching Tasmota device
templates from templates.blakadder.com which you can use to configure Tasmota
after installation.

## Build the firmware yourself

You can compile a binary version of this firmware using [mos tools](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md#1-download-and-install-mos-tool). Once installed, clone this repository and run
`mos build --build-var MODEL=Shelly1 --build-var TARGETFW=tasmota --platform esp8266`
to create a binary for e.g. a Shelly1 switch located in `build/fw.zip`.

## Acknowledgments
Thanks to [rojer](https://github.com/rojer) for helping me with the debugging of
the initial code.

This firmware is build using a fork of [Mongoose OS docker action](https://github.com/dea82/mongoose-os-action)
which can be found [here](https://github.com/yaourdt/mongoose-os-action).
