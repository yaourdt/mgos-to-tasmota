# A minimal firmware for ota flashing tasmota from mongoose os

## Overview

This is a first working draft for an intermediate firmware used to install
[tasmota](https://github.com/arendst/Tasmota) on a Shelly 1 switch.

There is no binary version of this firmware as of now, but you can compile it
yourself using [mos tools](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md#1-download-and-install-mos-tool).
One installed, clone this repository and run `mos build --build-var MODEL=Shelly1 --platform esp8266`
to create a binary.

**Warning:** _As of now, this application is not well-tested does not apply any
safeguards. If something fails, your device will likely be bricked if you don't
know how to flash a new firmware over a wired connection!_
