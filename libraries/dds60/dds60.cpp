/*
DDS60.cpp
Library for controlling DDS60 board from Midnight Solutions
      based on code from http://hamradioprojects.com/authors/w6akb/+sweeper/
*/
#include "Arduino.h"
#include "dds60.h"


DDS60::DDS60(int load, int clock, int data){
    _pload = (byte) load;
    _pclock = (byte) clock;
    _pdata = (byte) data;
    // set pin modes
    pinMode(_pclock, OUTPUT); // set pins to output mode
    pinMode(_pdata, OUTPUT);
    pinMode(_pload, OUTPUT);
}

void DDS60::tune(unsigned long _freq)  // set DDS freq in hz
{
  unsigned long clock = 180000000L;  // 180 mhz, may need calibration
  int64_t scale = 0x100000000LL;
  unsigned long tune;                // tune word is delta phase per clock
  int ctrl, i;                       // control bits and phase angle

  if (_freq == 0)
  {
    tune = 0;
    ctrl = 4;  // enable power down
  }
  else
  {
    tune = (_freq * scale) / clock;
    ctrl = 1;  // enable clock multiplier

    digitalWrite(_pload, LOW);  // reset load pin
    digitalWrite(_pclock, LOW); // data xfer on low to high so start low

    for (i = 32; i > 0; --i)  // send 32 bit tune word to DDS shift register
    {
      digitalWrite(_pdata, tune & 1); // present the data bit
      digitalWrite(_pclock, HIGH);  // clock in the data bit
      digitalWrite(_pclock, LOW);   // reset clock
      tune >>= 1;                    // go to next bit
    }
  }
  for (i = 8; i > 0; --i)  // send control byte to DDS
  {
    digitalWrite(_pdata, ctrl & 1);
    digitalWrite(_pclock, HIGH);
    digitalWrite(_pclock, LOW);
    ctrl >>= 1;
  }
  // DDS load data into operating register
  digitalWrite(_pload, HIGH);  // data loads on low to high
  digitalWrite(_pload, LOW);   // reset load pin
}

void DDS60::off()  // shut down DDS
{
  tune(0L);
}