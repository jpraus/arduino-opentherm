# Opentherm IO library for Arduino

Now its time to enhance your central heating system with Arduino. Using this library and special hardware interface provided below you will be able to create your own thermostat to control Opentherm boiler or build a man-in-the-middle gateway to capture or alter communication running between your thermostat and boiler.

#### What is it good for? ####

- Creating your own central heating thermostat that can be controlled via website or mobile phone
- Monitoring communication between thermostat and boiler and overseeing real-time heating status
- Combining your boiler with secondary heating source (fireplace, solar collectors) via custom built regulator
- ... and many more

## Hardware requirements ##

In order to connect your Arduino board to Opentherm device, you need to create a special hardware **interface** to convert voltage and current levels for Arudino to be able to handle. Voltage on Opentherm bus rises as high as 24V which would easily burn up your Arduino if connected to wires directly.

#### Schematics ####

Based on [otgw.tclcode.com](http://otgw.tclcode.com) project.

Interface consists of three parts that can be used separately:

- **master interface**: to send and receive data from your master device (thermostat) and to supply it with power
- **slave interface**: to send and receive data from your slave device (boiler)
- **power supply**: to supply 24V to thermostat via master interface and 5V to an Arduino board

![interface schmetics](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/eagle-opentherm-schema.png)

#### Board example ####

![interface board](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/eagle-opentherm.png)

_Power supply from interface board is needed only if you wish to connect to master device (thermostat). Otherwise you won't need both **master interface** and **power supply**. You can power your Arduino board and slave interface with standard 5V power supply._

## Wiring up with Arduino board ##

Wire up interface board and Arduino as following:

- **MASTER-OUT** of interface to pin **D4** of Arduino
- **MASTER-IN** to pin **D2**
- **SLAVE-OUT** to pin **D5**
- **SLAVE-IN** to pin **D3**
- **VCC** to **5V**
- **GND** to **GND**

## Working with examples ##

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

## The result ##

I've created this library originaly for my own project. I've built a custom Arduino based regulator to control central heating combining gas boiler and water heating fireplace. Central heating is still controlled by thermostat placed in living room. Regulator is gateway between thermostat and boiler and serves several purposes:

- decides whether central heating should be supplied with hot water from gas boiler or fireplace
- controls pumps and mixing valve
- displays metrics and status on built-in display
- uploads real-time metrics to cloud to display them on website for later analysis

| ![regulator](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/build-open.jpg) | ![regulator](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/build-display.jpg) |
|--------|--------|