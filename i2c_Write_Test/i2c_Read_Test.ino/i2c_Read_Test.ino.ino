#include <Wire.h>
int SLAVE_ADDR = 85;
int receivedByte_1;
int receivedByte_2;
int dataArray[5][8];
int avgDataArray[5][8];
int boardArray[5][8];
int prevBoardArray[5][8];
float one, two, tot, percent, diff;
void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(115200);
  delay(100);
}

void loop() {
  // put your main code here, to run repeatedly:
  int n = 0;
  for (byte i = 0x01; i <= 0x05; i++) {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(i);
    Wire.endTransmission();
    Wire.requestFrom(SLAVE_ADDR, 8);
    while (1 < Wire.available()) {
      for (int j = 0; j < 8; j++) {
        dataArray[n][j] = Wire.read();
        //Serial.println(dataArray[i][j]);
      }
      n++;
      delay(1);
      // Debug // Serial.println(dataArray[n-1][0]);
    }
  }
  /*
    for (int x = 0; x < 5; x++) {
    for (int y = 0; y < 8; y++) {
      Serial.print(dataArray[x][y]);
      Serial.print(" ");
    }
    Serial.println();
    }
    Serial.println();
    delay(250);
  */
  if (millis() < 5000) {
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 8; j++) {
        avgDataArray[i][j] = dataArray[i][j];
      }
    }
    //Serial.println(avgDataArray[0][0]);
  }
  else {
    //Serial.println("Diff percent: ");
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 8; j++) {
        one = avgDataArray[i][j];
        two = dataArray[i][j];
        percent = (((one - two)) / ((one + two) / 2)) * 100;
        //percent = ((abs(diff) / tot) * 100);
        if (percent > 5 && percent < 30) {
          //percent = -percent;
          percent = 1;
          boardArray[i][j] = 1;
          //noteOn(0x90, 25, 0x45);
          //delay(100);
          //noteOn(0x90, 0x5A, 0x45);
        } else if(percent < -5 && percent > -30) {
          percent = 2;
          boardArray[i][j] = 2;
        } else {
          //noteOn(0x90, , 0x00);
          percent = 0;
          boardArray[i][j] = 0;
        }
        //Serial.print(percent);
        //Serial.print(" ");
      }
      //Serial.println();
    }
    //Serial.println();
  }
  //delay(250);

  for(int i = 0; i < 5; i++){
    for(int j = 0; j < 8; j++){
      if(boardArray[i][j] != prevBoardArray[i][j]){
        
        if(boardArray[i][j] == 1){
          noteOn(0x90, 0x4A-j, 0x45);
        } else if(boardArray[i][j] == 0){
          noteOn(0x90, 0x4A-j, 0x00);
        }
        if(boardArray[i][j] == 2){
          noteOn(0x90, 0x4A+j, 0x45);
        } else if(boardArray[i][j] == 0){
          noteOn(0x90, 0x4A+j, 0x00);
        }
        prevBoardArray[i][j] = boardArray[i][j];
      }
    }
  }
}

void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}
