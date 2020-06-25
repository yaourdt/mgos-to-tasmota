# A minimal firmware for OTA (over the air) flashing Tasmota starting from Mongoose OS

## Overview

This is a first working draft for an intermediate firmware that can be used to
install [Tasmota](https://github.com/arendst/Tasmota) on a Shelly 1 switch. It
will install the same version of Tasmota as [Tuya Convert](https://github.com/ct-Open-Source/tuya-convert/),
and you can continue from there to your favourite target release.

## Build

There is no binary version of this firmware as of now, but you can compile it
yourself using [mos tools](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md#1-download-and-install-mos-tool).
Once installed, clone this repository and run `mos build --build-var MODEL=Shelly1 --platform esp8266`
to create a binary.

## Install

**Warning:** _This application is still at an early stage. If something fails,
your device may be bricked, if you don't know how to flash a new firmware over a
wired connection. Proceed with caution!_

**Warning:** _As of now, once you convert to Tasmota, there is no way back via
an OTA update to Mongoose OS! You'll need a wired connection to get your device
back to stock firmware._

Before flashing this firmware, connect your device to a WIFI network with
internet access. Apply `build/fw.zip` from the build step above as you would for
any other OTA update. The device will restart one or two times and attempt to
download Tasmota. If this download succeeds, the device will restart again and
you will see a new WIFI network labeled _tasmota-????_. This process should
take no longer than 4 - 5 minutes, depending on your network connection.

If the download fails, or your internet connection is disrupted, simply turn the
device off and on again, the intermediate firmware will retry until it succeeds.

In the unlikely event that the WIFI credentials are wrong, the device will try
to connect to a backup WIFI with SSID _mgos-recover_ and password _RJoPuKC3u5_,
which you can use for recovery.
