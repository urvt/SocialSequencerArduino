/* 
Ugniaus pakeitimai 2025-04:
* BPM skaičiavimas pagal delay
* Potenciometras nustatantis tempo (BPM)
* LCD ekraniukas
Ugniaus ir Adomo pakeitimai 2025-09:
* Reverb potenciometras
*/

#include <avr/wdt.h>
#include <Wire.h>
#include <FastLED.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,16,2);

#define LED_PIN 6
//#define LED2_PIN 5
#define NUM_LEDS 24
#define CUBE_TYPE_COUNT 2
#define ROW_COUNT 5
//#define BPM 2000 // 500ms = 120BPM in 8th notes, 60BPM in quarter notes
CRGB leds[NUM_LEDS];
//CRGB leds2[NUM_LEDS];


const byte midiNotes[][CUBE_TYPE_COUNT][ROW_COUNT] = {

  // Board 0
  {
    {0,  1,  2,  3,  4}, // positive cube
    {5,  6,  7,  8,  9} // negative 
    /*{60,  61,  62,  63,  64}, // positive cube
    {65,  66,  67,  68,  69}*/
  },

  // Board 1
  {
    /*{20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}*/
    {0,  1,  2,  3,  4}, // positive cube
    {5,  6,  7,  8,  9} // negative cube
  },
  // Board 2
  {
    /*{40, 41, 42, 43, 44},
    {45, 46, 47, 48, 49}*/
    {0,  1,  2,  3,  4}, // positive cube
    {5,  6,  7,  8,  9} // negative cube
  }

};
// cmd 0x04 - 0x08
//uint8_t NUM_BOARDS = 2;
uint16_t i2cData[8];
boolean i2cLeds[5][8];
uint8_t index = 0;
uint16_t seq_col_data[3][5][8];
uint16_t raw_seq_col_data[5][8];
uint8_t cmd(0), cmd_len(1);
uint8_t SlaveAddr[3] = {0x33, 0x32, 0x31};
unsigned long prev_millis = 0;
unsigned long current_millis = 0;
unsigned long prev_millis_fx_change = 0;
bool fx_changed = false;
int id = 0;
uint8_t activeRow = 0;
double cubeType = 0;

volatile bool shouldReset = false;
volatile unsigned long lastInterruptTime = 0;

int potReverb = A0;
int potTempo = A1; //Assign to pin A0
int potDelay1 = A3;
int potDelay2 = A2;
int resetButton = 2; //digitalPin for the interrupt
int pot_tempo_value = 0;
int pot_reverb_value = 0;
int pot_delay1_value = 0;
int pot_delay2_value = 0;
int reverb_value = 0;
int reverb_value_last = 0;
int delay1_value = 0;
int delay1_value_last = 0;
int delay2_value = 0;
int delay2_value_last = 0;
int tempo_delay_time = 500;
float tempo_delay_time_seconds;
int bpm;

//// Interrupt service routine to handle button press
void resetArduino() {
    shouldReset = true;
}

void getRawData(uint8_t SlaveAddr) {
  for (int k = 0; k < 5; k++) {
    cmd = 0x08 - k;
    cmd_len = 16;
    Wire.beginTransmission(SlaveAddr);
    Wire.write(cmd);
    Wire.endTransmission();
    Wire.requestFrom(SlaveAddr, cmd_len);    // request 6 bytes from slave device #8

    while (Wire.available() && index < 8) { // slave may send less than requested
      raw_seq_col_data[k][7 - index] = Wire.read() | Wire.read() << 8;
      index++;
      delay(1);
    } index = 0;
    /*
        for (int i = 0; i < 8; i++) {
          raw_seq_col_data[k][i] = i2cData[7 - i];
          //Serial.print(i2cData[i]);
          //Serial.print(" ");
        } //Serial.println();*/
    //Serial.println();
  }// Serial.println("----------");
}
void storeRawData() {
  //for (int k = 0; k < NUM_BOARDS; k++) {
    int k = 0;
    //getRawData(0x31);
    getRawData(0x33);
    //getRawData(SlaveAddr[k]);
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 8; j++) {
        seq_col_data[k][i][j] = raw_seq_col_data[i][j];
      }
    }
  //}
}
/*int getCubeType(int rawAnalogVal, int storedAnalogVal) {
  if (rawAnalogVal > storedAnalogVal) {
    return 3;
  } else if (rawAnalogVal < storedAnalogVal) {
    return 2;
  } else {
    return 0;
  }
}*/

double getActiveCube(double NewVal, double OldVal) {
  double val;
  val = ((NewVal - OldVal) / abs(OldVal)) * 100;
  return val;
}
void noteOn(byte channel, byte pitch, byte velocity) {
  channel += 0x90 - 1;
  /*Serial.print("channel: ");
  Serial.println(channel, HEX);
  Serial.print("pitch: ");
  Serial.println(pitch);
  Serial.print("velocity: ");
  Serial.println(velocity);*/
  if (channel >= 0x90 && channel <= 0x9F) {
    Serial.write(channel);
    Serial.write(pitch);
    Serial.write(velocity);
    
  }
}
void noteOff(byte channel, byte pitch) {
  channel += 0x80 - 1;

  if (channel >= 0x80 && channel <= 0x8F) {
    Serial.write(channel);
    Serial.write(pitch);
    Serial.write(byte(0x00));
  }
}
void onNoteOn(int boardIndex, int row,  int cubeType) {
  /*Serial.print("boardIndex: ");
  Serial.println(boardIndex);
  Serial.print("row: ");
  Serial.println(row);
  Serial.print("cubeType: ");
  Serial.println(cubeType);*/
  noteOn(1, midiNotes[boardIndex][cubeType][row], 0x7F);
  //matrixStates[boardIndex][row][col] = cubeType;
}
void silencePrevNotes(int boardIndex, int row, int cubeType) {
  noteOff(1, midiNotes[boardIndex][cubeType][row]);
}

void midiControlSend(byte channel, byte ctrl, byte ctrl_value) {
  Serial.write(channel);
  Serial.write(ctrl);
  Serial.write(ctrl_value);
}

void setDelay1Value() {
  pot_delay1_value = analogRead(potDelay1);
  delay1_value = map(pot_delay1_value, 0, 1023, 0, 127);
  if (abs(delay1_value_last - delay1_value) > 1) {
    midiControlSend(0xB0, 92, delay1_value);
  }
  delay1_value_last = delay1_value;
}

void setDelay2Value() {
  pot_delay2_value = analogRead(potDelay2);
  delay2_value = map(pot_delay2_value, 0, 1023, 0, 127);
  if (abs(delay2_value_last - delay2_value) > 1) {
    midiControlSend(0xB0, 93, delay2_value);
  }
  delay2_value_last = delay2_value;
}

void setReverbValue() {
  pot_reverb_value = analogRead(potReverb);
  reverb_value = map(pot_reverb_value, 0, 1023, 0, 127);
  if (abs(reverb_value_last - reverb_value) > 1) {

    midiControlSend(0xB0, 91, reverb_value);

    /*lcd.setCursor(0, 1);
    lcd.print("REVERB: ");
    lcd.setCursor(8, 1);
    lcd.print(reverb_value);
    if (reverb_value < 100) {
      lcd.setCursor(10, 1);
      lcd.print(" ");
    }
    lcd.setCursor(13, 1);
    lcd.print("   ");

    prev_millis_fx_change = millis();
    fx_changed = true;*/
  }
  reverb_value_last = reverb_value;
}

void setTempoValue() {
  pot_tempo_value = analogRead(potTempo);
  // 500 tempo_delay_time --> 60 BPM
  // 166 tempo_delay_time --> 180 BPM
  tempo_delay_time = map(pot_tempo_value, 0, 1023, 500, 40); //MAP delay time (for BPM)
  tempo_delay_time_seconds = tempo_delay_time / 1000.0f;
  bpm = 60 / tempo_delay_time_seconds / 2;

  /*if ((millis() - prev_millis_fx_change > 750) || (!fx_changed)) {
    lcd.setCursor(0, 1);
    lcd.print("TEMPO: ");
    lcd.setCursor(13, 1);
    lcd.print("BPM");
    if (bpm < 100) {
      lcd.setCursor(9, 1);
      lcd.print(" ");
    }
    lcd.setCursor(7, 1);
    lcd.print(bpm);
    lcd.setCursor(10, 1);
    lcd.print("   ");
    fx_changed = false;
  }*/
}

void printNumberToLCD(int num) {
  if (num > 99) {
    lcd.print(num);
  } else if(num > 9) {
    lcd.print(" ");
    lcd.print(num);
  } else {
    lcd.print("  ");
    lcd.print(num);
  }
}

void printInfoToLCD() {
  lcd.setCursor(0, 0); //stulpelis, eilutė
  lcd.print("T:");
  printNumberToLCD(bpm);
  lcd.print("  ");
  lcd.print("R:");
  printNumberToLCD(reverb_value);
  lcd.setCursor(0, 1); //stulpelis, eilutė
  lcd.print("D1:");
  printNumberToLCD(delay1_value);
  lcd.print(" ");
  lcd.print("D2:");
  printNumberToLCD(delay2_value);
}

void onTick() {
  int boardIndex = 0;
  /*int activeChange = 0;
  if (activeRow < 8) {
    boardIndex = 0;
    getRawData(0x31);
    activeChange = 0;
  } else {
    boardIndex = 1;
    getRawData(0x33);
    activeChange = 8;
  }*/
  if (millis() - prev_millis >= tempo_delay_time) {
    prev_millis = millis();
    // on beat do stuff
    //int boardIndex = 0;
    //onNoteOff(boardIndex, activeRow);
    //onNoteOn(boardIndex, activeRow, cubeType);

    setReverbValue();
    setDelay1Value();
    setDelay2Value();
    
    for (int i = 0; i < 5; i++) {
      //atkomentuoti, kai nebetestuosiu su 2 lentom:
      cubeType = getActiveCube(raw_seq_col_data[i][7 - activeRow], seq_col_data[boardIndex][i][7 - activeRow]);
      //cubeType = getActiveCube(raw_seq_col_data[i][7 - (activeRow-activeChange)], seq_col_data[boardIndex][i][7 - (activeRow-activeChange)]);
      //onNoteOnPrint(boardIndex, activeRow, cubeType);
      int cubType;
      if (cubeType > 5.0) {
        cubType = 0;
        //onNoteOn(boardIndex, cubType, i);
        onNoteOn(boardIndex, i, cubType);
        delayMicroseconds(10);
        //silencePrevNotes(boardIndex, cubType, i);
        silencePrevNotes(boardIndex, i, cubType);
        delayMicroseconds(10);
      } else if (cubeType < -5.0) {
        cubType = 1;
        //onNoteOn(boardIndex, cubType, i);
        onNoteOn(boardIndex, i, cubType);
        delayMicroseconds(10);
        //silencePrevNotes(boardIndex, cubType, i);
        silencePrevNotes(boardIndex, i, cubType);
        delayMicroseconds(10);
      } 

      //onNoteOn(boardIndex, activeRow, cubType);
      //silencePrevNotes(boardIndex, activeRow, cubType);
    }

    for(int l = 0; l < NUM_LEDS; l++) {
      leds[l] = CRGB::Blue;  // You can change CRGB::White to any color (e.g., CRGB::Red, CRGB::Green, etc.)
    }
    
    //if (activeRow < 8) {
      leds[activeRow * 3 + 0] = CRGB::Orange; //CRGB(120, 255, 0);
      leds[activeRow * 3 + 1] = CRGB::Orange; //CRGB(120, 255, 0);
      leds[activeRow * 3 + 2] = CRGB::Orange; //CRGB(120, 255, 0);
    /*} else {
      leds2[(activeRow-activeChange) * 3 + 0] = CRGB(120, 255, 0);
      leds2[(activeRow-activeChange) * 3 + 1] = CRGB(120, 255, 0);
      leds2[(activeRow-activeChange) * 3 + 2] = CRGB(120, 255, 0);
    }*/
    FastLED.show();
    FastLED.clear();
    activeRow++;
    if (activeRow > 7) { //15
      activeRow = 0;
    }
    printInfoToLCD();
  }
}
void setup() {
  lcd.init();          // Initiate the LCD module
  lcd.backlight();     // Turn on the backlight
  lcd.setCursor(0, 0); //stulpelis, eilutė
  lcd.print("LinkMenuMusicBox:");
  lcd.setCursor(0, 1);
  lcd.print("Social Sequencer");
  
  pinMode(potTempo, INPUT); //Sets the pinmode to input
  pinMode(potReverb, INPUT); //Sets the pinmode to input
  pinMode(resetButton, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(resetButton), resetArduino, FALLING);  // Interrupt on button press (falling edge)
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(31250);  // start serial for MIDI output
  //Serial.begin(115200);  // start serial for Monitor output
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  //FastLED.addLeds<WS2812B, LED2_PIN, GRB>(leds2, NUM_LEDS);  // GRB ordering is typical
  
  delay(1000);
  storeRawData();
  delay(1000);
  lcd.clear();
}

void loop() {

  /*if (shouldReset) {
    cli(); // Disable interrupts just in case
    wdt_enable(WDTO_15MS); // Trigger watchdog reset safely
    while (1) {
      // Wait for watchdog to reset the Arduino
    }
  }*/

  setTempoValue();

  //pot_reverb_value = analogRead(potReverb);
  //Serial.println(pot_reverb_value);
  //delay(200);
  
  //getRawData(0x31);
  getRawData(0x33);
  onTick();
}
