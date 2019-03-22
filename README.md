# Opentherm IO library for Arduino

Have you ever wondered when and why is your boiler running and heating your home? Do you want to automate your heating system with Arduino? The OpenTherm IO library together with an Arduino OpenTherm shield is designed for you. It will allow you to monitor and control your OpenTherm device with Arduino.

This repo contains both Arduino library and hardware adapter.

![Arduino UNO OpenTherm shield](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/otshield.jpg)

#### What is it good for? ####

- **Boiler** - create your own Arduino-based boiler with an existing OpenTherm thermostat.
- **Thermostat** - use the shield to build your own Arduino-based thermostat to fully take over your boiler and home heating. Perfect for home automation. This application only requires an external 5V power supply.
- **Gateway/Monitor** - place an OpenTherm shield into the lines between the existing boiler and thermostat and create a monitor to watch when how the boiler is heating your home.
- **Man-in-the-middle** -  instead of just listening to the communication like in gateway mode you can also alter the communication that is happening between the boiler and thermostat and adjusts the behavior as you want. Perfect for creating your own heating regulator.
- ... and many more

## Arduino shiled ##

In order to connect your Arduino board to Opentherm device, you need to create a special hardware **interface** to convert voltage and current levels for Arudino to be able to handle. Voltage on Opentherm bus rises as high as 24V which would easily burn up your Arduino if connected to wires directly.

#### OpenTherm Arduino shield ####

The easiest way to get the hardware is to purchase a Arduino shield kit from my [Tindie store](https://www.tindie.com/products/jiripraus/opentherm-arduino-shield-diy-kit). It's open source so you can build it on your own. You will find all the details below.

#### Schematics ####

Based on [otgw.tclcode.com](http://otgw.tclcode.com) project.

Interface consists of three parts that can be used separately:

- **master interface**: to send and receive data from your master device (thermostat) and to supply it with power
- **slave interface**: to send and receive data from your slave device (boiler)
- **power supply**: to supply 24V to thermostat via master interface and 5V to an Arduino board

![interface schmetics](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/shield-schema-doc.png)

_Power supply from interface board is needed only if you wish to connect to master device (thermostat). Otherwise you won't need both **master interface** and **power supply**. You can power your Arduino board and slave interface with standard 5V power supply._

## How it is wired up with Arduino ##

Wire up interface board and Arduino as following:

- **MASTER-OUT** of interface to pin **D4** of Arduino
- **MASTER-IN** to pin **D2** (requires pin changed interrupt)
- **SLAVE-OUT** to pin **D5**
- **SLAVE-IN** to pin **D3** (requires pin changed interrupt)
- **VCC** to **5V**
- **GND** to **GND**

## Working with library ##

Library contains 3 examples to test out your setup. These examples are configured to use pins defined above, but library will allow you to change pins to your custom ones.

- **master.ino** - Arduino acts as master device (thermostat)
- **slave.ino** - Arduino acts as slave device (boiler)
- **gateway.ino** - Arduino acts as gateway between master and slave devices

These examles should give you enough information to build your own code using Opentherm library. Check out header file of library source code to see methods documentation.

## Behind the scenes ##

Library uses following Arduino resources:

- **Timer2** - to properly read and write encoded data bites to bus
- **Pin changed interrupt** - bus is monitored for incomming data packets in order to save precious computing time on CPU. Only digital pins D2 and D3 are capable of this functionality on Arduino Uno and Arduino Nano boards.

Note that you won't be able to use libraries that are using Timer2 or pin changed interrupt together with this library (for example Servo library).

Tested with Arduino Nano and Arduino Uno boards.
