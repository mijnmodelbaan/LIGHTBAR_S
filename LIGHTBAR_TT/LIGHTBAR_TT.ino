
/** 
 * 
 *    LIGHTBAR_TT
 *
**/

/*  test has to be done with real DCC events
      calculateLedQueue( fv );
*/

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
#define TESTRUN
//


/* don't print warnings of unused functions from here */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"


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


#define _TASK_SLEEP_ON_IDLE_RUN // will compile the library with the sleep option enabled (AVR boards only).

#include <TaskScheduler.h>

Scheduler  taskTimer;


#pragma GCC diagnostic pop
/* don't print warnings of unused functions till here */


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

/* these tasks start the blinking of the LEDs */
void taskL01Callback();
Task taskL01( 1255, TASK_FOREVER, &taskL01Callback, NULL, NULL, NULL, NULL );
void taskL02Callback();
Task taskL02( 1255, TASK_FOREVER, &taskL02Callback, NULL, NULL, NULL, NULL );
void taskL03Callback();
Task taskL03( 1255, TASK_FOREVER, &taskL03Callback, NULL, NULL, NULL, NULL );
void taskL04Callback();
Task taskL04( 1255, TASK_FOREVER, &taskL04Callback, NULL, NULL, NULL, NULL );
void taskL05Callback();
Task taskL05( 1255, TASK_FOREVER, &taskL05Callback, NULL, NULL, NULL, NULL );
void taskL06Callback();
Task taskL06( 1255, TASK_FOREVER, &taskL06Callback, NULL, NULL, NULL, NULL );
void taskL07Callback();
Task taskL07( 1255, TASK_FOREVER, &taskL07Callback, NULL, NULL, NULL, NULL );
void taskL08Callback();
Task taskL08( 1255, TASK_FOREVER, &taskL08Callback, NULL, NULL, NULL, NULL );
void taskL09Callback();
Task taskL09( 1255, TASK_FOREVER, &taskL09Callback, NULL, NULL, NULL, NULL );
void taskL10Callback();
Task taskL10( 1255, TASK_FOREVER, &taskL10Callback, NULL, NULL, NULL, NULL );


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool run_switch_set[ ] = { false, true, true, true, true, true, true, true, true, true, true };


struct QUEUE
{
   uint8_t  outputState =   0 ;   /*  0, 1 or 2 status   */
   bool     isEnabled =  true ;   /*  output is in use   */
   bool     nowOn     =  true ;   /*  led is  ON or OFF  */
   bool     countUp   =  true ;   /*  counting Up or Dn  */
   long     cntValue  =  1255 ;   /*  countdown to zero  */
   long     highCount =  1255 ;   /*  pulses high count  */
   long     lowCount  =  1255 ;   /*  pulses  low count  */
};
QUEUE volatile *led_queue = new QUEUE[ 11 ];   /* F0 to F10 */


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

   {  40,   1},  /*  0 = disable all LEDs, 1 = enable all LEDs ()>> at start up or reset)  */
   {  41,   0},

   {  50,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  1  */
   {  51,  25},  /*  timing high pulses  */
   {  52,   1},  /*  timing high multiplier  */
   {  53,  50},  /*  timing  low pulses  */
   {  54,   1},  /*  timing  low multiplier  */
   {  55,  50},  /*  output   ON pulses  */
   {  56,   1},  /*  output   ON multiplier  */
   {  57,  50},  /*  output  OFF pulses  */
   {  58,   1},  /*  output  OFF multiplier  */
   {  59,   0},

   {  60,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  2  */
   {  61,  50},  /*  timing high pulses  */
   {  62,   1},  /*  timing high multiplier  */
   {  63, 150},  /*  timing  low pulses  */
   {  64,   1},  /*  timing  low multiplier  */
   {  65,  50},  /*  output   ON pulses  */
   {  66,   1},  /*  output   ON multiplier  */
   {  67,  50},  /*  output  OFF pulses  */
   {  68,   1},  /*  output  OFF multiplier  */
   {  69,   0},

   {  70,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  3  */
   {  71,  75},  /*  timing high pulses  */
   {  72,   1},  /*  timing high multiplier  */
   {  73,  50},  /*  timing  low pulses  */
   {  74,   1},  /*  timing  low multiplier  */
   {  75,  50},  /*  output   ON pulses  */
   {  76,   1},  /*  output   ON multiplier  */
   {  77,  50},  /*  output  OFF pulses  */
   {  78,   1},  /*  output  OFF multiplier  */
   {  79,   0},

   {  80,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  4  */
   {  81, 100},  /*  timing high pulses  */
   {  82,   1},  /*  timing high multiplier  */
   {  83, 250},  /*  timing  low pulses  */
   {  84,   1},  /*  timing  low multiplier  */
   {  85,  50},  /*  output   ON pulses  */
   {  86,   1},  /*  output   ON multiplier  */
   {  87,  50},  /*  output  OFF pulses  */
   {  88,   1},  /*  output  OFF multiplier  */
   {  89,   0},

   {  90,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  5  */
   {  91, 125},  /*  timing high pulses  */
   {  92,   1},  /*  timing high multiplier  */
   {  93,  50},  /*  timing  low pulses  */
   {  94,   1},  /*  timing  low multiplier  */
   {  95,  50},  /*  output   ON pulses  */
   {  96,   1},  /*  output   ON multiplier  */
   {  97,  50},  /*  output  OFF pulses  */
   {  98,   1},  /*  output  OFF multiplier  */
   {  99,   0},

   { 100,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  6  */
   { 101, 150},  /*  timing high pulses  */
   { 102,   1},  /*  timing high multiplier  */
   { 103,  50},  /*  timing  low pulses  */
   { 104,   1},  /*  timing  low multiplier  */
   { 105,  50},  /*  output   ON pulses  */
   { 106,   1},  /*  output   ON multiplier  */
   { 107,  50},  /*  output  OFF pulses  */
   { 108,   1},  /*  output  OFF multiplier  */
   { 109,   0},

   { 110,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  7  */
   { 111, 175},  /*  timing high pulses  */
   { 112,   1},  /*  timing high multiplier  */
   { 113,  50},  /*  timing  low pulses  */
   { 114,   1},  /*  timing  low multiplier  */
   { 115,  50},  /*  output   ON pulses  */
   { 116,   1},  /*  output   ON multiplier  */
   { 117,  50},  /*  output  OFF pulses  */
   { 118,   1},  /*  output  OFF multiplier  */
   { 119,   0},

   { 120,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  8  */
   { 121, 200},  /*  timing high pulses  */
   { 122,   1},  /*  timing high multiplier  */
   { 123,  50},  /*  timing  low pulses  */
   { 124,   1},  /*  timing  low multiplier  */
   { 125,  50},  /*  output   ON pulses  */
   { 126,   1},  /*  output   ON multiplier  */
   { 127,  50},  /*  output  OFF pulses  */
   { 128,   1},  /*  output  OFF multiplier  */
   { 129,   0},

   { 130,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  9  */
   { 131, 225},  /*  timing high pulses  */
   { 132,   1},  /*  timing high multiplier  */
   { 133,  50},  /*  timing  low pulses  */
   { 134,   1},  /*  timing  low multiplier  */
   { 135,  50},  /*  output   ON pulses  */
   { 136,   1},  /*  output   ON multiplier  */
   { 137,  50},  /*  output  OFF pulses  */
   { 138,   1},  /*  output  OFF multiplier  */
   { 139,   0},

   { 140,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED 10  */
   { 141, 250},  /*  timing high pulses  */
   { 142,   1},  /*  timing high multiplier  */
   { 143,  50},  /*  timing  low pulses  */
   { 144,   1},  /*  timing  low multiplier  */
   { 145,  50},  /*  output   ON pulses  */
   { 146,   1},  /*  output   ON multiplier  */
   { 147,  50},  /*  output  OFF pulses  */
   { 148,   1},  /*  output  OFF multiplier  */
   { 149,   0},


   { 251,   0},  //  NMRADCC_SIMPLE_RESET_CV */
   { 252,   0},  //  NMRADCC_MASTER_RESET_CV */
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
   DDRD  = DDRD  | 0b11110000;  // Set bits D[7-4] as outputs, leave the rest as-is.

   PORTB = PORTB & 0b11000000;  // Reset bits B[5-0], leave the rest as-is.
   PORTD = PORTD & 0b00001111;  // Reset bits D[7-4], leave the rest as-is.


#if defined( TESTRUN )

   PORTB = PORTB | 0b00111111;  // Switch bits B[5-0] to  ON, leave the rest as-is.
   PORTD = PORTD | 0b11110000;  // Switch bits D[7-4] to  ON, leave the rest as-is.

   PORTB = PORTB & 0b11000000;  // Switch bits B[5-0] to OFF, leave the rest as-is.
   PORTD = PORTD & 0b00001111;  // Switch bits D[7-4] to OFF, leave the rest as-is.

#endif


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


   //  Call the DCC 'pin' and 'init' functions to enable the DCC Receivermode
   Dcc.pin( digitalPinToInterrupt( FunctionPinDcc ), FunctionPinDcc, false );
   Dcc.init( MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, 0 );      delay( 1000 );
   //c.initAccessoryDecoder( MAN_ID_DIY,  201, FLAGS_MY_ADDRESS_ONLY,    0 );


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


   Dcc.setCV( NMRADCC_SIMPLE_RESET_CV, 0 );  /*  Reset the just_a_reset CV */


   _PL(F( "-------------------------------------" ));
   _PL(F( "***   TaskScheduler is Starting   ***" ));
   _PL(F( "-------------------------------------" ));


   /* initialize (and enable) all necessary schedules */
   taskTimer.init();  // Not really necessary - security

   taskTimer.addTask( taskL01 );
   taskTimer.addTask( taskL02 );
   taskTimer.addTask( taskL03 );
   taskTimer.addTask( taskL04 );
   taskTimer.addTask( taskL05 );
   taskTimer.addTask( taskL06 );
   taskTimer.addTask( taskL07 );
   taskTimer.addTask( taskL08 );
   taskTimer.addTask( taskL09 );
   taskTimer.addTask( taskL10 );

   taskL01.enableDelayed(    0);
   taskL02.enableDelayed(    0);
   taskL03.enableDelayed(    0);
   taskL04.enableDelayed(    0);
   taskL05.enableDelayed(    0);
   taskL06.enableDelayed(    0);
   taskL07.enableDelayed(    0);
   taskL08.enableDelayed(    0);
   taskL09.enableDelayed(    0);
   taskL10.enableDelayed(    0);


   _PL(F( "***   TaskScheduler has Started   ***" ));
   _PL(F( "-------------------------------------" ));


   run_switch_set[ 0 ] = ( Dcc.getCV( 40 ) == 0 ? false : true );

   /*  calculate the settings for every output at startup  */
   for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
   {
      calculateLedQueue( i );
   }


   displayText(); /* Shows the standard text. */


   interrupts();  /* Ready to rumble....*/
}


void loop()
{
   Dcc.process();  /*  Call this process for Dcc connection  */

   taskTimer.execute();   /*  Keeps all the timers running...*/

   if ( foundEom )  /*  Serial Input needs message-handling  */
   {
      foundBom = false;
      foundEom = false;
      parseCom( commandString );
   }

}

// bool run_switch_set[ ] = { true, true, true, true, true, true, true, true, true, true, true, true, true };


   // {  90,   1},  /*  0 = disabled - 1 = normal - 2 = blinking >> LED  5  */
   // {  91,  50},  /*  timing high pulses  */
   // {  92,   1},  /*  timing high multiplier  */
   // {  93,  50},  /*  timing  low pulses  */
   // {  94,   1},  /*  timing  low multiplier  */
   // {  95,  50},  /*  output   ON pulses  */
   // {  96,   1},  /*  output   ON multiplier  */
   // {  97,  50},  /*  output  OFF pulses  */
   // {  98,   1},  /*  output  OFF multiplier  */
   // {  99,   0},


// struct QUEUE
// {
//    uint8_t  outputState =  0 ;   /*  0, 1 or 2 status   */

//    bool  isEnabled = true ;  /*  output is in use   */

//    bool  nowOn     = true ;  /*  led is  ON or OFF  */

//    bool  countUp   = true ;  /*  counting Up or Dn  */
//    long  cntValue   =   0 ;  /*  countdown to zero  */

//    long  highCount  =   0 ;  /*  pulses high count  */ not here
//    long  lowCount   =   0 ;  /*  pulses  low count  */ not here
// };

void taskL01Callback()
{
   PORTD = ( run_switch_set[ 0 ] && led_queue[  1 ].nowOn ? PORTD ^ 0b00010000 : PORTD & 0b11101111 );

   taskL01.setCallback( &taskL01Callback );
}


void taskL02Callback()
{
  PORTD = ( run_switch_set[ 0 ] && led_queue[  2 ].nowOn ? PORTD ^ 0b00100000 : PORTD & 0b11011111 );

   taskL02.setCallback( &taskL02Callback );
}


void taskL03Callback()
{
  PORTD = ( run_switch_set[ 0 ] && led_queue[  3 ].nowOn ? PORTD ^ 0b01000000 : PORTD & 0b10111111 );

   taskL03.setCallback( &taskL03Callback );
}


void taskL04Callback()
{
  PORTD = ( run_switch_set[ 0 ] && led_queue[  4 ].nowOn ? PORTD ^ 0b10000000 : PORTD & 0b01111111 );

   taskL04.setCallback( &taskL04Callback );
}


void taskL05Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[  5 ].nowOn ? PORTB ^ 0b00000001 : PORTB & 0b11111110 );

   taskL05.setCallback( &taskL05Callback );
}


void taskL06Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[  6 ].nowOn ? PORTB ^ 0b00000010 : PORTB & 0b11111101 );

   taskL06.setCallback( &taskL06Callback );
}


void taskL07Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[  7 ].nowOn ? PORTB ^ 0b00000100 : PORTB & 0b11111011 );

   taskL07.setCallback( &taskL07Callback );
}


void taskL08Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[  8 ].nowOn ? PORTB ^ 0b00001000 : PORTB & 0b01110111 );

   taskL08.setCallback( &taskL08Callback );
}


void taskL09Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[  9 ].nowOn ? PORTB ^ 0b00010000 : PORTB & 0b11101111 );

   taskL09.setCallback( &taskL09Callback );
}


void taskL10Callback()
{
  PORTB = ( run_switch_set[ 0 ] && led_queue[ 10 ].nowOn ? PORTB ^ 0b00100000 : PORTB & 0b11011111 );

   taskL10.setCallback( &taskL10Callback );
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
   if ( run_switch_set[ 0 ] && led_queue[ ledNumber ].isEnabled )
   {
      led_queue[ ledNumber ].nowOn = true ;
      led_queue[ ledNumber ].countUp = true ;

      int getCVNumber = ( ( ledNumber + 4 ) * 10 ) ;

      led_queue[ ledNumber ].outputState = Dcc.getCV( getCVNumber ) ;

      unsigned long hInterval = Dcc.getCV( getCVNumber + 1 ) * Dcc.getCV( getCVNumber + 2 ) ;
      // igned long lInterval = Dcc.getCV( getCVNumber + 3 ) * Dcc.getCV( getCVNumber + 4 ) ;

      led_queue[ ledNumber ].highCount = Dcc.getCV( getCVNumber + 5 ) * Dcc.getCV( getCVNumber + 6 );
      led_queue[ ledNumber ].lowCount  = Dcc.getCV( getCVNumber + 7 ) * Dcc.getCV( getCVNumber + 8 );

      led_queue[ ledNumber ].cntValue = led_queue[ ledNumber ].highCount ;


      _PP( " cv: " );
      _2P( getCVNumber + 1 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 1 ), DEC );

      _PP( " cv: " );
      _2P( getCVNumber + 2 , DEC );
      _PP( " value: " );
      _2L( Dcc.getCV( getCVNumber + 2 ), DEC );

      _PP( " hInterval: " );
      _2L( hInterval, DEC );

      _PL(F( "-------------------------------------" ));




      switch (ledNumber)
      {
      case  1:
         taskL01.disable();   taskL01.setInterval( hInterval );    taskL01.enable();
         break;
      
      case  2:
         taskL02.disable();   taskL02.setInterval( hInterval );    taskL02.enable();
         break;
      
      case  3:
         taskL03.disable();   taskL03.setInterval( hInterval );    taskL03.enable();
         break;
      
      case  4:
         taskL04.disable();   taskL04.setInterval( hInterval );    taskL04.enable();
         break;
      
      case  5:
         taskL05.disable();   taskL05.setInterval( hInterval );    taskL05.enable();
         break;
      
      case  6:
         taskL06.disable();   taskL06.setInterval( hInterval );    taskL06.enable();
         break;
      
      case  7:
         taskL07.disable();   taskL07.setInterval( hInterval );    taskL07.enable();
         break;
      
      case  8:
         taskL08.disable();   taskL08.setInterval( hInterval );    taskL08.enable();
         break;
      
      case  9:
         taskL09.disable();   taskL09.setInterval( hInterval );    taskL09.enable();
         break;
      
      case 10:
         taskL10.disable();   taskL10.setInterval( hInterval );    taskL10.enable();
         break;
      
      default:
         break;
      }
   }
   else
   {
      led_queue[ ledNumber ].nowOn = false;
   }
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
         // if comandString still has space, append character just read from serial line
         // otherwise, character is ignored, (but we'll continue to look for '<' or '>')
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
 *    returns: <FD done check>
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
         int  fv ;
         uint8_t cvCheck ;

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
            case  7:
            case  8:
            case  9:
            case 10:
            {
               run_switch_set[ fv ] = ( !run_switch_set[ fv ] );

               // led_queue[ fv ].isEnabled = ( !led_queue[ fv ].isEnabled );
               led_queue[ fv ].isEnabled = ( run_switch_set[ fv ] );

               /*  calculate the time setting for almost every output  */
               for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
               {
                  calculateLedQueue( i );
               }

               break;
            }

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
         for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
         {
            calculateLedQueue( i );
         }

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

/************************************************************************************
            Call-back functions from DCC
************************************************************************************/


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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* **********************************************************************************
 *  notifyDccSpeed() Callback for a multifunction decoder speed command.
 *                   The received speed and direction are unpacked to separate values
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


/* **********************************************************************************
 *    notifyDccFunc() Callback for a multifunction decoder function command.
 *
 *  Inputs:
 *    Addr        - Active decoder address.
 *    AddrType    - DCC_ADDR_SHORT or DCC_ADDR_LONG.
 *    FuncGrp     - Function group. FN_0      - 14 speed headlight  Mask FN_BIT_00
 * 
 *                                  FN_0_4    - Functions  0 to  4. Mask FN_BIT_00 - FN_BIT_04
 *                                  FN_5_8    - Functions  5 to  8. Mask FN_BIT_05 - FN_BIT_08
 *                                  FN_9_12   - Functions  9 to 12. Mask FN_BIT_09 - FN_BIT_12
 *                                  FN_13_20  - Functions 13 to 20. Mask FN_BIT_13 - FN_BIT_20 
 *                                  FN_21_28  - Functions 21 to 28. Mask FN_BIT_21 - FN_BIT_28
 *    FuncState   - Function state. Bitmask where active functions have a 1 at that bit.
 *                                  You must AND FuncState with the appropriate
 *                                  FN_BIT_nn value to isolate a given bit.
 *
 *  Returns:
 *    None
 */
void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState )
{
   _PL("notifyDccFunc");
}


/* **********************************************************************************
 *  notifyCVChange()     Called when a CV value is changed.
 *                       This is called whenever a CV's value is changed.
 *
 *  notifyDccCVChange()  Called only when a CV value is changed by a Dcc packet or a lib function.
 *                       It is NOT called if the CV is changed by means of the setCV() method.
 *                       Note: It is not called if notifyCVWrite() is defined
 *                       or if the value in the EEPROM is the same as the value
 *                       in the write command. 
 *
 *  Inputs:
 *    CV        - CV number.
 *    Value     - Value of the CV.
 *
 *  Returns:
 *    None
 */
void    notifyCVChange( uint16_t CV, uint8_t Value )
// d notifyDccCVChange( uint16_t CV, uint8_t Value )
{
   _PP( "notifyCVChange: CV: " );
   _2P( CV,                DEC );
   _PP( "\t" " Value: "        );
   _2L( Value,             DEC );

   if ( ( ( CV == 251 ) && ( Value == 251 ) ) || ( ( CV == 252 ) && ( Value == 252 ) ) )
   {
      wdt_enable( WDTO_15MS );  //  Resets after 15 milliSecs

      while( 1 ) {}  // Wait for the prescaler time to expire
   }


   /*  calculate the time setting for almost every output  */
   for(int i = 0; i < (int)(sizeof( run_switch_set ) ); ++i )
   {
      calculateLedQueue( i );
   }

}       //   end notifyCVChange()


/* */
