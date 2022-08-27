
/**
    Arduino DDS60 signal generator / VFO

      Arduino + Protoshield
         +- DDS60 -> SMA
              P1-1 -> D5  confirm ... ?
              P1-2 -> D9
              P1-3 -> D8
         +- LCD display (16x2)
              Serial board on I2C (SDA/SCL) 0x27
         +- Rotary Encoder - not tested  (see https://playground.arduino.cc/Main/RotaryEncoders/)
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
//#include <EEPROM.h>
#include <LCD_I2C.h>
#include <Encoder.h>
#include <Bounce2.h>
#include <dds60.h>

// DDS60 pins - don't mess with these
#define DDSLOAD 8
#define DDSCLOCK 9
#define DDSDATA 10

//buttons
#define BAND 5
#define MODE 6

// use 2/3 pins for interupts - hardwired now
#define ROT_UP 2
#define ROT_DOWN 3 
#define ROT_PRESS 4

// analog a0, a1, a2 used by RCA ports

// static values
static unsigned long freq = 14000000L;  // 14mhz
static char mode = 'v';
static bool update = true;  // used to signal stopping a sweep/scan

// initialize LCD object
LCD_I2C display(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// initialize DDS60 object
DDS60 dds60module(DDSLOAD, DDSCLOCK, DDSDATA);

// initialize encoder object
Encoder enc(ROT_UP,ROT_DOWN);

Bounce2::Button bandB = Bounce2::Button();
Bounce2::Button modeB = Bounce2::Button();
Bounce2::Button rotB = Bounce2::Button();


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // initialize display
  displayInit();

  // initialize buttons
  buttonInit();


  // set frequency, uses this variable to not be constantly writing to VFO
  update = true;

  // check test
  checkTest();
  test_mode();

}


void checkTest() {
    // if BAND and MODE buttons are pressed, enter test mode
  //bandB.update();
  modeB.update();
  if(modeB.pressed()){
    mode = 't';  // enter test mode
  }
  display.setCursor(0,0);
  display.print(F("TEST MODE"));
  delay(500);
}

void displayInit() {
  // display.begin(16, 2);
  display.begin();
  // display.backlight();
  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  // display.display();
  //delay(250); // Pause for 1 seconds
  display.print(F("kc2ihx dds"));
  delay(250);

  // Clear the buffer
  display.clear();

}

void buttonInit()
{
   bandB.attach(BAND, INPUT_PULLUP);
   modeB.attach(MODE, INPUT_PULLUP);
   rotB.attach(ROT_PRESS, INPUT_PULLUP);
   bandB.interval(2);
   modeB.interval(2);
   rotB.interval(2);
   bandB.setPressedState(LOW);
   modeB.setPressedState(LOW);
   rotB.setPressedState(LOW);

}



 void test_mode(){
   bandB.update();
   modeB.update();
   rotB.update();
   display.setCursor(0,0);
   // display.print(F("TESTMODE1234567"));
   if(modeB.pressed()){
      //display.setCursor(0,0);
      display.print(F("MODE"));
      //delay(100);    
   }
   if(bandB.pressed()){
      //display.setCursor(0,0);
      display.print(F("BAND"));
      //delay(100);
   }
   if(rotB.pressed()){
      //display.setCursor(0,0);
      display.print(F("ROT_PRESS"));
      //delay(100);
   }
 }


long encPosition = -999;

void loop() {

  // update encoder
  long newPos = enc.read();
  if(newPos != encPosition){
    long diff = newPos - encPosition;
    encPosition = newPos;
    display.setCursor(0,1);
    display.print(newPos);
  }

  test_mode();

}
