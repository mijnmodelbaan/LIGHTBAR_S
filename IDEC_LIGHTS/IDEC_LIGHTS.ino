
/* 
 * 
 * Interactive Decoder Random Switches   IDEC2_4_FunctionSetsDev.ino
 * Version 1.08 Geoff Bunza 2020
 * Works with both short and long DCC Addesses
 * This decoder will control Switch Sequences, servos, sounds and LEDs
 * 
 * F0 = Master Function | OFF = Function | ON DISABLES the decoder
 * F0 is configured as Master Decoder Disable Override | ON == Disable Decoder
 * Input Pin for Decoder Disable | Pin 14/A0 Active LOW == Disable Decoder
 * 
 * F1-F8 Eight Switch Sets 1-8 controlled by Input pins 3, 4, 5, 6, 7, 8, 9, 10 respectively
 * F1-F10  also runs switch sets (1-8)   All Switch Sets are defined by groups of 16 CVs
 * - Either a DCC Function 1-10 on OR an Input Pin (3,4,5,6,7,8,9,10) Switched Low enables a decoder function (ON)
 * - BOTH the respective DCC Decoder Function 1-8 must be Off AND its respective Input Pin  (3,4,5,6,7,8,9,10) 
 *   MUST be High for a decoder function to be considered disabled
 * - A decoder function LEFT ENABLED will repreat the respecpective action as long as it is enabled
 * 
 * Switch Set CV's are 5 groups of 3 CVs each:  
 *    CV1 - A delay (0 - 255) which will be multiplied by the MasterTimeConstant
 *   	      setting time increments from milliseconds to minutes | 0 = No Delay
 *    CV2 - A Mode or Command byte Describing what will be executed in this Switch Step, including:
 *          0 = No Operation / Null / Skip
 *          1 = Simple pin switch on / off
 *          2 = Random pin switch on / off
 *          3 = Weighted Random pin switch on / off | default is 60% ON time but can be set to anything 1 - 99%
 *          4 = Play sound track using fpin value for the track 1-126, 0 = Skip Play, 127 = Select Random Track
 *              from First_Track to Last_Track inclusive; MSB=0->No Volume Change MSB=1 -> Set Volume to default
 *          5 = Position Servo to 0 - 180 full speed of travel
 *          6 = Dual pin on / off used for alternate blink fpin and fpin + 1 (MSB set value for fpin state)
 *          7 = Start another Switching set based on the fpin argument (Used to chain Switch Sets)
 *          8 = Start another Switching set based on the fpin argument ONLY if NOT already started
 *    CV3 - An argument representing the Pin number affected in the lower 7 bits | High bit (0x80 or 128) a value
 *          or a general parameter like a servo position, a Sound track, or a sound set number to jump to
 * 
 * Switch sets start with CVs:  50, 66, 82, 98, 114, 130, 146, 162, 178, 194
 * MAX one of 11 Configurations per pin function: 0 = DISABLE On / Off, 1 - 10 = Switch Control 1 - 8
 * 
 * PRO MINI PIN ASSIGNMENT:
 *   2 D2 - DCC Input Pin
 *   3 D3 - Input Pin Switch   1
 *   4 D4 - Input Pin Switch   2
 *   5 D5 - Input Pin Switch   3
 *   6 D6 - Input Pin Switch   4
 *   7 D7 - Input Pin Switch   5
 *   8 B0 - Input Pin Switch   6
 *   9 B1 - Input Pin Switch   7
 *  10 B2 - Input Pin Switch   8
 *  11 B3 - Output Pin 1
 *  12 B4 - Output Pin 2
 *  13 B5 - Output Pin 3
 *  14 A0 - Input  Pin 9  for MasterDecoderDisable | Active  LOW
 *  15 A1 - Output Pin 4  or  default sound player pin (TX) connected to DFPlayer1 (RX) via 1K Ohm 1/4W Resistor
 *  16 A2 - Output Pin 5  or  default_servo_pin 
 *  17 A3 - Output Pin 6
 *  18 A4 - Output Pin 7
 *  19 A5 - Output Pin 8
 * 
 * Decoder has been changed to act as a function decoder for 10 dimmable lights.
 * Only the FL/F0 is the master switch | Off is disabled - On is Enabled.
 * Master Switch FL/F0 is over ruled by CV# 250 | !0 is always On | =0 is FL/F0.
 * 
 */


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
// ******** REMOVE THE "//" IN THE NEXT LINE UNLESS YOU WANT ALL CV'S RESET ON EVERY POWER-UP
#define DECODER_LOADED

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO RUN A TEST DURING SETUP SEQUENCES
// #define TESTRUN

// ******** REMOVE THE "//" IN THE NEXT LINE TO SEE DETAILED INFO ABOUT INCOMING C.V MESSAGES
// #define DEBUGCV

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL OUTPUT
// #define DEBUGID

#if defined( DEBUGID ) || defined( DEBUGCV )
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
#define  MasterTimeConstant  1L        //   1 milliseconds Timing
// fine  MasterTimeConstant  10L       //  10 milliseconds Timing
// fine  MasterTimeConstant  100L      // 100 milliseconds Timing
// fine  MasterTimeConstant  1000L     //   1 Seconds Timing
// fine  MasterTimeConstant  10000L    //  10 Seconds Timing
// fine  MasterTimeConstant  60000L    //   1 Minutes Timing
// fine  MasterTimeConstant  3600000L  //   1 Hours   Timing


/* Array of bytes, used by the Switch Sets. The bytes keep track of the Actions we need
 * to take for the respective Set. Setting the first byte to Non-Zero starts the action
 * right after Setup has finished. Setting the last CV ( 65, 81, 97, 113, 129, 145, 161
 * 177, 193, 209) of a Switch Set to Non-Zero, keeps the Set running untill F0 Disables
 * the decoder or forever. Setting the CV to 0 again stops the Set when it runs out. */
byte  ss1[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss1delay   =  0;  //  Switch Set  1
byte  ss2[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss2delay   =  0;  //  Switch Set  2
byte  ss3[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss3delay   =  0;  //  Switch Set  3
byte  ss4[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss4delay   =  0;  //  Switch Set  4
byte  ss5[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss5delay   =  0;  //  Switch Set  5
byte  ss6[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss6delay   =  0;  //  Switch Set  6
byte  ss7[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss7delay   =  0;  //  Switch Set  7
byte  ss8[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss8delay   =  0;  //  Switch Set  8
byte  ss9[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss9delay   =  0;  //  Switch Set  9
byte ss10[ ] = { 0, 0, 0, 0, 0, 0};   unsigned long ss10delay  =  0;  //  Switch Set 10


const int num_active_functions     =   11;  //  Number of Functions starting with F0
const int FunctionPin0             =   22;  //  Dummy variable used in some function
const int FunctionPinDcc           =    2;  //  Inputpin DCCsignal


uint8_t ttemp             =  0;  //  Temporary variable used in the loop functions
int MasterDecoderDisable  =  0;  //  Standard value decoder is Disabled by default
byte MasterDecoderSwitch  =  0;  //  Detection single switch Off  Switch Sets 1-10

int Function0_value       =  0;
byte function_value[ ]    =  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

bool run_switch_set[ ]    =  { false, false, false, false, false, false, false, false, false, false, false };


#if defined( ARDUINO_AVR_UNO )

   const int numINpins    =  1;                                     //  Number of INput pins to initialize
   byte inputpins[ ]      =  {  2 };                                //  These are all the used  Input Pins

   const int numfpins     =  10;                                    // Number of Output pins to initialize
   byte fpins[ ]          =  { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 16, 17, 18, 19, 13 };  //  Output Pins

#endif

#if defined( ARDUINO_attinyxy6 )

   const int numINpins    =  1;                                     //  Number of INput pins to initialize
   byte inputpins[ ]      =  {  8 };                                //  These are all the used  Input Pins

   const int numfpins     =  11;                                    // Number of Output pins to initialize
   byte fpins[ ]          =  { 14, 13, 10,  9, 20,  1,  2,  5,  6,  7, 18 };  //  All the used Output Pins

#endif

#if defined( ARDUINO_AVR_ATmega8 )

   const int numINpins    =  1;                                     //  Number of INput pins to initialize
   byte inputpins[ ]      =  {  2 };                                //  These are all the used  Input Pins

   const int numfpins     =  11;                                    // Number of Output pins to initialize
   byte fpins[ ]          =  {  7, 15,  5,  3,  6, 14,  4, 17, 18, 19, 13 };  //  All the used Output Pins

#endif


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
// {CV_29_CONFIG, CV29_F0_LOCATION},                        // Short Address 28/128 Speed Steps
   {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},  // Long  Address 28/128 Speed Steps

   {CV_DECODER_MASTER_RESET,     0},

   { 30,   0},  //  F0  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 31,   1},  //  F1  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 32,   2},  //  F2  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 33,   3},  //  F3  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 34,   4},  //  F4  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 35,   5},  //  F5  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 36,   6},  //  F6  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 37,   7},  //  F7  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 38,   8},  //  F8  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 39,   9},  //  F9  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 40,  10},  // F10  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 41,  11},  // F11  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 42,  11},  // F12  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 43,  11},  // F13  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 44,  11},  // F14  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 45, 255},  // F15  not used

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 50,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  1
   { 51,   1},  //
   { 52, fpins[ 0] | 0x80},  //  Switch Pin  On
   { 53,   1},  //
   { 54,   1},  //
   { 55, fpins[ 0] & 0x7F},  //  Switch Pin Off
   { 56,   0},  //
   { 57,   0},  //
   { 58,   0},  //
   { 59,   0},  //
   { 60,   0},  //
   { 61,   0},  //
   { 62,   0},  //
   { 63,   0},  //
   { 64,   0},  //
   { 65,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 66,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  2
   { 67,   1},  //
   { 68, fpins[ 1] | 0x80},  //  Switch Pin  On
   { 69,   1},  //
   { 70,   1},  //
   { 71, fpins[ 1] & 0x7F},  //  Switch Pin Off
   { 72,   0},  //
   { 73,   0},  //
   { 74,   0},  //
   { 75,   0},  //
   { 76,   0},  //
   { 77,   0},  //
   { 78,   0},  //
   { 79,   0},  //
   { 80,   0},  //
   { 81,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 82,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  3
   { 83,   1},  //
   { 84, fpins[ 2] | 0x80},  //  Switch Pin  On
   { 85,   1},  //
   { 86,   1},  //
   { 87, fpins[ 2] & 0x7F},  //  Switch Pin Off
   { 88,   0},  //
   { 89,   0},  //
   { 90,   0},  //
   { 91,   0},  //
   { 92,   0},  //
   { 93,   0},  //
   { 94,   0},  //
   { 95,   0},  //
   { 96,   0},  //
   { 97,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 98,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  4
   { 99,   1},  //
   {100, fpins[ 3] | 0x80},  //  Switch Pin  On
   {101,   1},  //
   {102,   1},  //
   {103, fpins[ 3] & 0x7F},  //  Switch Pin Off
   {104,   0},  //
   {105,   0},  //
   {106,   0},  //
   {107,   0},  //
   {108,   0},  //
   {109,   0},  //
   {110,   0},  //
   {111,   0},  //
   {112,   0},  //
   {113,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {114,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  5
   {115,   1},  //
   {116, fpins[ 4] | 0x80},  //  Switch Pin  On
   {117,   1},  //
   {118,   1},  //
   {119, fpins[ 4] & 0x7F},  //  Switch Pin Off
   {120,   0},  //
   {121,   0},  //
   {122,   0},  //
   {123,   0},  //
   {124,   0},  //
   {125,   0},  //
   {126,   0},  //
   {127,   0},  //
   {128,   0},  //
   {129,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {130,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  6
   {131,   1},  //
   {132, fpins[ 5] | 0x80},  //  Switch Pin  On
   {133,   1},  //
   {134,   1},  //
   {135, fpins[ 5] & 0x7F},  //  Switch Pin Off
   {136,   0},  //
   {137,   0},  //
   {138,   0},  //
   {139,   0},  //
   {140,   0},  //
   {141,   0},  //
   {142,   0},  //
   {143,   0},  //
   {144,   0},  //
   {145,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {146,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  7
   {147,   1},  //
   {148, fpins[ 6] | 0x80},  //  Switch Pin  On
   {149,   1},  //
   {150,   1},  //
   {151, fpins[ 6] & 0x7F},  //  Switch Pin Off
   {152,   0},  //
   {153,   0},  //
   {154,   0},  //
   {155,   0},  //
   {156,   0},  //
   {157,   0},  //
   {158,   0},  //
   {159,   0},  //
   {160,   0},  //
   {161,   1},  //  Not 0 if the set has to be kept running after starting it

#if defined( LIGHTS6 )

      //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
      {162,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  8
      {163,   1},  //
      {164, fpins[ 7] | 0x80},  //  Switch Pin  On
      {165,   1},  //
      {166,   1},  //
      {167, fpins[ 7] & 0x7F},  //  Switch Pin Off
      {168,   0},  //
      {169,   0},  //
      {170,   0},  //
      {171,   0},  //
      {172,   0},  //
      {173,   0},  //
      {174,   0},  //
      {175,   0},  //
      {176,   0},  //
      {177,   1},  //  Not 0 if the set has to be kept running after starting it

      //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
      {178,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET  9
      {179,   1},  //
      {180, fpins[ 8] | 0x80},  //  Switch Pin  On
      {181,   1},  //
      {182,   1},  //
      {183, fpins[ 8] & 0x7F},  //  Switch Pin Off
      {184,   0},  //
      {185,   0},  //
      {186,   0},  //
      {187,   0},  //
      {188,   0},  //
      {189,   0},  //
      {190,   0},  //
      {191,   0},  //
      {192,   0},  //
      {193,   1},  //  Not 0 if the set has to be kept running after starting it

      //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
      {194,  15},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 10
      {195,   1},  //
      {196, fpins[ 9] | 0x80},  //  Switch Pin  On
      {197,   1},  //
      {198,   1},  //
      {199, fpins[ 9] & 0x7F},  //  Switch Pin Off
      {200,   0},  //
      {201,   0},  //
      {202,   0},  //
      {203,   0},  //
      {204,   0},  //
      {205,   0},  //
      {206,   0},  //
      {207,   0},  //
      {208,   0},  //
      {209,   1},  //  Not 0 if the set has to be kept running after starting it

#endif // LIGHTS6

   {250,   0},  //  MASTER SWITCH OVER-RULE */
   {251,   0},  //  CV_DECODER_JUST_A_RESET */
   {252,   0},  //  CV_DECODER_MASTER_RESET */
   {253,   0},  //  Extra
};


/* ******************************************************************************* */


uint8_t FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
void notifyCVResetFactoryDefault()
{
   // Make FactoryDefaultCVIndex non-zero and equal to # CV's to be reset.
   FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
};


/* ******************************************************************************* */


void LargeSwitchSet01( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet02( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet03( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet04( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet05( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet06( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet07( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet08( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet09( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );
void LargeSwitchSet10( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false );


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

     // initialize the digital pins as  inputs
	for ( int i = 0; i < numINpins; ++i )
   {
      digitalWrite( inputpins[ i ], 1 );        // Switch the pin  on first.
      pinMode( inputpins[ i ], INPUT_PULLUP );  // Then set as input_pullup.
	}

   #if defined( DEBUGID ) || defined( DEBUGCV )

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

      //  Call the DCC 'pin' and 'init' functions to enable the DCC Receiver
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

   #if defined( TESTRUN )

      for ( int i = 0; i < numfpins; ++i )  //  Turn all LEDs  ON in sequence
      {
         digitalWrite( fpins[ i ], HIGH );
         delay( 60 );
      }

      delay( 400 );

      for ( int i = 0; i < numfpins; ++i )  //  Turn all LEDs OFF in sequence
      {
         digitalWrite( fpins[ i ],  LOW );
         delay( 60 );
      }

   #endif

   #if defined( DEBUGID ) || defined( DEBUGCV )

      _PL( "\n" "CV Dump:"  );
      for ( int i =  30; i <  46; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  30; i <  46; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  1:" );
      for ( int i =  50; i <  66; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  50; i <  66; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  2:" );
      for ( int i =  66; i <  82; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  66; i <  82; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  3:" );
      for ( int i =  82; i <  98; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  82; i <  98; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  4:" );
      for ( int i =  98; i < 114; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  98; i < 114; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  5:" );
      for ( int i = 114; i < 130; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 114; i < 130; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  6:" );
      for ( int i = 130; i < 146; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 130; i < 146; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  7:" );
      for ( int i = 146; i < 162; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 146; i < 162; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  8:" );
      for ( int i = 162; i < 178; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 162; i < 178; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set  9:" );
      for ( int i = 178; i < 194; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 178; i < 194; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Switch Set 10:" );
      for ( int i = 194; i < 210; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 194; i < 210; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

   #endif

   interrupts();  // Ready to rumble....
}


/* ******************************************************************************** */


void loop()
{
   //  MUST call the NmraDcc.process() method frequently for correct library operation
   Dcc.process();

   unsigned long _Mmillis = millis();  //  Holder of time.

   //  Check Master Input Over ride
   MasterDecoderDisable = ( Function0_value & 1 ) || ( Dcc.getCV( 250 ) != 0 );

   if ( MasterDecoderDisable != 0 )
   {
       //  Enable Swich Sets only once
      if ( MasterDecoderSwitch != 0 )
      {
         for ( int i = 1; i < num_active_functions; ++i )
         {
               // Check allowance before setting CV
            if ( function_value[ i ] != 0 )
            {
               Dcc.setCV( 65 + (i - 1) * 16, 1 );
            }
            MasterDecoderSwitch -= 1 ;  //  Run only once counter
         }
      }

      for ( int i = 0; i < num_active_functions; ++i )
      {
         uint8_t cv_value = Dcc.getCV( 30 + i );

         switch ( cv_value )
         {
            case  0:   // Master Decoder Disable
            {
               break;
            }

            case  1:   // 
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss1[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }

               #if defined( DEBUG01 )

                  _PP( "\n" " cv_value: "              );
                  _2L( cv_value,                   DEC );
                  _PP( " function_value[cv_value]: "   );
                  _2L( function_value[ cv_value ], DEC );
                  _PP( " ss1[0]: "                     );
                  _2L( ss1[ 0 ],                   DEC );
                  _PP( " run_switch_set[cv_value]: "   );
                  _2L( run_switch_set[ cv_value ], DEC );

               #endif

               break;
            }

            case  2:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss2[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  3:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss3[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  4:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss4[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  5:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss5[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  6:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss6[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  7:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss7[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  8:   //   
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss8[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case  9:   // 
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss9[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            case 10:   // 
            {
               if ( ( function_value[ cv_value ] == 1 ) && !run_switch_set[ cv_value ] )
               {
                  ss10[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }
               break;
            }

            default:
            {
               break;
            }
         }
      }
   }
   else
   {
       //  Disable Swich Sets only once
      if ( MasterDecoderSwitch == 0 )
      {
         for ( int i = 1; i < num_active_functions; ++i )
         {
            Dcc.setCV( 65 + (i - 1) * 16, 0 );
            MasterDecoderSwitch += 1 ;  //  Run once counter
         }
      }
   }


      //    ==========================   switch Set  1 Start Run LS1
   if ( ss1[ 0 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV(  50 ) * MasterTimeConstant ) ); //  Wait1
      ss1[ 0 ] = 0;
      ss1[ 1 ] = 1;
   }

   LargeSwitchSet01( _Mmillis,  1,  51, false );

   LargeSwitchSet01( _Mmillis,  2,  54, false );

   LargeSwitchSet01( _Mmillis,  3,  57, false );

   LargeSwitchSet01( _Mmillis,  4,  60, false );

   LargeSwitchSet01( _Mmillis,  5,  63,  true );


      //    ==========================   switch Set  2 Start Run LS2
   if ( ss2[ 0 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV(  66 ) * MasterTimeConstant ) ); //  Wait1
      ss2[ 0 ] = 0;
      ss2[ 1 ] = 1;
   }

   LargeSwitchSet02( _Mmillis,  1,  67, false );

   LargeSwitchSet02( _Mmillis,  2,  70, false );

   LargeSwitchSet02( _Mmillis,  3,  73, false );

   LargeSwitchSet02( _Mmillis,  4,  76, false );

   LargeSwitchSet02( _Mmillis,  5,  79,  true );


      //    ==========================   switch Set  3 Start Run LS3
   if ( ss3[ 0 ] == 1 )
   {
      ss3delay = _Mmillis + ( long( Dcc.getCV(  82 ) * MasterTimeConstant ) ); //  Wait1
      ss3[ 0 ] = 0;
      ss3[ 1 ] = 1;
   }

   LargeSwitchSet03( _Mmillis,  1,  83, false );

   LargeSwitchSet03( _Mmillis,  2,  86, false );

   LargeSwitchSet03( _Mmillis,  3,  89, false );

   LargeSwitchSet03( _Mmillis,  4,  92, false );

   LargeSwitchSet03( _Mmillis,  5,  95,  true );


      //    ==========================   switch Set  4 Start Run LS4
   if ( ss4[ 0 ] == 1 )
   {
      ss4delay = _Mmillis + ( long( Dcc.getCV(  98 ) * MasterTimeConstant ) ); //  Wait1
      ss4[ 0 ] = 0;
      ss4[ 1 ] = 1;
   }

   LargeSwitchSet04( _Mmillis,  1,  99, false );

   LargeSwitchSet04( _Mmillis,  2, 102, false );

   LargeSwitchSet04( _Mmillis,  3, 105, false );

   LargeSwitchSet04( _Mmillis,  4, 108, false );

   LargeSwitchSet04( _Mmillis,  5, 111,  true );


      //    ==========================   switch Set  5 Start Run LB1
   if ( ss5[ 0 ] == 1 )
   {
      ss5delay = _Mmillis + ( long( Dcc.getCV( 114 ) * MasterTimeConstant ) ); //  Wait1
      ss5[ 0 ] = 0;
      ss5[ 1 ] = 1;
   }

   LargeSwitchSet05( _Mmillis,  1, 115, false );

   LargeSwitchSet05( _Mmillis,  2, 118, false );

   LargeSwitchSet05( _Mmillis,  3, 121, false );

   LargeSwitchSet05( _Mmillis,  4, 124, false );

   LargeSwitchSet05( _Mmillis,  5, 127,  true );


      //    ==========================   switch Set  6 Start Run LB2
   if ( ss6[ 0 ] == 1 )
   {
      ss6delay = _Mmillis + ( long( Dcc.getCV( 130 ) * MasterTimeConstant ) ); //  Wait1
      ss6[ 0 ] = 0;
      ss6[ 1 ] = 1;
   }

   LargeSwitchSet06( _Mmillis,  1, 131, false );

   LargeSwitchSet06( _Mmillis,  2, 134, false );

   LargeSwitchSet06( _Mmillis,  3, 137, false );

   LargeSwitchSet06( _Mmillis,  4, 140, false );

   LargeSwitchSet06( _Mmillis,  5, 143,  true );


      //    ==========================   switch Set  7 Start Run LB3
   if ( ss7[ 0 ] == 1 )
   {
      ss7delay = _Mmillis + ( long( Dcc.getCV( 146 ) * MasterTimeConstant ) ); //  Wait1
      ss7[ 0 ] = 0;
      ss7[ 1 ] = 1;
   }

   LargeSwitchSet07( _Mmillis,  1, 147, false );

   LargeSwitchSet07( _Mmillis,  2, 150, false );

   LargeSwitchSet07( _Mmillis,  3, 153, false );

   LargeSwitchSet07( _Mmillis,  4, 156, false );

   LargeSwitchSet07( _Mmillis,  5, 159,  true );

   #if defined( LIGHTS6 )

         //    ==========================   switch Set  8 Start Run LB4
      if ( ss8[ 0 ] == 1 )
      {
         ss8delay = _Mmillis + ( long( Dcc.getCV( 162 ) * MasterTimeConstant ) ); //  Wait1
         ss8[ 0 ] = 0;
         ss8[ 1 ] = 1;
      }

      LargeSwitchSet08( _Mmillis,  1, 163, false );

      LargeSwitchSet08( _Mmillis,  2, 166, false );

      LargeSwitchSet08( _Mmillis,  3, 169, false );

      LargeSwitchSet08( _Mmillis,  4, 172, false );

      LargeSwitchSet08( _Mmillis,  5, 175,  true );


   //    ==========================   switch Set  9 Start Run LB5
      if ( ss9[ 0 ] == 1 )
      {
         ss9delay = _Mmillis + ( long( Dcc.getCV( 178 ) * MasterTimeConstant ) ); //  Wait1
         ss9[ 0 ] = 0;
         ss9[ 1 ] = 1;
      }

      LargeSwitchSet09( _Mmillis,  1, 179, false );

      LargeSwitchSet09( _Mmillis,  2, 182, false );

      LargeSwitchSet09( _Mmillis,  3, 185, false );

      LargeSwitchSet09( _Mmillis,  4, 188, false );

      LargeSwitchSet09( _Mmillis,  5, 191,  true );


   //    ==========================   switch Set 10 Start Run LB6
      if ( ss10[ 0 ] == 1 )
      {
         ss10delay = _Mmillis + ( long( Dcc.getCV( 194 ) * MasterTimeConstant ) ); //  Wait1
         ss10[ 0 ] = 0;
         ss10[ 1 ] = 1;
      }

      LargeSwitchSet10( _Mmillis,  1, 195, false );

      LargeSwitchSet10( _Mmillis,  2, 198, false );

      LargeSwitchSet10( _Mmillis,  3, 201, false );

      LargeSwitchSet10( _Mmillis,  4, 204, false );

      LargeSwitchSet10( _Mmillis,  5, 207,  true );

   #endif // LIGHTS6

}       // end  loop() 


/* ******************************************************************************* */


void LargeSwitchSet01( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss1[ idx ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss1[ idx + 0 ] = 0;

      if ( !test )
      {
         ss1delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss1[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss1[  0 ] = 1;
            run_switch_set[  1 ] =  true;
         }
         else
         {
            run_switch_set[  1 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet02( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss2[ idx ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss2[ idx + 0 ] = 0;

      if ( !test )
      {
         ss2delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss2[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss2[  0 ] = 1;
            run_switch_set[  2 ] =  true;
         }
         else
         {
            run_switch_set[  2 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet03( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss3[ idx ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss3[ idx + 0 ] = 0;

      if ( !test )
      {
         ss3delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss3[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss3[  0 ] = 1;
            run_switch_set[  3 ] =  true;
         }
         else
         {
            run_switch_set[  3 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet04( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss4[ idx ] == 1 ) && ( ss4delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss4[ idx + 0 ] = 0;

      if ( !test )
      {
         ss4delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss4[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss4[  0 ] = 1;
            run_switch_set[  4 ] =  true;
         }
         else
         {
            run_switch_set[  4 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet05( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss5[ idx ] == 1 ) && ( ss5delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss5[ idx + 0 ] = 0;

      if ( !test )
      {
         ss5delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss5[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss5[  0 ] = 1;
            run_switch_set[  5 ] =  true;
         }
         else
         {
            run_switch_set[  5 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet06( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss6[ idx ] == 1 ) && ( ss6delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss6[ idx + 0 ] = 0;

      if ( !test )
      {
         ss6delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss6[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss6[  0 ] = 1;
            run_switch_set[  6 ] =  true;
         }
         else
         {
            run_switch_set[  6 ] = false;
         }
      }
   }
}


/* ******************************************************************************* */


void LargeSwitchSet07( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
{
   if ( ( ss7[ idx ] == 1 ) && ( ss7delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( CV + 1 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
      }
      ss7[ idx + 0 ] = 0;

      if ( !test )
      {
         ss7delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
         ss7[ idx + 1 ] = 1;
      }
      else
      {
         if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
         {
            ss7[  0 ] = 1;
            run_switch_set[  7 ] =  true;
         }
         else
         {
            run_switch_set[  7 ] = false;
         }
      }
   }
}

#if defined ( LIGHTS6 )

   /* ******************************************************************************* */


   void LargeSwitchSet08( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
   {
      if ( ( ss8[ idx ] == 1 ) && ( ss8delay <= _Mmillis ) )
      {
         ttemp = ( Dcc.getCV( CV + 1 ) );
         if ( ttemp != 0 )
         {
            exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
         }
         ss8[ idx + 0 ] = 0;

         if ( !test )
         {
            ss8delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
            ss8[ idx + 1 ] = 1;
         }
         else
         {
            if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
            {
               ss8[  0 ] = 1;
               run_switch_set[  8 ] =  true;
            }
            else
            {
               run_switch_set[  8 ] = false;
            }
         }
      }
   }


   /* ******************************************************************************* */


   void LargeSwitchSet09( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
   {
      if ( ( ss9[ idx ] == 1 ) && ( ss9delay <= _Mmillis ) )
      {
         ttemp = ( Dcc.getCV( CV + 1 ) );
         if ( ttemp != 0 )
         {
            exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
         }
         ss9[ idx + 0 ] = 0;

         if ( !test )
         {
            ss9delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
            ss9[ idx + 1 ] = 1;
         }
         else
         {
            if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
            {
               ss9[  0 ] = 1;
               run_switch_set[  9 ] =  true;
            }
            else
            {
               run_switch_set[  9 ] = false;
            }
         }
      }
   }


   /* ******************************************************************************* */


   void LargeSwitchSet10( unsigned long _Mmillis, int idx, uint16_t CV, bool test = false )
   {
      if ( ( ss10[ idx ] == 1 ) && ( ss10delay <= _Mmillis ) )
      {
         ttemp = ( Dcc.getCV( CV + 1 ) );
         if ( ttemp != 0 )
         {
            exec_switch_function( Dcc.getCV( CV + 0 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function
         }
         ss10[ idx + 0 ] = 0;

         if ( !test )
         {
            ss10delay = _Mmillis + ( long( Dcc.getCV( CV + 2 ) * MasterTimeConstant ) );
            ss10[ idx + 1 ] = 1;
         }
         else
         {
            if ( Dcc.getCV( CV + 2 ) != 0 )  //  Do we jump back to the beginning or move on...
            {
               ss10[  0 ] = 1;
               run_switch_set[ 10 ] =  true;
            }
            else
            {
               run_switch_set[ 10 ] = false;
            }
         }
      }
   }

#endif

/* ******************************************************************************* */


void exec_switch_function( byte switch_function, byte fpin, byte fbit )
{
      // 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next 
   switch ( switch_function )  //  Find the switch function to execute
   {
      case  0:    //  0 == No function
      {
         break;
      }

      case  1:    //
      {
         digitalWrite( fpin, fbit );  //  Simple pin switch on/off
         break;
      }

      default:
      {
         break;
      }
   }
}     //   end    exec_switch_function()


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

   if ( ( ( CV == 251 ) && ( Value == 251 ) ) || ( ( CV == 252 ) && ( Value == 252 ) ) )
   {
      wdt_enable( WDTO_15MS );  //  Resets after 15 milliSecs

      while( 1 ) {}  // Wait for the prescaler time to expire
   }
}       //   end notifyCVChange()


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
 *                                  You must &FuncState with the appropriate
 *                                  FN_BIT_nn value to isolate a given bit.
 *
 *  Returns:
 *    None
 */
void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState )
{
   #if defined( DEBUG02 )

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
      case FN_0_4:    //  Function Group 1 F0 F4 F3 F2 F1
      {
         exec_function(  0, FunctionPin0, ( FuncState & FN_BIT_00 ) >> 4 );
         break ;
      }

      case FN_5_8:    //  Function Group 1    F8 F7 F6 F5
      {
         break ;
      }

      case FN_9_12:   //  Function Group 1    F12 F11 F10 F9
      {
         break ;
      }

      case FN_13_20:  //  Function Group 2  ==  F20 - F13
      {
         break ;
      }

      case FN_21_28:  //  Function Group 2  ==  F28 - F21
      {
         break ;
      }
   }
}                     //  End notifyDccFunc()


/* ******************************************************************************* */


void exec_function ( int function, int pin, int FuncState )
{
   #if defined( DEBUG03 )

      _PP( "exec_function = " );
      _2L( function,      DEC );
      _PP( "pin = "           );
      _2L( pin,           DEC );
      _PP( "FuncState = "     );
      _2L( FuncState,     DEC );

   #endif

   switch ( Dcc.getCV( 30 + function ) )
   {
      case  0:  // Master Disable by Function 0
      {
         Function0_value = byte( FuncState );
         break ;
      }

      default:
      {
         break ;
      }
   }
}                //  End exec_function()


/* ******************************************************************************* */


//
