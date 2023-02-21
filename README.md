> ⚠ Unfortunately, due to missing time, I will no longer be able to maintain this repository or the downloads server. If you are interested, I encourage you to fork the project and continue development. Or, if you are willing (and able) to take over this repo, drop me a message.

# Mongoose to Tasmota, Home Accessory Architect or ESPurna

A minimal firmware for OTA (over the air) flashing various target firmwares
starting from Mongoose OS.

## Overview

This is an intermediate firmware that can be used to install [Tasmota](https://github.com/arendst/Tasmota),
[Home Accessory Architect](https://github.com/RavenSystem/haa), or [ESPurna](https://github.com/xoseperez/espurna)
on various Shelly models. It will install the latest stable release of the
target firmware.

## Install

⚠ This application should generally be safe to use for all supported
devices. Still, overwriting a device's boot loader via OTA update is a risky
operation. If something unexpected fails, your device may be bricked, unless you
know how to flash a new firmware over a wired connection.

ℹ You can go back to Mongoose OS via OTA as well, using [this firmware](https://github.com/yaourdt/tasmota-to-mgos),
but be aware that if something fails, your device may be bricked, if you don't
know how to flash a new firmware over a wired connection.

> Instead of flashing the device manually (see below), you may try the new automagic [flashing script](tools/flash-shelly.py). The script requires Python 3 to be installed on your computer. After downloading the script, run `python3 flash-shelly.py -a` from the download location. All Shelly devices with stock firmware, which are connected to the same WIFI network, will be found. For each device that is found, you will be asked if you like to convert it.
>
> Use `python3 flash-shelly.py -a -t haa` or `python3 flash-shelly.py -a -t espurna` to install alternate targets.
>
> ⚠ The script is still in an early testing stage, and I would appreciate your feedback. If you want to give feedback, please open an issue.

Before flashing this firmware, connect your device to a WIFI network with
internet access. From your browser, open the update URL for your device from the
table below. Replace `shellyip` with the IP address of your Shelly. The device
will restart one or two times and attempt to download the traget firmware. If
this download succeeds, the device will restart again, and you will see a new
WIFI network labeled _tasmota-????_, _HAA-????_, or _ESPURNA-????_.
This process should take no longer than 4 - 5 minutes, depending on your network
connection.

If you replace `mg2tasmota` by `mg2haa` in the update URLs, your device will
install the Home Accessory Architect firmware instead of Tasmota. For
`mg2espurna` instead of `mg2tasmota`, ESPurna Firmware will be installed.

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
Shelly Plug S   | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyPlugS.zip`   | `{"NAME":"Shelly Plug S","GPIO":[56,255,158,255,255,134,0,0,131,17,132,21,255],"FLAG":2,"BASE":45}`
Shelly 2        | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly2.zip`       | `{"NAME":"Shelly 2","GPIO":[0,135,0,136,21,22,0,0,9,0,10,137,0],"FLAG":0,"BASE":47}`
Shelly 2.5      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly25.zip`      | `{"NAME":"Shelly 2.5","GPIO":[56,0,17,0,21,83,0,0,6,82,5,22,156],"FLAG":2,"BASE":18}`
Shelly RGBW2 (needs to be put in a color mode and have latest firmware) | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyRGBW2.zip`   | `{"NAME":"Shelly RGBW2","GPIO":[0,0,52,0,40,255,0,0,37,17,39,38,0],"FLAG":0,"BASE":18}`
Shelly Dimmer 1 | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDimmer1.zip` | `{"NAME":"Shelly Dimmer 1","GPIO":[0,3200,0,3232,5568,5600,0,0,192,0,193,288,0,4736],"FLAG":0,"BASE":18}`
Shelly Dimmer 2 | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDimmer2.zip` | `{"NAME":"Shelly Dimmer 2","GPIO":[0,3200,0,3232,5568,5600,0,0,193,0,192,0,320,4736],"FLAG":0,"BASE":18}`
Shelly EM       | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyEM.zip`      | `{"NAME":"Shelly EM","GPIO":[0,0,0,0,0,0,0,0,6,156,5,21,0],"FLAG":15,"BASE":18}`
Shelly Bulb     | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyBulb.zip`    | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly Vintage  | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyVintage.zip` | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly Plug US  | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyPlugUS.zip`  | `{"NAME":"Shelly Plug US","GPIO":[52,0,57,0,21,134,0,0,131,17,132,157,0],"FLAG":0,"BASE":45}`
Shelly Duo      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDuo.zip`     | `{"NAME":"Shelly Duo","GPIO":[0,0,0,0,38,37,0,0,0,0,0,0,0],"FLAG":0,"BASE":18}`
Shelly H&T      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyHT.zip`      | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly i3       | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyI3.zip`      | `{"NAME":"Shelly i3","GPIO":[0,0,0,0,0,0,0,0,83,84,82,0,0],"FLAG":2,"BASE":18}`
Shelly 1L       | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-Shelly1L.zip`      | `{"NAME":"Shelly 1L","GPIO":[320,0,0,0,192,224,0,0,0,0,193,0,0,4736],"FLAG":0,"BASE":18}`
Shelly Plug 2   | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyPlug2.zip`   | **not yet available, only flash if you a perfectly certain about what you are doing**
Shelly Uni      | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyUni.zip`     | `{"NAME":"Shelly Uni","GPIO":[320,0,0,0,225,0,0,0,192,193,0,224,0,4864],"FLAG":0,"BASE":18}`
Shelly Duo RGBW | `http://shellyip/ota?url=http://dl.dasker.eu/firmware/mg2tasmota-ShellyDuoRGBW.zip` | **not yet available, only flash if you a perfectly certain about what you are doing**

For your convenience, the table above also lists the matching Tasmota device
templates from [templates.blakadder.com](https://templates.blakadder.com) which
you can use to configure Tasmota after installation.

Home Accessory Architect will boot in setup mode, which is explained in the
[HAA Wiki](https://github.com/RavenSystem/esp-homekit-devices/wiki/Setup-Mode).

For ESPurna, the device will boot into the ESPurna base image. You can then
either switch to a devices specific version, if your device is supported, or
[start configuration](https://github.com/xoseperez/espurna/wiki/Configuration)
from the base image.

If device fails to convert, especially the RGBW2, please update to the latest
stock firmware and try again.

## My device is not listed!

If your device is not listed, please open a new issue so we can try to make it
work. Please mention which of the many [Shelly firmware files](https://api.shelly.cloud/files/firmware)
is the right one for your device.

## Build the firmware yourself

You can compile a binary version of this firmware using [mos tools](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md#1-download-and-install-mos-tool). Once installed, clone this repository and run
`mos build --build-var MODEL=Shelly1 --build-var TARGETFW=tasmota --platform esp8266`
to create a binary for e.g. a Shelly1 switch located in `build/fw.zip`.

## Acknowledgments
Thanks to [rojer](https://github.com/rojer) for helping me with the debugging of
the initial code.

This firmware is build using a fork of [Mongoose OS docker action](https://github.com/dea82/mongoose-os-action)
which can be found [here](https://github.com/yaourdt/mongoose-os-action).

The flash-script is a modified version from [mongoose-os-apps/shelly-homekit](https://github.com/mongoose-os-apps/shelly-homekit),
originally written by [andyblac](https://github.com/andyblac) and [rojer](https://github.com/rojer).

## License

Copyright (C) 2020, Mark Dornbach

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
