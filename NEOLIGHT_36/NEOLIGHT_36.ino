
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

#define DATA_PIN_0   5   /*  Which pin on the Arduino is connected to NEO 0  */

#define DATA_PIN_1  14   /*  Which pin on the Arduino is connected to NEO 1  */
#define DATA_PIN_2  15   /*  Which pin on the Arduino is connected to NEO 2  */
#define DATA_PIN_3  16   /*  Which pin on the Arduino is connected to NEO 3  */
#define DATA_PIN_4   8   /*  Which pin on the Arduino is connected to NEO 4  */
#define DATA_PIN_5   9   /*  Which pin on the Arduino is connected to NEO 5  */
#define DATA_PIN_6  10   /*  Which pin on the Arduino is connected to NEO 6  */

#define NUM_NEOS     1   /*  How many NEOs are attached to each output pin   */
#define NUM_STRIPS   6   /*  How many strips are attached to the processor   */
#define BRIGHTNESS  99   /*  LED brightness, 0 (min) to 255 (max)            */

CRGB NEOs[ ( NUM_STRIPS * NUM_NEOS ) + 1 ];   /*  one extra for F0 function  */


#include <SoftPWM.h>

#define DATA_PIN_A   3   /*  Which pin on the Arduino is connected to LED A  */
#define DATA_PIN_B   4   /*  Which pin on the Arduino is connected to LED B  */
#define DATA_PIN_C   6   /*  Which pin on the Arduino is connected to LED C  */
#define DATA_PIN_D   7   /*  Which pin on the Arduino is connected to LED D  */

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
   uint8_t  outputState =   0 ;   /*  0, 1 or 2 status   */
   uint8_t  colorRed    = 255 ;   /*  value Red   color  */
   uint8_t  colorGreen  = 255 ;   /*  value Green color  */
   uint8_t  colorBlue   = 155 ;   /*  value Blue  color  */
   uint8_t  dimmingFact = 127 ;   /*  a dimming factor   */
   bool     isEnabled =  true ;   /*  output is in use   */
   bool     nowOn     =  true ;   /*  led is  ON or OFF  */
   // bool     countUp   =  true ;   /*  counting Up or Dn  */
   // long     cntValue  =  1255 ;   /*  countdown to zero  */
   // long     highCount =  1255 ;   /*  pulses high count  */
   // long     lowCount  =  1255 ;   /*  pulses  low count  */
};
QUEUE volatile *NEO_queue = new QUEUE[ ( NUM_STRIPS * NUM_NEOS ) + 1 ];   /*  one extra for F0 function  */



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
   {  45,   0},
   {  46,   0},
   {  47,   0},
   {  48,   0},
   {  49,   0},

   {  50,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F1  */
   {  51, 255},  /*  color value  red  channel  */
   {  52, 255},  /*  color value green channel  */
   {  53, 155},  /*  color value blue  channel  */
   {  54, 127},  /*  dimming factor of channel  */
   {  55,   0},  /*  fade-up time  ms  setting  */
   {  56,   0},  /*  fade-up time   multiplier  */
   {  57,   0},  /*  fade-down time   setting   */
   {  58,   0},  /*  fade-down time multiplier  */
   {  59,   0},  /*    */

   {  60,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F2  */
   {  61, 255},  /*  color value  red  channel  */
   {  62, 255},  /*  color value green channel  */
   {  63, 155},  /*  color value blue  channel  */
   {  64, 127},  /*  dimming factor of channel  */
   {  65,   0},  /*  fade-up time  ms  setting  */
   {  66,   0},  /*  fade-up time   multiplier  */
   {  67,   0},  /*  fade-down time   setting   */
   {  68,   0},  /*  fade-down time multiplier  */
   {  69,   0},  /*    */

   {  70,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F3  */
   {  71, 255},  /*  color value  red  channel  */
   {  72, 255},  /*  color value green channel  */
   {  73, 155},  /*  color value blue  channel  */
   {  74, 127},  /*  dimming factor of channel  */

   {  80,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F4  */
   {  81, 255},  /*  color value  red  channel  */
   {  82, 255},  /*  color value green channel  */
   {  83, 155},  /*  color value blue  channel  */
   {  84, 127},  /*  dimming factor of channel  */

   {  90,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F5  */
   {  91, 255},  /*  color value  red  channel  */
   {  92, 255},  /*  color value green channel  */
   {  93, 155},  /*  color value blue  channel  */
   {  94, 127},  /*  dimming factor of channel  */

   { 100,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> NEO   F6  */
   { 101, 255},  /*  color value  red  channel  */
   { 102, 255},  /*  color value green channel  */
   { 103, 155},  /*  color value blue  channel  */
   { 104, 127},  /*  dimming factor of channel  */





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
   // noInterrupts();


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


   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_0, starting at index 0 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_0, GRB>( NEOs,  0, NUM_NEOS );

   // tell FastLED there's 1 NEOPIXEL led on DATA_PIN_1, starting at index 1 in the NEOs array
   FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>( NEOs,  1, NUM_NEOS );

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
      SoftPWMSet( LEDs[ i ], 99 );

   SoftPWMSetFadeTime( ALL, 50, 400 );    /*  Set fade-up time to 50 ms, fade-down time to 400 ms  */




   // _PL(F( "***   TaskScheduler has Started   ***" ));
   _PL(F( "-------------------------------------" ));



   /*  calculate the settings for every output at startup  */
   for(int i = 0; i < FastLED.count(); ++i )
   {
      calculateNeoQueue( i );
   }


   displayText(); /* Shows the standard text. */


   interrupts();  /* Ready to rumble....*/

}


void loop()
{
   // for( int i = 0; i < FastLED.count(); ++i ) {
   //    NEOs[ i ] = CRGB( 255, 255, 155 );
   //    if ( i > 3 ) { fadeToBlackBy( &NEOs[ i ], 1, 127 ); }
   //    FastLED.show();
      // NEOs[ i ] = CRGB::Black;
   // }


   //  NEOs[ 0 ] = CRGB( led_queue[ 0 ].red, led_queue[ 0 ].green, led_queue[ 0 ].blue );
   //  NEOs[ 1 ] = CRGB( led_queue[ 1 ].red, led_queue[ 1 ].green, led_queue[ 1 ].blue );
   //  NEOs[ 2 ] = CRGB( led_queue[ 2 ].red, led_queue[ 2 ].green, led_queue[ 2 ].blue );
   //  NEOs[ 3 ] = CRGB( led_queue[ 3 ].red, led_queue[ 3 ].green, led_queue[ 3 ].blue );
   //  NEOs[ 4 ] = CRGB( led_queue[ 4 ].red, led_queue[ 4 ].green, led_queue[ 4 ].blue );
   //  NEOs[ 5 ] = CRGB( led_queue[ 5 ].red, led_queue[ 5 ].green, led_queue[ 5 ].blue );
   //  NEOs[ 6 ] = CRGB( led_queue[ 6 ].red, led_queue[ 6 ].green, led_queue[ 6 ].blue );
   //  FastLED.show();

/*  brightness control  */
// leds[5] = CRGB::Red;
// fadeToBlackBy(&leds[5], 1, 127);
// FastLED.show();

// CHSV _targetHSV = rgb2hsv_approximate(CRGB(red, green, blue)); // choose RGB color
// //*** OR ***
// CHSV _targetHSV = rgb2hsv_approximate(leds[0]); // to use the current color of leds[0]
// _targetHSV.v = value;  // set the brightness 0 -> 100
// leds[0] = _targetHSV; // update leds[0] with the HSV result
// FastLED.show(); // show result

// sensVal = constrain(sensVal, 10, 150);  // limits range of sensor values to between 10 and 150
// x = 9 % 5;  // x now contains 4



   Dcc.process();  /*  Call this process for Dcc connection  */

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


void calculateNeoQueue( int ledNumber )
{
   if ( run_switch_set[ 0 ] && NEO_queue[ ledNumber ].isEnabled )
   {
      int getCVNumber = ( ( ledNumber + 4 ) * 10 ) ;  /*  calculate the correct CVaddress  */

      NEO_queue[ ledNumber ].colorRed    = constrain( Dcc.getCV( getCVNumber + 1 ), 0, 255 );
      NEO_queue[ ledNumber ].colorGreen  = constrain( Dcc.getCV( getCVNumber + 2 ), 0, 255 );
      NEO_queue[ ledNumber ].colorBlue   = constrain( Dcc.getCV( getCVNumber + 3 ), 0, 255 );
      NEO_queue[ ledNumber ].dimmingFact = constrain( Dcc.getCV( getCVNumber + 4 ), 0, 255 );




   //    led_queue[ ledNumber ].outputState = Dcc.getCV( getCVNumber ) ;

   //    unsigned long hInterval = Dcc.getCV( getCVNumber + 1 ) * Dcc.getCV( getCVNumber + 2 ) ;
   //    // igned long lInterval = Dcc.getCV( getCVNumber + 3 ) * Dcc.getCV( getCVNumber + 4 ) ;

   //    led_queue[ ledNumber ].highCount = Dcc.getCV( getCVNumber + 5 ) * Dcc.getCV( getCVNumber + 6 );
   //    led_queue[ ledNumber ].lowCount  = Dcc.getCV( getCVNumber + 7 ) * Dcc.getCV( getCVNumber + 8 );

   //    led_queue[ ledNumber ].cntValue = led_queue[ ledNumber ].highCount ;

      NEOs[ ledNumber ] = CRGB( NEO_queue[ ledNumber ].colorRed, NEO_queue[ ledNumber ].colorGreen, NEO_queue[ ledNumber ].colorBlue );
      fadeToBlackBy( &NEOs[ ledNumber ], 1, NEO_queue[ ledNumber ].dimmingFact );


      _PP( " cv: " );
      _2P( getCVNumber + 0 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 0 ), DEC );
      _PP( " cv: " );
      _2P( getCVNumber + 1 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 1 ), DEC );
      _PP( " cv: " );
      _2P( getCVNumber + 2 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 2 ), DEC );
      _PP( " cv: " );
      _2P( getCVNumber + 3 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 3 ), DEC );
      _PP( " cv: " );
      _2P( getCVNumber + 4 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 4 ), DEC );


      _PL(F( "-------------------------------------" ));

   }
   else
   {
      // NEO_queue[ ledNumber ].nowOn = false;
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
 *    returns: <FD done cvCheck>
 */
      {
         uint8_t cvCheck = Dcc.setCV( NMRADCC_MASTER_RESET_CV, NMRADCC_MASTER_RESET_CV );

         _PP( "\t"  "<FD done>"  "\t" );
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
         int  fv, cvCheck;

         if( sscanf( com + 2, "%d", &fv ) != 1 ) { return; }  /*  get value and check for valid number  */

         switch ( fv )  /*  F0 to F12  */
         {

            case  0:
            case  1:
            case  2:
            case  3:
            case  4:
            case  5:
            case  6:
            {
               run_switch_set[ fv ] = ( !run_switch_set[ fv ] );

               NEO_queue[ fv ].isEnabled = ( run_switch_set[ fv ] );

               /*  calculate the time setting for almost every output  */
               for(int i = 0; i < FastLED.count(); ++i )
               {
                  calculateNeoQueue( i );
               }

               break;
            }

            case  7:
            case  8:
            case  9:
            case 10:
            case 11:
            case 12:
            default:
               break;
         }


         cvCheck = Dcc.getCV( 40 + ( fv * 10 ) );

         _PL(F( "wait for the cycle to end..." ) );
         _PP(F( "<f F"                         ) );
         _2P(   fv + 0 ,                   DEC )  ;
         _PP(F( " "                            ) );
         _2P(   cvCheck,                   DEC )  ;
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

         _PP( "<r " );
         _2P( cv + 0 , DEC );
         _PP( " "   );
         _2P( cvCheck, DEC );
         _PL( ">"   );

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
         for(int i = 0; i < FastLED.count(); ++i )
         {
            calculateNeoQueue( i );
         }

/*  TODO: here we still need the calculation for the LEDs  */

         _PP( "<w " );
         _2P( cv + 0 , DEC );
         _PP( " "   );
         _2P( cvCheck, DEC );
         _PL( ">"   );

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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





/*  */
