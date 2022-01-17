# frisquet-RFLink

RFLink tools for Frisquet Eco Radio System boiler / Utilitaires RFLink pour chaudière Frisquet Eco Radio System
Flash RFLink hardware to be able to decode or encode Frisquet Eco Radio System protocol. This will enable remote control of a Frisquet boilers using Eco Radio System.

Tested successfully on models:
- Hydromotrix

# Usage

Using Arduino IDE, load inside RFLink 433.42 Mhz hardware :
- frisquet-ERS-decode to decode ERS protocol and retrieve the ID of your boiler
- frisquet-ERS-command to command your boiler using ERS protocol (don't forget to set the ID of your boiler)

Setup tutorial (in French) : https://github.com/ChristopheHD/frisquet-arduino/wiki/Installation

# Support

You can have help :
- On this github
- On this thread (French only) : http://easydomoticz.com/forum/viewtopic.php?f=17&t=1486
- On this community (mention @ChristopheHD) : https://community.jeedom.com/
