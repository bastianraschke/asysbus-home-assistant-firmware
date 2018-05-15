# Firmware for Asysbus for Home Assistant nodes

The firmwares are intended to run on an Arduino Pro Mini (8 MHz) for low power consumption and be connected to a MCP2515 CAN bus module with a 8 MHz crystal.

<img alt="A generic Asysbus node" src="https://github.com/bastianraschke/asysbus-home-assistant-firmware/blob/master/projectcover.jpg" width="650">

## asbserialbridge

The `asbserialbridge` firmware is intented to act as the gateway between the CAN bus and the device running Home Assistant (for example a Raspberry Pi). The Home Assistant components connects to the serial port which will be provided by the node running `asbserialbridge`. The serial bidge node has the node id `0x0001` by default, which shouldn't be necessary to change.

## asbswitchnode

The `asbswitchnode` firmware is intented to provide a node that is a physical switch for Home Assistant. The switch node must have a network unique node id (for example `0x07D0`) which need to be set in the sketch as well in the corresponding Home Assistant switch configuration. The firmware was designed for hardware that provides a vibration motor on port `D4` and a LED on port `D5` for press indication. The button itself is connected to port `D3`. 

## asblightnode

The `asblightnode` firmware is intented to provide a node that powers a RGB(W) light strip. It also must have a network unique node id (for example `0x03E8`) which need to be set in the sketch as well in the corresponding Home Assistant light configuration. According to the strip type, the `LED_TYPE` in the node configuration must be set to `RGB` or `RGBW`. The LED pin configuration is also customizable like the offsets for each color channel (to compensate potential LED strip color inconsistencies). A crossfade effect is enabled by default to provide a smooth change animation, but it can be disabled of course.
