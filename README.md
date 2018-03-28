# Opentherm IO library for Arduino

Now its time to enhance your central heating system with Arduino. Using this library and special hardware interface you will be able to create your own thermostat to control Opentherm devices (boiler) or build a man-in-the-middle gateway to capture or alter communication running between your thermostat and boiler. For example to monitor and oversee your current heating status over your website or mobile phone.

#### What is it good for? ####

- Creating your own thermostat that can be controlled via website or mobile phone
- Monitoring communication between thermostat and boiler and overseeing real-time graphs via website or mobile phone
- Combining your boiler with secondary heating source (fireplace, solar collectors) via custom built regulator

## Hardware requirements ##

In order to connect your Arduino board to Opentherm device, you need to create a special hardware **interface** to convert voltage and current levels for Arudino to be able to handle. Voltage levels on Opentherm bus are up to 24V which would easily burn up your Arduino if connected to wires directly.

#### Interface schematics ####

Based on [otgw.tclcode.com](http://otgw.tclcode.com) project.

Interface consists of three parts:

- **master interface**: to send and receive data from your master device (thermostat) and to supply it with power
- **slave interface**: to send and receive data from your slave device (boiler)
- **power supply**: to supply 24V to thermostat via master interface and 5V to Arduino board

![interface schmetics](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/eagle-opentherm-schema.png)

#### Board example ####

![interface board](https://raw.githubusercontent.com/jpraus/arduino-opentherm/master/doc/eagle-opentherm.png)

_Power supply from interface board is needed only if you wish to connect to mater device (thermostat). Otherwise you won't need both **master interface** and **power supply**. You can power your Arduino board and slave interface with standard 5V power supply._

## Wiring up with Arduino board ##

Wire up interface board and Arduino as following:

- **MASTER-OUT** to pin **D4**
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

## Behind the scene ##

Library uses following Arduino resources:

- **Timer2** - to properly read and write encoded data bites to bus
- **Digital pin rising signal interrupt** - input is monitored for incomming data packets in order not to waste precious computing time on CPU. Only digital pins D2 and D3 are capable of this functionality on Arduino Uno and Arduino Nano boards.

Note that you won't be able to use libraries that are using Timer2 or rising signal interrupt as well (for example Servo library).

Tested with Arduino Nano and Arduino Uno boards.

