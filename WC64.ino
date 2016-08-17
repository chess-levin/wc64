/***************************************************************************************************************

 This sketch uses the following Libs
 
 * FastLED-3.0.3
 * ds3231-master
 
 
 (c) by Sven Scheil, 2015

*****************************************************************************************************************/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <config.h>
#include <ds3231.h>
#include <Wire.h>
#include <FastLED.h>
#include <SoftwareSerial.h>

/////////////////////////
// DS3231 Definitions
//
#define BUFF_MAX 128
//uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;

////////////////////////////
// WS2812 Definitions
//
// Params for width and height
const uint8_t kMatrixWidth = 8;
const uint8_t kMatrixHeight = 8;
// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;
// Number of RGB LEDs in the strand
#define NUM_LEDS 68

// Define the array of leds
CRGB leds[NUM_LEDS];
// Arduino pin used for LED (WS2812) Data
#define DATA_PIN 4

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Colors for display in different set modes and normal time mode
#define COLOR_NORMAL_DISPLAY CRGB( 255, 255, 255)
#define COLOR_SET_DISPLAY CRGB( 255, 136, 0)
#define COLOR_SET_DAY CRGB( 255, 0, 0); 
#define COLOR_SET_MONTH CRGB( 0, 255, 0); 
#define COLOR_SET_YEAR CRGB( 0, 0, 255);
#define LETTER_OFF 0

#define BTN_MIN_PRESSTIME 95    //doftware debouncing: ms button to be pressed before action
#define TIMEOUT_SET_MODE 30000  //ms no button pressed

#define SET_MODE_DUMMY -1
#define SET_MODE_OFF 0
#define SET_MODE_LEDTEST 1
#define SET_MODE_YEAR 2
#define SET_MODE_MONTH 3
#define SET_MODE_DAY 4
#define SET_MODE_HOURS 5
#define SET_MODE_5MINUTES 6
#define SET_MODE_1MINUTES 7
#define SET_MODE_MAX 7

volatile int setModeState = SET_MODE_OFF;

#define START_WITH_YEAR 2015  // start year setting with

#define MIN_BRIGHTNESS 4 
#define MAX_BRIGHTNESS 65

#define SET_BTN1_PIN 2      // set mode button; Interrupt 0 is on DIGITAL PIN 2!
#define SET_BTN2_PIN 3      // set value button; Interrupt 1 is on DIGITAL PIN 3!
#define SET_BTN1_IRQ 0      // set mode button; Interrupt 0 is on DIGITAL PIN 2!
#define SET_BTN2_IRQ 1      // set value button; Interrupt 1 is on DIGITAL PIN 3!

#define INTERNAL_LED_PIN 13

#define BRIGHNTNESS_SENSOR_PIN A3

#define TERM 255

const byte whone[] PROGMEM = {28,29,30,31,TERM};
const byte whtwo[] PROGMEM = {35,36,41,42,TERM};
const byte whthree[] PROGMEM = {41,42,43,44,TERM};
const byte whfour[] PROGMEM = {20,21,22,23,TERM};
const byte whfive[] PROGMEM = {39,40,55,56,TERM};
const byte whsix[] PROGMEM = {24,25,26,27,28,TERM};
const byte whseven[] PROGMEM = {32,33,34,47,46,45,TERM};
const byte wheight[] PROGMEM = {63,62,61,60,TERM};
const byte whnine[] PROGMEM = {52,53,54,55,TERM};
const byte whten[] PROGMEM = {49,50,51,52,TERM};
const byte wheleven[] PROGMEM = {56,57,58,TERM};
const byte whtwelve[] PROGMEM = {35,36,37,38,39,TERM};

const byte* const whours[] PROGMEM = {whone, whtwo, whthree, whfour, whfive, whsix, whseven, wheight, whnine, whten, wheleven, whtwelve};

const byte wmone[] PROGMEM = {66,TERM};
const byte wmtwo[] PROGMEM = {66,65,TERM};
const byte wmthree[] PROGMEM = {66,65,64,TERM};
const byte wmfour[] PROGMEM = {66,65,64,67,TERM};
const byte wmfive[] PROGMEM = {0,1,2,3,TERM};
const byte wmten[] PROGMEM = {4,5,6,7,TERM};
const byte wmfiveten[] PROGMEM = {0,1,2,3,4,5,6,7,TERM};

#define mfive_idx 4
#define mten_idx 5
#define mfiveten_idx 6

const byte* const wminutes[] PROGMEM = {wmone, wmtwo, wmthree, wmfour, wmfive, wmten, wmfiveten};

const byte wto[] PROGMEM = {13,14,15,TERM};
const byte wpast[] PROGMEM = {9,10,11,12,TERM};
const byte whalf[] PROGMEM = {16,17,18,19,TERM};

#define mto_idx 0
#define mpast_idx 1
#define mhalf_idx 2

const byte* const wtime[] PROGMEM = {wto, wpast, whalf};

byte letterMatrix[NUM_LEDS];

uint8_t minLastDisplayed = 0;

long lastPressedTime = 0;
struct ts t;

void (*animationCalback)();

#define UPDATES_PER_SECOND 8

byte startpoint[] = {0,2,3,0,1,0,2};
int averageBrigtness[] = {500,500,500,500,500,500};
byte averageBrigtnessIdx = 0;

///////////////////////////////////////////////////////////////////
// Bluetooth definitions
//
#define BT_BUFF_MAX 25
#define BT_END_TOKEN '$'
#define BT_PARAM_SEP '&'
#define rxPin 10
#define txPin 11

// set up a new serial port
SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);

unsigned long bt_prev, bright_prev, interval = 5000;
char uptime[20] = "12.45.78, 12:45:78";
char bt_buff[BT_BUFF_MAX];
int bt_i = 0;

void setup() {
  //delay(3000);
  Serial.begin(9600);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);  
  mySerial.begin(9600);

  Wire.begin(); // init Wire Library
  DS3231_init(DS3231_INTCN);
  memset(recv, 0, BUFF_MAX);
  DS3231_get(&t);
  sprintf(uptime, "%02d.%02d.%04d, %02d:%02d:%02d", t.mday, t.mon, t.year, (uint8_t) t.hour, t.min, t.sec);
  Serial.print(F("Started at")); Serial.println(uptime);
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  digitalWrite(INTERNAL_LED_PIN, LOW);  // turn LED OFF
  randomSeed(analogRead(0));  
}

void loop() {
  //FastLED.show();                        // see https://github.com/FastLED/FastLED/wiki/FastLED-Temporal-Dithering
  unsigned long now = millis();

////////////////
  char txtbuf[75];     
  char c;
    
  if (mySerial.available()) {
    c = (char) mySerial.read();
    //Serial.write(c);
    bt_buff[bt_i++] = c;
    if (bt_i==1) {
      Serial.println(F("Receiving new data via BT."));
      bt_prev = now;
    }
  }
  
  if ((bt_i>0) && (now - bt_prev > interval)) {
    Serial.println(F("Timeout. Resetting transmission."));
    bt_i = 0;
    setModeState = SET_MODE_OFF;
  }
  
  if (bt_i < BT_BUFF_MAX-1) {
    if (c == BT_END_TOKEN) {
      bt_buff[bt_i++] = 0;   
      
      sprintf(txtbuf, "End Token received. Received %d Bytes Data: ", bt_i-1);
      Serial.println(txtbuf);

      Serial.println(bt_buff);
      parseCommand(bt_buff);
      bt_i = 0;
      setModeState = SET_MODE_OFF;
    } 
  } else {
    bt_buff[bt_i++] = 0;
    sprintf(txtbuf, "Buffer overflow: %d Bytes received. Missing end token.", bt_i-1);
    Serial.println(txtbuf);
    Serial.println(bt_buff);
    bt_i = 0;
    setModeState = SET_MODE_OFF;
  }

  if (setModeState == SET_MODE_OFF) {
    animationCalback = &showMatrixAnimation;
//    animationCalback = &showNoAnimation;
    getRTCData(&t);
        
    if (t.min != minLastDisplayed) {
      minLastDisplayed = t.min;
      showTime(t.hour, t.min);
    }
    
    if (now - bright_prev > interval) {
      readBrightnessSensor();
      printRTCDataStruct(&t);
      bright_prev = now;
    }
  
  } else {
     Serial.print(F("setModeState in main loop: "));
     Serial.println(setModeState);
     animationCalback = &showNoAnimation;
     FastLED.clear();
     //showTime(t.hour, t.min);
  }
  
}

void loopReadBT() {
  char txtbuf[75];     
  char c;
  unsigned long now = millis();
    
  if (mySerial.available()) {
    c = (char) mySerial.read();
    //Serial.write(c);
    bt_buff[bt_i++] = c;
    if (bt_i==1) {
      Serial.println(F("Receiving new data via BT."));
      bt_prev = now;
    }
  }
  
  if ((bt_i>0) && (now - bt_prev > interval)) {
    Serial.println(F("Timeout. Resetting transmission."));
    bt_i = 0;
    setModeState = SET_MODE_OFF;
  }
  
  if (bt_i < BT_BUFF_MAX-1) {
    if (c == BT_END_TOKEN) {
      bt_buff[bt_i++] = 0;   
      
      sprintf(txtbuf, "End Token received. Received %d Bytes Data: ", bt_i-1);
      Serial.println(txtbuf);

      Serial.println(bt_buff);
      parseCommand(bt_buff);
      bt_i = 0;
      setModeState = SET_MODE_OFF;
    } 
  } else {
    bt_buff[bt_i++] = 0;
    sprintf(txtbuf, "Buffer overflow: %d Bytes received. Missing end token.", bt_i-1);
    Serial.println(txtbuf);
    Serial.println(bt_buff);
    bt_i = 0;
    setModeState = SET_MODE_OFF;
  }
}

void parseCommand(char* commandstr) {
  char * pch;
  char token[] = {BT_PARAM_SEP, BT_END_TOKEN, 0};

  pch = strtok (commandstr, token);
  
  if (strcmp(commandstr, "01") == 0) {
    parseConnect(&commandstr[3]);
  } else if (strcmp(commandstr, "02") == 0) {
    parseDateTime(&commandstr[3]);
  } else if (strcmp(commandstr, "03") == 0) {
    sendSysInfo();
  }
  else {
    Serial.print(F("Unknown command: "));
    Serial.println(pch);
  }
}

void sendSysInfo() {
  char txtbuf [60];
  char tempbuf[6];

  float t = DS3231_get_treg();
  
  dtostrf(t, 4, 2, tempbuf);
  sprintf(txtbuf,"Running since %s, Temperatur %s C", uptime, tempbuf);
  Serial.println(txtbuf);  

  mySerial.write(txtbuf);
}

void parseConnect(char* con) {
  char * pch;
  int i = 0;
  char tokens[] = {BT_END_TOKEN, BT_PARAM_SEP, 0};
  String arg;
  
  Serial.print("Status: ");
  pch = strtok(con, tokens);

  while (pch != NULL)
  {
    i++;
    if (i==1) {
      arg = pch;
    }
    pch = strtok (NULL, tokens);
  }
  
  if (i == 1) {
    Serial.println(arg);
    setModeState = SET_MODE_LEDTEST;
  } else {
    char txtbuf [40]; 
    sprintf(txtbuf, "Expecting %d arguments, parsed %d.", 1, i);
    Serial.println(txtbuf);
  }
}

void parseDateTime(char* datetime) {
  char * pch;
  int i = 0;
  struct ts t;
  char tokens[] = {BT_END_TOKEN, BT_PARAM_SEP, 0};

  Serial.print(F("Set date/time to : "));
  pch = strtok(datetime, tokens);

  while (pch != NULL)
  {
    i++;
    if (i==1) {
      t.hour = atoi(pch);
    } else if (i==2) {
      t.min = atoi(pch);
    } else if (i==3) {
      t.sec = atoi(pch);
    } else if (i==4) {
      t.mday = atoi(pch);
    } else if (i==5) {
      t.mon = atoi(pch);
    } else if (i==6) {
      t.year = atoi(pch);    
    }    
    //Serial.println (pch);
    pch = strtok(NULL, tokens);
  }
  
  if (i == 6) {
    DS3231_set(t);
    showTime(t.hour, t.min);
  } else {
    char buffer [40]; 
    sprintf(buffer, "Expecting %d arguments, parsed %d.", 6, i);
    Serial.println(buffer);
  }
}

void showAnimation( void (*animation)() ) {
  animation();
  resetLetterMatrix();
}

void showTime(int hours, int minutes) {
  Serial.print(F("Show time: "));
  printTime(hours, minutes);
  int hcorrection = showMinutes(minutes);
  showHours(hours + hcorrection);
  showAnimation(animationCalback);
}


void getRTCData(struct ts *t) {
    float temperature;
    char buff[BUFF_MAX];
   
    DS3231_get(t); //Get time

    //printRTCDataStruct(t);
}


// If there are <= 20 minutes in a hour a correction value of 1 is returned
// to be added to the hour
byte showMinutes(int minutes) {
  
  int tidx = -1; // 5, 10, 15
  int ridx = -1; // 1 past , 2 to 
  int qidx = -1; // 1 half
  int showMinutes = minutes;
  
  if (minutes == 0) {
  } else if (minutes < 5) {
    ridx = mpast_idx;
  } else if (minutes < 10) {
    tidx = mfive_idx;
    ridx = mpast_idx;
  } else if (minutes < 15) {
    tidx = mten_idx;
    ridx = mpast_idx;
  } else if (minutes < 20) {
    tidx = mfiveten_idx;
    ridx = mpast_idx;
  } else if (minutes == 20) {
    tidx = mten_idx;
    ridx = mto_idx;
    qidx = mhalf_idx;
  } else if (minutes < 25) {
    tidx = mfive_idx;
    ridx = mto_idx;
    qidx = mhalf_idx;
    showMinutes = 25 - minutes;
  } else if (minutes == 25) {
    tidx = mfive_idx;
    ridx = mto_idx;
    qidx = mhalf_idx;
  } else if (minutes < 30) {
    ridx = mto_idx;
    qidx = mhalf_idx;
    showMinutes = 30 - minutes;
  } else if (minutes == 30) {
    qidx = mhalf_idx;  
  } else if (minutes < 35) {
    qidx = mhalf_idx;  
    ridx = mpast_idx;
  } else if (minutes < 40) {
    tidx = mfive_idx;
    ridx = mpast_idx;
    qidx = mhalf_idx;  
  } else if (minutes == 40) {
    tidx = mten_idx;
    ridx = mpast_idx;
    qidx = mhalf_idx;  
  } else if (minutes < 45) {
    tidx = mfiveten_idx;
    ridx = mto_idx;
    showMinutes = 45 - minutes;
  } else if (minutes == 45) {
    tidx = mfiveten_idx;
    ridx = mto_idx;
  } else if (minutes < 50) {
    tidx = mten_idx;
    ridx = mto_idx;
    showMinutes = 50 - minutes;
  } else if (minutes == 50) {
    tidx = mten_idx;
    ridx = mto_idx;
  } else if (minutes < 55) {
    tidx = mfive_idx;
    ridx = mto_idx;
    showMinutes = 55 - minutes;
  } else if (minutes == 55) {
    tidx = mfive_idx;
    ridx = mto_idx;
  } else if (minutes < 60) {
    showMinutes = 60 - minutes;
    ridx = mto_idx;
  }
  
  if (tidx>=0) {
    showWord((byte*) pgm_read_word (&wminutes[tidx]));
  }
  
  if (ridx>=0) {
    showWord((byte*) pgm_read_word (&wtime[ridx]));
  }

  if (qidx>=0) {
    showWord((byte*) pgm_read_word (&wtime[qidx]));
  }

  if ((showMinutes % 5) > 0) {
    showWord((byte*) pgm_read_word (&wminutes[(showMinutes % 5)-1]));
  }
  
  if (minutes < 20) {
    return 0;
  } else {
    return 1;
  }
}

void showHours(int hours) {
  if (hours > 12) {
    hours -= 12;
  }
  
  showWord((byte*) pgm_read_word (&whours[hours-1]));
}
  
void testShowAllWordsSeq() {
  FastLED.clear();  
  showWord((byte*) pgm_read_word (&wminutes[0]));
  showWord((byte*) pgm_read_word (&wminutes[1]));
  showWord((byte*) pgm_read_word (&wminutes[2]));
  showWord((byte*) pgm_read_word (&wminutes[3]));
  showWord((byte*) pgm_read_word (&wminutes[mfive_idx]));
  showWord((byte*) pgm_read_word (&wminutes[mten_idx]));
  showWord((byte*) pgm_read_word (&wminutes[mfiveten_idx]));
  showWord((byte*) pgm_read_word (&wtime[mto_idx]));
  showWord((byte*) pgm_read_word (&wtime[mpast_idx])); 
  showWord((byte*) pgm_read_word (&wtime[mhalf_idx]));
  showWord((byte*) pgm_read_word (&whours[0]));
  showWord((byte*) pgm_read_word (&whours[1])); 
  showWord((byte*) pgm_read_word (&whours[2]));
  showWord((byte*) pgm_read_word (&whours[3]));
  showWord((byte*) pgm_read_word (&whours[4]));
  showWord((byte*) pgm_read_word (&whours[5])); 
  showWord((byte*) pgm_read_word (&whours[6]));
  showWord((byte*) pgm_read_word (&whours[7])); 
  showWord((byte*) pgm_read_word (&whours[8]));
  showWord((byte*) pgm_read_word (&whours[9])); 
  showWord((byte*) pgm_read_word (&whours[10]));
  showWord((byte*) pgm_read_word (&whours[11]));
  FastLED.show();  
}

void showWord(const byte* wordLeds) {
  byte idx = pgm_read_byte(wordLeds);
  
  while (idx < TERM) {
    letterMatrix[idx] = 1; 
    wordLeds++;
    idx = pgm_read_byte( wordLeds);
  }
}

void readBrightnessSensor() {
  char buffer[26];
  int sum = 0;

  averageBrigtness[averageBrigtnessIdx] = analogRead(BRIGHNTNESS_SENSOR_PIN);

  Serial.print(F("Brightness Measures : "));

  averageBrigtnessIdx++;
  if (averageBrigtnessIdx >= (sizeof(averageBrigtness)/sizeof(int))) averageBrigtnessIdx = 0;
  
  for (int i = 0; i < (sizeof(averageBrigtness)/sizeof(int)); i++) {
    Serial.print(averageBrigtness[i]);
    Serial.print(", ");  
    sum += averageBrigtness[i];
  }

  int average = sum / (sizeof(averageBrigtness)/sizeof(int));
  Serial.print(F("Average of Measures: "));
  Serial.println(average);  

  int brightnessVal = map(average, 0, 1023, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  
  FastLED.setBrightness(brightnessVal);

  sprintf(buffer, "Brightness %d => %d", average, brightnessVal);
  Serial.println(buffer);
}

void printRTCDataStruct(struct ts *t) {
   printDate(t);
   printTime(t);
}

void printTime(int hours, int minutes) {
    char buffer [6]; 
    sprintf(buffer, "%02d:%02d", hours, minutes);
    Serial.println(buffer);
}

void printTime(struct ts *t) {
  char buffer [12]; 
  sprintf(buffer, "%02d:%02d:%02d", (uint8_t) t->hour, t->min, t->sec);
  Serial.println(buffer);
}

void printDate(struct ts *t)
{
   char buffer [15];
   sprintf(buffer, "%02d.%02d.%04d, ", t->mday, t->mon, t->year);
   Serial.print(buffer);
}

void showNoAnimation() {
  FastLED.clear();
  for(byte i = 0; i < NUM_LEDS; i++) {
      if (letterMatrix[i] == 1) {
          leds[i] = CRGB(255,255,255);
      }
  }
  FastLED.show();
}

void showMatrixAnimation() {
  Serial.println("showMatrixAnimation()");
  setMatrixAnimStartpoints();
  
  for (byte loopCount = 0; loopCount < kMatrixHeight+3; loopCount++) {
    matrixRainAnimation();
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}

void matrixRainAnimation() {
    Serial.println("matrixRainAnimation()");
    
    for (byte x = 0; x < kMatrixWidth; x++) {
      matrixRainAnimCol(startpoint[x]++, x);
  }
}

void matrixRainAnimCol(int currentLine, byte x) {
  byte startline = MAX(currentLine - 2, 0);
  byte endline = MIN(kMatrixHeight, currentLine + 1);
  
  for (byte y = 0; y < kMatrixHeight; y++) {
  
    if (y == 0)  {
      if (letterMatrix[66] == 1) {
        leds[66] = CRGB(255,255,255);
      } else {
        leds[66] = CRGB::Black;
      }
      if (letterMatrix[65] == 1) {
        leds[65] = CRGB(255,255,255);
      } else {
        leds[65] = CRGB::Black;
      }
    }

    if (y == kMatrixHeight-1)  {
      if (letterMatrix[64] == 1) {
        leds[64] = CRGB(255,255,255);
      } else {
        leds[64] = CRGB::Black;
      }
      if (letterMatrix[67] == 1) {
        leds[67] = CRGB(255,255,255);
      } else {
        leds[67] = CRGB::Black;
      }
    } 
   
    if ((y >= startline) && (y < endline) ) {
      if ((y+1 == endline) && (endline < kMatrixHeight))
        leds[xy2LedIndex(x, y)] = CRGB(255,255,255);
      else 
        leds[xy2LedIndex(x, y)] = CRGB(45, 45, 45);
    } else {
      if (y < endline) {
        if (letterMatrix[xy2LedIndex(x, y)] == 1) {
          leds[xy2LedIndex(x, y)] = CRGB(255,255,255);
        } else {  
          leds[xy2LedIndex(x, y)] = CRGB::Black;
        }
      }
    }
    
  }
}

void setMatrixAnimStartpoints() {
  startpoint[0] = random(3);
  startpoint[1] = random(3);
  startpoint[2] = random(3);
  startpoint[3] = random(3);
  startpoint[4] = random(3);
  startpoint[5] = random(3);
  startpoint[6] = random(3);
  startpoint[7] = random(3);
}


void resetLetterMatrix() {
  for (int i=0; i <NUM_LEDS; i++) {
    letterMatrix[i] = LETTER_OFF;
  }
}

uint16_t xy2LedIndex( uint8_t x, uint8_t y)
{
  uint16_t i;
  
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
 
  return i;
}


