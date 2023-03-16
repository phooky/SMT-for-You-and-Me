# SMT For You and Me: VU display, Version 1

## Overview

This is a simple board with sixteen LEDs arranged in a semicircle,
that can be used as a VU meter or as a random attractive display.
It was designed for the SMT For You and Me class to introduce students
to surface mount soldering. It is based around an ATMega48 in a TQFP32
package (chosen primarily to help students practice drag soldering).

## BOM (Parts List)

BOM stands for Bill Of Materials. The BOM is a list of all the parts needed
to assemble a project. The "Reference" column of the table shows where the
part is installed on the board-- just find the same marking on your PCB!

You'll need 16 LEDs in all to fill the board, but you can select any color
combination you wish. Virtually and 0805-footprint LED will do.

Keep in mind that all of these parts are small, cheap, and easy to lose!
With the exception of the ATMega48 microcontroller, you're better off buying
extras of all the components listed. Many are actually cheaper to buy in larger
quantities-- not just per comoponent, but in total.

|Quantity |Manufacturer Part Number|Description                     |Reference|
|-----|------------------------|--------------------------------|------------------|
|1    |ATMEGA48PB-AU           |IC MCU 8BIT 4KB FLASH 32TQFP    |U1                |
|2    |CL21B104MACNNNC         |CAP CER 0.1UF 25V X7R 0805      |C1, C2            |
|1    |10118194-0001LF         |CONN RCPT USB2.0 MICRO B SMD R/A|J2                |
|1    |CL21A475KPFNNNG         |CAP CER 4.7UF 10V X5R 0805      |C4                |
|N    |LTST-C171KRKT           |LED RED CLEAR SMD               |D?                |
|N    |LTST-C171KGKT           |LED GREEN CLEAR SMD             |D?                |
|N    |LTST-C171KSKT           |LED YELLOW CLEAR CHIP SMD       |D?                |
|16   |CR0805-FX-3300ELF       |RES SMD 330 OHM 1% 1/8W 0805    |R2-17             |
|1    |CR0805-JW-472ELF        |RES SMD 4.7K OHM 5% 1/8W 0805   |R1                |
|1    |SMD4U+ME                |SMD For You and Me VU PCB       |                  |

## Assembly

Be careful to observe the correct polarity when installing the LEDs-- the cathode (negative)
end should be pointing towards the center of the board. There's a small flag with the number "1"
inside pointing to the first pin on the microcontroller; you'll find a small circle in the
corner of the chip indicating the corner that pin 1 is on. If the markings on the microcontroller
appear "upside down", you've got it right.

## Programming

The code is in the "firmware" directory and includes a Makefile for building and installing
the firmware. You may need to make some modifications to handle your setup; for instance 
change the AVRDUDE_PROGRAMMER variable to reflect the programmer you're using. There is a 
six-pin AVR ISP programming header in the lower left corner of the board. If you don't
have an AVR programmer handy, there are guides for wiring up most Arduinos to serve as a 
makeshift in-system programmer available online.

### To build and install the software:

* Hook up the ISP programmer.
* Build the firmware with "make".
* Set the fuses with "make fuses". (Fuses are an old-fashioned name for chip configuration
bits; they only need to be set once.)
* Upload the firmware with "make program".

## Using the VU meter

This section is incomplete.
