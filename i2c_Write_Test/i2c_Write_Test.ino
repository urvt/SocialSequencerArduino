#include <Wire.h>
#include "Type4051Mux.h"
/******** DEFINES ************/
#define SLAVE_ADDR 85
#define REG_MAP_SIZE 5
#define MAX_SENT_BYTES 8
/***** Global Variables ******/
byte registerMap[REG_MAP_SIZE];
byte byteArray[REG_MAP_SIZE];
//byte dataArray[5][MAX_SENT_BYTES]; // 8 bytes
byte dataArray[5][8];
int newArray[5][8];
Type4051Mux muxMain(A0, INPUT, ANALOG, 9, 8 , 7);
Type4051Mux muxSub(A1, INPUT, ANALOG, 6, 5 , 4);
void setup() {

  // put your setup code here, to run once:
  Wire.begin(SLAVE_ADDR);
  Serial.begin(115200);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {
  //Serial.println(byteArray[0]);
  //delay(250);
  for (int i = 0; i < 8; i ++) {
    muxSub.setChannel(i);
    for (int j = 0; j < 5; j++) {
      dataArray[j][i] = map(muxMain.read(j), 0, 1023, 0 , 255);
      delayMicroseconds(1);
      //Serial.print("  ");
      //Serial.print(muxMain.read(i));
    }
    delayMicroseconds(10);
  }
  //int x = dataArray[1][0];
  //Serial.println(x);
  //delay(250);
}

void requestEvent() {

  switch (byteArray[0]) {
    case 0x01:
      Wire.write(dataArray[0], MAX_SENT_BYTES);
      break;
    case 0x02:
      Wire.write(dataArray[1], MAX_SENT_BYTES);
      break;
    case 0x03:
      Wire.write(dataArray[2], MAX_SENT_BYTES);
      break;
    case 0x04:
      Wire.write(dataArray[3], MAX_SENT_BYTES);
      break;
    case 0x05:
      Wire.write(dataArray[4], MAX_SENT_BYTES);
      break;
  }
  // Wire.write(registerMap, sizeof(registerMap));
}
void receiveEvent(int numBytes) {
  while (0 < Wire.available()) {
    for (int i = 0; i < numBytes; i++) {
      byteArray[i] = Wire.read();
    }
  }
}
