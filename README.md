# dds60-siggen
Signal generator with DDS60 module from AmQRP and Arduino Micro, created on an arduino shield to diagnose issues with a QRP kit.  Some features were added as a fun coding exercise, mostly written during my morning coffee, so offered without warranty.

This will likely never get updated again as I mostly wrote this to get a functioning signal generator out of a few parts I had on hand from a project planned long ago.  Code is provided as an example, building the circuits to interface with parts is left as an exercise for the reader.

## Modes
* VFO: Fixed frequency adjustable by dial, limited to bands.
* Sig-Gen: Fixed frquency, adjustable by dial, band button increments frequency by MHz instead.
* Sweep: Sweep over a small range of frequencies at a constant rate.
* Memory: Recall frequency from memories stored in EEPROM
* Memory Store frequencies to memory from serial



# Directories
## dds60-siggen
Main piece of code for this project.

## dds60-test
Test project to facilitate checking hardware functionality

## libraries/dds60
Arduino library for dds60 module

## Hardware
Misc files for hardware design, includes example panel layout.

# references / sources / inspiriation
* [W6AKB Sweeper](http://hamradioprojects.com/authors/w6akb/+sweeper/) - using different display, don't need SWR meter.
* [AmQRP DDS60](https://midnightdesignsolutions.com/dds60/)
* [AA0ZZ Si570 daughtercard](http://www.cbjohn.com/aa0zz/PPLL/Article.pdf) - similar to DDS60, wider range, interface over I2c.  Planning to add as alternate daughter card.
Arduino and git: https://arduino.stackexchange.com/questions/348/how-to-organize-my-arduino-projects-structure-for-easy-source-control

# alternative devices that do more and do better - buy one of these instead
Of the following, probably closest to what I was intendeding to build ~8 years ago was the AA-30.Zero.
* [RigExpert AA-30.Zero](https://rigexpert.com/products/kits-analyzers/aa-30-zero/)
* [SARK-110](https://www.seeedstudio.com/SARK-110-ULM-Antenna-Analyzer-p-4126.html)
## VNAs
* [NanoVNA](https://www.tindie.com/products/hcxqsgroup/nanovna-v2/)
* [Analog Discovery 2](https://www.sparkfun.com/products/13929)
* [DG8SAQ VNWA3](https://www.sdr-kits.net/introducing-DG8SAQ-VNWA3)