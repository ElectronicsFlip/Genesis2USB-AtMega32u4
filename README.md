# Genesis2USB

An Atmega32u4 based USB Adapter for 3- and 6-Button Sega Genesis / Mega Drive controllers. Physical prototypes are built using the Sparkfun Micro Pro USB-C based boards.

## License

MIT License. See LICENSE for details.

## How To Build

For the build that is used here, the following boards from Amazon were used: [Arduino Pro Micro generic](https://www.amazon.com/dp/B09J2FLLD7)

These are based on the Sparkfun Pro Micro but with a USB-C port instead of MicroUSB. The USB-C port was preferred for our builds as we have tons of these cables lying around and it was easier to shape our 3D printed housings (not part of this repository).

### Wiring

![Genesis Controller Pinout](/docs/pinout-controller-side.gif)

**Pinout**

* Pin 1: Up
* Pin 2: Down
* Pin 3: Left
* Pin 4: Right
* Pin 5: +5V
* Pin 6: TL
* Pin 7: TH
* Pin 8: GND
* Pin 9: TR

### Wiring to the Arduino

The boards linked above are by default set to 5v. This is what we want as the Genesis controller uses +5v for operation.

![Genesis wiring to Arduino](/docs/pro-micro-wiring-genesis.gif)

**Wiring**

*Genesis Port -> Arduino Pin*

* Pin 7 -> Pin 2
* Pin 1 -> Pin 3
* Pin 2 -> Pin 4
* Pin 3 -> Pin 5
* Pin 4 -> Pin 6
* Pin 6 -> Pin 7
* Pin 9 -> Pin 8

* Pin 8 -> GND
* Pin 5 -> VCC

*OPTIONAL: RXI pin is used for LED+ as it's right next to a ground pin for a pin header that we use here to indicate power if you build this into a case. This is entirely optional.*
