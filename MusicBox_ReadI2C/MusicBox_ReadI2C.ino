// Written by Nick Gammon
// May 2012

#include <Wire.h>
#include <I2C_Anything.h>

const byte MY_ADDRESS = 42;

void setup()
{
  Wire.begin (MY_ADDRESS);
  Serial.begin (115200);
  Wire.onReceive (receiveEvent);
}  // end of setup

volatile boolean haveData = false;

volatile int dataArray[8][8];
volatile int data;

void loop()
{
  if (haveData)
  {
    Serial.println("Received data: ");
    for(int i = 0; i < 5; i++){
      for (int j =0; j < 8;j++){
        Serial.print(dataArray[i][j]);
        Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println("----------------");
    haveData = false;
  }  // end if haveData

}  // end of loop

// called by interrupt service routine when incoming data arrives
void receiveEvent (int howMany)
{
  if (howMany >= data)
  {
    for(byte i = 0; i < 8 ; i++){
      I2C_readAnything(data);
      Serial.print(data);
      Serial.print(" "); 
    }
    Serial.println();
     for(byte i = 0; i < 8 ; i++){
      I2C_readAnything(data);
      Serial.print(data);
      Serial.print(" "); 
    }
    Serial.println();
     for(byte i = 0; i < 8 ; i++){
      I2C_readAnything(data);
      Serial.print(data);
      Serial.print(" "); 
    }
    //I2C_readAnything (data);
    //I2C_readAnything (data2);
  haveData = false;
    
    

  }  // end if have enough data
}  // end of receiveEvent
