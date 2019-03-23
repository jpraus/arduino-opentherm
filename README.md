# Opentherm IO library for Arduino

Have you ever wondered when and why is your boiler running and heating your home? Do you want to automate your heating system with Arduino? The OpenTherm IO library together with an Arduino OpenTherm shield is designed for you. It will allow you to monitor and control your OpenTherm device with Arduino.

This repo contains both Arduino library and hardware adapter.

![Arduino UNO OpenTherm shield](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/otshield.jpg)

#### What is it good for? ####

- Create your own Arduino-based **boiler** with an existing OpenTherm thermostat. Most unlikely anybody would do that but it is possible.
- Use the shield to build your own Arduino-based **thermostat** to fully take over your boiler and home heating. Perfect for home automation. This application only requires an external 5V power supply.
- Place an OpenTherm shield into the lines between the existing boiler and thermostat and create a **monitor** to watch when and how the boiler is heating your home. You can even intercept the communication to for example wirelessly control the heating.
- OpenTherm allows having a **man-in-the-middle** device that communicates with both boiler and thermostat. This is is how I used the shield to create an [OpenTherm regulator](https://hackaday.io/project/162819-opentherm-regulator).

## Arduino shield ##

In order to connect your Arduino board to Opentherm device, you need to create a special hardware **interface** to convert voltage and current levels for Arudino to be able to handle. Voltage on Opentherm bus rises as high as 24V which would easily burn up your Arduino if connected to wires directly.

#### Buy or DIY ####

The easiest way to get the hardware is to purchase a Arduino shield kit from my [Tindie store](https://www.tindie.com/products/jiripraus/opentherm-arduino-shield-diy-kit). On the other hand the shield is Open Source so you can manufacture and buy all the components on your own!

#### Features ####

- OpenTherm slave interface to communicate with a boiler
- OpenTherm master interface to communicate with a thermostat
- can implement master, slave and gateway modes
- 5V 3A built-in power supply
- Arduino UNO compatible shield

![interface schmetics](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/shield-schema-doc.png)

Based on [otgw.tclcode.com](http://otgw.tclcode.com) project.

#### Fabrication files ####

Are you able to manufacture the PCB yourself? There are [Gerber files](gerber/) included in the repository. Or you can get the DIY KIT from my [Tindie store](https://www.tindie.com/products/jiripraus/opentherm-arduino-shield-diy-kit).

#### Parts list ####

- C1 100u/35V electrolytic capacitor
- C2 100u electrolytic capacitor
- D1 SS24
- D2 zener diode 4V7 SOD80C
- D3 zener diode 13V SOD80C
- D4 zener diode 4V3 SOD80C
- D5,D6,D7,D8 - 4148
- L1 150uH CDRH104RT
- LED1 blue 0805 LED
- LED2 green 0805 LED
- LED3 red 0805 LED
- Q1,Q2,Q3,Q5 BC858B SOT-23
- Q4 BC848C SOT-23
- R1,R4 330 0805
- R2 220 0805
- R3,R7 100 0805
- R5,R11 10k 0805
- R6,R8 33k 0805
- R9 39 0805
- R10 4k7 0805
- R12,R13,R14 1k 0805
- U1 LM2596SX-5.0
- U2,U3 PC817B 

#### How it is wired up with Arduino ####

- **MASTER-OUT** of interface to pin **D4** of Arduino
- **MASTER-IN** to pin **D2** (requires pin changed interrupt)
- **SLAVE-OUT** to pin **D5**
- **SLAVE-IN** to pin **D3** (requires pin changed interrupt)
- **+5V** to **5V**
- **GND** to **GND**

## Working with library ##

Library contains 3 examples to test out your setup. These examples are configured to use pins defined above, but library will allow you to change pins to your custom ones.

- **master.ino** - Arduino acts as master device (thermostat)
- **slave.ino** - Arduino acts as slave device (boiler)
- **gateway.ino** - Arduino acts as gateway between master and slave devices

These examles should give you enough information to build your own code using Opentherm library. Check out header file of library source code to see methods documentation.

#### Behind the scenes ####

Library uses following Arduino resources:

- **Timer2** - to properly read and write encoded data bites to bus
- **Pin changed interrupt** - bus is monitored for incomming data packets in order to save precious computing time on CPU. Only digital pins D2 and D3 are capable of this functionality on Arduino Uno and Arduino Nano boards.

Note that you won't be able to use libraries that are using Timer2 or pin changed interrupt together with this library (for example Servo library).

Tested with Arduino Nano and Arduino Uno boards.

![Arduino UNO OpenTherm shield](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/otshield_with_thermostat.JPG)
