/*
   Sample I2C Slave project organization.

   Copyright (C) 2019 Pat Deegan, https://psychogenic.com/

   Described fully, with more details, at
   https://inductive-kickback.com/2019/04/creating-an-i2c-slave-interface-for-a-sensor-or-peripheral/

   It includes:
 *  * setup of Wire (I2C) Slave
 *  * callbacks for writes and requests from master
 *  * sample I2C Peripheral class that handles commands
      and sends responses of various lengths

   The setup:
     We have a slave device that expects to receive the
     occasional request or command, via an I2C write to
     it's address.

     All writes (from master) will look like
       COMMANDBYTE [ADDITIONAL PAYLOAD...]
     so that the first byte indicates the register we want
     to read, or the command/setting we're sending over.

     All requests for bytes from master will return some list of bytes...
     Just how many we send to the out-buffer here will depend on previous
     writes from the master--the main thing is that master and slave agree
     on an API and stick to it.  Here master will always:
       - write some command
       - request bytes to see the result/response


     COMMANDBYTEs defined here are:
      0x00: get status register (a single byte command)
      0x01: set the "time" (a 32 bit uint) (a command byte, followed by 4 payload bytes)
      0x02: get the time (a single command byte)
      0x03: set Row
      0x04: get 1st column values
      0x05: get 2nd column values
      0x06: get 3th column values
      0x07: get 4th column values
      0x08: get 5th column values

     after a given command, a readrequest will be made for:
      CMD : num bytes written back to master
      0x00: 1 byte
      0x01: 1 byte
      0x02: 4 bytes
      0x03: 1 byte
      0x04: 16 bytes
      0x05: 16 bytes
      0x06: 16 bytes
      0x07: 16 bytes
      0x08: 16 bytes

*/
/* Include the I2C library */
#include <Wire.h>
#include "Type4051Mux.h"

Type4051Mux muxMain(A0, INPUT, ANALOG, 9, 8, 7);
Type4051Mux muxSub(A1, INPUT, ANALOG, 6, 5, 4);

/* Select a slave address,
    - 7 < address < 120
    - something not used by another component on the board!
*/
#define MY_SLAVE_ADDRESS  0x32
#define RCV_COMMAND_MAX_BYTES   20

/* SlaveResponse: just a little container
   to hold a byte buffer and it's length
*/
typedef struct SlaveResponseStruct {
  uint8_t * buffer;
  uint8_t size;
  SlaveResponseStruct() : buffer(NULL), size(0) {}
  SlaveResponseStruct(uint8_t * buf, uint8_t len) : buffer(buf), size(len) {}
} SlaveResponse;


/* MyI2CPeripheral
   A class to centralize our peripheral's state and message processing.
*/
class MyI2CPeripheral {
  public:
    MyI2CPeripheral() : current_register(0), status_value(0x10), some_value(0x123456) {}

    /*
       expectedReceiveLength(REGISTERID)
       Returns the number of bytes to receive for a given
       command or register, REGISTERID
    */
    uint8_t expectedReceiveLength(uint8_t forRegister) {
      switch (forRegister) {
        case 0: /* get status */
          return 1;
        case 1: /* set time: uint32_t */
          return 5; // 4 bytes for the 32bits int, and the command byte itself
        case 2: /* get time */
          return 1;
        case 3:
          return 1;
        case 4:
          return 1;
        case 5:
          return 1;
        case 6:
          return 1;
        case 7:
          return 1;
        case 8:
          return 1;
        default:
          return 0; // unknown command
      }
    }




    /*
       process(BUFFER, BUFLEN)
       Process incoming data from master.  According to our
       protocol, the first byte in this buffer will be the register or command id.
       Subsequent bytes depend on the situation/register specifics, but this
       method will only be triggered once enough bytes have been received for
       the given register (as specified by expectedReceiveLength() above).
    */
    void process(volatile uint8_t * buffer, uint8_t len) {
      // keep track of last register
      // sent/accessed for any read requests later
      current_register = buffer[0];
      // etc etc
      // do interesting things based on contents of
      // current_register and rest of buffer

      // here we just keep incrementing the status
      status_value++;
      // scan the values
      setMuxRows();
    }



    /*
       doThings() -- any quick scan/sense/activity you need
       performed on every loop
    */
    void doThings() {
      // do some interesting things on each loop
      //some_value++; // whatever
      // debug
      //setMuxRows();
      //printMuxValues();
      //delay(1000);
    }
    uint16_t getMuxVal(uint8_t setCol, uint8_t setRow) {
        //uint16_t muxVal;
        muxSub.setChannel(setRow);
        muxMain.setChannel(setCol);
        muxVal = muxMain.read(); 
        return muxVal;
    }
    void setMuxRows(){
      for(int i = 0; i < 8; i++){
        seq_col_1[i] = getMuxVal(0, i);
        seq_col_2[i] = getMuxVal(1, i);
        seq_col_3[i] = getMuxVal(2, i);
        seq_col_4[i] = getMuxVal(3, i);
        seq_col_5[i] = getMuxVal(4, i);
      }
    }

    void printMuxValues(){
        Serial.println("---------------------");
      for(int i=0;i<8;i++){
        Serial.print(seq_col_1[i]);
        Serial.print(" ");
        Serial.print(seq_col_2[i]);
        Serial.print(" ");
        Serial.print(seq_col_3[i]);
        Serial.print(" ");
        Serial.print(seq_col_4[i]);
        Serial.print(" ");
        Serial.print(seq_col_5[i]);
        Serial.println();
      } Serial.println("---------------------");
    }

    /*
       getResponse -- returns an appropriate buffer, and its length,
       according to whatever commands have been received prior, or defaults.

       The idea is to maintain intelligence and state in this object, and
       simply have the any request event triggered be able to say:
       "give me whichever response is appropriate, and I'll send it over
       to the master"
    */
    SlaveResponse getResponse() {

      // just doing whatever, here
      SlaveResponse resp;

      // main idea is to return a pointer to
      // a buffer that's perhaps somehow related
      // to the last processed command.

      // NOTE: the important thing is that this buffer
      // still be around/valid after the method call...
      switch (current_register) {
        // in real life, you'd probably have one or more
        // uint8_t buffer[N], fill them with interesting things
        // and return pointers to said buffers, but here
        // I'm being lazy and just forcing integer values to
        // be treated as the returned buffer
        case 0:
          resp.buffer = &status_value;
          resp.size = 1;
          break;
        case 1:
          resp.buffer = &status_value;
          resp.size = 1;
          break;
        case 2: /* get time */
          resp.buffer = (uint8_t*) &some_value;
          resp.size = 4;
          break;
        // Music Box raw analog data
        // 8 diff values in 5 rows total 40
        // sending as 2 bytes in total 10 bytes
        case 3: // set row
          resp.buffer = 0;
        case 4:
          resp.buffer = (uint8_t*) &seq_col_1;
          resp.size = 2 * 8;
          // 2 * 5 because sending two bytes and array const of 8 values
          // should be: 2*seqArraySize;
          // or: 2 * ((sizeof(seqArraySize) / sizeof(seqArraySize[0]))
          // I'm too lazy i think...
          break;
        case 5:
          resp.buffer = (uint8_t*) &seq_col_2;
          resp.size = 2 * 8;
          break;
        case 6:
          resp.buffer = (uint8_t*) &seq_col_3;
          resp.size = 2 * 8;
          break;
        case 7:
          resp.buffer = (uint8_t*) &seq_col_4;
          resp.size = 2 * 8;
          break;
        case 8:
          resp.buffer = (uint8_t*) &seq_col_5;
          resp.size = 2 * 8;
          break;
        default:
          resp.buffer = (uint8_t*)"booya";
          resp.size = 5;
          break;

      }
      // you can use this spot to do things like
      // reset the current_register to some default
      // or do a little processing.
      // Just don't dawdle too much as your master
      // awaits your reply

      return resp;
    }

  private:
    uint8_t current_register;
    uint8_t status_value;
    uint32_t some_value;
    // Mux values in column(8values)
    uint16_t muxVal;
    uint16_t seq_col_1[8];
    uint16_t seq_col_2[8];
    uint16_t seq_col_3[8];
    uint16_t seq_col_4[8];
    uint16_t seq_col_5[8];
};





/*
   I2CDevice -- the global that represents this device and handles
   messages and state.
*/
MyI2CPeripheral I2CDevice;



/*
    i2cRequestEvent --
    this will be called when the master is asking to get some data
    from our peripheral device.
*/
void i2cRequestEvent()
{
  // Transfer request

  // get the response
  SlaveResponse resp = I2CDevice.getResponse();

  // write it to the out buffer
  Wire.write(resp.buffer, resp.size);

  //Serial.println(resp.buffer[0]);
  // important thing to remember is that, for request events,
  // the Wire.write should happen *here* in the callback.
  // If you lollygag, or try do set some flag and handle the
  // write() later in your main loop(), the master will probably
  // just see a bunch of 0xff bytes and you'll get out of sync.

}



/* Two sets of buffers...
    receivedBytes[] will store incoming bytes as they are accumulated

    pendingCommand[] will hold commands as they come in from the master
    and be processed in our main loop.

   We're using a little double buffering type deal here to keep
   the interrupt routine simple and "safely" process the writes in
   the main loop.

   This is still subject to a race condition if the master is
   writing bytes too fast, but we're operating under the assumption
   that each write will be followed for a request for a response
   and enough of a delay to allow processing to take place.
*/
volatile uint8_t receivedBytes[RCV_COMMAND_MAX_BYTES];
volatile uint8_t receivedByteIdx = 0;


// pendingCommand buffer and len
// volatile because it's the way we're talking between
// our interrupt and our main 'thread'--either side may
// change the values at will
volatile uint8_t pendingCommand[RCV_COMMAND_MAX_BYTES];
volatile uint8_t pendingCommandLength = 0;


/* i2cReceiveEvent
   Called when the master sends us bytes.
   The important thing to note is that we may not get
   all the bytes we need in a single call, and the number of
   bytes we expect depends on what is actually being transmitted.

   So i2cReceiveEvent does a little footwork to be able to send
   packets to our global I2CDevice only when we have complete
   messages.

   It also arranges to have messages processed in the main loop, rather
   than in this callback.
*/
void i2cReceiveEvent(int bytesReceived)

{
  uint8_t msgLen = 0;

  // loop over each incoming byte
  for (int i = 0; i < bytesReceived; i++)
  {
    // stick that byte in our receive buffer
    receivedBytes[receivedByteIdx] = Wire.read();


    // now, we're sure we have _at least_ one byte in the buffer
    if (! msgLen) {
      // this was the first byte of a message, so we couldn't know the
      // expected message length until now...
      // ask our device what to expect:
      msgLen = I2CDevice.expectedReceiveLength(receivedBytes[0]);
    }

    receivedByteIdx++; /* increment in-byte counter */

    if (receivedByteIdx >= msgLen) {

      // we have a complete request/command in our buffer!

      // 1) copy that into our pending commands buffer
      for (uint8_t i = 0; i < msgLen; i++) {
        pendingCommand[i] = receivedBytes[i];
      }

      // 2) tell the main loop we've got something
      // of interest in pending cmd buffer
      pendingCommandLength = msgLen;

      // 3) zero our in-bytes buffer, to start
      // the next message
      receivedByteIdx = 0;

      // 4) zero our expected msgLen, so we'll refresh it
      // for the next command
      msgLen = 0;
    }

    // we keep accumulating bytes in receivedBytes[].
    // as long as we don't receive more than 1 _complete_
    // message per interrupt, we'll be good to actually process
    // all messages in the main loop

  }
}





/*
   standard setup() function
*/
void setup() {

  // SETUP wire
  Serial.begin(9600);
  Wire.begin(MY_SLAVE_ADDRESS);
  Wire.onRequest(i2cRequestEvent);
  Wire.onReceive(i2cReceiveEvent);

  // do other setup you may need...

}


/*
   main loop.
   Here, we'll just loop around and handle
   pending commands when they come in.
*/
void loop() {

  if (pendingCommandLength) {
    // oh my, we've received a command!

    // if you're going to be very slow in processing this,
    // you could copy the contents of pendingCommand[]
    // over to yet another buffer.
    // Here we just do it "real time" for simplicity

    // 1) process that command
    I2CDevice.process(pendingCommand, pendingCommandLength);
    //Serial.println(pendingCommand[0]);
    //SlaveResponse resp = I2CDevice.getResponse();
    /*for (int i = 0; i < resp.size; i++) {
      Serial.print(resp.buffer[0]);
      Serial.print(" ");
      } Serial.println();*/
    // 2) zero that flag, so we don't process multiple times
    pendingCommandLength = 0;

  }

  // do anything else that needs doin'
  I2CDevice.doThings();

  // main thing is that you must process any pendingCommand
  // before the next full command bytes come in through the wire



}
