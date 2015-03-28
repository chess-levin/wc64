#include <config.h>
#include <ds3231.h>
#include <Wire.h>
#include <FastLED.h>

// Number of RGB LEDs in the strand
#define NUM_LEDS 68
const uint8_t NUM_ROWS  = 8;

// DC3231
#define BUFF_MAX 128
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];
// Arduino pin used for Data
#define DATA_PIN 2

#define BTN_MIN_PRESSTIME 95   //ms button to be pressed before action
#define TIMEOUT_SET_MODE 60000  //ms no button pressed

#define SET_MODE_OFF 1
#define SET_MODE_LEDTEST 2
#define SET_MODE_DAY 3
#define SET_MODE_MONTH 4
#define SET_MODE_YEAR 5
#define SET_MODE_HOURS 6
#define SET_MODE_5MINUTES 7
#define SET_MODE_1MINUTES 8
#define SET_MODE_MAX 8

#define START_WITH_YEAR 2015

#define MIN_BRIGHTNESS 25        // 16 is minimum
#define MAX_BRIGHTNESS 122

#define IDLE_LOOP_TIME 5000

#define SET_BTN1_PIN 7
#define SET_BTN2_PIN 6
#define INTERNAL_LED_PIN 13

#define BRIGHNTNESS_SENSOR_PIN 2

uint8_t setMode = SET_MODE_OFF;
int brightnessVal = 37;  // Default hue

#define TERM 255

uint8_t whone[] = {28,29,30,31,TERM};
uint8_t whtwo[] = {35,36,41,42,TERM};
uint8_t whthree[] = {41,42,43,44,TERM};
uint8_t whfour[] = {20,21,22,23,TERM};
uint8_t whfive[] = {39,40,55,56,TERM};
uint8_t whsix[] = {24,25,26,27,28,TERM};
uint8_t whseven[] = {32,33,34,47,46,45,TERM};
uint8_t wheight[] = {63,62,61,60,TERM};
uint8_t whnine[] = {52,53,54,55,TERM};
uint8_t whten[] = {49,50,51,52,TERM};
uint8_t wheleven[] = {56,57,58,TERM};
uint8_t whtwelve[] = {35,36,37,38,39,TERM};

uint8_t* whours[] = {whone, whtwo, whthree, whfour, whfive, whsix, whseven, wheight, whnine, whten, wheleven, whtwelve};

uint8_t wmone[] = {66,TERM};
uint8_t wmtwo[] = {66,65,TERM};
uint8_t wmthree[] = {66,65,64,TERM};
uint8_t wmfour[] = {66,65,64,67,TERM};
uint8_t wmfive[] = {0,1,2,3,TERM};
uint8_t wmten[] = {4,5,6,7,TERM};
uint8_t wmfiveten[] = {0,1,2,3,4,5,6,7,TERM};

#define mfive_idx 4
#define mten_idx 5
#define mfiveten_idx 6

uint8_t* wminutes[] = {wmone, wmtwo, wmthree, wmfour, wmfive, wmten, wmfiveten};

uint8_t wto[] = {13,14,15,TERM};
uint8_t wpast[] = {9,10,11,12,TERM};
uint8_t whalf[] = {16,17,18,19,TERM};

#define mto_idx 0
#define mpast_idx 1
#define mhalf_idx 2

uint8_t* wtime[] = {wto, wpast, whalf};

uint8_t minLastDisplayed = 0;

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  pinMode(SET_BTN2_PIN, INPUT);
  digitalWrite(SET_BTN2_PIN, HIGH); // connect internal pull-up
  pinMode(SET_BTN1_PIN, INPUT);     // uses external pull-up
  digitalWrite(INTERNAL_LED_PIN, LOW);  // turn LED OFF

  Serial.begin(9600);  
  Wire.begin(); // init Wire Library
  DS3231_init(DS3231_INTCN);
  memset(recv, 0, BUFF_MAX);
  
  //Serial.println("Setting time");
  //           TssmmhhWDDMMYYYY aka set time
  //parse_cmd("T001919631012015",16);
}

void loop() {
    struct ts t;
    
    if (setMode == SET_MODE_OFF) {
      hueSensor();
      getRTCData(&t);
      if (t.min != minLastDisplayed)
      showTime(t.hour, t.min);
      minLastDisplayed = t.min;
    }

    idleLoop(&t);
}

void idleLoop(struct ts *t) {
    long startTime = millis();
    long pressedTime1 = 0;
    long pressedTime2 = 0;
    long lastPressedTime = 0;
    
    while (millis() - startTime < IDLE_LOOP_TIME) {
      
      int val1 = digitalRead(SET_BTN1_PIN);
      if (val1 == LOW) {      // Button pressed
       if (pressedTime1 == 0) {
          pressedTime1 = millis();
          lastPressedTime = pressedTime1;
       }
      } else {               // Button released
        if (pressedTime1 > 0) {
          long duration = millis() - pressedTime1;
          if (duration > BTN_MIN_PRESSTIME) {
            nextSetMode(t);
          }
        }
        pressedTime1 = 0;
      }

      int val2 = digitalRead(SET_BTN2_PIN);
      if (val2 == LOW) {      // Button pressed
       if (pressedTime2 == 0) {
          pressedTime2 = millis();
          lastPressedTime = pressedTime2;
       }
      } else {               // Button released
        if (pressedTime2 > 0) {
          long duration = millis() - pressedTime2;
          if (duration > BTN_MIN_PRESSTIME) {
            nextStep(t);
          }
        }
        pressedTime2 = 0;
      }
    }
    if (millis() - lastPressedTime > TIMEOUT_SET_MODE) {
      resetSetMode();
    }
}

void nextStep(struct ts *t) {
  confirmationLEDFlash();
  if (setMode == SET_MODE_DAY) {
    Serial.println("Set next day");
    t->mday+=1;
    if (t->mday > 31) t->mday = 1;
    showDay(t->mday);
  } else   if (setMode == SET_MODE_MONTH) {
    Serial.println("Set next month");
    t->mon+=1;
    if (t->mon > 12) t->mon = 1;
    showMonth(t->mon);
  } else   if (setMode == SET_MODE_YEAR) {
    Serial.println("Set next year");
    t->year+=1;
    if (t->year > 2020) t->year = START_WITH_YEAR;
    showYear(t->year);
  } else   if (setMode == SET_MODE_HOURS) {
    Serial.println("Set next hour");
    t->hour++;
    if (t->hour > 23) t->hour = 0;
    showTime(t->hour, t->min);
  } else   if (setMode == SET_MODE_5MINUTES) {
    Serial.println("Set next 5 minute word");
    t->min+=5;
    if (t->min > 55) t->min = 0;
    showTime(t->hour, t->min);
  } else   if (setMode == SET_MODE_1MINUTES) {
    Serial.println("Set next minute dot");
    t->min+=1;
    if (t->min > 60) t->min = 0;
    showTime(t->hour, t->min);
  } else if (setMode == SET_MODE_LEDTEST) {
    if ((t->min%5) == 0) {
      t->min+=2;
    } else {
      t->min+=3;
    }
    if (t->min > 60) { 
      t->min = 0;
    }
    showTime(t->hour, t->min);
  } else {
    Serial.println("Unknown setMode");
  }
}

void nextSetMode(struct ts *t) {
  confirmationLEDFlash();
  setMode++;

  Serial.print("Switching to setMode: ");
  Serial.println(setMode);
  
  if (setMode == SET_MODE_HOURS) {
    t->hour=12;
    t->min=0;
    t->sec=0;
    showTime(t->hour, t->min);
  } else if (setMode == SET_MODE_LEDTEST) {
    t->hour=12;
    t->min=0;
    t->sec=0;
    testShowAllWordsSeq();
  } else if (setMode == SET_MODE_DAY) {
    t->mday;
    t->mon;
    t->year=START_WITH_YEAR;
    showDay(t->mday);
  } else if (setMode == SET_MODE_MONTH) {
    showMonth(t->mon);
  } else if (setMode == SET_MODE_YEAR) {
    showYear(t->year);
  }

  if (setMode > SET_MODE_MAX) {
    setMode = SET_MODE_OFF;
  }
  
  if (setMode == SET_MODE_OFF) {
    Serial.print("Setting new Date & Time to: ");
    printRTCDataStruct(t);
    DS3231_set(*t);
    digitalWrite(INTERNAL_LED_PIN, LOW);
  } else {
    digitalWrite(INTERNAL_LED_PIN, HIGH);
  }

}

void confirmationLEDFlash() {
  digitalWrite(INTERNAL_LED_PIN, LOW);
  delay (200);
  digitalWrite(INTERNAL_LED_PIN, HIGH);
}

void resetSetMode() {
  if (setMode != SET_MODE_OFF) {
    setMode = SET_MODE_OFF;
    Serial.println("Resetting setMode after timeout pressing no button. Fallback to old Time & Date.");
    digitalWrite(INTERNAL_LED_PIN, LOW);
  }
}

void getRTCData(struct ts *t) {
    float temperature;
    char tempF[6];
    char buff[BUFF_MAX];
   
    DS3231_get(t); //Get time

    printRTCDataStruct(t);
    
    parse_cmd("C",1);
    temperature = DS3231_get_treg(); //Get temperature
    dtostrf(temperature, 5, 1, tempF);

    Serial.print(' ');
    Serial.print(tempF);
    Serial.print((char)223);
    Serial.println("C ");
}

void printRTCDataStruct(struct ts *t) {
    Serial.print(t->mday);
    
    printMonth(t->mon);
    
    Serial.print(t->year);
    Serial.print(", ");   
    Serial.print(t->hour);
    Serial.print(":");
    if(t->min<10)
    {
      Serial.print("0");
    }
    Serial.print(t->min);
    Serial.print(":");
    if(t->sec<10)
    {
      Serial.print("0");
    }
    Serial.println(t->sec);
}

void showTime(int hours, int minutes) {
  Serial.println();
  Serial.print(hours);
  Serial.print(":");
  Serial.println(minutes);

  FastLED.clear();
  int hcorrection = showMinutes(minutes);
  showHours(hours + hcorrection);
  FastLED.show();
}

// If there are <= 20 minutes in a hour a correction value of 1 is returned
// to be added to the hour
uint8_t showMinutes(int minutes) {
  
  int tidx = -1; // 5, 10, 15
  int ridx = -1; // 1 nach , 2 vor 
  int qidx = -1; // 1 halb
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
    showWord(wminutes[tidx]);
  }
  
  if (ridx>=0) {
    showWord(wtime[ridx]);
  }

  if (qidx>=0) {
    showWord(wtime[qidx]);
  }

  if ((showMinutes % 5) > 0) {
    showWord(wminutes[(showMinutes % 5)-1]);
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
  
  showWord(whours[hours-1]);
}
  
void testShowAllWordsSeq() {
  FastLED.show();  
  showWord(wminutes[0]);
  showWord(wminutes[1]);
  showWord(wminutes[2]);
  showWord(wminutes[3]);
  showWord(wminutes[mfive_idx]);
  showWord(wminutes[mten_idx]);
  showWord(wminutes[mfiveten_idx]);
  showWord(wtime[mto_idx]);
  showWord(wtime[mpast_idx]); 
  showWord(wtime[mhalf_idx]);
  showWord(whours[0]);
  showWord(whours[1]); 
  showWord(whours[2]);
  showWord(whours[3]);
  showWord(whours[4]);
  showWord(whours[5]); 
  showWord(whours[6]);
  showWord(whours[7]); 
  showWord(whours[8]);
  showWord(whours[9]); 
  showWord(whours[10]);
  showWord(whours[11]);
  FastLED.show();  
}

void showWord(uint8_t* wordLeds) {

  uint8_t idx = * wordLeds;
  
  while (idx < TERM) {
    Serial.print(idx, DEC);
    Serial.print(", ");
    leds[idx] = CHSV( 0, 0, brightnessVal); 
    wordLeds++;
    idx = * wordLeds;
  }
  Serial.println(idx);

}

void showDay(int day) {
  FastLED.clear();
  for(byte i = 0; i < day; i++) {
    leds[i] = CRGB( 255, 0, 0); 
  }
  FastLED.show();
}
  
void showMonth(int month) {
  FastLED.clear();
  for(byte i = 0; i < month; i++) {
    leds[i] = CRGB( 0, 255, 0); 
  }
  FastLED.show();
}
  
void showYear(int year) {
  FastLED.clear();
  for(byte i = 0; i < year-2000; i++) {
    leds[i] = CRGB( 0, 0, 255); 
  }
  FastLED.show();
}

void hueSensor() {
  int val = analogRead(BRIGHNTNESS_SENSOR_PIN);    // read the value from the sensor
  brightnessVal = map(val, 0, 1023, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  Serial.print("Brightness [");
  Serial.print(MIN_BRIGHTNESS);
  Serial.print(", ");
  Serial.print(MAX_BRIGHTNESS);
  Serial.print("] = ");
  Serial.println(brightnessVal);
}

/*
int ledIndex(int col, int row) {
  if (row  % 2 == 0)
    return row * NUM_ROWS + col;
   else
     return row * NUM_ROWS + NUM_ROWS - 1 - col;
}
*/

void printMonth(int month)
{
  switch(month)
  {
    case 1: Serial.print(" January ");break;
    case 2: Serial.print(" February ");break;
    case 3: Serial.print(" March ");break;
    case 4: Serial.print(" April ");break;
    case 5: Serial.print(" May ");break;
    case 6: Serial.print(" June ");break;
    case 7: Serial.print(" July ");break;
    case 8: Serial.print(" August ");break;
    case 9: Serial.print(" September ");break;
    case 10: Serial.print(" October ");break;
    case 11: Serial.print(" November ");break;
    case 12: Serial.print(" December ");break;
    default: Serial.print(" Error ");break;
  } 
}

void parse_cmd(char *cmd, int cmdsize)
{
    uint8_t i;
    uint8_t reg_val;
    char buff[BUFF_MAX];
    struct ts t;

    //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
    //Serial.print(buff);

    // TssmmhhWDDMMYYYY aka set time
    if (cmd[0] == 84 && cmdsize == 16) {
        //T355720619112011
        t.sec = inp2toi(cmd, 1);
        t.min = inp2toi(cmd, 3);
        t.hour = inp2toi(cmd, 5);
        t.wday = inp2toi(cmd, 7);
        t.mday = inp2toi(cmd, 8);
        t.mon = inp2toi(cmd, 10);
        t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
        DS3231_set(t);
        Serial.println("OK");
    } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
        Serial.print("aging reg is ");
        Serial.println(DS3231_get_aging(), DEC);
    } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
        DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
        //ASSMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
        }
        boolean flags[5] = { 0, 0, 0, 0, 0 };
        DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
        DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
        //BMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
        }
        boolean flags[5] = { 0, 0, 0, 0 };
        DS3231_set_a2(time[0], time[1], time[2], flags);
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
        //Serial.print("temperature reg is ");
        //Serial.println(DS3231_get_treg(), DEC);
    } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
        reg_val = DS3231_get_sreg();
        reg_val &= B11111100;
        DS3231_set_sreg(reg_val);
    } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
        reg_val = DS3231_get_addr(0x5);
        Serial.print("orig ");
        Serial.print(reg_val,DEC);
        Serial.print("month is ");
        Serial.println(bcdtodec(reg_val & 0x1F),DEC);
    } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
        DS3231_set_aging(0);
    } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
        Serial.print("status reg is ");
        Serial.println(DS3231_get_sreg(), DEC);
    } else {
        Serial.print("unknown command prefix ");
        Serial.println(cmd[0]);
        Serial.println(cmd[0], DEC);
    }
}
