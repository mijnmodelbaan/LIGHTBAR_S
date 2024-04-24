
/* 
 * 
 * Interactive Decoder                             IDEC1_1_MotSound5Led.ino
 * Version 1.08 Geoff Bunza 2020
 * Works with both short and long DCC Addesses
 * 
 * This decoder uses switches / sensors to control 2 motors and 5 LEDs with Sound
 * F0 = Master Function  OFF = Functioning,  ON DISABLES the decoder
 * Input Pin for Decoder Disable  Pin 16 Active LOW
 * Motor speed via DCC speed for one motor, second motor on/off via function
 * Speed Over-Ride =  CV = Non-Zero Value (1-127) over-rides the DCC speed commands Bit 8 is direction 1=Forward
 * Input1 Pin for Throttle Down/Pause/Throttle Up  Pin 5 
 *    CV for Throttle Down Time, CV for Throttle Up Time, CV for Pause Time
 * Input2 Pin for Throttle Down/Pause/Throttle Up  Pin 6
 *    CV for Throttle Down Time, CV for Throttle Up Time, CV for Pause Time
 * Input Pin1 for Throttle Down/Reverse/Throttle Up Pin 7
 *    CV for Throttle Down Time, CV for Throttle Up Time, CV for Reverse Pause Time
 * Input Pin2 for Throttle Down/Reverse/Throttle Up Pin 8
 *    CV for Throttle Down Time, CV for Throttle Up Time, CV for Reverse Pause Time
 * 
 * Input Pin for immediate Stop  Pin 11
 * Input Pin for Immediate Start Pin 12
 * 
 * Functions for lights on/off:
 * F1-F5  5 Functions LED ON/OFF by default PINS 13,14,17,18,19
 * Pro Mini  D15 A1 (TX) connected to  DFPlayer1 Receive  (RX) Pin 2 via 1K Ohm 1/4W Resistor
 *  Remember to connect +5V and GND to the DFPlayer too: DFPLAYER PINS 1 & 7 respectively
 *  This is a “mobile / function” decoder with audio play to dual motor control and
 *  LED functions.  Audio tracks or clips are stored on a micro SD card for playing
 *  in a folder labeled 'mp3', with tracks named 0001.mp3, 0002.mp3, 0003.mp3, etc.
 * 
 * MAX 3 Configurations per pin function:
 *   Config 0 = Decoder DISABLE On / Off, 1 = LED; 2 = Motor2 Control On / Off
 *   F0 == Master Decoder Disable == ON
 *   F1 == LED == D13
 *   F2 == LED == D14/A0
 *   F3 == LED == D17/A3
 *   F4 == LED == D18/A4
 *   F5 == LED == D19/A5
 *   F6 == Motor2 On / Off speed & direction, set by CV 80.
 *   Normally Base Station will Transmit F5 as initial OFF.
 *   If no DCC present Decoder will power up with Motor2 ON at speed specified in CV 80
 *   Motor1 speed control is via throttle or overridden by non zero value in CV 50 
 *   High Bit = Direction, Lower 7 Bits = Speed (DSSSSSSS).
 * 
 * PRO MINI PIN ASSIGNMENT:
 *  2 D2 - DCC Input
 *  3 D3 - M1h Motor Control
 *  4 D4 - M1l Motor Control
 *  5 D5 - Input1 Pin for Throttle Down/ Pause /Throttle Up
 *  6 D6 - Input2 Pin for Throttle Down/ Pause /Throttle Up
 *  7 D7 - Input1 Pin for Throttle Down/Reverse/Throttle Up
 *  8 B0 - Input2 Pin for Throttle Down/Reverse/Throttle Up
 *  9 B1 - M0h Motor Control
 * 10 B2 - M0l Motor Control
 * 11 B3 - Input Pin for immediate  Stop
 * 12 B4 - Input Pin for Immediate Start 
 * 13 B5 - LED F1
 * 14 A0 - Input Pin for MasterDecoderDisable Active LOW
 * 15 A1 - (TX) connected to  DFPlayer1 Receive  (RX) Pin 2 via 1K Ohm 1/4W Resistor
 * 16 A2 - LED F2
 * 17 A3 - LED F3
 * 18 A4 - LED F4
 * 19 A5 - LED F5
 * 
 */

#include <Arduino.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ***** Uncomment to use the PinChangeInterrupts iso External Interrupts *****
// #define PIN_CHANGE_INT

#include <NmraDcc.h>

NmraDcc     Dcc ;
DCC_MSG  Packet ;


#include <avr/wdt.h>  //  Needed for automatic reset functions.


#define This_Decoder_Address       40

uint8_t CV_DECODER_JUST_A_RESET = 251;
uint8_t CV_DECODER_MASTER_RESET = 252;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE THE SOUND FUNCTIONS (SD CARD)
// #define SOUNDON

#define default_volume  25     //  Sets default volume 0-30,  0 == OFF,  >30 == Skip Track

#if defined( SOUNDON )

   #include <SoftwareSerial.h>

   SoftwareSerial DFSerial1( 21, 15 ); // PRO MINI RX, PRO MINI TX serial to DFPlayer


   #include <DFRobotDFPlayerMini.h>

   DFRobotDFPlayerMini Player1;


   #define First_Track  1   //  Play Random Tracks - First_Track = Start_Track  >= First_Track
   #define Last_Track  12   //  Play Random Tracks - Last_Track = Last Playable Track in Range  <= Last_Track

   void playTrackOnChannel( byte dtrack  );
   void setVolumeOnChannel( byte dvolume );

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** REMOVE THE "//" IN THE NEXT LINE UNLESS YOU WANT ALL CV'S RESET ON EVERY POWER-UP
#define DECODER_LOADED

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE THE TWO MOTOR CONTROL OUTPUTS
// #define MOTORON

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE YOUR SERIAL PORT FOR COMMANDS
// #define MONITOR

// ******** REMOVE THE "//" IN THE NEXT LINE TO SEE DETAILED INFO ABOUT INCOMING C.V MESSAGES
// #define DEBUGCV

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL OUTPUT
// #define DEBUGID

#if defined( DEBUGID ) || defined( DEBUGCV ) || defined( MONITOR )
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Uncomment ONLY ONE of the following timing constants:
//
//#define  MasterTimeConstant  10L      // 10 milliseconds Timing
#define  MasterTimeConstant  100L     // 100 milliseconds Timing
//#define  MasterTimeConstant  1000L    // Seconds Timing
//#define  MasterTimeConstant  10000L   // 10 Seconds Timing
//#define  MasterTimeConstant  60000L   // Minutes Timing
//#define  MasterTimeConstant  3600000L // Hours Timing


const int audiocmddelay            =   34;
boolean Use_DCC_speed              = true;  //  Switch to disable DCC Speed updates

int Motor1Speed                    =    0;  //  Variablw for Motor1 Speed
int Starting_Motor1Speed           =    0;
int Motor1ForwardDir               =    1;  //  Variable for Motor1 Dir
int ForcedStopSpeedMotor1          =    0;  //  Holding Variable for Last Speed when Immediate Stop
int ForcedStopDirMotor1            =    1;  //  Holding Variable for Last Direction when Immediate Stop
int Motor2Speed                    =    0;  //  Variable for Motor2 Speed
int Motor2ForwardDir               =    1;  //  Variable for Motor2 Dir
int Motor2ON                       =    0;
int fwdon                          =    0;
int fwdtime                        =    1;
int bwdon                          =    0;
int bwdtime                        =    1;
int cyclewidth                     = 2048;
int loopdelay                      =    0;

const int m2h                      =    3;  //  R H Bridge    Motor1
const int m2l                      =    4;  //  B H Bridge    Motor1
const int ThrottlePause1Pin        =    5;  //  Throttle Speed Pause1 Input Pin
const int ThrottlePause2Pin        =    6;  //  Throttle Speed Pause2 Input Pin
const int ThrottleInputReversePin1 =    7;  //  Throttle Speed Reverse Input Pin
const int ThrottleInputReversePin2 =    8;  //  Throttle Immediate Speed Reverse Input Pin
const int m0h                      =    9;  //  R H Bridge    Motor2
const int m0l                      =   10;  //  B H Bridge    Motor2
const int ImmediateStopPin         =   11;  //  Throttle Immediate Stop Input Pin
const int ImmediateStartPin        =   12;  //  Throttle Immediate Start Input Pin
const int MasterDecoderDisablePin  =   14;  //  A0  Master Decoder Disable Input Pin Active LOW

const int numfpins                 =   10;  //  Number of Output pins to initialize
const int num_active_functions     =    7;  //  Number of functions stating with F0

byte fpins []                      = { 13, 15, 16, 17, 18, 19,  3,  4,  9, 10 };
                                            //  These are all the Output Pins

const int FunctionPin0             =   20;  //  A1 DFPlayer Transmit (TX)
const int FunctionPin1             =   13;  //  B5 LED 1
const int FunctionPin2             =   16;  //  A2 LED 2
const int FunctionPin3             =   17;  //  A3 LED 3
const int FunctionPin4             =   18;  //  A4 LED 4
const int FunctionPin5             =   19;  //  A5 LED 5
const int FunctionPin6             =   20;  //  Turns on Motor2 DCC Function Control Only NO Local Input Pin
const int FunctionPinDcc           =    2;  //  Input signal from Dcc input to decoder

int MasterDecoderDisable           =    0;
int Function0_value                =    0;


struct QUEUE
{
   int            inuse;   // pin in use or not
   int current_position;
   int        increment;
   int       stop_value;
   int      start_value;
};
QUEUE *ftn_queue = new QUEUE[ 17 ];

struct CVPair
{
   uint16_t    CV;
   uint8_t  Value;
};


CVPair FactoryDefaultCVs [] =
{
   {CV_MULTIFUNCTION_PRIMARY_ADDRESS, This_Decoder_Address & 0x7F },

   // These two CVs define the Long DCC Address
   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, (( This_Decoder_Address >> 8 ) & 0x7F ) + 192 },
   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB,    This_Decoder_Address & 0xFF },

   // ONLY uncomment 1 CV_29_CONFIG line below as approprate DEFAULT IS SHORT ADDRESS
// {CV_29_CONFIG,                0},                        // Short Address 14 Speed Steps
   {CV_29_CONFIG, CV29_F0_LOCATION},                        // Short Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},  // Long  Address 28/128 Speed Steps

   {CV_DECODER_MASTER_RESET,     0},

   { 30,   0}, //  F0 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 31,   1}, //  F1 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 32,   1}, //  F2 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 33,   1}, //  F3 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 34,   1}, //  F4 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 35,   1}, //  F5 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 36,   2}, //  F6 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented

   { 37,   4}, //  F7 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 38,   4}, //  F8 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 39,   4}, //  F9 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 40,   4}, // F10 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 41,   4}, // F11 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 42,   4}, // F12 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 43,   4}, // F13 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 44,   4}, // F14 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 45,   4}, // F15 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 46,   4}, // F16 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 47,   4}, // F17 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 48,   4}, // F18 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented
   { 49,   4}, // F19 Config 0=DISABLE On/Off,1=LED,2=Motor2 Control On/Off,3=NOT Implemented

   { 50,   0}, // Speed Over-Ride = Non-Zero. Value (1-127) over-rides the DCC speed commands
               // Bit 8 (128 or 0x80) 1 = Forward Direction 0 = Reverse Direction

   { 51,  75}, // ThrottlePause1         Pause Time 0 - 255  (0.1 secs)
   { 52,   3}, // ThrottlePause1         Ramp DOWN Delay 0 - 255
   { 53,   3}, // ThrottlePause1         Ramp UP   Delay 0 - 255
   { 54,   1}, // ThrottlePause1         Sound Clip 1 - nn 0 = No Sound
   { 55,  20}, // ThrottlePause1         Sound Clip Volume 0 - 30

   { 56,  22}, // ThrottlePause2         Pause Time 0 - 255  (0.1 secs)
   { 57,   5}, // ThrottlePause2         Ramp DOWN delay 0 - 255
   { 58,   5}, // ThrottlePause2         Ramp UP   delay 0 - 255
   { 59,   1}, // ThrottlePause2         Sound Clip 1 - nn 0 = No Sound
   { 60,  20}, // ThrottlePause2         Sound Clip Volume 0 - 30

   { 61,  28}, // ThrottleInputReverse1  Pause Time 0 - 255  (0.1 secs)
   { 62,   3}, // ThrottleInputReverse1  Ramp DOWN Delay 0 - 255
   { 63,   3}, // ThrottleInputReverse1  Ramp UP   Delay 0 - 255
   { 64,   2}, // ThrottleInputReverse1  Sound Clip 1 - nn 0 = No Sound
   { 65,  20}, // ThrottleInputReverse1  Sound Clip Volume 0 - 30

   { 66,  22}, // ThrottleInputReverse2  Pause Time 0 - 255  (0.1 secs)
   { 67,   5}, // ThrottleInputReverse2  Ramp DOWN Delay 0 - 255
   { 68,   5}, // ThrottleInputReverse2  Ramp UP   Delay 0 - 255
   { 69,   2}, // ThrottleInputReverse2  Sound Clip 1 - nn 0 = No Sound
   { 70,  20}, // ThrottleInputReverse2  Sound Clip Volume 0 - 30

   { 71,   0}, // ThrottleImmediateStop  Sound Clip 1 - nn 0 = No Sound
   { 72,  20}, // ThrottleImmediateStop  Sound Clip Volume 0 - 30

   { 73,   0}, // ThrottleImmediateStart Sound Clip 1 - nn 0 = No Sound
   { 74,  20}, // ThrottleImmediateStart Sound Clip Volume 0 - 30

   { 80,   0}, // Motor2 Speed 0-127 
               // Bit 8 (128 or 0x80) 1 = Forward Direction 0 = Reverse Direction

   {251,   0},  //  CV_DECODER_JUST_A_RESET */
/* {252,   0},  //  CV_DECODER_MASTER_RESET */
   {253,   0},  //  Extra
};


/* ******************************************************************************* */


uint8_t FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset 
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
};


/* ******************************************************************************* */


#if defined ( MOTORON )

   void gofwd1( int fcnt, int fcycle );
   void gobwd1( int bcnt, int bcycle );
   void gofwd2( int fcnt, int fcycle );
   void gobwd2( int bcnt, int bcycle );

#endif


/* ******************************************************************************* */


void setup()
{
   noInterrupts();

      // initialize the digital pins as outputs
   for ( int i = 0; i < numfpins; ++i )
   {
      digitalWrite( fpins[ i ], 0 );  // Switch the pin off first.
      pinMode( fpins[ i ], OUTPUT );  // Then set it as an output.
   }

   pinMode( ThrottlePause1Pin,        INPUT_PULLUP );  //  Throttle Speed Pause1 Input Pin    - Active LOW
   pinMode( ThrottlePause2Pin,        INPUT_PULLUP );  //  Throttle Speed Pause2 Input Pin    - Active LOW
   pinMode( ThrottleInputReversePin1, INPUT_PULLUP );  //  Throttle Speed Reverse Input Pin1  - Active LOW
   pinMode( ThrottleInputReversePin2, INPUT_PULLUP );  //  Throttle Speed Reverse Input Pin2  - Active LOW
   pinMode( ImmediateStopPin,         INPUT_PULLUP );  //  Throttle Immediate Stop Input Pin  - Active LOW
   pinMode( ImmediateStartPin,        INPUT_PULLUP );  //  Throttle Immediate Start Input Pin - Active LOW
   pinMode( MasterDecoderDisablePin,  INPUT_PULLUP );  //  Master Decoder Disable Input Pin   - Active LOW

   #if defined( DEBUGID ) || defined( DEBUGCV ) || defined( MONITOR )

      Serial.begin( 115200 );

      while ( !Serial )
      {
         ; // wait for Serial port to connect. Needed for native USB port.
      }
      Serial.flush();    // Wait for all the rubbish to finish displaying.
      while ( Serial.available() > 0 )
      {
         Serial.read(); // Clear the input buffer to get 'real' inputdata.
      }

   #endif

   #if defined( SOUNDON )

      DFSerial1.begin( 9600 );
      Player1.begin( DFSerial1 );

   #endif

      //  Call the DCC 'pin' and 'init' functions to enable the DCC Receiver.
   Dcc.pin( digitalPinToInterrupt( FunctionPinDcc ), FunctionPinDcc, false );
   Dcc.init( MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, 0 );      delay( 1000 );

   #if defined( DECODER_LOADED )

      if ( Dcc.getCV( CV_DECODER_MASTER_RESET ) == CV_DECODER_MASTER_RESET ) 
      {
      #endif

         FactoryDefaultCVIndex =  sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

         for ( int i = 0; i < FactoryDefaultCVIndex; ++i )
         {
            Dcc.setCV( FactoryDefaultCVs[ i ].CV, FactoryDefaultCVs[ i ].Value );
         }

      #if defined(DECODER_LOADED)
      }
   #endif

   Dcc.setCV( CV_DECODER_JUST_A_RESET, 0 );  //  Reset the just_a_reset CV

   for ( int i = 0; i < num_active_functions; ++i )
   {
      uint8_t  cv_value = Dcc.getCV( 30 + i );
      switch ( cv_value )
      {
         case 0:   // Master Decoder Disable
         {
            MasterDecoderDisable = 0;
            if ( digitalRead( MasterDecoderDisablePin ) == LOW )
            {
               MasterDecoderDisable = 1;
            }
            break;
         }

         case 1:   // LED On/Off
         {
            ftn_queue[ i ].inuse = 0;
            break;       
         }

         case 2:   // Motor2 Control
         {
            if ( Dcc.getCV( 80 ) != 0 )
            {
               Motor2ON = 1;
               Motor2Speed = ( Dcc.getCV( 80 ) ) & 0x7F ;
               Motor2ForwardDir = ( byte )( ( Dcc.getCV( 80 ) ) & 0x80 ) >> 7 ;
            }
            else
            {
               Motor2ON = 0;
            }
            break;
         }

         default:
         {
            break;  
         }
      }
   }

   Motor1ForwardDir = 1;     // Default start value for direction if throttle controlled
   if ( Dcc.getCV( 50 ) != 0 )
   {
      Motor1Speed = ( Dcc.getCV( 50 ) ) & 0x7F ;
      Motor1ForwardDir = ( byte )( ( Dcc.getCV( 50 ) ) & 0x80 ) >> 7 ;
   }

   #if defined( DEBUGID ) || defined( DEBUGCV ) || defined( MONITOR )

      _PL( "\n" "CV Dump:" );
      for ( int i = 30; i < 51; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Throttle Pause 1" );
      for ( int i = 51; i < 56; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Throttle Pause 2" );
      for ( int i = 56; i < 61; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Throttle Reverse 1" );
      for ( int i = 61; i < 66; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Throttle Reverse 2" );
      for ( int i = 66; i < 71; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Immediate Stop" );
      for ( int i = 71; i < 73; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Immediate Start" );
      for ( int i = 73; i < 75; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Motor2 Speed" );
      for ( int i = 80; i < 81; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "My Address" );
      _2P( Dcc.getCV( CV_MULTIFUNCTION_PRIMARY_ADDRESS ), DEC ); _PP( "\t \n" );
      _PL( "" );

   #endif

   interrupts();  // Ready to rumble....
}


/* ******************************************************************************** */


void loop()
{
   //  MUST call the NmraDcc.process() method frequently for correct library operation
   idle();

   //  Check Master Input Over ride
   MasterDecoderDisable = 0 ;
   if ( digitalRead( MasterDecoderDisablePin ) == LOW )
   {
      MasterDecoderDisable = 1 ;
   }
   else
   {
      MasterDecoderDisable = Function0_value & 1 ;
   }

   if ( MasterDecoderDisable == 1 )
   {
      Motor1Speed = 0 ;
      Motor2Speed = 0 ;
   }

   // ========   Throttle Pause 1  ========================
   if ( digitalRead( ThrottlePause1Pin ) == LOW )              //  Throttle Speed Pause1 Input Pin
   {
      Use_DCC_speed = false;                                   //  Do not update speed via DCC
      Starting_Motor1Speed = Motor1Speed ;

      while ( Motor1Speed > 0 )
      {
         --Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 52 ) );                             //  Throttle Ramp DOWN Delay 0-255
      }
      Motor1Speed = 0;

      playTheclip( 54, 55 );                                   //  Play clip

      delay( int( Dcc.getCV( 51 ) * MasterTimeConstant ) );    //  Pause Time 0 - 255 (0.1 secs)

      while ( Motor1Speed <= Starting_Motor1Speed )
      {
         ++Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 53 ) );                             //  Throttle Ramp UP Delay 0-255
      }
      Motor1Speed = Starting_Motor1Speed ;

      run_at_speed();

      while ( digitalRead( ThrottlePause1Pin ) == LOW )
      {
         idle();                                               //  Wait for Sensor
      }
      Use_DCC_speed =  true ;                                  //  Do not update speed via DCC
   }

   // ========   Throttle Pause 2  ========================
   if ( digitalRead( ThrottlePause2Pin ) == LOW )              //  Throttle Speed Pause2 Input Pin
   {
      Use_DCC_speed = false;                                   //  Do not update speed via DCC
      Starting_Motor1Speed = Motor1Speed ;

      while ( Motor1Speed > 0 )
      {
         --Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 57 ) );                             //  Throttle Ramp DOWN Delay 0-255
      }
      Motor1Speed = 0 ;

      playTheclip( 59, 60 );                                   //  Play clip

      delay( int( Dcc.getCV( 56 ) * MasterTimeConstant ) );    //  Pause Time 0-255 (0.1 secs)

      while ( Motor1Speed <= Starting_Motor1Speed)
      {
         ++Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 58 ) );                             //  Throttle Ramp UP Delay 0-255
      }
      Motor1Speed = Starting_Motor1Speed;

      run_at_speed();

      while ( digitalRead( ThrottlePause2Pin ) == LOW )
      {
         idle();                                               //  Wait for Sensor
      }
      Use_DCC_speed = true;                                    //  Do not update speed via DCC
   }

   // ========   Throttle Reverse 1  ========================
   if ( digitalRead( ThrottleInputReversePin1 ) == LOW )       //  Throttle Speed Reverse1 Input Pin
   {
      Use_DCC_speed = false;                                   //  Do not update speed via DCC
      Starting_Motor1Speed = Motor1Speed;
      --Motor1Speed ;

      while ( Motor1Speed > 1 )
      {
         run_at_speed();
         --Motor1Speed ;
         delay( Dcc.getCV( 62 ) );                             //  Throttle Ramp DOWN Delay 0-255
      }
      Motor1Speed = 0;

      playTheclip( 64, 65 );                                   //  Play clip

      Motor1ForwardDir = ( Motor1ForwardDir ^ 0x01 ) & 0x01 ;
      delay( Dcc.getCV( 61 ) * MasterTimeConstant );           //  Pause Time 0-255 (0.1 secs)

      while ( Motor1Speed < Starting_Motor1Speed )
      {
         ++Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 63 ) );                             //  Throttle Ramp UP Delay 0-255  
      }
      Motor1Speed = Starting_Motor1Speed;

      run_at_speed();

      while ( digitalRead( ThrottleInputReversePin1 ) == LOW )
      {
         idle(); //  Wait for Sensor
      }
      Use_DCC_speed =  true ;                                  //  Do not update speed via DCC
   }

   // ========   Throttle Reverse 2  ========================
   if ( digitalRead( ThrottleInputReversePin2 ) == LOW )       //  Throttle Speed Reverse Input Pin
   {
      Use_DCC_speed = false;                                   //  Do not update speed via DCC
      Starting_Motor1Speed = Motor1Speed ;

      while ( Motor1Speed > 0 )
      {
         --Motor1Speed;
         run_at_speed();
         delay( Dcc.getCV( 67 ) );                             //  Throttle Ramp DOWN Delay 0-255
      }
      Motor1Speed = 0 ;

      playTheclip( 69, 70 );                                   //  Play clip

      Motor1ForwardDir = ( Motor1ForwardDir ^ 0x01 ) & 0x01 ;
      delay( int( Dcc.getCV( 66 ) * MasterTimeConstant) );     //  Pause Time 0-255 (0.1 secs)

      while ( Motor1Speed <= Starting_Motor1Speed )
      {
         ++Motor1Speed ;
         run_at_speed();
         delay( Dcc.getCV( 68 ) );                             //  Throttle Ramp UP Delay 0-255  
      }
      Motor1Speed = Starting_Motor1Speed ;

      run_at_speed();

      while ( digitalRead( ThrottleInputReversePin2 ) == LOW )
      {
         idle(); //Wait for Sensor
      }
      Use_DCC_speed = true;                                    //  Do not update speed via DCC
   }

   // ========   Throttle Immediate Stop  ========================
   if ( digitalRead( ImmediateStopPin ) == LOW )               //  Throttle Immediate Stop Input Pin
   {
      ForcedStopSpeedMotor1 = Motor1Speed ;
      ForcedStopDirMotor1 = Motor1ForwardDir ;
      Motor1Speed = 0 ;

      playTheclip( 71, 72 );                                   //  Play clip
   }

   // ========   Throttle Immediate Start  ========================
   if ( digitalRead( ImmediateStartPin ) == LOW )              //  Throttle Immediate Start Input Pin
   {
      playTheclip( 73, 74 );                                   //  Play clip

      if ( ForcedStopSpeedMotor1 != 0 )
      {
         Motor1Speed = ForcedStopSpeedMotor1 ;
         Motor1ForwardDir = ForcedStopDirMotor1 ;
      } 
      else
      {
         if ( Dcc.getCV( 50 ) != 0 )
         {
            Motor1Speed = ( Dcc.getCV( 50 ) ) & 0x7F ;
            Motor1ForwardDir = ( byte )( ( Dcc.getCV( 50 ) ) & 0x80 ) >> 7 ;
         }

         ForcedStopSpeedMotor1 = 0;                            //  Take us out of forced stop mode
         run_at_speed();

         while ( digitalRead( ImmediateStartPin ) == LOW )
         {
            idle();                                            //  Wait for Sensor
         }
      }
   }

   //  ********************************************************************************

   for ( int i = 0; i < num_active_functions; ++i )
   {
      switch ( Dcc.getCV( 30 + i ) )
      {
         case 0:                                               //   Master Decoder Disable Ops
         {
            break;
         }

         case 1:                                               //  LED On/Off
         {
            if ( MasterDecoderDisable == 1 )
            {
               digitalWrite( fpins[ i ], 0 );                  //  Decoder disabled so LEDs off
            }
            break;
         }

         #if defined ( MOTORON )

            case 2:                                               //  Motor2 Control
            {
               Motor2Speed = ( Dcc.getCV( 80 ) ) & 0x7F ;         // Re-read Motor2Speed and Motor2ForwardDir if CV was updated
               Motor2ForwardDir = ( byte )( ( Dcc.getCV( 80 ) ) & 0x80 ) >> 7 ;

               if ( ( MasterDecoderDisable == 0 ) && ( Motor2ON == 1 ) )
               {
                  if ( Motor2ForwardDir == 0 )
                  {
                     gofwd2 ( fwdtime, Motor2Speed << 4 );
                  }
                  else
                  {
                     gobwd2 ( bwdtime, Motor2Speed << 4 );
                  }
               }

               if ( MasterDecoderDisable == 1 )
               {
                  digitalWrite( m0h, LOW);                        //  Motor2 OFF
                  digitalWrite( m0l, LOW);                        //  Motor2 OFF
               }
               break;
            }

         #endif

         default:
         {
            break;
         }
      }
   }

   #if defined( MONITOR )

      _PP( "loop4 Motor1Speed = " );
      _2P( Motor1Speed,       DEC );
      _PP( "\t" " Motor2Speed = " );
      _2L( Motor2Speed,       DEC );

   #endif

}         //  end loop()


/* ******************************************************************************* */


void idle()
{
   Dcc.process( );
   run_at_speed();
}     //   end  idle()


/* ******************************************************************************* */


void run_at_speed()
{
   #if defined( MONITOR )

      _PP( "run Motor1Speed = "       );
      _2P( Motor1Speed,           DEC );
      _PP( "\t" "Motor1ForwardDir = " );
      _2L( Motor1ForwardDir,      DEC );

   #endif

   #if defined ( MOTORON )

      if ( MasterDecoderDisable == 0 )
      {
         if ( Motor1Speed != 0 )
         {
            if ( Motor1ForwardDir == 0 )
            {
               gofwd1( fwdtime, Motor1Speed << 4 );
            }
            else
            {
               gobwd1( bwdtime, Motor1Speed << 4 );
            }
         }
      }

      if ( MasterDecoderDisable == 1 )
      {
         digitalWrite( m2h, LOW );     //Motor1 OFF
         digitalWrite( m2l, LOW );     //Motor1 OFF
         digitalWrite( m0h, LOW );     //Motor2 OFF
         digitalWrite( m0l, LOW );     //Motor2 OFF
      }

      if ( ( MasterDecoderDisable == 0 ) && ( Motor2ON == 1 ) )
      {
         if ( Motor2ForwardDir == 0 )
         {
            gofwd2( fwdtime, Motor2Speed << 4 );
         }
         else
         {
            gobwd2( bwdtime, Motor2Speed << 4 );
         }
      }

   #endif

}     //    end  run_at_speed()


/* ******************************************************************************* */

#if defined( MOTORON )

   void gofwd1( int fcnt, int fcycle )
   {
      int delta_tp = ( fcycle + loopdelay ) << 2;
      int delta_tm = cyclewidth - fcycle - loopdelay;
      int icnt = 0;

      while ( icnt < fcnt )
      {
         digitalWrite( m2h, HIGH );     // Motor1
         delayMicroseconds( delta_tp );
         digitalWrite( m2h,  LOW );     // Motor1
         delayMicroseconds( delta_tm );
         icnt++;
      }
   }     //  end   gofwd1()


/* ******************************************************************************* */


   void gobwd1( int bcnt, int bcycle )
   {
      int delta_tp = ( bcycle + loopdelay ) << 2;
      int delta_tm = cyclewidth - bcycle - loopdelay;
      int icnt = 0;

      while ( icnt < bcnt )
      {
         digitalWrite( m2l, HIGH );    // Motor1
         delayMicroseconds( delta_tp );
         digitalWrite( m2l,  LOW );    // Motor1
         delayMicroseconds( delta_tm );
         icnt++;
      }
   }      //  end   gobwd1()


/* ******************************************************************************* */


   void gofwd2( int fcnt, int fcycle )
   {
      int delta_tp = ( fcycle + loopdelay ) << 2;
      int delta_tm = cyclewidth - fcycle - loopdelay;
      int icnt = 0;

      while ( icnt < fcnt )
      {
         digitalWrite( m0h, HIGH );    // Motor2
         delayMicroseconds( delta_tp );
         digitalWrite( m0h,  LOW );    // Motor2
         delayMicroseconds( delta_tm );
         icnt++;
      }
   }      //    end   gofwd2()


/* ******************************************************************************* */


   void gobwd2( int bcnt, int bcycle )
   {
      int delta_tp = ( bcycle + loopdelay ) << 2;
      int delta_tm = cyclewidth - bcycle - loopdelay;
      int icnt = 0;

      while ( icnt < bcnt )
      {
         digitalWrite( m0l, HIGH );    // Motor2
         delayMicroseconds( delta_tp );
         digitalWrite( m0l,  LOW );    // Motor2
         delayMicroseconds( delta_tm );
         icnt++;
      }
   }     //    end gobwd2()

#endif

/* ******************************************************************************* */


#if defined( SOUNDON )

   void playTrackOnChannel( byte dtrack )
   {
      if ( dtrack != 255 )
      {
         Player1.play( dtrack );
         delay( audiocmddelay );
      }
      else
      {
         Player1.play( random( First_Track, Last_Track + 1 ) );
         delay( audiocmddelay );
      }
   }     //    end   playTrackOnChannel()


   void setVolumeOnChannel( byte dvolume )
   {
      if ( dvolume > 30 )
      {
         return;    // Don't change the volume if out of range
      }
      else
      {
         Player1.volume( dvolume );
         delay( audiocmddelay );
      }
   }     //    end   setVolumeOnChannel()

#endif


void playTheclip( uint16_t clipCV, uint16_t volumeCV )
{
   #if defined( SOUNDON )

      int ttemp = ( Dcc.getCV( clipCV ) );
      setVolumeOnChannel( Dcc.getCV( volumeCV ) );

      if ( ttemp != 0 )
      {
         playTrackOnChannel( ttemp );                       //  Play clip
      }

   #endif
}


/* **********************************************************************************
 *  notifyCVChange()  Called when a CV value is changed.
 *                    This is called whenever a CV's value is changed.
 *  notifyDccCVChange()  Called only when a CV value is changed by a Dcc packet or a lib function.
 *                    it is NOT called if the CV is changed by means of the setCV() method.
 *                    Note: It is not called if notifyCVWrite() is defined
 *                    or if the value in the EEPROM is the same as the value
 *                    in the write command. 
 *
 *  Inputs:
 *    CV        - CV number.
 *    Value     - Value of the CV.
 *
 *  Returns:
 *    None
 */
void    notifyCVChange( uint16_t CV, uint8_t Value )
{
   #if defined( DEBUGCV )

      _PP( "notifyCVChange: CV: " );
      _2P( CV,                DEC );
      _PP( "\t" " Value: "        );
      _2L( Value,             DEC );

   #endif

   if ( CV ==  50 )
   {
      Motor1Speed = ( Dcc.getCV( 50 ) ) & 0x7F ;
      Motor1ForwardDir = ( byte )( ( Dcc.getCV( 50 ) ) & 0x80 ) >> 7 ;
   }

   if ( ( ( CV == 251 ) && ( Value == 251 ) ) || ( ( CV == 252 ) && ( Value == 252 ) ) )
   {
      wdt_enable( WDTO_15MS );  //  Resets after 15 milliSecs

      while( 1 ) {}  // Wait for the prescaler time to expire
   }
}       //   end notifyCVChange()


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
void notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION ForwardDir, DCC_SPEED_STEPS SpeedSteps )
{
   if ( !Use_DCC_speed )
   {
      return ;
   }

   #if defined( DEBUGCV )

      _PP( "Address = "    );
      _2L( Addr,        DEC);
      _PP( "AddrType = "   );
      _2L( AddrType,    DEC);
      _PP( "Speed = "      );
      _2L( Speed,       DEC);
      _PP( "ForwardDir = " );
      _2L( ForwardDir,  DEC);
      _PP( "SpeedSteps = " );
      _2L( SpeedSteps, DEC );

   #endif

   if ( Dcc.getCV( 50 ) == 0 )
   {
      Motor1Speed = ( Speed & 0x7F );
   }

   if ( Motor1Speed == 1 )
   {
     Motor1Speed = 0 ;
   }
}        //   end   notifyDccSpeed()


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
 *                                  You must & FuncState with the appropriate
 *                                  FN_BIT_nn value to isolate a given bit.
 *
 *  Returns:
 *    None
 */
void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState )
{
   #if defined( DEBUGCV )

      _PP( "Address = "   );
      _2L( Addr,       DEC);
      _PP( "AddrType = "  );
      _2L( AddrType,   DEC);
      _PP( "FuncGrp = "   );
      _2L( FuncGrp,    DEC);
      _PP( "FuncState = " );
      _2L( FuncState, DEC );

   #endif

   switch( FuncGrp )
   {
      case FN_0_4:    //Function Group 1 F0 F4 F3 F2 F1
      {
         exec_function( 0, FunctionPin0, ( FuncState & FN_BIT_00 ) >> 4 );
         exec_function( 1, FunctionPin1, ( FuncState & FN_BIT_01 ) >> 0 );
         exec_function( 2, FunctionPin2, ( FuncState & FN_BIT_02 ) >> 1 );
         exec_function( 3, FunctionPin3, ( FuncState & FN_BIT_03 ) >> 2 );
         exec_function( 4, FunctionPin4, ( FuncState & FN_BIT_04 ) >> 3 );
         break ;
      }

      case FN_5_8:    //Function Group 1    F8 F7 F6 F5
      {
         exec_function( 5, FunctionPin5, ( FuncState & FN_BIT_05 ) >> 0 );
         exec_function( 6, FunctionPin6, ( FuncState & FN_BIT_06 ) >> 1 );
         break ;
      }

      case FN_9_12:   //Function Group 1 F12 F11 F10 F9
      {
         break ;
      }

      case FN_13_20:  //Function Group 2  ==  F20 - F13
      {
         break ;
      }

      case FN_21_28:  //Function Group 2  ==  F28 - F21
      {
         break ;
      }
   }
}       //   end notifyDccFunc()


/* ******************************************************************************* */


void exec_function ( int function, int pin, int FuncState )
{
   #if defined( DEBUGCV )

      _PP( "exec_function = " );
      _2L( function,      DEC );
      _PP( "pin = "           );
      _2L( pin,           DEC );
      _PP( "FuncState = "     );
      _2L( FuncState,     DEC );

   #endif

   switch ( Dcc.getCV( 30 + function ) )
   {
      case 0:    // Master Disable Function0 Value will transfer to MasterDecoderDisable in loop()
      {
         Function0_value = byte( FuncState );
         break ;
      }

      case 1:    // On - Off LED
      {
         if ( MasterDecoderDisable == 0 )
         {
            digitalWrite ( pin, FuncState );
         }
         break ;
      }

      case 2:    // Motor2 Control
      {
         if ( MasterDecoderDisable == 0 )
         {
            Motor2ON = FuncState ;
         }
         break ;
      }

      default:
      {
         ftn_queue[ function ].inuse = 0 ;
         break ;
      }
   }
}    //   end  exec_function()


/* ******************************************************************************* */


//
