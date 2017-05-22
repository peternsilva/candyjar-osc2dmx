/**
 *  Responds to i2c control data, but also has all the lighting patterns written here
 */
#include <Wire.h>
#include <Math.h>
#include "DmxMaster.h"
#include "rgbdefs.h"
#include "project_defs.h"

#define DEBUG 0
#define NUM_SCENES 14

int systemOff = 0;
boolean gotoLoop = false;

int areaParams[NUM_LIGHTING_AREAS+1][NUM_AREA_PARAMS];   // Holds values of params for different lighting areas
int selectedArea = AREA_GLOBAL;
int selectedAreaOld = AREA_GLOBAL;
int selectedScene = 1;
int rotateScenes = 0;
int blankOnSceneChange = 0;

float globalSpeed = 1.0; //[0.0 - 2.0]
int h2oSpeed = 10;
int h2oAddress = 5;
int swarm_wash = 255;
int swarm_derby = 255;
int swarm_lasers = 255;
int swarm_strobe = 255;
int swarm_sound_reactive = 0;
uint16_t cycleCount = 0;
uint8_t swarm_wash_color = 52;
uint8_t swarm_derby_color = 10;
uint8_t swarm_smd_strobe = 16;


// Control "pickup" takeover mode, so we don't have sudden jumps when interface values don't match system values for a control
boolean huePickedUp = false;
boolean saturationPickedUp = false;
boolean brightnessPickedUp = false;
int pickupWindow = 2;

/**
 *  Initial setup of the program prior to loop
 */
void setup() {
    randomSeed(analogRead(0));

    /*
    The most common pin for DMX output is pin 3, which DmxMaster
    uses by default. If you need to change that, do it here.
    */
    DmxMaster.usePin(4);

    // enable DE pin on dmx chip to set as an output
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);

    /*
    DMX devices typically need to receive a complete set of channels
    even if you only need to adjust the first channel. You can
    easily change the number of channels sent here. If you don't
    do this, DmxMaster will set the maximum channel number to the
    highest channel you writeDMX() to.
    */
    DmxMaster.maxChannel(MAX_DMX_CHANNELS);

    // join i2c bus (address optional for master)
    Wire.begin();
    if(DEBUG) Serial.begin(19200);

    // Find the ADJ H2O fixture, it's special
    for(int i=0; i<NUM_FIXTURES; i++) {
        if(fixtures[i][1] == FIXTURE_ADJ_H2O_3CH ) {
            h2oAddress = i;
        }
    }

    //reset areaParams
    for(int a=1; a<=NUM_LIGHTING_AREAS; a++) {
      for(int p=0; p<NUM_AREA_PARAMS; p++) {
        areaParams[a][p] = 255;
      }
    }

}

/**
 *  Main loop that always runs
 */
void loop() {

    gotoLoop = false;
    if( blankOnSceneChange ) blankAllFixtures();

    switch(selectedScene) {
        case 1:
            // Plasma
            fx_runPlasma(3000, 1, 12, false, 0, false, false);
            //does a broadness of anything but 12 cause the "asymptotic condition"?
            break;

        case 2:
            // Plasma w/ Saturation Flux
            fx_runPlasma(3000, 1, 12, false, 0, true, false);
            break;

        case 3:
            // Plasma w/ Saturation & Brightness Flux
            fx_runPlasma(3000, 1, 12, false, 0, true, true);
            break;

        case 4:
            // "Plasma pulse 1", heartbeatMode 1
            fx_runPlasma(3000, 1, 5, false, 1, false, false);
            break;

        case 5:
            // "Plasma pulse 2", heartbeatMode 1, Saturation Flux
            fx_runPlasma(3000, 1, 5, false, 1, true, false);
            break;

        case 6:
            // "Plasma pulse 3", heartbeatMode 2
            fx_runPlasma(3000, 1, 5, false, 2, false, false);
            break;

        case 7:
            // Red colored heartbeat, 1 random fixture at a time
            fx_redHeartbeats(10);
            break;

        case 8:
            // RGB heartbeat, 1 random fxiture at a time
            //setFixtureRGB(fixtures[h2oAddress][0], fixtures[h2oAddress][1], fixtures[h2oAddress][2], 255,255,255 );     //h2o just doesn't wanna pulse like the others, stuttery. Let it do it's own thing
            fx_rgbHeartbeats(200);
            break;

        case 9:
            // Color wipes, wheel
            fx_colorWipes(50);
            break;

        case 10:
            // basic r, g ,b on/off chase
            fx_rgbChase(100);
            break;

        case 11:
            // basic r, g ,b on/off chase
            fx_fadeChase(100);
            break;

        case 12:
            // Random strobe
            for(int i=0; i<5; i++){
              fx_randomStrobe(random(40,80),random(100,300));
            }
            break;

        case 13:
            // Noisy/glitchy
            fx_noisy(100,60,random(10)*3,random(10)*3,random(10)*3);
            break;

        case 14:
            // Solid color
            fx_solidColor();
            break;

        default:
            selectedScene = 1;
            break;
    }

    // rotate scenes
    if( rotateScenes ) nextScene();

}

/*************************************************************************************************
*  GENERAL SUPPORT FUNCTIONS
*************************************************************************************************/

/**
 *  Make a request for i2c data via Wire library
 */
//int oldselectedScene = selectedScene;
void requestWireData() {
    // request 16 bytes from slave device #2
    Wire.requestFrom(2, 16);

    int byteIndex = 0;
    // slave may send less than requested
    while( Wire.available() ) {
        byte val = Wire.read(); // receive a byte as character

        // Process bytes received
        switch (byteIndex) {
            case 0:
                // Area picker
                if (val != selectedArea) { huePickedUp = saturationPickedUp = brightnessPickedUp = false; }
                selectedArea = val;
                if (DEBUG) { Serial.print(", Area="); Serial.print(val); }
                break;

            case 1:
                // Area hue
                setSelectedAreaParam(AREA_PARAM_HUE, val);
                if (DEBUG) { Serial.print(", Hue="); Serial.print(val); }
                break;

            case 2:
                // Area saturation
                setSelectedAreaParam(AREA_PARAM_SATURATION, val);
                if (DEBUG) { Serial.print(", Saturation="); Serial.print(val); }
                break;

            case 3:
                // Area brightness
                setSelectedAreaParam(AREA_PARAM_BRIGHTNESS, val);
                if (DEBUG) { Serial.print(", Brightness="); Serial.print(val); }
                break;

            case 4:
                // Global speed [0.0 - 2.0]
                globalSpeed = ((float)(255-val)) / 127.0;
                if (DEBUG) { Serial.print(", Speed="); Serial.print(val); }
                break;

            case 5:
                // Scene picker
                if( selectedScene != val && !rotateScenes) { selectedScene = val; gotoLoop = true; }
                if (DEBUG) { Serial.print(", Scene="); Serial.print(selectedScene); }
                break;

            case 6:
                // Rotate scenes toggle
                if( rotateScenes != val ) rotateScenes = val;
                if (DEBUG) { Serial.print(", rotateScenes="); Serial.print(val); }
                break;

            case 7:
                // scene blank toggle
                if( blankOnSceneChange != val ) blankOnSceneChange = val;
                if (DEBUG) { Serial.print(", blankOnSceneChange="); Serial.print(val); }
                break;

            case 8:
                // System off toggle
                if( val ) {
                  //system off
                  selectedArea = AREA_GLOBAL;
                  setSelectedAreaParam(AREA_PARAM_BRIGHTNESS, 0);
                }
                if (DEBUG) { Serial.print(", systemOff="); Serial.println(val); }
                break;

            case 9:
                // Swarm Mode Wash Toggle
                if( swarm_wash != val ) swarm_wash = val;
                if (DEBUG) { Serial.print(", swarm_wash="); Serial.print(val); }
                break;

            case 10:
                // Swarm Mode Derby Toggle
                if( swarm_derby != val ) swarm_derby = val;
                if (DEBUG) { Serial.print(", swarm_derby="); Serial.print(val); }
                break;

            case 11:
                // Swarm Mode Lasers Toggle
                if( swarm_lasers != val ) swarm_lasers = val;
                if (DEBUG) { Serial.print(", swarm_lasers="); Serial.print(val); }
                break;

            case 12:
                // Swarm Mode Strobe Toggle
                if( swarm_strobe != val ) swarm_strobe = val;
                if (DEBUG) { Serial.print(", swarm_strobe="); Serial.print(val); }
                break;

            case 13:
                // Swarm Sound Reactive Toggle
                if( swarm_sound_reactive != val ) swarm_sound_reactive = val;
                if (DEBUG) { Serial.print(", swarm_sound_reactive="); Serial.print(val); }
                break;


        }

        if ( gotoLoop ) return;
        byteIndex++;
    }
}

/**
 *  Proxy function for writing DMX data. Let's me modify data before it gets sent out.
 */
void writeDMX(int dmxAddress, uint8_t value) {
    DmxMaster.write(dmxAddress, value);
}

/*
 *  Simple fcn to change scenes
 */
int nextScene() {
    selectedScene++;
    if( selectedScene > NUM_SCENES ) selectedScene = 1;
}

/*
 *  Shorthand to set the areaParams. Uses selectedArea to determine with area to work with
 */
void setSelectedAreaParam(int param, int val) {
  // if global, set this param for all areas
  if( selectedArea == AREA_GLOBAL ) {
    for(int a=1; a<=NUM_LIGHTING_AREAS; a++) {
      areaParams[a][param] = val;
    }
  } else {
    // Provides "pickup" takeover mode on HSV sliders
    // BASICALLY IF newVal == oldVal, flip a flag "matched" or somehting
    switch ( param ) {
      case AREA_PARAM_HUE:
        if ( val >= areaParams[selectedArea][param]-pickupWindow && val <= areaParams[selectedArea][param]+pickupWindow ) huePickedUp = true;
        if ( huePickedUp ) areaParams[selectedArea][param] = val;
        break;

      case AREA_PARAM_SATURATION:
        if ( val >= areaParams[selectedArea][param]-pickupWindow && val <= areaParams[selectedArea][param]+pickupWindow ) saturationPickedUp = true;
        if ( saturationPickedUp ) areaParams[selectedArea][param] = val;
        break;

      case AREA_PARAM_BRIGHTNESS:
        if ( val >= areaParams[selectedArea][param]-pickupWindow && val <= areaParams[selectedArea][param]+pickupWindow ) brightnessPickedUp = true;
        if ( brightnessPickedUp ) areaParams[selectedArea][param] = val;
        break;
    }
  }
}


/*************************************************************************************************
*  EFFECTS SUPPORT
*************************************************************************************************/

/**
* Renders a plasma field.
*/
int sparkleSkip, sparkleBrightness, randomPx;
void Plasma(int offset, float centerX, float frequency, boolean withSparkles, int heartbeatMode, boolean saturationFlux, boolean brightnessFlux) {
    int f, f2;
    int f3 = 10;
    boolean hbm2_flip = false;

    if ( heartbeatMode == 2 ) {
        offset = random(0, 360);

        while(true) {
            for(int x = 0; x < NUM_FIXTURES; x++)
            {
                float color = ((1.0 + sin(abs(x - 8.0 + centerX) / frequency + offset*3.14159/180)))/2.0;
                color = fmod(color+0.5,1.0);

                HSVType plasHSV;
                RGBType plasRGB;

                plasHSV.H = color*6.0;
                plasHSV.S = 1.0;
                plasHSV.V = 1.0;

                plasRGB = HSV_to_RGB(plasHSV);

                setFixtureRGB(fixtures[x][0], fixtures[x][1], fixtures[x][2], plasRGB.R*f3, plasRGB.G*f3, plasRGB.B*f3 );
                if ( gotoLoop ) return;
            }

            delay((int)(10 * globalSpeed));

            if (!hbm2_flip) f3++;
            if (f3 > 255) hbm2_flip = true;

            if (hbm2_flip) f3--;
            if (f3 < 10) { hbm2_flip = false; f3=10; break; }
        }
        //delay(1000);

        offset = random(0, 360);
        while(true) {
            for(int x = 0; x < NUM_FIXTURES; x++)
            {
                float color = ((1.0 + sin(abs(x - 8.0 + centerX) / frequency + offset*3.14159/180)))/2.0;
                color = fmod(color+0.8,1.0);

                HSVType plasHSV;
                RGBType plasRGB;

                plasHSV.H = color*6.0;
                plasHSV.S = saturationFlux ? color : 1.0;
                plasHSV.V = brightnessFlux ? color : 1.0;

                plasRGB = HSV_to_RGB(plasHSV);

                setFixtureRGB(fixtures[x][0], fixtures[x][1], fixtures[x][2], plasRGB.R*f3, plasRGB.G*f3, plasRGB.B*f3 );
                if ( gotoLoop ) return;
            }

            delay((int)(20 * globalSpeed));

            if (!hbm2_flip) f3++;
            if (f3 > 255) hbm2_flip = true;

            if (hbm2_flip) f3--;
            if (f3 < 10) { hbm2_flip = false; f3=10; break; }
        }
        //delay(1000);
    } else {
        for(int x = 0; x < NUM_FIXTURES; x++)
        {
            float color = ((1.0 + sin(abs(x - 8.0 + centerX) / frequency + offset*3.14159/180)))/2.0;

            HSVType plasHSV;
            RGBType plasRGB;

            //whatever the hue, shift it 0.3
            //color = fmod(color+0.5,1.0);

            //float rotate = (float)(offset/360);
            //color = fmod((color+rotate),1.0);

            plasHSV.H = color*6.0;
            plasHSV.S = saturationFlux ? color : 1.0;
            plasHSV.V = brightnessFlux ? color : 1.0;

            plasRGB = HSV_to_RGB(plasHSV);

            if( heartbeatMode == 1 ) {
                for ( f2=0; f2<2; f2++ ) {
                    for ( f=0; f<255; f++ ) {
                        setFixtureRGB(fixtures[x][0], fixtures[x][1], fixtures[x][2], plasRGB.R*f, plasRGB.G*f, plasRGB.B*f );
                        if ( gotoLoop ) return;
                        delay( globalSpeed>=1 ? (int)(1*globalSpeed) : 1 );
                    }
                    for ( f=255; f>=0; f-- ) {
                        setFixtureRGB(fixtures[x][0], fixtures[x][1], fixtures[x][2], plasRGB.R*f, plasRGB.G*f, plasRGB.B*f );
                        if ( gotoLoop ) return;
                        delay( globalSpeed>=1 ? (int)(1*globalSpeed) : 1 );
                    }
                    delay( globalSpeed>=1 ? (int)(1*globalSpeed) : 1 );
                }
            } else {
                setFixtureRGB(fixtures[x][0], fixtures[x][1], fixtures[x][2], plasRGB.R*255, plasRGB.G*255, plasRGB.B*255 );
                if ( gotoLoop ) return;
            }

            //delay(5);

            //sparkles disabled
            /*
            if( withSparkles && (sparkleSkip++ % 10 == 0) )
            {
              int smoothSensorValue = 255;
              sparkleBrightness = map(smoothSensorValue, 0,236, 31,0);
              for(int i=0; i<1; i++)
              {
                randomPx = random(0,NUM_PIXELS);

                Display[randomPx]=additiveBlend(  Display[randomPx],
                                                  Color(sparkleBrightness,sparkleBrightness,sparkleBrightness)
                                                  );
              }
            }
            */
        }
    }
}

/**
*  Convert HSVType color to RGBType color.
*/
float h, s, v, m, n, f;
int q;
RGBType RGB;
RGBType HSV_to_RGB( HSVType HSV ) {
    // H is given on [0, 6] or UNDEFINED. S and V are given on [0, 1].
    // RGB are each returned on [0, 1].
    h = HSV.H;
    s = HSV.S;
    v = HSV.V;

    //wrap around H = 23
    if (h>=6.0) {
     //subtract the greatest multiple of 6 that fits in H
     h -= floor(h/6)*6;
    }

    if (h == UNDEFINED) RETURN_RGB(v, v, v);
    q = floor(h);
    f = h - q;
    if ( !(q&1) ) f = 1 - f; // if i is even
    m = v * (1 - s);
    n = v * (1 - s * f);
    switch (q) {
        case 6:
        case 0: RETURN_RGB(v, n, m);
        case 1: RETURN_RGB(n, v, m);
        case 2: RETURN_RGB(m, v, n)
        case 3: RETURN_RGB(m, n, v);
        case 4: RETURN_RGB(n, m, v);
        case 5: RETURN_RGB(v, m, n);
    }
}

/**
*  Convert RGBType color to HSVType color.
*  r,g,b values are from 0 to 1
*  h = [0,6.0], s = [0,1], v = [0,1]
*  if s == 0, then h = -1 (undefined)
*/
float r, g, b, min, max, delta;
HSVType HSV;
HSVType RGB_to_HSV( RGBType RGB ) {
  r = RGB.R;
  g = RGB.G;
  b = RGB.B;

  min = MIN3( r, g, b );
  max = MAX3( r, g, b );

  v = max;       // v
  delta = max - min;
  if( max != 0 )
    s = delta / max;   // s
  else {
    // r = g = b = 0    // s = 0, v is undefined
    s = 0;
    h = -1;
    RETURN_HSV(h, s, v);
  }
  if( r == max )
    h = ( g - b ) / delta;   // between yellow & magenta
  else if( g == max )
    h = 2 + ( b - r ) / delta; // between cyan & yellow
  else
    h = 4 + ( r - g ) / delta; // between magenta & cyan
  h *= 60;       // degrees
  if( h < 0 )
    h += 360;

  RETURN_HSV( ((float)h)/60.0 , s, v);
}

/**
* Create a 15 bit color value from R,G,B. Preferred way to set pixels of Display array
*/
unsigned int Color(byte r, byte g, byte b) {
  //Take the lowest 5 bits of each value and append them end to end
  return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
}

/**
 *  Input a value 0 to 127 to get a color value.
 *  The colours are a transition r - g -b - back to r
 */
unsigned int Wheel(byte WheelPos) {
  byte r,g,b;
  switch(WheelPos >> 5)
  {
    case 0:
      r=31- WheelPos % 32;   //Red down
      g=WheelPos % 32;      // Green up
      b=0;                  //blue off
      break;
    case 1:
      g=31- WheelPos % 32;  //green down
      b=WheelPos % 32;      //blue up
      r=0;                  //red off
      break;
    case 2:
      b=31- WheelPos % 32;  //blue down
      r=WheelPos % 32;      //red up
      g=0;                  //green off
      break;
  }
  return(Color(r,g,b));
}

/**
 *  Utility function to black out everything
 */
void blankAllFixtures() {
      for(int i=0; i<NUM_FIXTURES; i++) {
          setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], 0,0,0);
          if ( gotoLoop ) return;
      }
}

/*************************************************************************************************
*  EFFECTS
*************************************************************************************************/

/**
*  Runs the plasma effect for a number of cycles, and sets initial variables
*  int      cycles        How many cycles to run for
*  int      effectSpeed   Speed of the movement [1-50]. Default is 2.
*  int      broadness     How broad or blunt the transition areas are, overall broadness of the effect [-100 to 100]. Default is 12.
*  boolean  withSparkles  Composite with randomStrobe effect, where brightness of "sparkles" controlled by a sensor
*/
void fx_runPlasma(int cycles, int effectSpeed, int broadness, boolean withSparkles, int heartbeatMode, boolean saturationFlux, boolean brightnessFlux) {
  int givenCycles = cycles;

   int ofs = 0;

   float centerX = 4.0;
   float centerF = 0;

   float incrX = (random(1,20)/100.0 - 0.1)/5.0;
   float incrF = (random(1,20)/100.0 - 0.1)/5.0;

   int incrcount = 0;

   while(cycles-- > 0) {
     ofs+= globalSpeed>=1 ? (int)(effectSpeed * globalSpeed) : 1;  //also speed, if greater incrementer used

     if (ofs >= 360) ofs = 0;

     Plasma(ofs,centerX, centerF + broadness, withSparkles, heartbeatMode, saturationFlux, brightnessFlux); //centerF + X, where X is "broadness" of the effect
     if ( gotoLoop ) return;
     delay((int)(50 * globalSpeed)); //speed

     centerX = centerX + incrX;
     centerF = centerF + incrF;

     if (abs(centerX) > 9.0) incrX = incrX*(-1.0);
     if (abs(centerF) > 10.0) incrF = incrF*(-1.0);

     if (incrcount++ > 500) {
       incrX = (random(1,20)/100.0 - 0.1)/5.0;
       incrF = (random(1,20)/100.0 - 0.1)/5.0;
       incrcount = 0;
     }
   }
}

/**
* random strobe different pixels
*/
void fx_randomStrobe(int rate, int blinks) {
    for (int i = 0; i<blinks; i++) {
      int randomLEDx = random(0, NUM_FIXTURES);

      setFixtureRGB(fixtures[randomLEDx][0], fixtures[randomLEDx][1], fixtures[randomLEDx][2], 255, 255, 255 );
      if ( gotoLoop ) return;

      delay((int)(rate*globalSpeed));

      for (int j1 = 0; j1 < NUM_FIXTURES; j1++) {
          setFixtureRGB(fixtures[j1][0], fixtures[j1][1], fixtures[j1][2], 0, 0, 0 );
          if ( gotoLoop ) return;
      }
      delay((int)(rate*globalSpeed));
    }
}

/**
* blink/Noisy color fx
*/
void fx_noisy(int blinks, int delayAmt, int red, int green, int blue) {
  for (int i = 0; i<blinks; i++) {
    for (int j1 = 0; j1 < NUM_FIXTURES; j1++) {
      int rndval = random(0,10);
      setFixtureRGB(fixtures[j1][0], fixtures[j1][1], fixtures[j1][2], rndval*red/10, rndval*blue/10, rndval*green/10 );
      if ( gotoLoop ) return;
    }
    delay((int)(delayAmt*globalSpeed));
  }
}

/**
 *  Heartbeat
 */
void fx_redHeartbeats(int delayAmt){
    int randFixture = random(0, NUM_FIXTURES);
    boolean doGlobal = (random(0, 3) == 1) ? true : false;

    //fade up
    for( int f=0; f<255; f++) {
        if( doGlobal ){
            for (int p=0; p<NUM_FIXTURES; p++) {
                setFixtureRGB(fixtures[p][0], fixtures[p][1], fixtures[p][2], f, 0, 0 );
            }
        } else {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+3)%NUM_FIXTURES][0], fixtures[(randFixture+3)%NUM_FIXTURES][1], fixtures[(randFixture+3)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+6)%NUM_FIXTURES][0], fixtures[(randFixture+6)%NUM_FIXTURES][1], fixtures[(randFixture+6)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+9)%NUM_FIXTURES][0], fixtures[(randFixture+9)%NUM_FIXTURES][1], fixtures[(randFixture+9)%NUM_FIXTURES][2], f, 0, 0 );
            delay((int)(delayAmt*globalSpeed));
        }
        if ( gotoLoop ) return;
        delay((int)(delayAmt/150*globalSpeed));
    }
    delay((int)(delayAmt*globalSpeed));

    //fade down
    for( int f=255; f>=0; f--) {
        if( doGlobal ){
            for (int p=0; p<NUM_FIXTURES; p++) {
                setFixtureRGB(fixtures[p][0], fixtures[p][1], fixtures[p][2], f, 0, 0 );
            }
        } else {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+3)%NUM_FIXTURES][0], fixtures[(randFixture+3)%NUM_FIXTURES][1], fixtures[(randFixture+3)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+6)%NUM_FIXTURES][0], fixtures[(randFixture+6)%NUM_FIXTURES][1], fixtures[(randFixture+6)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+9)%NUM_FIXTURES][0], fixtures[(randFixture+9)%NUM_FIXTURES][1], fixtures[(randFixture+9)%NUM_FIXTURES][2], f, 0, 0 );
            delay((int)(delayAmt*globalSpeed));
        }
        if ( gotoLoop ) return;
        delay((int)(delayAmt/150*globalSpeed));
    }

    delay((int)(delayAmt*2*globalSpeed));

    //fade up
    for( int f=0; f<255; f++) {
        if( doGlobal ){
            for (int p=0; p<NUM_FIXTURES; p++) {
                setFixtureRGB(fixtures[p][0], fixtures[p][1], fixtures[p][2], f, 0, 0 );
            }
        } else {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+3)%NUM_FIXTURES][0], fixtures[(randFixture+3)%NUM_FIXTURES][1], fixtures[(randFixture+3)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+6)%NUM_FIXTURES][0], fixtures[(randFixture+6)%NUM_FIXTURES][1], fixtures[(randFixture+6)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+9)%NUM_FIXTURES][0], fixtures[(randFixture+9)%NUM_FIXTURES][1], fixtures[(randFixture+9)%NUM_FIXTURES][2], f, 0, 0 );
            delay((int)(delayAmt*globalSpeed));
        }
        if ( gotoLoop ) return;
        delay((int)(delayAmt/150*globalSpeed));
    }
    delay((int)(delayAmt*globalSpeed));

    //fade down
    for( int f=255; f>=0; f--) {
        if( doGlobal ){
            for (int p=0; p<NUM_FIXTURES; p++) {
                setFixtureRGB(fixtures[p][0], fixtures[p][1], fixtures[p][2], f, 0, 0 );
            }
        } else {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+3)%NUM_FIXTURES][0], fixtures[(randFixture+3)%NUM_FIXTURES][1], fixtures[(randFixture+3)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+6)%NUM_FIXTURES][0], fixtures[(randFixture+6)%NUM_FIXTURES][1], fixtures[(randFixture+6)%NUM_FIXTURES][2], f, 0, 0 );
            setFixtureRGB(fixtures[(randFixture+9)%NUM_FIXTURES][0], fixtures[(randFixture+9)%NUM_FIXTURES][1], fixtures[(randFixture+9)%NUM_FIXTURES][2], f, 0, 0 );
            delay((int)(delayAmt*globalSpeed));
        }
        if ( gotoLoop ) return;
        delay((int)(delayAmt/150*globalSpeed));
    }

    delay((int)(delayAmt*4*globalSpeed));
}

/**
 *  Multi colored Heartbeat
 */
void fx_rgbHeartbeats(int delayAmt){
    int randFixture = random(0, NUM_FIXTURES);

    float hue = (float)(random(0,360));

    HSVType cHSV;
    RGBType cRGB;
    cHSV.H = (hue/360.0)*6.0;
    cHSV.S = 1.0;
    cHSV.V = 1.0;
    cRGB = HSV_to_RGB(cHSV);

    float randR = cRGB.R;
    float randG = cRGB.G;
    float randB = cRGB.B;

    if( true ){ //fixtures[randFixture][1] != FIXTURE_ADJ_H2O_3CH ) {
        //fade up
        for( int f=0; f<255; f++) {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], (int)(randR*f), (int)(randG*f), (int)(randB*f) );
            if ( gotoLoop ) return;
            delay((int)(delayAmt/150*globalSpeed));
        }
        delay((int)(delayAmt*globalSpeed));

        //fade down
        for( int f=255; f>=0; f--) {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], (int)(randR*f), (int)(randG*f), (int)(randB*f) );
            if ( gotoLoop ) return;
            delay((int)(delayAmt/150*globalSpeed));
        }

        delay((int)(delayAmt*2*globalSpeed));

        //fade up
        for( int f=0; f<255; f++) {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], (int)(randR*f), (int)(randG*f), (int)(randB*f) );
            if ( gotoLoop ) return;
            delay((int)(delayAmt/150*globalSpeed));
        }
        delay((int)(delayAmt*globalSpeed));

        //fade down
        for( int f=255; f>=0; f--) {
            setFixtureRGB(fixtures[randFixture][0], fixtures[randFixture][1], fixtures[randFixture][2], (int)(randR*f), (int)(randG*f), (int)(randB*f) );
            if ( gotoLoop ) return;
            delay((int)(delayAmt/150*globalSpeed));
        }

        delay((int)(delayAmt*globalSpeed)*4);
    }
}

/**
 *  Color wipes
 */
int c1, c2, c3, rb, gb, bb;
unsigned int color;
void fx_colorWipes(int delayAmt) {
    for(c1=0; c1 < 1000 ; c1++) {
        c3=c1 * 1;
        for(c2=0; c2 < NUM_FIXTURES; c2++) {
            //setPixel(c2, Wheel(c3%95));  //There's only 96 colors in this pallette.
            color = Wheel(c3%95);
            rb = (color & 0x1F) * 8;
            gb = ((color >> 10) & 0x1F) * 8;
            bb = ((color >> 5) & 0x1F) * 8;

            setFixtureRGB(fixtures[c2][0], fixtures[c2][1], fixtures[c2][2], rb, gb, bb);
            if ( gotoLoop ) return;

            c3+=(96 / NUM_FIXTURES);
        }
        delay(delayAmt*globalSpeed);
    }
}

/**
 *  Basic on-off chase, one color at a time
 */
void fx_rgbChase(int delayAmt) {
    for(int c=0; c<3; c++) {
        for(int i=0; i<NUM_FIXTURES; i++) {
            setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], c==0 ? 255 : 0, c==1 ? 255 : 0, c==2 ? 255 : 0);
            if ( gotoLoop ) return;

            delay(delayAmt*globalSpeed);

            setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], 0, 0, 0);
            if ( gotoLoop ) return;
        }
    }
}

/**
 *  Basic fade up/down chase, one color at a time
 */
void fx_fadeChase(int delayAmt) {
    for(int c=0; c<3; c++) {
        for(int i=0; i<NUM_FIXTURES; i++) {
            for(int f=0; f<255; f++) {
                setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], c==0 ? (int)((255*f)/255) : 0, c==1 ? (int)((255*f)/255) : 0, c==2 ? (int)((255*f)/255) : 0);
                if ( gotoLoop ) return;

                delay((int)(delayAmt/50*globalSpeed));
            }
            delay((int)(delayAmt*globalSpeed));
            for(int f=255; f>=0; f--) {
                setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], c==0 ? (int)((255*f)/255) : 0, c==1 ? (int)((255*f)/255) : 0, c==2 ? (int)((255*f)/255) : 0);
                if ( gotoLoop ) return;

                delay((int)(delayAmt/50*globalSpeed));
            }
            delay((int)(delayAmt*globalSpeed));
        }
    }
}

/**
 * Put capable fixtures in sound reactive mode
 *
 * DON'T USE THIS FUNCTION. SOUND REACTIVE MODE SUCKS BALLS.
 */
void fx_soundReactive(int cycles) {
    while( cycles-- > 0 ) {
        for(int i=0; i<NUM_FIXTURES; i++) {
            switch (fixtures[i][1]) {
                case FIXTURE_ADJ_38B_7CH:
                    setFixture_ADJ38B(fixtures[i][2], 127,255,64, 0, 0, true, 255, 255);
                    //setFixtureRGB_ADJ38B(fixtures[i][2], 127,255,64);
                    break;

                default:
                    break;

                /*
                case FIXTURE_ADJ_MEGA_PAR_7CH:
                    setFixture_ADJMegaPar(fixtures[i][2], 255,255,255, 0, 0, true, 127, 255);
                    break;

                case FIXTURE_MAGICAL_BALL_6CH:
                    setFixture_MagicalBall(fixtures[i][2], 255,255,255, 0,0,0, true);
                    break;
                    */
            }

            if ( gotoLoop ) return;
        }
        delay(2);
    }

}

void fx_solidColor() {
  for(int i=0; i<NUM_FIXTURES; i++) {
    setFixtureRGB(fixtures[i][0], fixtures[i][1], fixtures[i][2], 255, 0, 0); // Red
    if ( gotoLoop ) return;
  }
}

void superBasicDiag() {
    //fixture 1

    int dmxStart = 1;

    writeDMX(dmxStart, 255);
    writeDMX(dmxStart+1, 255);
    writeDMX(dmxStart+2, 255);

    writeDMX(dmxStart+6, 255);   //full brightness
    writeDMX(dmxStart+5, 0);   //no special sound/macro mode
}

/*************************************************************************************************
*  FIXTURE RGB FUNCTIONS
*   Most params passed here will be [0-255]. Exposes only RGB functions.
*************************************************************************************************/

/**
 *  Single function to set a fixture as an RGB pixel, given the fixtureType, DMX start address and color values.
 *
 *  @param  fixtureArea   AREA_NUM int that is associated with this fixture
 *  @param  fixtureType   An int representing the fixture type
 *  @param  dmxStart      An int representing the DMX start address for the fixture
 */
RGBType colorRGB;
HSVType colorHSV;
int tmpBrightness=255;
float fAreaHue, fAreaSaturation, fAreaBrightness;
void setFixtureRGB(int fixtureArea, int fixtureType, int dmxStart, int r, int g, int b  ) {

    // Counter that increases with every call to here
    cycleCount++;

    // Convert RGB -> HSV
    colorRGB.R = r; colorRGB.G = g; colorRGB.B = b;
    colorHSV = RGB_to_HSV(colorRGB);

    // Get areaParams into usable ranges
    fAreaHue = ( ((float)areaParams[fixtureArea][0])/255.0*6.0 );             //[0.0 - 6.0]
    fAreaSaturation = ( ((float)areaParams[fixtureArea][1])/255.0 );          //[0.0 - 1.0]
    fAreaBrightness = ( ((float)areaParams[fixtureArea][2])/255.0 );          //[0.0 - 1.0]

    // Modify colors based on HSV areaParams
    colorHSV.H = fmod(colorHSV.H + fAreaHue, 6.0);
    colorHSV.S = fAreaSaturation * colorHSV.S;
    //colorHSV.V = fAreaBrightness * colorHSV.V; //[0.0-1.0]
    tmpBrightness = fAreaBrightness * colorHSV.V;


    switch (fixtureType)
    {
        case FIXTURE_ADJ_38B_7CH:
            colorRGB = HSV_to_RGB(colorHSV);
            setFixture_ADJ38B(dmxStart, colorRGB.R, colorRGB.G, colorRGB.B, 0,0,0,0, tmpBrightness);
            break;

        case FIXTURE_ADJ_MEGA_PAR_7CH:
            colorRGB = HSV_to_RGB(colorHSV);
            setFixture_ADJMegaPar(dmxStart, colorRGB.R, colorRGB.G, colorRGB.B, 0,0,0,0, tmpBrightness);
            break;

        case FIXTURE_ADJ_H2O_3CH:
            colorRGB = HSV_to_RGB(colorHSV);
            setFixture_ADJH2O(dmxStart, colorRGB.R, colorRGB.G, colorRGB.B, tmpBrightness, 0);
            break;

        case FIXTURE_RGB_STRIP_3CH:
            colorHSV.V = ((float)tmpBrightness)/255.0;
            colorRGB = HSV_to_RGB(colorHSV);
            setFixture_RGBStrip(dmxStart, colorRGB.R*255, colorRGB.G*255, colorRGB.B*255);
            break;

        case FIXTURE_MAGICAL_BALL_6CH:
            colorHSV.V = ((float)tmpBrightness)/255.0;
            colorRGB = HSV_to_RGB(colorHSV);
            setFixtureRGB_MagicalBall(dmxStart, colorRGB.R, colorRGB.G, colorRGB.B);
            break;

        case FIXTURE_SWARM_FX_18CH:
            colorRGB = HSV_to_RGB(colorHSV);
            /**
                int dmxStart, int r, int g, int b, int brightness

                int autoMode,
                int autoModeParam,
                int washColor,
                int washStrobe,

                int derbyColor,
                int derbyStrobe,
                int derbySpeed,
                int smdStrobe,

                int laserColor,
                int laserStrobe,
                int motorLED

            **/
            // Every 500 cycles, re-randomize
            if (cycleCount%1000>0 && cycleCount%1000<10) {
              swarm_wash_color = random(1, 255);
              swarm_derby_color = random(35, 84);
              swarm_smd_strobe = 10*random(0, 12);
            }
            setFixture_SWARMFX(dmxStart, colorRGB.R, colorRGB.G, colorRGB.B, tmpBrightness, 0, 0, swarm_wash_color, 0, swarm_derby_color, 0, 5, swarm_smd_strobe, 255, 0, 5);
            break;
    }

    requestWireData();  //Call as often as possible
    if ( gotoLoop ) return;
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 *
 * THIS FUNCTION IS BROKE OR SOMETHING
 */
void setFixtureRGB_MagicalBall(int dmxStart, int r, int g, int b) {
    writeDMX(dmxStart+1, r);
    writeDMX(dmxStart+2, g);
    writeDMX(dmxStart+3, b);

    writeDMX(dmxStart, 0); //no strobe
    writeDMX(dmxStart+4, 192);  // motor
    writeDMX(dmxStart+5, 0);  // keep from any auto mode
}

/*************************************************************************************************
*  FIXTURE EXTENDED FUNCTIONS
*   Most params passed here will be [0-255]. Exposes all DMX functions.
*************************************************************************************************/

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 *  @param  colorProgram        [0-7] = Nothing, [8-255] = Color Programs
 *  @param  strobeSpeed         [0-255]
 *  @param  soundActive         true/false
 *  @param  soundSensitivity    [0-255]
 */
void setFixture_ADJ38B(int dmxStart, int r, int g, int b, int colorProgram, int strobeSpeed, bool soundActive, int soundSensitivity, int brightness) {
    writeDMX(dmxStart, r);
    writeDMX(dmxStart+1, g);
    writeDMX(dmxStart+2, b);
    writeDMX(dmxStart+6, brightness);

    writeDMX(dmxStart+3, colorProgram);
    writeDMX(dmxStart+4, (strobeSpeed > 0) ? map(strobeSpeed, 0, 255, 16, 255) : 0);
    writeDMX(dmxStart+5, (soundActive) ? 230 : 0);
    writeDMX(dmxStart+4, (soundActive) ? soundSensitivity : 0);
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 *  @param  colorProgram        [128-239] = Color Programs
 *  @param  strobeSpeed         [0-255]
 *  @param  soundActive         true/false
 *  @param  soundSensitivity    [0-255]
 */
void setFixture_ADJMegaPar(int dmxStart, int r, int g, int b, int colorProgram, int strobeSpeed, bool soundActive, int soundSensitivity, int brightness) {
    writeDMX(dmxStart, r);
    writeDMX(dmxStart+1, g);
    writeDMX(dmxStart+2, b);
    writeDMX(dmxStart+6, brightness);

    writeDMX(dmxStart+5, colorProgram);
    writeDMX(dmxStart+4, (strobeSpeed > 0) ? map(strobeSpeed, 0, 255, 16, 255) : 0);
    writeDMX(dmxStart+5, (soundActive) ? 255 : 0);
    writeDMX(dmxStart+4, (soundActive) ? soundSensitivity : 0);
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 */
void setFixture_RGBStrip(int dmxStart, int r, int g, int b) {
    writeDMX(dmxStart, r);
    writeDMX(dmxStart+1, g);
    writeDMX(dmxStart+2, b);
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 *  @param  brightness
 *  @param  rotationSpeed       [0-9] = Still, [10-120] = Clockwise, [121-134] = Still, [135-245] = Counter Clockwise, [246-255] = Still
 *  @param  colorMode           [11-21] = White/Orange
 *                              [33-43] = Orange/Green
 *                              [55-65] = Green/Blue
 *                              [77-87] = Blue/Yellow
 *                              [99-109] = Yellow/Purple
 *                              [121-127] = Purple/White
 *                              [128-186] = Fade Colors (Fast to Slow)
 *                              [197-255] = Rev Fade Colors (Slow to Fast)
 */
void setFixture_ADJH2O(int dmxStart, int r, int g, int b, int brightness, int colorMode) {
  int rotationSpeed = constrain(map((globalSpeed*100), 0, 200, 80, 122), 80, 122);
  int fadeSpeed = constrain(map((globalSpeed*100), 0, 200, 196, 210), 196, 210);

  if (!colorMode) {
      // if (r>0 && g>0 && b>0) {
      //     //  IF Orange-ish, White/Orange
      //     if (r>150 && g>90 && b<90) colorMode = 21;
      //     //  IF Teal-ish, Green/Blue
      //     if (r<120 && g>90 && b>90) colorMode = 65;
      //     //  IF Purple-sh, Yellow/Purple
      //     if (r>80 && g<135 && b>130) colorMode = 109;
      //     //  IF White-ish, Purple/White
      //     if (r>127 && g>127 && b>127) colorMode = 127;
      // }

      colorMode = fadeSpeed;

      // if (r>0 && g>0 && b>0) {
      //
      //   // If Whitish set to White
      //   // if (r>200 && g>200 && b>200 && r==g && g==b) { colorMode = 10; }
      //
      //   // If Bluish-Purple set to Purple
      //   if (b>75 && b-r>50 && b-g>50 && r>g) { colorMode = 120; }
      //
      //   // If Amberish set to White/Orange
      //   else if (r>g && g-b>50 && g>100) { colorMode = 21; }
      //
      //   // If Reddish set to Orange
      //   else if (r>75 && r-g>50 && r-b>50) { colorMode = 32; }
      //
      //   // If Greenish set to Green
      //   else if (g>75 && g-r>50 && g-b>50) { colorMode = 54; }
      //
      //   // If Bluish set to Blue
      //   else if (b>75 && b-r>50 && b-g>50) { colorMode = 76; }
      //
      //   // Else fade slow
      //   else { colorMode = fadeSpeed; }
      // }
  }

  writeDMX(dmxStart, brightness);
  writeDMX(dmxStart+1, rotationSpeed);
  writeDMX(dmxStart+2, colorMode);
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 *  @param  strobeSpeed
 *  @param  motorPosition
 *  @param  motorSpeed          [0-127] = Clockwise fast to slow, [128-255] = Counter clockwise fast to slow
 */
void setFixture_MagicalBall(int dmxStart, int r, int g, int b, int strobeSpeed, int motorPosition, int motorSpeed, bool soundActive) {
    writeDMX(dmxStart, strobeSpeed);
    writeDMX(dmxStart+1, r);
    writeDMX(dmxStart+2, g);
    writeDMX(dmxStart+3, b);

    if (motorPosition) {
        writeDMX(dmxStart+4, map(motorPosition, 0, 255, 0, 127));
    } else if (motorSpeed) {
        writeDMX(dmxStart+4, map(motorSpeed, 0, 255, 128, 255));
    }

    if(soundActive) {
        writeDMX(dmxStart+5, 255);
    } else {
        writeDMX(dmxStart+5, 0);
    }
}

/**
 *  @param  dmxStart     DMX starting address for the fixture
 *  @param  r                   Amount of red
 *  @param  g                   Amount of green
 *  @param  b                   Amount of blue
 */
 void setFixture_SWARMFX(int dmxStart, int r, int g, int b, int brightness,
     int autoMode,
     int autoModeParam,
     int washColor,
     int washStrobe,
     int derbyColor,
     int derbyStrobe,
     int derbySpeed,
     int smdStrobe,
     int laserColor,
     int laserStrobe,
     int motorLED
 ) {

      // This comparison really needs to be done in HSV space, ranges of H-- rgb seems to yield weird gaps between main ranges
      // Even if you do it in HSV, the colors will still jump suddenly

    //  // Set color modes based on RGB input
    //  // I have RGBUV to work with, cannot combine them in anyway, they are mutually exclusive
    //  // If Bluish-Purple set to UV
    //  if (b>75 && b-r>50 && b-g>50 && r>g) {
    //    washColor = 230;
    //   //  derbyColor = 75;
    //  }
    //  // If Amberish set to Ambers/Yellows/Oranges
    //  else if (r>g && g-b>50 && g>100) {
    //    washColor = 77;
    //   //  derbyColor = 25;
    //  }
    //  // If Reddish set to Red
    //  else if (r>75 && r-g>50 && r-b>50) {
    //    washColor = 77;
    //   //  derbyColor = 10;
    //  }
    //  // If Greenish set to Green
    //  else if (g>75 && g-r>50 && g-b>50) {
    //    washColor = 128;
    //   //  derbyColor = 15;
    //  }
    //  // If Bluish set to Blue
    //  else if (b>75 && b-r>50 && b-g>50) {
    //    washColor = 179;
    //   //  derbyColor = 20;
    //  }

     // Set speeds
    //  autoModeParam = (int)(globalSpeed*100);
     derbySpeed = constrain(map((int)(globalSpeed*100), 0, 200, 70, 4), 4, 70);
     motorLED = derbySpeed;
     smdStrobe = smdStrobe + constrain(map((int)(globalSpeed*100), 0, 200, 9, 0), 0, 9);
     //  washStrobe = constrain(map((int)(globalSpeed*100), 0, 200, 250, 10), 10, 250); //Don't like this either
     //  derbyStrobe = constrain(map((int)(globalSpeed*100), 0, 200, 5, 200), 5, 200); //I don't like this
     //  laserStrobe = constrain(map((int)(globalSpeed*100), 0, 200, 5, 254), 5, 254);   // or this

     // If sound reactive mode on, override with sound reactive values
     if (swarm_sound_reactive) {
        // autoModeParam = 255;
        washStrobe = 0;
        derbyColor = 255;
        derbyStrobe = 255;
        smdStrobe = 255;
        laserStrobe = 255;
        laserColor = 169;
     }

     // Blackout all, stop motors
     if (!brightness) {
       washColor = 1;
       derbyColor = 1;
       laserColor = 1;
       smdStrobe = 1;
       derbySpeed = 1;
       motorLED = 1;
     }

     // If these subfeatures are toggled off, set to black out
     if (!swarm_wash) washColor = 1;
     if (!swarm_derby) derbyColor = 1;
     if (!swarm_lasers) laserColor = 1;
     if (!swarm_strobe) smdStrobe = 1;

     // WRITE DMX
     writeDMX(dmxStart, autoMode);           // ch1
    //  writeDMX(dmxStart+1, autoMode ? autoModeParam : 0);    // ch2
     for (int i=2; i<=9; i++) {
       writeDMX(dmxStart+i, washColor);      // ch3-10
     }
     writeDMX(dmxStart+10, washStrobe);      // ch11
     writeDMX(dmxStart+11, derbyColor);      // ch12
     writeDMX(dmxStart+12, derbyStrobe);     // ch13
     writeDMX(dmxStart+13, derbySpeed);      // ch14
     writeDMX(dmxStart+14, smdStrobe);       // ch15
     writeDMX(dmxStart+15, laserColor);      // ch16
     writeDMX(dmxStart+16, laserStrobe);     // ch17
     writeDMX(dmxStart+17, motorLED);        // ch18
 }
