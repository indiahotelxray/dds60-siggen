/**
 *  Arduino DDS60 signal generator / VFO
 *    based on code from http://hamradioprojects.com/authors/w6akb/+sweeper/
 *    
 *    Arduino / SeeedStudio Protoboard
 *       +- DDS60 -> BNC
 *            P1-1 -> D5
 *            P1-2 -> D9
 *            P1-3 -> D8
 *       +- OLED display (SSD1306)
 *            I2C - SDA - D2; SCL - D3; Reset - D4
 *       +- Rotary Encoder - not implemented
 *            Up/down - freq change
 *            Press - increment step 10/50/100/500/1000 kHz
 *       +- Buttons - not implemented
 *            B1 - Band increment 160/80/40/20/17/15/12/10/6
 *            B2 - Sweep
 *            
 *   Features
 *     Fixed/sweep mode  (Sweep takes input frequency up to input + STEP at a given rate)
 *     
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins 2-3)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// DDS60 pins
#define DDSCLOCK 9
#define DDSDATA 8
#define DDSLOAD 5

#define START 1000000L   // low frequency limit 3 mhz, min 1 mhz
#define END  30000000L   // high frequency limit 30 mhz, max 60 mhz

void DDS_init()
{
  pinMode(DDSCLOCK,OUTPUT);  // set pins to output mode
  pinMode(DDSDATA,OUTPUT);
  pinMode(DDSLOAD,OUTPUT);
}

void DDS_pintest()
// output millisecond level signals on DDS pins for scope testing
// unplug DDS for these tests
{
  //lcd.cursorTo(1,0);
  //lcd.printIn("DDS PinTest     ");
  digitalWrite(DDSDATA,HIGH);
  digitalWrite(DDSCLOCK,HIGH);
  digitalWrite(DDSLOAD,HIGH);
  delay(10);
  digitalWrite(DDSDATA,LOW);   // 10ms on data DDS p3
  delay(10);
  digitalWrite(DDSCLOCK,LOW);  // 20ms on clock DDS p2
  delay(10);
  digitalWrite(DDSLOAD,LOW);   // 30ms on load DDS p1
  delay(10);
}

void DDS_test_alt()  // alternate between two frequencies
{
  //lcd.cursorTo(1,0);
  //lcd.printIn("DDS 7150/7140   ");
  DDS_freq(7150000);
  delay(1000);
  DDS_freq(7140000);
  delay(500);
}

void DDS_test_onoff()  // turn carrier on and off
{
  //lcd.cursorTo(1,0);
  //lcd.printIn("DDS 7150 OnOff  ");
  DDS_freq(7150000);
  delay(1000);
  DDS_off();
  delay(500);
}



void DDS_freq(unsigned long freq)  // set DDS freq in hz
{
  unsigned long clock = 180000000L;  // 180 mhz, may need calibration
  int64_t scale = 0x100000000LL;
  unsigned long tune;                // tune word is delta phase per clock
  int ctrl,i;                        // control bits and phase angle
  
  if (freq == 0)
  {
    tune = 0;
    ctrl = 4;  // enable power down
  }
  else
  {
    tune = (freq * scale) / clock;
    ctrl = 1;  // enable clock multiplier
    
    digitalWrite(DDSLOAD, LOW);  // reset load pin
    digitalWrite(DDSCLOCK,LOW);  // data xfer on low to high so start low
    
    for (i = 32; i > 0; --i)  // send 32 bit tune word to DDS shift register
    {
      digitalWrite(DDSDATA,tune&1);  // present the data bit
      digitalWrite(DDSCLOCK,HIGH);   // clock in the data bit
      digitalWrite(DDSCLOCK,LOW);    // reset clock
      tune >>= 1;                    // go to next bit
    }
  }
  for (i = 8; i > 0; --i)  // send control byte to DDS
  {
    digitalWrite(DDSDATA,ctrl&1);
    digitalWrite(DDSCLOCK,HIGH);
    digitalWrite(DDSCLOCK,LOW);
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

void writeFreq(int mhz, int khz, uint8_t step_size, char mode[]) {
  /**
   * Wrties frequency to display
   *   MHZ,KHZ
   *   MODE STEPkHz
   */
   display.clearDisplay();
   display.setTextSize(2);
   display.setTextColor(SSD1306_WHITE);
   display.cp437(true); 
   display.setCursor(0,0);
   display.print(mhz);
   display.print(F(","));
   display.print(khz);
   display.display();
   Serial.print(mhz);
   Serial.print(F(","));
   Serial.print(khz);
}

void setFreq(long
mhz, long khz){
  long outFreq;
  outFreq = (mhz * 1000 * 1000);
  outFreq += (khz * 1000);
  DDS_freq(outFreq); // output as long
  writeFreq(mhz, khz, 0, "");
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 1 seconds

  // Clear the buffer
  display.clearDisplay();

  // setup DDS
  DDS_init();
  //DDS_pintest();
}

static const int PROGMEM test_bands[] = 
{
  3,
  3,
  7,
  7,
  14,
  14
};

static const int PROGMEM test_freqs[] = 
{
  500,
  600,
  0,
  100,
  0,
  150
};


void loop() {
  // put your main code here, to run repeatedly:
  //for(int i = 0; i < 3; i++){
    // loop through these for testing, 2 seconds each
    setFreq(3,564);
    delay(2000);
    setFreq(7,31);
    delay(2000);
    setFreq(14,57);
    delay(2000);
  //}
}
