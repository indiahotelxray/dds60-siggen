#ifndef Morse_h
#define Morse_h

#include "Arduino.h"
class DDS60
{
    public:
        DSS60();
        void tune(int freq);
        void off();
    private:
        byte _pload;
        byte _pclock;
        byte _pdata;

};

#endif
