#ifndef dss60_h
#define dds60_h

#include "Arduino.h"
class DDS60
{
    public:
        DDS60(int load, int clock, int data);
        void tune(unsigned long freq);
        void off();
    private:
        byte _pload;
        byte _pclock;
        byte _pdata;
};

#endif
