
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ***** Uncomment to use the PinChangeInterrupts in stead of External Interrupts *****
// #define PIN_CHANGE_INT
//
// ******** REMOVE THE "//" IN THE NEXT LINE UNLESS YOU WANT ALL CV'S RESET ON EVERY POWER-UP
// #define DECODER_LOADED
//
// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO RUN A TEST DURING SETUP SEQUENCES
// #define TESTRUN
//


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* don't print warnings of unused functions from here */
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"


#include <Arduino.h> // Needed for C++ conversion of INOfile.

#include <avr/wdt.h> // Needed for automatic reset functions.

#include <avr/io.h>  // AVR device-specific  IO  definitions.

#include <NmraDcc.h> // Needed for computing the DCC signals.

NmraDcc     Dcc ;
DCC_MSG  Packet ;

#define This_Decoder_Address     40
#define NMRADCC_SIMPLE_RESET_CV 251
#define NMRADCC_MASTER_RESET_CV 252
#define FunctionPinDcc            2  /* Inputpin DCCsignal */


#include <FastLED.h>

#define DATA_PIN_0   5   /*  Which pin on the Arduino is connected to DUMMY  */

#define DATA_PIN_1  14   /*  Which pin on the Arduino is connected to NEO 1  */
#define DATA_PIN_2  15   /*  Which pin on the Arduino is connected to NEO 2  */
#define DATA_PIN_3  16   /*  Which pin on the Arduino is connected to NEO 3  */
#define DATA_PIN_4   8   /*  Which pin on the Arduino is connected to NEO 4  */
#define DATA_PIN_5   9   /*  Which pin on the Arduino is connected to NEO 5  */
#define DATA_PIN_6  10   /*  Which pin on the Arduino is connected to NEO 6  */

#define NUM_NEOS     1   /*  How many NEOs are attached to each output pin   */
#define NUM_STRIPS   6   /*  How many strips are attached to the processor   */

CRGB NEOs[ ( NUM_STRIPS * NUM_NEOS ) + 1 ];   /*  one extra for F0 function  */


#include <SoftPWM.h>

#define DATA_PIN_A   3   /*  Which pin on the Arduino is connected to LED A  */
#define DATA_PIN_B   4   /*  Which pin on the Arduino is connected to LED B  */
#define DATA_PIN_C   6   /*  Which pin on the Arduino is connected to LED C  */
#define DATA_PIN_D   7   /*  Which pin on the Arduino is connected to LED D  */

#define BRIGHTNESS  99   /*  LED brightness, 0 (min) to 255 (max)            */

uint8_t LEDs[4] = { DATA_PIN_A, DATA_PIN_B, DATA_PIN_C, DATA_PIN_D };


#pragma GCC diagnostic pop
/* don't print warnings of unused functions till here */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* printing of debug and test options */

#if defined(_DEBUG_) || defined(TESTRUN)
   #define _PP( a ) Serial.print(    a );
   #define _PL( a ) Serial.println(  a );
   #define _2P(a,b) Serial.print( a, b );
   #define _2L(a,b) Serial.println(a, b);
#else
   #define _PP( a )
   #define _PL( a )
   #define _2P(a,b)
   #define _2L(a,b)
#endif


char bomMarker  =   '<' ;   /*  Begin of message marker.  */
char eomMarker  =   '>' ;   /*  End   of message marker.  */
char commandString[ 32] ;   /*  Max length for a buffer.  */
char sprintfBuffer[ 32] ;   /*  Max length for a buffer.  */
bool foundBom   = false ;   /*  Found begin of messages.  */
bool foundEom   = false ;   /*  Found  end  of messages.  */


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  *****  small macro used to create all the timed events  *****
#define runEvery( n ) for ( static unsigned long lasttime; millis() - lasttime > ( unsigned long )( n ); lasttime = millis() )


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool run_switch_set[ ] = { false, true, true, true, true, true, true, true, true, true, true };  /*  one for every function F0-F10  */


struct QUEUE
{
   uint8_t        outputState    =     0 ;   /*  0, 1 or ?? status  */
   uint8_t        colorRed       =   255 ;   /*  value Red   color  */
   uint8_t        colorGreen     =   255 ;   /*  value Green color  */
   uint8_t        colorBlue      =   155 ;   /*  value Blue  color  */
   uint8_t        brightNess     =   127 ;   /*  brightness factor  */
   bool           isEnabled      =  true ;   /*  output is in use   */
   bool           isBlinking     = false ;   /*  output is blinker  */
   unsigned long  blinkOnTime    =     0 ;   /*  millisec  ON time  */
   unsigned long  blinkOffTime   =     0 ;   /*  millisec OFF time  */
   unsigned long  blinkMillies   =     0 ;   /*  blink cycle start  */
   unsigned long  blinkPeriod    =     0 ;   /*  run time of cycle  */
   bool           isFading       = false ;   /*  output is a fader  */
   unsigned long  fadingOnTime   =     0 ;   /*  millisec fade  ON  */
   unsigned long  fadingOffTime  =     0 ;   /*  millisec fade OFF  */
   unsigned long  fadingMillies  =     0 ;   /*  fade cycle  start  */
   unsigned long  fadingPeriod   =     0 ;   /*  run time of cycle  */
   bool           nowOn          =  true ;   /*  led is  ON or OFF  */
};
QUEUE volatile *NEO_queue = new QUEUE[ ( NUM_STRIPS * NUM_NEOS ) + sizeof( LEDs ) + 1 ];   /*  one extra for F0 function  */


struct CVPair
{
   uint16_t    CV;
   uint8_t  Value;
};


CVPair FactoryDefaultCVs [] =
{
   /*  this is the primary short decoder address  */
   { CV_MULTIFUNCTION_PRIMARY_ADDRESS,      (( This_Decoder_Address >> 0 ) & 0x7F ) +   0 },

   /*  these two CVs define the Long DCC Address  */
   { CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, (( This_Decoder_Address >> 8 ) & 0x7F ) + 192 },
   { CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, (( This_Decoder_Address >> 0 ) & 0xFF ) +   0 },

   // ONLY uncomment 1 CV_29_CONFIG line below as approprate -          DEFAULT IS SHORT ADDRESS
// { CV_29_CONFIG,                0},                        // Short Address 14     Speed Steps
// { CV_29_CONFIG, CV29_F0_LOCATION},                        // Short Address 28/128 Speed Steps
   { CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},  // Long  Address 28/128 Speed Steps

   { NMRADCC_MASTER_RESET_CV,     0},

   {  40,   1},  /*  0 = disable ALL, 1 = enable ALL ( >> at start up or reset)  F0  */
   {  41, 255},
   {  42, 255},
   {  43, 155},
   {  44, 127},
   {  45,  10},   /*  fade up time  multiplier  */
   {  46,  10},   /*  fade down     multiplier  */
   {  47,  10},   /*  blinking  ON  multiplier  */
   {  48,  10},   /*  blinking OFF  multiplier  */
   {  49,   0},   /*    */

   {  50,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F1  */
   {  51, 255},  /*  color value  red  channel  */
   {  52, 255},  /*  color value green channel  */
   {  53, 155},  /*  color value blue  channel  */
   {  54, 127},  /*  dimming factor of channel  */
   {  55,   0},  /*  blinking  ON time setting  */
   {  56,   0},  /*  blinking OFF time setting  */
   {  57,  40},  /*  fade-up   time    setting  */
   {  58,  60},  /*  fade-down time    setting  */
   {  59,   0},  /*    */

   {  60,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F2  */
   {  61, 255},  /*  color value  red  channel  */
   {  62, 255},  /*  color value green channel  */
   {  63, 155},  /*  color value blue  channel  */
   {  64, 127},  /*  dimming factor of channel  */
   {  65,   0},  /*  blinking  ON time setting  */
   {  66,   0},  /*  blinking OFF time setting  */
   {  67,   0},  /*  fade-up   time    setting  */
   {  68,   0},  /*  fade-down time    setting  */
   {  69,   0},  /*    */

   {  70,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F3  */
   {  71, 255},  /*  color value  red  channel  */
   {  72, 255},  /*  color value green channel  */
   {  73, 155},  /*  color value blue  channel  */
   {  74, 127},  /*  dimming factor of channel  */
   {  75,   0},  /*  blinking  ON time setting  */
   {  76,   0},  /*  blinking OFF time setting  */
   {  77,   0},  /*  fade-up   time    setting  */
   {  78,   0},  /*  fade-down time    setting  */
   {  79,   0},  /*    */

   {  80,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F4  */
   {  81, 255},  /*  color value  red  channel  */
   {  82, 255},  /*  color value green channel  */
   {  83, 155},  /*  color value blue  channel  */
   {  84, 127},  /*  dimming factor of channel  */
   {  85,   0},  /*  blinking  ON time setting  */
   {  86,   0},  /*  blinking OFF time setting  */
   {  87,   0},  /*  fade-up   time    setting  */
   {  88,   0},  /*  fade-down time    setting  */
   {  89,   0},  /*    */

   {  90,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F5  */
   {  91, 255},  /*  color value  red  channel  */
   {  92, 255},  /*  color value green channel  */
   {  93, 155},  /*  color value blue  channel  */
   {  94, 127},  /*  dimming factor of channel  */
   {  95,   0},  /*  blinking  ON time setting  */
   {  96,   0},  /*  blinking OFF time setting  */
   {  97,   0},  /*  fade-up   time    setting  */
   {  98,   0},  /*  fade-down time    setting  */
   {  99,   0},  /*    */

   { 100,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F6  */
   { 101, 255},  /*  color value  red  channel  */
   { 102, 255},  /*  color value green channel  */
   { 103, 155},  /*  color value blue  channel  */
   { 104, 127},  /*  dimming factor of channel  */
   { 105,   0},  /*  blinking  ON time setting  */
   { 106,   0},  /*  blinking OFF time setting  */
   { 107,   0},  /*  fade-up   time    setting  */
   { 108,   0},  /*  fade-down time    setting  */
   { 109,   0},  /*    */

   { 110,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED   F7  */
   { 111, 255},  /*  color value  red  channel  */
   { 112, 255},  /*  color value green channel  */
   { 113, 155},  /*  color value blue  channel  */
   { 114, 127},  /*  dimming factor of channel  */
   { 115,   0},  /*  blinking  ON time setting  */
   { 116,   0},  /*  blinking OFF time setting  */
   { 117,   0},  /*  fade-up   time    setting  */
   { 118,   0},  /*  fade-down time    setting  */
   { 119,   0},  /*    */

   { 120,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F8  */
   { 121, 255},  /*  color value  red  channel  */
   { 122, 255},  /*  color value green channel  */
   { 123, 155},  /*  color value blue  channel  */
   { 124, 127},  /*  dimming factor of channel  */
   { 125,   0},  /*  blinking  ON time setting  */
   { 126,   0},  /*  blinking OFF time setting  */
   { 127,   0},  /*  fade-up   time    setting  */
   { 128,   0},  /*  fade-down time    setting  */
   { 129,   0},  /*    */

   { 130,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F9  */
   { 131, 255},  /*  color value  red  channel  */
   { 132, 255},  /*  color value green channel  */
   { 133, 155},  /*  color value blue  channel  */
   { 134, 127},  /*  dimming factor of channel  */
   { 135,   0},  /*  blinking  ON time setting  */
   { 136,   0},  /*  blinking OFF time setting  */
   { 137,   0},  /*  fade-up   time    setting  */
   { 138,   0},  /*  fade-down time    setting  */
   { 139,   0},  /*    */

   { 140,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO  F10  */
   { 141, 255},  /*  color value  red  channel  */
   { 142, 255},  /*  color value green channel  */
   { 143, 155},  /*  color value blue  channel  */
   { 144, 127},  /*  dimming factor of channel  */
   { 145,   0},  /*  blinking  ON time setting  */
   { 146,   0},  /*  blinking OFF time setting  */
   { 147,   0},  /*  fade-up   time    setting  */
   { 148,   0},  /*  fade-down time    setting  */
   { 149,   0},  /*    */

   { 150,   0},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO  F11  */
   { 151,   0},  /*  color value  red  channel  */
   { 152,   0},  /*  color value green channel  */
   { 153,   0},  /*  color value blue  channel  */
   { 154,   0},  /*  dimming factor of channel  */
   { 155,   0},  /*  blinking  ON time setting  */
   { 156,   0},  /*  blinking OFF time setting  */
   { 157,   0},  /*  fade-up   time    setting  */
   { 158,   0},  /*  fade-down time    setting  */
   { 159,   0},  /*    */


   { 251,   0},  /*  NMRADCC_SIMPLE_RESET_CV  */
   { 252,   0},  /*  NMRADCC_MASTER_RESET_CV  */
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


uint8_t FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
void notifyCVResetFactoryDefault()
{
      // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset 
      // Flag to the loop() function a reset to FactoryDefaults has to be done.
        FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup()
{
   noInterrupts();


   DDRB  = DDRB  | 0b00111111;  // Set bits B[5-0] as outputs, leave the rest as-is.
   DDRC  = DDRC  | 0b00111111;  // Set bits C[5-0] as outputs, leave the rest as-is.
   DDRD  = DDRD  | 0b11111000;  // Set bits D[7-3] as outputs, leave the rest as-is.


#if defined(TESTRUN)

   PORTB = PORTB | 0b00111111;  // Switch bits B[5-0] to  ON, leave the rest as-is.
   PORTC = PORTC | 0b00111111;  // Switch bits C[5-0] to  ON, leave the rest as-is.
   PORTD = PORTD | 0b11111000;  // Switch bits D[7-3] to  ON, leave the rest as-is.

#endif

   PORTB = PORTB & 0b11000000;  // Switch bits B[5-0] to OFF, leave the rest as-is.
   PORTC = PORTC & 0b11000000;  // Switch bits C[5-0] to OFF, leave the rest as-is.
   PORTD = PORTD & 0b00000111;  // Switch bits D[7-3] to OFF, leave the rest as-is.


   interrupts();  /* Ready to rumble....*/


   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_0, starting at index 0 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_0, GRB>( NEOs,  0, NUM_NEOS );  /*  DUMMY  */

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_1, starting at index 1 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>( NEOs,  1, NUM_NEOS );  /*  F1  */

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_2, starting at index 2 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_2, GRB>( NEOs,  2, NUM_NEOS );

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_3, starting at index 3 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_3, GRB>( NEOs,  3, NUM_NEOS );

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_4, starting at index 4 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_4, GRB>( NEOs,  4, NUM_NEOS );

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_5, starting at index 5 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_5, GRB>( NEOs,  5, NUM_NEOS );

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_6, starting at index 6 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_6, GRB>( NEOs,  6, NUM_NEOS );


#if defined(_DEBUG_) || defined(TESTRUN)

   Serial.begin(115200);

   while (!Serial)
   {
      ; // wait for Serial port to connect. Needed for native USB port.
   }

   Serial.flush();    // Wait for all the rubbish to finish displaying.

   while (Serial.available() > 0)
   {
      Serial.read(); // Clear the input buffer to get 'real' inputdata.
   }

   _PL(F( "-------------------------------------" ));

#endif


#if defined(_DECODER_LOADED_)

   if ( Dcc.getCV( NMRADCC_MASTER_RESET_CV ) == NMRADCC_MASTER_RESET_CV ) 
   {

#endif

      _PL(F( "wait for the copy process to finish.." ));

      FactoryDefaultCVIndex =  sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

      for ( int i = 0; i < FactoryDefaultCVIndex; ++i )
      {
         Dcc.setCV( FactoryDefaultCVs[ i ].CV, FactoryDefaultCVs[ i ].Value );
      }

#if defined(_DECODER_LOADED_)

   }

#endif


   //  Call the DCC 'pin' and 'init' functions to enable the DCC Receivermode
   Dcc.pin( digitalPinToInterrupt( FunctionPinDcc ), FunctionPinDcc, false );
   Dcc.init( MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, 0 );      delay( 1000 );
   //c.initAccessoryDecoder( MAN_ID_DIY,  201, FLAGS_MY_ADDRESS_ONLY,    0 );

   Dcc.setCV( NMRADCC_SIMPLE_RESET_CV, 0 );  /*  Reset the just_a_reset CV */

   run_switch_set[ 0 ] = ( Dcc.getCV( 40 ) == 0 ? false : true );  /*  F0  */



   SoftPWMBegin( SOFTPWM_NORMAL );     /*  Initialize the SoftPWM library  */

   for ( int i = 0; i < (int)sizeof( LEDs ); ++i )
      SoftPWMSet( LEDs[ i ], BRIGHTNESS );

   // SoftPWMSetFadeTime( ALL, 50, 400 );    /*  Set fade-up time to 50 ms, fade-down time to 400 ms  */


   _PL(F( "-------------------------------------" ));


   /*  calculate the time setting for almost every output  */
   for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
   {
      if ( i < 7 ) { calculateNeoQueue( i ); } else { calculateLedQueue( i ); }
   }


/////////////////////////////////////////////////////////////////////////////////////////////////////////
   // uint8_t        outputState    =     0 ;   /*  0, 1 or ?? status  */
   // uint8_t        colorRed       =   255 ;   /*  value Red   color  */
   // uint8_t        colorGreen     =   255 ;   /*  value Green color  */
   // uint8_t        colorBlue      =   155 ;   /*  value Blue  color  */
   // uint8_t        brightNess     =   127 ;   /*  brightness factor  */
   // bool           isEnabled      =  true ;   /*  output is in use   */
   // bool           isBlinking     = false ;   /*  output is blinker  */
   // unsigned long  blinkOnTime    =     0 ;   /*  millisec  ON time  */
   // unsigned long  blinkOffTime   =     0 ;   /*  millisec OFF time  */
   // unsigned long  blinkMillies   =     0 ;   /*  cycle starttime    */
   // unsigned long  blinkPeriod  =     0 ;   /*  run time of cycle  */
   // bool           nowOn          =  true ;   /*  led is  ON or OFF  */


   NEO_queue[ 4 ].isBlinking = true;
   NEO_queue[ 4 ].nowOn      = true;

   NEO_queue[ 4 ].blinkOffTime = 1500;
   NEO_queue[ 4 ].blinkOnTime  =  500;

   NEO_queue[ 6 ].isBlinking = true;
   NEO_queue[ 6 ].nowOn      = true;

   NEO_queue[ 6 ].blinkOffTime = 150;
   NEO_queue[ 6 ].blinkOnTime  =  50;


   NEO_queue[ 7 ].isBlinking = true;
   NEO_queue[ 7 ].nowOn      = true;

   NEO_queue[ 7 ].blinkOffTime = 1500;
   NEO_queue[ 7 ].blinkOnTime  =  500;

   NEO_queue[ 9 ].isBlinking = true;
   NEO_queue[ 9 ].nowOn      = true;

   NEO_queue[ 9 ].blinkOffTime = 150;
   NEO_queue[ 9 ].blinkOnTime  =  50;



/////////////////////////////////////////////////////////////////////////////////////////////////////////


   displayText(); /* Shows the standard text if applicable */


   interrupts();  /* Ready to rumble....*/

}



byte indexOne = 0;
unsigned long currentMillis = 0;



void loop()
{
   Dcc.process();  /*  Call this process for a Dcc connection  */


   currentMillis = millis();  /*  lets keep track of the time  */

   if ( run_switch_set[ indexOne ] && NEO_queue[ indexOne ].isBlinking )   /*  check if valid to run  */
   {
      if ( currentMillis - NEO_queue[ indexOne ].blinkMillies >= NEO_queue[ indexOne ].blinkPeriod )
      {
         NEO_queue[ indexOne ].blinkMillies = currentMillis;

         NEO_queue[ indexOne ].nowOn = !NEO_queue[ indexOne ].nowOn; /*  reverse the setting  */



         if ( indexOne < 7 ) { calculateNeoBlink( indexOne ); } else { calculateLedBlink( indexOne ); }



         if ( NEO_queue[ indexOne ].nowOn )
         {
            NEO_queue[ indexOne ].blinkPeriod = NEO_queue[ indexOne ].blinkOnTime;
         }
         else
         {
            NEO_queue[ indexOne ].blinkPeriod = NEO_queue[ indexOne ].blinkOffTime;
         }
      }
   }



   indexOne = ( indexOne + 1 ) % 11; /*  index form 0 to 10  */


   // taskTimer.execute();   /*  Keeps all the timers running...*/


   if ( foundEom )  /*  Serial Input needs message-handling  */
   {
      foundBom = false;
      foundEom = false;
      parseCom( commandString );
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void softwareReset( uint8_t preScaler )
{
   wdt_enable( preScaler );

   while( 1 ) {}  // Wait for the prescaler time to expire and do an auto-reset
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void calculateLedQueue( int ledNumber )
{
   if ( run_switch_set[ 0 ] && NEO_queue[ ledNumber ].isEnabled )
   {
      int getCVNumber = ( ( ledNumber + 4 ) * 10 ) ;  /*  calculate the correct CVaddress  */

   // {  45,  10},   /*  fade up time  multiplier  */
   // {  46,  10},   /*  fade down     multiplier  */
   // {  47,  10},   /*  blinking  ON  multiplier  */
   // {  48,  10},   /*  blinking OFF  multiplier  */

   // uint8_t        outputState    =     0 ;   /*  0, 1 or ?? status  */
   // uint8_t        colorRed       =   255 ;   /*  value Red   color  */
   // uint8_t        colorGreen     =   255 ;   /*  value Green color  */
   // uint8_t        colorBlue      =   155 ;   /*  value Blue  color  */
   // uint8_t        brightNess     =   127 ;   /*  brightness factor  */
   // bool           isEnabled      =  true ;   /*  output is in use   */
   // bool           isBlinking     = false ;   /*  output is blinker  */
   // unsigned long  blinkOnTime    =     0 ;   /*  millisec  ON time  */
   // unsigned long  blinkOffTime   =     0 ;   /*  millisec OFF time  */
   // unsigned long  blinkMillies   =     0 ;   /*  blink cycle start  */
   // unsigned long  blinkPeriod    =     0 ;   /*  run time of cycle  */
   // bool           isFading       = false ;   /*  output is a fader  */
   // unsigned long  fadingOnTime   =     0 ;   /*  millisec fade  ON  */
   // unsigned long  fadingOffTime  =     0 ;   /*  millisec fade OFF  */
   // unsigned long  fadingMillies  =     0 ;   /*  fade cycle  start  */
   // unsigned long  fadingPeriod   =     0 ;   /*  run time of cycle  */
   // bool           nowOn          =  true ;   /*  led is  ON or OFF  */




/*  TODO: check if we REALLY need the colorRed, colorGreen and colorBlue in the NEO_queue  */

      // NEO_queue[ ledNumber ].colorRed   = constrain( Dcc.getCV( getCVNumber + 1 ), 0, 255 );
      // NEO_queue[ ledNumber ].colorGreen = constrain( Dcc.getCV( getCVNumber + 2 ), 0, 255 );
      // NEO_queue[ ledNumber ].colorBlue  = constrain( Dcc.getCV( getCVNumber + 3 ), 0, 255 );
      // NEO_queue[ ledNumber ].brightNess = constrain( Dcc.getCV( getCVNumber + 4 ), 0, 255 );

      // NEOs[ ledNumber ] = CRGB( NEO_queue[ ledNumber ].colorRed, NEO_queue[ ledNumber ].colorGreen, NEO_queue[ ledNumber ].colorBlue );

      // NEOs[ ledNumber ].nscale8_video( NEO_queue[ ledNumber ].brightNess );
      // nscale8x3_video( NEOs[ ledNumber ].r, NEOs[ ledNumber ].g, NEOs[ ledNumber ].b, NEO_queue[ ledNumber ].brightNess );
      // cleanup_R1();


      _PP( " cv: " );
      _2P( getCVNumber + 0 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 0 ), DEC );

      _PL(F( "-------------------------------------" ));
   }
   else
   {
      // NEOs[ ledNumber ] = CRGB::Black;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void calculateNeoQueue( int ledNumber )
{
   if ( run_switch_set[ 0 ] && NEO_queue[ ledNumber ].isEnabled )
   {
      int getCVNumber = ( ( ledNumber + 4 ) * 10 ) ;  /*  calculate the correct CVaddress  */


/*  TODO: check if we REALLY need the colorRed, colorGreen and colorBlue in the NEO_queue  */

      NEO_queue[ ledNumber ].colorRed   = constrain( Dcc.getCV( getCVNumber + 1 ), 0, 255 );
      NEO_queue[ ledNumber ].colorGreen = constrain( Dcc.getCV( getCVNumber + 2 ), 0, 255 );
      NEO_queue[ ledNumber ].colorBlue  = constrain( Dcc.getCV( getCVNumber + 3 ), 0, 255 );
      NEO_queue[ ledNumber ].brightNess = constrain( Dcc.getCV( getCVNumber + 4 ), 0, 255 );

      NEOs[ ledNumber ] = CRGB( NEO_queue[ ledNumber ].colorRed, NEO_queue[ ledNumber ].colorGreen, NEO_queue[ ledNumber ].colorBlue );

      NEOs[ ledNumber ].nscale8_video( NEO_queue[ ledNumber ].brightNess );
      // nscale8x3_video( NEOs[ ledNumber ].r, NEOs[ ledNumber ].g, NEOs[ ledNumber ].b, NEO_queue[ ledNumber ].brightNess );
      // cleanup_R1();

   // {  45,  10},   /*  fade up time  multiplier  */
   // {  46,  10},   /*  fade down     multiplier  */
   // {  47,  10},   /*  blinking  ON  multiplier  */
   // {  48,  10},   /*  blinking OFF  multiplier  */

   // uint8_t        outputState    =     0 ;   /*  0, 1 or ?? status  */
   // uint8_t        colorRed       =   255 ;   /*  value Red   color  */
   // uint8_t        colorGreen     =   255 ;   /*  value Green color  */
   // uint8_t        colorBlue      =   155 ;   /*  value Blue  color  */
   // uint8_t        brightNess     =   127 ;   /*  brightness factor  */
   // bool           isEnabled      =  true ;   /*  output is in use   */
   // bool           isBlinking     = false ;   /*  output is blinker  */
   // unsigned long  blinkOnTime    =     0 ;   /*  millisec  ON time  */
   // unsigned long  blinkOffTime   =     0 ;   /*  millisec OFF time  */
   // unsigned long  blinkMillies   =     0 ;   /*  blink cycle start  */
   // unsigned long  blinkPeriod    =     0 ;   /*  run time of cycle  */
   // bool           isFading       = false ;   /*  output is a fader  */
   // unsigned long  fadingOnTime   =     0 ;   /*  millisec fade  ON  */
   // unsigned long  fadingOffTime  =     0 ;   /*  millisec fade OFF  */
   // unsigned long  fadingMillies  =     0 ;   /*  fade cycle  start  */
   // unsigned long  fadingPeriod   =     0 ;   /*  run time of cycle  */
   // bool           nowOn          =  true ;   /*  led is  ON or OFF  */



      _PP( " cv: " );
      _2P( getCVNumber + 0 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 0 ), DEC );

      _PL(F( "-------------------------------------" ));
   }
   else
   {
      NEOs[ ledNumber ] = CRGB::Black;
   }

   FastLED.show();   /*  Show all the NEOs, changed or not  */
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void calculateLedBlink( int ledNumber )
{
   if ( run_switch_set[ 0 ] && NEO_queue[ ledNumber ].isEnabled && NEO_queue[ ledNumber ].nowOn )
   {

      SoftPWMSet( LEDs[ ledNumber - ( NUM_STRIPS * NUM_NEOS ) - 1 ] , BRIGHTNESS );

   }
   else
   {
      SoftPWMSet( LEDs[ ledNumber - ( NUM_STRIPS * NUM_NEOS ) - 1 ] ,          0 );
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void calculateNeoBlink( int ledNumber )
{
   if ( run_switch_set[ 0 ] && NEO_queue[ ledNumber ].isEnabled && NEO_queue[ ledNumber ].nowOn )
   {
      NEOs[ ledNumber ] = CRGB( NEO_queue[ ledNumber ].colorRed, NEO_queue[ ledNumber ].colorGreen, NEO_queue[ ledNumber ].colorBlue );

      NEOs[ ledNumber ].nscale8_video( NEO_queue[ ledNumber ].brightNess );
   }
   else
   {
      NEOs[ ledNumber ] = CRGB::Black;
   }

   FastLED.show();   /*  Show all the NEOs, changed or not  */
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The next part will / can only be used for an mpu with enough memory


/* ******************************************************************************* */
      // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On  ***
/* **********************************************************************************

   <0>   all outputs: Off
   <1>   all outputs: On
   <2>   all outputs: Blink Off
   <3>   all outputs: Blink On
   <4>   all outputs: Fade Off
   <5>   all outputs: Fade On

   <C>   clear everything: Factory Default
   <D>   dumps everything: to Serial.Print

   <F>   controls mobile engine decoder functions F0-F12: <f x y>
   <f>   lists all funtions and settings for all outputs: <F>

   <M>   list the available SRAM on the chip

   <R>   reads a configuration variable: <R x>
   <W>   write a configuration variable: <W x y>

*/
void displayText()
{
   _PL("");    // An extra empty line for better understanding
   _PL(F("Put in one of the following commands: "          ));
   _PL(F("------------------------------------------------"));
   _PL(F("<C>   clear everything: Factory Default"         ));
   _PL(F("<D>   prints CV values: to your monitor"         ));
   _PL("");
   _PL(F("<F>   control decoder functions F0-F12: <Fx>"    ));
   _PL("");
   _PL(F("<R>   reads a configuration variable: <R x>"     ));
   _PL(F("<W>   write a configuration variable: <W x y>"   ));
   _PL(F("----------------------------------------------- "));
   _PL(F("* include '<', '>' and spaces in your command * "));
   _PL("");    // An extra empty line for better understanding
}


/* ******************************************************************************* */
/*
.*    SerialEvent occurs whenever a new data comes in the hardware serial RX. This
.*    routine is run between each time loop() runs, so using delay inside loop can
.*    delay response.  Multiple bytes may be available and be put in commandString
.*/
void serialEvent() {
   while (Serial.available())
   {
      char inChar = (char)Serial.read(); // get the new byte

      if (inChar == bomMarker)       // start of new command
      {
         sprintf(commandString, "%s%c", ""           ,  inChar);
         foundBom = true;
      }
      else if (inChar == eomMarker)   // end of this command
      {
         foundEom = true;
         sprintf(commandString, "%s%c", commandString,  inChar);
      }
      else if (strlen(commandString) <  16) // put it all in
      {
         sprintf(commandString, "%s%c", commandString,  inChar);
         /* if comandString still has space, append character just read from serial line
         otherwise, character is ignored, (but we'll continue to look for '<' or '>') */
      }
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void parseCom( char *com )
{
   switch ( com[1] )  // com[0] = '<' or ' '
   {

/****  CLEAR SETTINGS TO FACTORY DEFAULTS  ****/

      case 'C':     // <C>
/*
 *    clears settings to Factory Defaults
 *
 *    returns: <FD done> cvCheck
 */
      {
         uint8_t cvCheck = Dcc.setCV( NMRADCC_MASTER_RESET_CV, NMRADCC_MASTER_RESET_CV );

         _PP( "<FD done>"  );
         _2L( cvCheck, DEC );

         softwareReset(  WDTO_15MS  );
         break;
      }

/***** DUMPS CONFIGURATION VARIABLES FROM DECODER ****/

      case 'D':     // <D>
/*
 *    dumps all Configuration Variables from the decoder
 *
 *    returns a list of: <CV VALUE)
 *    where VALUE is a number from 0-255 as read from the CV, or -1 if read could not be verified
 */
      {
         FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

         for (int i = 0; i < FactoryDefaultCVIndex; i++)
         {
            uint8_t cvValue = Dcc.getCV( FactoryDefaultCVs[ i ].CV );

            _PP( " cv: "                      );
            _2P( FactoryDefaultCVs[i].CV, DEC );
            _PP( "\t"              " value: " );
            _2L( cvValue,                 DEC );

         }
         break;
      }

/******** CONTROL DECODER FUNCTIONS F0 and F1 ********/

      case 'F':      // <F0> to <F10>
/*
 *    writes, and then verifies, a Function Variable to the decoder
 *
 *    FV: the number of the Function Variable in the decoder to write to (0-1x)
 *    VALUE: the value to be written to the Function Variable memory location 
 *
 *    returns: <f FV Value)
 *    where VALUE is a number from 0-1 as read from the requested FV, or -1 if verificaiton fails
 */
      {
         int  fv;  /* , cvCheck;  */

         if( sscanf( com + 2, "%d", &fv ) != 1 ) { return; }  /*  get value and check for valid number  */

         switch ( fv )
         {

            case  0:  /*  F0 to F10  */
            case  1:
            case  2:
            case  3:
            case  4:
            case  5:
            case  6:
            case  7:
            case  8:
            case  9:
            case 10:
            {
               run_switch_set[ fv ] = ( !run_switch_set[ fv ] );

               NEO_queue[ fv ].isEnabled = ( run_switch_set[ fv ] );

               break;
            }

            case 11:  /*  F11 to F12  */
            case 12:
            default:
               break;
         }

         /*  calculate the time setting for almost every output  */
         for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
         {
            if ( i < 7 ) { calculateNeoQueue( i ); } else { calculateLedQueue( i ); }
         }

         _PL(F( "wait for the cycle to end..." ) );
         _PP(F( "<f F"                         ) );
         _2P(   fv + 0 ,                   DEC   );
         _PL(F( ">"                            ) );

         break;
      }


/***** READ CONFIGURATION VARIABLE BYTE FROM DECODER ****/

      case 'R':     // <R CV>
/*
 *    reads a Configuration Variable from the decoder
 *
 *    CV: the number of the Configuration Variable memory location in the decoder to read from (1-1024)
 *
 *    returns: <r CV VALUE>
 *    where VALUE is a number from 0-255 as read from the requested CV, or -1 if read could not be verified
 */
      {
         uint16_t cv;

         if( sscanf( com + 2, "%d", &cv ) != 1 ) { return; }

         uint8_t cvCheck = Dcc.getCV( cv );

         _PP( "<r "        );
         _2P( cv + 0 , DEC );
         _PP( " "          );
         _2P( cvCheck, DEC );
         _PL( ">"          );

         break;
      }


/***** WRITE CONFIGURATION VARIABLE BYTE TO DECODER ****/

      case 'W':      // <W CV VALUE>
/*
 *    writes, and then verifies, a Configuration Variable to the decoder
 *
 *    CV: the number of the Configuration Variable memory location in the decoder to write to (1-1024)
 *    VALUE: the value to be written to the Configuration Variable memory location (0-255) 
 *
 *    returns: <w CV Value)
 *    where VALUE is a number from 0-255 as read from the requested CV, or -1 if verificaiton read fails
 */
      {
         int  bValue, cv;

         if( sscanf( com + 2, "%d %d", &cv, &bValue ) != 2 ) { return; }

         uint8_t cvCheck = Dcc.setCV( cv, bValue ); /*  write action  */

         /*  calculate the time setting for almost every output  */
         for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
         {
            if ( i < 7 ) { calculateNeoQueue( i ); } else { calculateLedQueue( i ); }
         }

         _PP( "<w "        );
         _2P( cv + 0 , DEC );
         _PP( " "          );
         _2P( cvCheck, DEC );
         _PL( ">"          );

         break;
      }


/****  PRINT CARRIAGE RETURN IN SERIAL MONITOR WINDOW  ****/

      case ' ':     // < >
/*
 *    simply prints a carriage return - useful when interacting with Ardiuno through serial monitor window
 *
 *    returns: a carriage return and the menu text
 */
      {
         _PL("");

         displayText(); // Shows the standard explanation text

         break;
      }


      default:    /****  DEFAULT FOR THE SWITCH FUNCTION = NO ACTION  ****/
         break;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/************************************************************************************
                        Call-back functions from DCC
************************************************************************************/

/*  TODO:  some of the following functions can be removed after testing with DCC  */


void    notifyDccAccTurnoutBoard (uint16_t BoardAddr, uint8_t OutputPair, uint8_t Direction, uint8_t OutputPower)
{
            _PL("notifyDccAccTurnoutBoard");
}

void    notifyDccAccTurnoutOutput (uint16_t Addr, uint8_t Direction, uint8_t OutputPower)
{
            _PL("notifyDccAccTurnoutOutput");
}


void    notifyDccAccBoardAddrSet (uint16_t BoardAddr)
{
            _PL("notifyDccAccBoardAddrSet");
}

void    notifyDccAccOutputAddrSet (uint16_t Addr)
{
            _PL("notifyDccAccOutputAddrSet");
}


void    notifyDccSigOutputState (uint16_t Addr, uint8_t State)
{
            _PL("notifyDccSigOutputState");
}

void    notifyDccMsg (DCC_MSG * Msg)
{
            _PL("notifyDccMsg");
}


// Deprecated, only for backward compatibility with version 1.4.2.
// Don't use in new designs. These functions may be dropped in future versions
void notifyDccAccState (uint16_t Addr, uint16_t BoardAddr, uint8_t OutputAddr, uint8_t State)
{
            _PL("notifyDccAccState");
}

void notifyDccSigState (uint16_t Addr, uint8_t OutputIndex, uint8_t State)
{
            _PL("notifyDccSigState");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* **********************************************************************************
    notifyCVChange()  Called when a CV value is changed.
                      This is called whenever a CV's value is changed.
    notifyDccCVChange()  Called only when a CV value is changed by a Dcc packet or a lib function.
                      it is NOT called if the CV is changed by means of the setCV() method.
                      Note: It is not called if notifyCVWrite() is defined
                      or if the value in the EEPROM is the same as the value
                      in the write command.

    Inputs:
      CV        - CV number.
      Value     - Value of the CV.

    Returns:
      None
*/
void    notifyCVChange( uint16_t CV, uint8_t Value )
{
   _PP( "notifyCVChange: CV: " );
   _2L( CV,                DEC );
   _PP( "Value: "              );
   _2L( Value,             DEC );

   if ( ( ( CV == 251 ) && ( Value == 251 ) ) || ( ( CV == 252 ) && ( Value == 252 ) ) )
   {
      wdt_enable( WDTO_15MS );  //  Resets after 15 milliSecs

      while ( 1 ) {} // Wait for the prescaler time to expire
   }

   /*  calculate the time setting for almost every output  */
   for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
   {
      if ( i < 7 ) { calculateNeoQueue( i ); } else { calculateLedQueue( i ); }
   }

}       //   end notifyCVChange()


/* **********************************************************************************
      notifyDccFunc() Callback for a multifunction decoder function command.

    Inputs:
      Addr        - Active decoder address.
      AddrType    - DCC_ADDR_SHORT or DCC_ADDR_LONG.
      FuncGrp     - Function group. FN_0      - 14 speed headlight  Mask FN_BIT_00

                                    FN_0_4    - Functions  0 to  4. Mask FN_BIT_00 - FN_BIT_04
                                    FN_5_8    - Functions  5 to  8. Mask FN_BIT_05 - FN_BIT_08
                                    FN_9_12   - Functions  9 to 12. Mask FN_BIT_09 - FN_BIT_12
                                    FN_13_20  - Functions 13 to 20. Mask FN_BIT_13 - FN_BIT_20
                                    FN_21_28  - Functions 21 to 28. Mask FN_BIT_21 - FN_BIT_28
      FuncState   - Function state. Bitmask where active functions have a 1 at that bit.
                                    You must &FuncState with the appropriate
                                    FN_BIT_nn value to isolate a given bit.

    Returns:
      None
*/
void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState )
{
   _PL( "notifyDccFunc" );
   _PP( "Address = "    );
   _2L( Addr,        DEC);
   _PP( "AddrType = "   );
   _2L( AddrType,    DEC);
   _PP( "FuncGrp = "    );
   _2L( FuncGrp,     DEC);
   _PP( "FuncState = "  );
   _2L( FuncState,   DEC);

  switch ( FuncGrp )
  {
    case FN_0_4:    //  Function Group 1    F0 F4 F3 F2 F1
    {
      //   exec_function(  0, DATA_PIN_0, ( FuncState & FN_BIT_00 ) >> 4 );
      //   exec_function(  1, FunctionPinDum, ( FuncState & FN_BIT_01 ) >> 0 );
      //   exec_function(  2, FunctionPinDum, ( FuncState & FN_BIT_02 ) >> 1 );
      //   exec_function(  3, FunctionPinDum, ( FuncState & FN_BIT_03 ) >> 2 );
      //   exec_function(  4, FunctionPinDum, ( FuncState & FN_BIT_04 ) >> 3 );
        break ;
    }

    case FN_5_8:    //  Function Group 1    F8  F7  F6  F5
    {
      //   exec_function(  5, FunctionPinDum, ( FuncState & FN_BIT_05 ) >> 0 );
      //   exec_function(  6, FunctionPinDum, ( FuncState & FN_BIT_06 ) >> 1 );
      //   exec_function(  7, FunctionPinDum, ( FuncState & FN_BIT_07 ) >> 2 );
      //   exec_function(  8, FunctionPinDum, ( FuncState & FN_BIT_08 ) >> 3 );
        break ;
    }
    case FN_9_12:   //  Function Group 1    F12 F11 F10 F9
    {
      //   exec_function(  9, FunctionPinDum, ( FuncState & FN_BIT_09 ) >> 0 );
      //   exec_function( 10, FunctionPinDum, ( FuncState & FN_BIT_10 ) >> 1 );
      //   exec_function( 11, FunctionPinDum, ( FuncState & FN_BIT_11 ) >> 2 );
      //   exec_function( 12, FunctionPinDum, ( FuncState & FN_BIT_12 ) >> 3 );
        break ;
    }
    case FN_13_20:  //  Function Group 2  ==  F20 - F13
    case FN_21_28:  //  Function Group 2  ==  F28 - F21
    default:
      {
        break ;
      }
  }
}                     //  End notifyDccFunc()


/* **********************************************************************************
 *  notifyDccSpeed() Callback for a multifunction decoder speed command.
 *                   The received speed and direction are unpacked to separate values.
 *
 *  Inputs:
 *    Addr        - Active decoder address.
 *    AddrType    - DCC_ADDR_SHORT or DCC_ADDR_LONG.
 *    Speed       - Decoder speed. 0               = Emergency stop
 *                                 1               = Regular stop
 *                                 2 to SpeedSteps = Speed step 1 to max.
 *    Dir         - DCC_DIR_REV or DCC_DIR_FWD
 *    SpeedSteps  - Highest speed, SPEED_STEP_14   =  15
 *                                 SPEED_STEP_28   =  29
 *                                 SPEED_STEP_128  = 127
 *
 *  Returns:
 *    None
 */
void    notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION Direction, DCC_SPEED_STEPS SpeedSteps )
{
    _PL("notifyDccSpeed  and  DCC_DIRECTION");
    
    _PP( "Address = "    );
    _2L( Addr,        DEC);
    _PP( "AddrType = "   );
    _2L( AddrType,    DEC);
    _PP( "Speed = "      );
    _2L( Speed,       DEC);
    _PP( "Direction = "  );
    _2L( Direction,   DEC);
    _PP( "SpeedSteps = " );
    _2L( SpeedSteps, DEC );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void exec_function ( int function, int pin, int FuncState )
{
   _PP( "exec_function = " );
   _2L( function,      DEC );
   _PP( "pin (n/u) = "     );
   _2L( pin,           DEC );
   _PP( "FuncState = "     );
   _2L( FuncState,     DEC );

  switch ( Dcc.getCV( 40 + function ) )
  {
    case  0:  // Function  F0
    case  1:
    case  2:
    case  3:
    case  4:
    case  5:
    case  6:
    case  7:
    case  8:
    case  9:
    case 10:  // Function F10
      {
      //   function_value[ function ] = byte( FuncState );
        break ;
      }

    default:
      {
        break ;
      }
  }
}                //  End exec_function()


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





/*  */
