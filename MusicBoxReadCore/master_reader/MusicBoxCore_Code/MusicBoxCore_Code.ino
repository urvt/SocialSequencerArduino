#include <Wire.h>
#include <FastLED.h>

#define DATA_PIN 6
#define NUM_LEDS 24
#define BOARD_COUNT 2
#define CUBE_TYPE_COUNT 2
#define ROW_COUNT 5
#define BPM 500L // 500ms = 120BPM
CRGB leds[NUM_LEDS];

const byte midiNotes[][CUBE_TYPE_COUNT][ROW_COUNT] = {

  // Board 0
  {
    {0,  1,  2,  3,  4}, // positive cube
    {5,  6,  7,  8,  9} // negative cube
  },

  // Board 1
  {
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}
  },
  // Board 2
  {
    {40, 41, 42, 43, 44},
    {45, 46, 47, 48, 49}
  }

};
// cmd 0x04 - 0x08
uint8_t NUM_BOARDS = 1;
uint16_t i2cData[8];
boolean i2cLeds[5][8];
uint8_t index = 0;
uint16_t seq_col_data[3][5][8];
uint16_t raw_seq_col_data[5][8];
uint8_t cmd(0), cmd_len(1);
uint8_t SlaveAddr[3] = {0x31, 0x32, 0x33};
unsigned long prev_millis = 0;
unsigned long current_millis = 0;
int id = 0;
uint8_t activeRow = 0;
double cubeType = 0;
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
  for (int k = 0; k < NUM_BOARDS; k++) {
    getRawData(SlaveAddr[k]);
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 8; j++) {
        seq_col_data[k][i][j] = raw_seq_col_data[i][j];
      }
    }
  }
}
int getCubeType(int rawAnalogVal, int storedAnalogVal) {
  if (rawAnalogVal > storedAnalogVal) {
    return 3;
  } else if (rawAnalogVal < storedAnalogVal) {
    return 2;
  } else {
    return 0;
  }
}

double getActiveCube(double NewVal, double OldVal) {
  double val;
  val = ((NewVal - OldVal) / abs(OldVal)) * 100;
  return val;
}
void noteOn(byte channel, byte pitch, byte velocity) {
  channel += 0x90 - 1;

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
  noteOn(1, midiNotes[boardIndex][cubeType][row], 0x7F);
  //matrixStates[boardIndex][row][col] = cubeType;
}
void silencePrevNotes(int boardIndex, int row, int cubeType) {
  noteOff(1, midiNotes[boardIndex][cubeType][row]);
}
void onNoteOnPrint(int boardIndex, int activeRow, double cubeType) {
  Serial.print("BoardIndex:  ");
  Serial.print(boardIndex);
  Serial.print(" Active row:  ");
  Serial.print(activeRow);
  Serial.print(" CubeType:  ");
  if (cubeType > 5.0) {
    Serial.println("+1");
    //Serial.println(cubeType);
  } else if (cubeType < -5.0) {
    Serial.println("-1");
    //Serial.println(cubeType);
  } else {
    Serial.println("0");
  }
}
void onTick() {
  if (millis() - prev_millis >= BPM) {
    prev_millis = millis();
    // on beat do stuff
    int boardIndex = 1;
    //onNoteOff(boardIndex, activeRow);
    //onNoteOn(boardIndex, activeRow, cubeType);
    for (int i = 0; i < 5; i++) {
      //cubeType = getActiveCube(raw_seq_col_data[i][activeRow], seq_col_data[boardIndex][i][activeRow]);
      cubeType = getActiveCube(raw_seq_col_data[i][7 - activeRow], seq_col_data[boardIndex][i][7 - activeRow]);
      //onNoteOnPrint(boardIndex, activeRow, cubeType);
      int cubType;
      if (cubeType > 5.0) {
        cubType = 0;
        onNoteOn(boardIndex, cubType, i);
        delayMicroseconds(10);
        silencePrevNotes(boardIndex, cubType, i);
        delayMicroseconds(10);
      } else if (cubeType < -5.0) {
        cubType = 1;
        onNoteOn(boardIndex, cubType, i);
        delayMicroseconds(10);
        silencePrevNotes(boardIndex, cubType, i);
        delayMicroseconds(10);
      } 

      //onNoteOn(boardIndex, activeRow, cubType);
      //silencePrevNotes(boardIndex, activeRow, cubType);
    } 
    leds[activeRow * 3 + 0] = CRGB(120, 0, 0);
    leds[activeRow * 3 + 1] = CRGB(120, 0, 0);
    leds[activeRow * 3 + 2] = CRGB(120, 0, 0);
    FastLED.show();
    FastLED.clear();
    activeRow++;
    if (activeRow > 7) {
      activeRow = 0;
    }
  }
}
void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(115200);  // start serial for output
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  delay(1000);
  storeRawData();
  delay(1000);
}

void loop() {
  getRawData(0x31);
  onTick();

  /*
    Serial.println("Stored Values");
    for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 8; j++) {
      //Serial.print(seq_col_data[1][i][j]);
      //Serial.print(raw_seq_col_data[i][j]);
      double errorPerc = getActiveCube(raw_seq_col_data[i][j], seq_col_data[1][i][j]);
      if (errorPerc > 5.0) {
        // cube with dot
        Serial.print(1);
      } else if (errorPerc < -5.0) {
        // cube with hole
        Serial.print(-1);
      } else {
        Serial.print(0);
      }
      Serial.print(" ");
    } Serial.println();
    } Serial.println("------------------");
    delay(400);
  */
}
