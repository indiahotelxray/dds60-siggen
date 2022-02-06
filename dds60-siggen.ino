
/**
    Arduino DDS60 signal generator / VFO
      based on code from http://hamradioprojects.com/authors/w6akb/+sweeper/

      Arduino + Protoshield
         +- DDS60 -> SMA
              P1-1 -> D5  confirm ... ?
              P1-2 -> D9
              P1-3 -> D8
         +- LCD display (16x2)
              Adafruit Display used I2C - SDA - D2; SCL - D3; Reset - D4
              rs, rw, enable, d4, d5, d6, d7)
         +- Rotary Encoder - not tested
              Up/down - freq change
              Press - increment step 10/50/100/500/1000 kHz
         +- Buttons - not tested
              BAND - change band
              MODE - change mode (sweep,vfo,memory,sig-gen)
         +- RCA plugs on A3, A4, A5 for measurements.

     TODOs:
        - Debounce buttons
*/

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>


// LCD 1602 pins
#define RS 0
//#define LCD_RW 
#define EN 1
#define LCD_D4 2
#define LCD_D5 3
#define LCD_D6 4
#define LCD_D7 5

// DDS60 pins - don't mess with these
#define DDSLOAD 8
#define DDSCLOCK 9
#define DDSDATA 10

//buttons
#define BAND 6
#define MODE 7

// use analog pins for these
#define ROT_UP A0  
#define ROT_DOWN A1  
#define ROT_PRESS A2 

// analog a3, a4, a5 used by RCA ports

// static values
static long freq = 14000000L;
static char mode = 'v';
static bool update = false;  // used to signal stopping a sweep/scan

void DDS_freq(unsigned long _freq)  // set DDS freq in hz
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

    digitalWrite(DDSLOAD, LOW);  // reset load pin
    digitalWrite(DDSCLOCK, LOW); // data xfer on low to high so start low

    for (i = 32; i > 0; --i)  // send 32 bit tune word to DDS shift register
    {
      digitalWrite(DDSDATA, tune & 1); // present the data bit
      digitalWrite(DDSCLOCK, HIGH);  // clock in the data bit
      digitalWrite(DDSCLOCK, LOW);   // reset clock
      tune >>= 1;                    // go to next bit
    }
  }
  for (i = 8; i > 0; --i)  // send control byte to DDS
  {
    digitalWrite(DDSDATA, ctrl & 1);
    digitalWrite(DDSCLOCK, HIGH);
    digitalWrite(DDSCLOCK, LOW);
    ctrl >>= 1;
  }
  // DDS load data into operating register
  digitalWrite(DDSLOAD, HIGH);  // data loads on low to high
  digitalWrite(DDSLOAD, LOW);   // reset load pin
}



void DDS_off()  // shut down DDS
{
  DDS_freq(0L);
}

// initialize LCD
LiquidCrystal display(RS, EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // initialize display
  display_init();

  // setup DDS
  DDS_init();

  // initialize buttons
  button_init();

  // set frequency
  update = true;
}



void display_init() {
  display.begin(16, 2);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  //delay(250); // Pause for 1 seconds
  display.print(F("kc2ihx dds"));
  delay(250);

  // Clear the buffer
  display.clear();

}

void button_init()
{
  pinMode(BAND, INPUT);
  pinMode(MODE, INPUT);
  pinMode(ROT_UP, INPUT);
  pinMode(ROT_DOWN, INPUT);
  pinMode(ROT_PRESS, INPUT);

  attachInterrupt(digitalPinToInterrupt(ROT_UP), incr, FALLING);
  attachInterrupt(digitalPinToInterrupt(ROT_DOWN), decr, FALLING);
  attachInterrupt(digitalPinToInterrupt(ROT_PRESS), ch_step, FALLING);
  attachInterrupt(digitalPinToInterrupt(BAND), ch_band, FALLING);
  attachInterrupt(digitalPinToInterrupt(MODE), ch_mode, FALLING);
}

void DDS_init()
{
  pinMode(DDSCLOCK, OUTPUT); // set pins to output mode
  pinMode(DDSDATA, OUTPUT);
  pinMode(DDSLOAD, OUTPUT);
}




// mode functions and variables
/**
   swp - sweep
   mem - memory not yet implemented
   vfo - simple case with setfreq limited by bands
   sig - free ranging vfo
*/




// sweeps constants
// sweep reach in khz
static uint8_t i_sweep_size = 0;
static const PROGMEM uint8_t n_sweep_size = 10;
static const PROGMEM int sweep_size[] = {
  1, 2, 5, 10, 20, 50, 100, 500, 1000, 5000
};
// sweep size in hz
static uint8_t i_sweep_step = 0;
static const PROGMEM uint8_t n_sweep_step = 7;
static const PROGMEM int sweep_step[] = {
  1, 5, 10, 50, 100, 500, 1000
};

void sweep() {
  /**
     loop from _start to _end once
  */
  int end_freq = freq + sweep_size[i_sweep_size] * 1000;
  int sweep_freq = freq;
  while ((sweep_freq >= end_freq) & not update) {
    setFreq(sweep_freq);
    sweep_freq += sweep_step[i_sweep_step];
  }
}




// memory constants
//see eprom get
// up to 255 memories (maybe less  1024kb on Leonardo, 4 bytes for freq, 8 byte name = 85 memories, maybe limit to 64)
// store 4 bytes of metadata: n_mem, last_mem, 
static uint8_t n_mem; // get from eeprom as first field
static uint8_t i_mem = 0;
static char mem_name[8];

struct MemItem {
  int freq;
  char memName[8];
};

void memory_write() {
  /**
     use eeprom to store/retrieve memory; eventually implement serial for storing names.
  */
  EEPROM.get(1,n_mem);
  int max_mem = (EEPROM.length()-4)/sizeof(MemItem);   

  MemItem entry;
  if(Serial.available() > 0){
    // input format = 1,1400
    char memLoc=(char)Serial.parseInt();
    entry.freq = Serial.parseInt(); // docs claim this reads a long
    String nm = Serial.readStringUntil("\n");
    nm.toCharArray(entry.memName, 8);
    if(memLoc > max_mem or memLoc > (n_mem+1)){ // check that in valid range.
      Serial.print(F("invalid mem num; no write"));
      return;
    }
    EEPROM.put(memLoc*sizeof(MemItem)+4,entry);
    if(memLoc > n_mem){
      EEPROM.put(1, n_mem+1); // update memory count
    }
    i_mem = memLoc;
  }
  memory_read();
}

void memory_read() {
  /**
     read from eeprom
  */
  EEPROM.get(1,n_mem);
  int max_mem = min(n_mem, (EEPROM.length()-4)/sizeof(MemItem));   
  int address = i_mem*sizeof(MemItem) + 4;
  MemItem entry;
  EEPROM.get(address, entry);
  freq = entry.freq;
  //mem_name = entry.memName;
  strncpy(mem_name, entry.memName, sizeof(entry.memName) - 1);
}

// vfo constants
// vfo steps in hz
static uint8_t i_vfo_step = 6; // start at 1khz step
static const PROGMEM uint8_t n_vfo_step = 7;
static const PROGMEM uint16_t vfo_step[] = {
  50, 100, 250, 500, 1000, 2500, 5000
};
// band constants
static uint8_t i_band = 5; //start at 20m
static const PROGMEM uint8_t n_band = 11;
static const PROGMEM int band_name[] = {
  630, 160, 80, 40, 30, 20, 17, 15, 12, 10, 6
};
static const PROGMEM int band_start[] = {
  472, 1800, 3500, 7000, 10100, 14000, 18068, 21000, 29890, 28000, 50000
};
static const PROGMEM int band_end[] = {
  479, 2000, 4000, 7300, 10150, 14350, 18168, 21450, 24990, 29700, 54000
};


// control functions
void decr() {
  rotate(-1);
}

void incr() {
  rotate(1);
}

void rotate(int sign) {
  update = true;
  switch (mode) {
    case 'm':
      // in/decrement memory index
      i_mem = (i_mem + sign) % n_mem;
      break;
    case 's':
      // adjust sweep start freq
      freq += sign * sweep_step[i_sweep_step];
      break;
    case 'g':
      // adjust frequency
      freq += (sign * vfo_step[i_vfo_step]);
      break;
    case 'v':
      freq += (sign * vfo_step[i_vfo_step]);
      if (freq > band_end[i_band] * 1000) {
        freq = band_start[i_band] * 1000;
      }
      if (freq < band_start[i_band] * 1000){
        freq = band_end[i_band]*1000;
      }
      break;
  }
}

void ch_mode() {
  update = true;
  switch (mode) {
    case 's':
      mode = 'v'; // skip to v until mem is implemented
      break;
    case 'm':
      // memory read mode
      mode = 'v';
      break;
    case 'w':
      // memory write mode (serial input)
      mode = 'v';
      break;
    case 'v':
      mode = 'g';
      break;
    case 'g':
      mode = 's';
      break;
  }
}

void ch_step() {
  update = true;
  switch (mode) {
    case 'm':
      break;
    case 's':
      i_sweep_step = (i_sweep_step + 1) % n_sweep_step;
      break;
    case 'v':
    case 'g':
      i_vfo_step = (i_vfo_step + 1) % n_vfo_step;
      break;
  }
}

void ch_band() {
  update = true;
  switch (mode) {
    case 's':
      // in sweep mode this should set sweep size
      i_sweep_size = (i_sweep_size + 1) % n_sweep_size;
      break;
    case 'v':
      i_band = (i_band + 1) % n_band;
      freq = band_start[i_band] * 1000;
      break;
    case 'g':
      // increment Mhz by 1
      int mhz = freq / (1000 * 1000);
      mhz += 1;
      if (mhz > 59) {
        mhz = 1;
      }
      freq = mhz * (1000 * 1000);
      break;
  }
}




void displayFreq(long _freq) {
  /**
     Wrties frequency to display
       MHZ,KHZ
       MODE STEPkHz
  */
  int mhz = (_freq / 1000) / 1000;
  int khz = (_freq / 1000) % 1000;
  int _10hz = ((_freq % 1000) % 1000) / 10;

  display.clear();
  // print frequency
  char strBuf[10];
  sprintf(strBuf, "%2d.%03d.%02d", mhz, khz, _10hz);
  display.print(strBuf);
  Serial.print(strBuf);
  Serial.print("Mhz ");

  // followed by printing the mode
  char strBuf2[10];
  switch (mode) {
    case 's':
      sprintf(strBuf2, "SWP - %4d khz", sweep_step[i_sweep_step]);
      break;
    case 'v':
      sprintf(strBuf2, "VFO - %dHz", vfo_step[i_vfo_step]);
      break;
    case 'm':
      sprintf(strBuf2, "M%4d%s", i_mem, mem_name);
      break;
    case 'g':
      sprintf(strBuf2, "SG  - %dHz", vfo_step[i_vfo_step]);
      break;
  }

  display.print(strBuf2);
  Serial.println(strBuf2);

  display.display(); // need this to update display!

}

void setFreq(long _freq) {
  DDS_freq(_freq); // output as long
  displayFreq(_freq);
}




void loop() {
  switch (mode) {
    case 's':
      sweep();
      break;
    case 'w':
      memory_write();
      break;
    case 'm':
      memory_read(); //somehow call update first time only.
      break;
    default: // cases v,g,m
      if (update) setFreq(freq);
      break;
  }
  update = false;
}
