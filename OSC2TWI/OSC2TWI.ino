/**
 * This program reads in OSC data via an ethernet shield, and then spits out the
 * necessary i2c data to an arduino that simply converts that i2c into DMX commands/scene changes
 */
#include <SPI.h>
#include <Ethernet.h> // Tested in version IDE 0022 Arduino UNO
#include "Z_OSC.h"
#include "project_defs.h"

Z_OSCServer server;
Z_OSCMessage *rcvMes;

#include <Wire.h>

#define DEBUG 0

// Variable to set value for Digital pin 3 to pwm. This gets written via i2c. Do no more than 16 bytes if possible
// also set starting values
byte i2cArray[16] = {AREA_GLOBAL,255,255,255,127,1,0,0,0,0,0,0,0,0,0,0};

/**
 *  Configure ethernet, wire, serial port etc.
 */
void setup() {
    // ETHERNET SETUP
    byte myMac[] = {   0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    //  SET MAC ADDRESS
    byte myIp[]  = { 192, 168, 11, 99 };                            //  SET IP ADDRESS
    int  serverPort  = 8000;                                    //  SET OSC SERVER PORT

    //Pins 4,10 are used for the Ethernet shield . Adding pins 8 and 9 for use made this code buggy.
    Ethernet.begin(myMac ,myIp);
    server.sockOpen(serverPort);

    // i2c SETUP
    Wire.begin(2);                // join i2c bus with address #2
    Wire.onRequest(requestEvent); // register event

    if(DEBUG) Serial.begin(19200);
}

/**
 *  The loop the arduino will run infinitely
 */
void loop() {
  if(server.available()){
      rcvMes=server.getMessage();
      if(DEBUG) logMessage();

      //  LoungeLight template controls

      //  Area picker
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/1" ) )
      { i2cArray[0] =  AREA_GLOBAL; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/2" ) )
      { i2cArray[0] =  AREA_STANDUP_KIT; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/3" ) )
      { i2cArray[0] =  AREA_DRUM_RISERS; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/4" ) )
      { i2cArray[0] =  AREA_DRUMS_OVERHEAD; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/5" ) )
      { i2cArray[0] =  AREA_HALFWALL; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/6" ) )
      { i2cArray[0] =  AREA_DJ_BOOTH; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/7" ) )
      { i2cArray[0] =  AREA_AUX1; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/area/1/8" ) )
      { i2cArray[0] =  AREA_AUX2; }

      //  Area Parameters
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/hue" ) )
      { i2cArray[1] = (rcvMes->getFloat(0)*255.0); }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/saturation" ) )
      { i2cArray[2] = (rcvMes->getFloat(0)*255.0); }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/brightness" ) )
      { i2cArray[3] = (rcvMes->getFloat(0)*255.0); }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/speed" ) )
      { i2cArray[4] = (rcvMes->getFloat(0)*255.0); }

      //  Scene selection
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/3/1" ) )
      { i2cArray[5] =  1; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/3/2" ) )
      { i2cArray[5] =  2; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/3/3" ) )
      { i2cArray[5] =  3; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/3/4" ) )
      { i2cArray[5] =  4; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/3/5" ) )
      { i2cArray[5] =  5; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/2/1" ) )
      { i2cArray[5] =  6; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/2/2" ) )
      { i2cArray[5] =  7; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/2/3" ) )
      { i2cArray[5] =  8; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/2/4" ) )
      { i2cArray[5] =  9; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/2/5" ) )
      { i2cArray[5] =  10; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/1/1" ) )
      { i2cArray[5] =  11; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/1/2" ) )
      { i2cArray[5] =  12; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/1/3" ) )
      { i2cArray[5] =  13; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/1/4" ) )
      { i2cArray[5] =  14; }
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene/1/5" ) )
      { i2cArray[5] =  15; }

      //rotate scene toggle
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene_rotate" ) )
      { i2cArray[6] =  (int)(rcvMes->getFloat(0)*255.0); }

      //blank on scene change toggle
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/2/scene_blank" ) )
      { i2cArray[7] =  (rcvMes->getFloat(0)*255.0); }

      //system off toggle
      if( !strcmp( rcvMes->getZ_OSCAddress() ,  "/1/system_off" ) )
      { i2cArray[8] =  (rcvMes->getFloat(0)*255.0); }

  }
}

//Log maker
void logMessage() {
  uint16_t i;
  byte *ip=rcvMes->getIpAddress();

  long int intValue;
  float floatValue;
  char *stringValue;

  Serial.print(ip[0],DEC);
  Serial.print(".");
  Serial.print(ip[1],DEC);
  Serial.print(".");
  Serial.print(ip[2],DEC);
  Serial.print(".");
  Serial.print(ip[3],DEC);
  Serial.print(":");

  Serial.print(rcvMes->getPortNumber());
  Serial.print(" ");
  Serial.print(rcvMes->getZ_OSCAddress());
  Serial.print(" ");
  Serial.print(rcvMes->getTypeTags());
  Serial.print("--");

  for(i=0 ; i<rcvMes->getArgsNum(); i++){

    switch( rcvMes->getTypeTag(i) ){

    case 'i':
      intValue = rcvMes->getInteger32(i);

      Serial.print(intValue);
      Serial.print(" ");
      break;


    case 'f':
      floatValue = rcvMes->getFloat(i);

      Serial.print(floatValue);
      Serial.print(" ");
      break;


    case 's':
      stringValue = rcvMes->getString(i);

      Serial.print(stringValue);
      Serial.print(" ");
      break;

    }


  }
  Serial.println("");
}

/**
 *  Function that executes whenever data is requested by master
 *  this function is registered as an event, see setup()
 */
void requestEvent() {
   Wire.write(i2cArray, 16); // respond with message of 16 bytes, as expected by master
}

void printI2CArray() {
  Serial.print("{");
  for(int x=0; x<16; x++) {
    Serial.print(i2cArray[x]);
    Serial.print(",");
  }
  Serial.println("}");
}
