
/* 
 * 
 * Interactive Decoder Random Switches   IDEC2_5_LargeFunctionSetsDev.ino
 * Version 1.08 Geoff Bunza 2020
 * Works with both short and long DCC Addesses
 * This decoder will control Switch Sequences, servos, sounds and LEDs
 * 
 * F0 = Master Function | OFF = Function | ON DISABLES the decoder
 * F0 is configured as Master Decoder Disable Override | ON == Disable Decoder
 * Input Pin for Decoder Disable | Pin 14/A0 Active LOW == Disable Decoder
 * 
 * F1-F2 Eight Switch Sets 1-2 controlled by Input pins 8, 9  respectively
 * All Switch Sets are defined by groups of 80 CVs
 *  - Either a DCC Function 1-2 on OR an Input Pin ( 8, 9) Switched Low enables a decoder function (ON)
 *  - BOTH the respective DCC Decoder Function 1-2 must be Off AND its respective Input Pin  ( 8, 9)
 *    MUST be High for a decoder function to be considered disabled
 *  - A decoder function LEFT ENABLED will repreat the respecpective action as long as it is enabled
 * 
 * Switch Set CV's are 25 groups of 3 CVs each:
 *    CV1 - A delay (0-255) which will be multiplied by the
 *          MasterTimeConstant setting time increments from milliseconds to minutes | 0 = No Delay
 *    CV2 - A Mode or Command byte Describing what will be executed in this Switch Step, including:
 *          0 = No Operation / Null /Skip
 *          1 = Simple pin switch on/off
 *          2 = Random pin switch on/off
 *          3 = Weighted Random pin switch on/off  default is 60% ON time but can be set to anything 1-99%
 *          4 = Play sound track  using fpin value for the track 1-126, 0 = Skip Play, 127 = Select Random Track
 *                from First_Track to Last_Track inclusive;
 *                MSB = 0 -> No Volume Change | MSB = 1 -> Set Volume to default_volume
 *          5 = Position Servo to 0-180 full speed of travel
 *          6 = Dual pin on/off used for alternate blink fpin and fpin+1 (MSB set value for fpin state)
 *          7 = Start another Switching set based on the fpin argument (Used to chain Switch Sets)
 *          8 = Start another Switching set based on the fpin argument ONLY if NOT already started
 *    CV3 - An argument representing the Pin number affected in the lower 7 bits and the High bit (0x80 or 128)
 *             or a general parameter like a servo position, a Sound track, or a sound set number to jump to
 * 
 * Switch sets include CVs:  50 - 129 and 130 - 209
 * 
 * Switch sets start with CVs:  50, (66, 82, 98, 114,) 130, (146, 162, 178, 194)
 * MAX one of 3 Configurations per pin function: 0 = DISABLE On / Off, 1 - 2 = Switch Control 1 - 2
 * 
 * PRO MINI PIN ASSIGNMENT:
 *   2 D2 - DCC Input Pin
 *   3 D3 - Input Pin Switch   1***
 *   4 D4 - Input Pin Switch   2***
 *   5 D5 - Input Pin Switch   3***
 *   6 D6 - Input Pin Switch   4***
 *   7 D7 - Input Pin Switch   5***
 *   8 B0 - Input Pin Switch   1
 *   9 B1 - Input Pin Switch   2
 *  10 B2 - Input Pin Switch   6***
 *  11 B3 - Output Pin 1***
 *  12 B4 - Output Pin 2***
 *  13 B5 - Output Pin 3***
 *  14 A0 - Input  Pin 9  for MasterDecoderDisable | Active  LOW
 *  15 A1 - Output Pin 4  or  default sound player pin (TX) connected to DFPlayer1 (RX) via 1K Ohm 1/4W Resistor
 *  16 A2 - Output Pin 5  or  default_servo_pin 
 *  17 A3 - Output Pin 6***
 *  18 A4 - Output Pin 7***
 *  19 A5 - Output Pin 8***
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


// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE THE SOUND FUNCTIONS (SD CARD)
// #define SOUNDON

#define default_volume  25     //  Sets default volume 0-30,  0 == OFF,  >30 == Skip Track

#if defined( SOUNDON )

   #include <SoftwareSerial.h>

   SoftwareSerial DFSerial1( 21, 15 ); // PRO MINI RX, PRO MINI TX serial to DFPlayer


   #include <DFRobotDFPlayerMini.h>

   DFRobotDFPlayerMini Player1;


   #define First_Track      1  //  Play Random Tracks - First_Track = Start_Track  >= First_Track
   #define Last_Track      12  //  Play Random Tracks - Last_Track = Last Playable Track in Range  <= Last_Track

   void playTrackOnChannel( byte  dtrack );
   void setVolumeOnChannel( byte dvolume );

#endif


// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE THIS SOFTWARE MOTOR FUNCTIONS
// #define MOTORON

#if defined( MOTORON )

   #include <SoftwareServo.h>

   SoftwareServo servo[ 1 ];

   #define default_servo_pin  16

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  *****  small macro used to create all the timed events  *****
#define runEvery( n ) for ( static unsigned long lasttime; millis() - lasttime > ( unsigned long )( n ); lasttime = millis() )


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Uncomment ONLY ONE of the following timing constants:
//
// fine  MasterTimeConstant  10L       // 10 milliseconds Timing
#define  MasterTimeConstant  100L      // 100 milliseconds Timing
// fine  MasterTimeConstant  1000L     // Seconds Timing
// fine  MasterTimeConstant  10000L    // 10 Seconds Timing
// fine  MasterTimeConstant  60000L    // Minutes Timing
// fine  MasterTimeConstant  3600000L  // Hours Timing


/* Array of bytes, used by the Switch Sets. The bytes keep track of the Actions we need
 * to take for the respective Set. Setting the first byte to Non-Zero starts the action
 * right after Setup has finished. Setting the last CV ( 65, 81, 97, 113, 129, 145, 161
 * 177, 193, 209) of a Switch Set to Non-Zero, keeps the Set running untill F0 Disables
 * the decoder or forever. Setting the CV to 0 again stops the Set when it runs out. */
unsigned long ss1delay = 0;  //  Switch Set  1
byte ss1[ ] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned long ss2delay = 0;  //  Switch Set  2
byte ss2[ ] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


const int MasterDecoderDisablePin  =   14;  //  A0  Master Decoder Disable Input Pin | Active  LOW
const int num_active_functions     =    3;  //  Number of Functions starting with F0
const int FunctionPin0             =   16;  //  A2  DFPlayer Transmit (TX)
const int FunctionPinDcc           =    2;  //  Inputpin DCCsignal
const int audiocmddelay            =   34;


uint8_t ttemp             =  0;  //  Temporary variable used in the loop function
int MasterDecoderDisable  =  0;  //  Standard value decoder is Enabled by default

int Function0_value       =  0;
byte function_value[ ]    =  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int numINpins       =  3;                                //  Number of INput pins to initialize
byte inputpins[ ]         =  {  8, 9, 14 };                    //  These are all the used  Input Pins

const int numfpins        = 14;                                // Number of Output pins to initialize
byte fpins[ ]             =  {  3, 4, 5, 6, 7, 10, 11, 12, 13, 15, 16, 17, 18, 19 };  //  Output Pins

bool run_switch_set[ ]    =  { false, false, false };
byte switchset_channel[ ] =  { 0, 0, 0 };


struct QUEUE
{
   int             inuse;
   int  current_position;
   int         increment;
   int        stop_value;
   int       start_value;
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

   { 30,   0},  //  F0  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 31,   1},  //  F1  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 32,  11},  //  F2  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 33,  11},  //  F3  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 34,  11},  //  F4  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 35,  11},  //  F5  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 36,  11},  //  F6  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 37,  11},  //  F7  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 38,  11},  //  F8  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 39,  11},  //  F9  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 40,  11},  // F10  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 41,  11},  // F11  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 42,  11},  // F12  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 43,  11},  // F13  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 44,  11},  // F14  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 45, 255},  // F15  not used


   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 50,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 1
   { 51,   1},  //  Switch Mode
   { 52, 141},  //  Switch Pin1
   { 53,  10},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   { 54,   1},  //  Switch Mode
   { 55,  13},  //  Switch Pin2
   { 56,  10},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   { 57,   1},  //  Switch Mode
   { 58, 141},  //  Switch Pin3
   { 59,  10},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   { 60,   1},  //  Switch Mode
   { 61,  13},  //  Switch Pin4
   { 62,  10},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   { 63,   0},  //  Switch Mode
   { 64,   0},  //  Switch Pin5
   { 65,   0},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 66,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 2
   { 67,   1},  //  Switch Mode
   { 68, 141},  //  Switch Pin1
   { 69,   1},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   { 70,   1},  //  Switch Mode
   { 71,  13},  //  Switch Pin2
   { 72,   1},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   { 73,   1},  //  Switch Mode
   { 74, 141},  //  Switch Pin3
   { 75,   1},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   { 76,   1},  //  Switch Mode
   { 77,  13},  //  Switch Pin4
   { 78,  10},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   { 79,   0},  //  Switch Mode
   { 80,   0},  //  Switch Pin5
   { 81,   1},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 82,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 3
   { 83,   0},  //  Switch Mode
   { 84,   0},  //  Switch Pin1
   { 85,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   { 86,   0},  //  Switch Mode
   { 87,   0},  //  Switch Pin2
   { 88,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   { 89,   0},  //  Switch Mode
   { 90,   0},  //  Switch Pin3
   { 91,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   { 92,   0},  //  Switch Mode
   { 93,   0},  //  Switch Pin4
   { 94,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   { 95,   0},  //  Switch Mode
   { 96,   0},  //  Switch Pin5
   { 97,   0},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 98,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 4
   { 99,   0},  //  Switch Mode
   {100,   0},  //  Switch Pin1
   {101,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {102,   0},  //  Switch Mode
   {103,   0},  //  Switch Pin2
   {104,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {105,   0},  //  Switch Mode
   {106,   0},  //  Switch Pin3
   {107,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {108,   0},  //  Switch Mode
   {109,   0},  //  Switch Pin4
   {110,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {111,   0},  //  Switch Mode
   {112,   0},  //  Switch Pin5
   {113,   0},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {114,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 5
   {115,   0},  //  Switch Mode
   {116,   0},  //  Switch Pin1
   {117,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {118,   0},  //  Switch Mode
   {119,   0},  //  Switch Pin2
   {120,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {121,   0},  //  Switch Mode
   {122,   0},  //  Switch Pin3
   {123,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {124,   0},  //  Switch Mode
   {125,   0},  //  Switch Pin4
   {126,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {127,   0},  //  Switch Mode
   {128,   0},  //  Switch Pin5
   {129,   0},  //  Not 0 if the set has to be kept running after starting it


   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {130,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 1
   {131,   1},  //  Switch Mode
   {132, 131},  //  Switch Pin1
   {133,  10},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {134,   1},  //  Switch Mode
   {135, 132},  //  Switch Pin2
   {136,  10},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {137,   1},  //  Switch Mode
   {138, 133},  //  Switch Pin3
   {139,  10},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {140,   1},  //  Switch Mode
   {141, 134},  //  Switch Pin4
   {142,  10},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {143,   0},  //  Switch Mode
   {144,   0},  //  Switch Pin5
   {145,   0},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {146,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 2
   {147,   1},  //  Switch Mode
   {148,   6},  //  Switch Pin1
   {149,  10},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {150,   1},  //  Switch Mode
   {151,   5},  //  Switch Pin2
   {152,  10},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {153,   1},  //  Switch Mode
   {154,   4},  //  Switch Pin3
   {155,  10},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {156,   1},  //  Switch Mode
   {157,   3},  //  Switch Pin4
   {158,  10},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {159,   0},  //  Switch Mode
   {160,   0},  //  Switch Pin5
   {161,   2},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {162,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 3
   {163,   0},  //  Switch Mode
   {164,   0},  //  Switch Pin1
   {165,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {166,   0},  //  Switch Mode
   {167,   0},  //  Switch Pin2
   {168,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {169,   0},  //  Switch Mode
   {170,   0},  //  Switch Pin3
   {171,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {172,   0},  //  Switch Mode
   {173,   0},  //  Switch Pin4
   {174,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {175,   0},  //  Switch Mode
   {176,   0},  //  Switch Pin5
   {177,   0},  //  Not 0 if the set has to be kept running after starting it

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {178,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 4
   {179,   0},  //  Switch Mode
   {180,   0},  //  Switch Pin1
   {181,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {182,   0},  //  Switch Mode
   {183,   0},  //  Switch Pin2
   {184,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {185,   0},  //  Switch Mode
   {186,   0},  //  Switch Pin3
   {187,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {188,   0},  //  Switch Mode
   {189,   0},  //  Switch Pin4
   {190,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {191,   0},  //  Switch Mode
   {192,   0},  //  Switch Pin5
   {193,   0},  //  Not 0 if the set has to be kept running after starting it
  
   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {194,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 5
   {195,   0},  //  Switch Mode
   {196,   0},  //  Switch Pin1
   {197,   0},  //  Wait2 = 0 - 254 * MasterTimeConstant Seconds
   {198,   0},  //  Switch Mode
   {199,   0},  //  Switch Pin2
   {200,   0},  //  Wait3 = 0 - 254 * MasterTimeConstant Seconds
   {201,   0},  //  Switch Mode
   {202,   0},  //  Switch Pin3
   {203,   0},  //  Wait4 = 0 - 254 * MasterTimeConstant Seconds
   {204,   0},  //  Switch Mode
   {205,   0},  //  Switch Pin4
   {206,   0},  //  Wait5 = 0 - 254 * MasterTimeConstant Seconds
   {207,   0},  //  Switch Mode
   {208,   0},  //  Switch Pin5
   {209,   0},  //  Not 0 if the set has to be kept running after starting it

   {251,   0},  //  CV_DECODER_JUST_A_RESET */
/* {252,   0},  //  CV_DECODER_MASTER_RESET */
   {253,   0},  //  Extra
};


/* ******************************************************************************* */


uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset 
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
};


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
      digitalWrite( inputpins[ i ], 0 );        // Switch the pin off first.
      pinMode( inputpins[ i ], INPUT_PULLUP );  // Then set as input_pullup.
	}

   #if defined( SOUNDON )

      DFSerial1.begin( 9600 );
      Player1.begin( DFSerial1 );

   #endif

   #if defined( MOTORON )

      servo[ 0 ].attach( default_servo_pin );  // Start Servo on default_servo_pin  //Position Servo
      delay( 50 );
      SoftwareServo::refresh();

   #endif

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

      // Master Decoder Disable
   MasterDecoderDisable = 0;
   if ( digitalRead( MasterDecoderDisablePin ) == LOW )
   {
      MasterDecoderDisable = 1;
   }

   #if defined( DEBUGID ) || defined( DEBUGCV )

      _PL( "\n" "CV Dump:"  );
      for ( int i =  30; i <  46; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  30; i <  46; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Large Switch Set  1:" );
      for ( int i =  50; i <  66; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  50; i <  66; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i =  66; i <  82; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  66; i <  82; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i =  82; i <  98; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  82; i <  98; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i =  98; i < 114; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i =  98; i < 114; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 114; i < 130; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 114; i < 130; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");

      _PL( "Large Switch Set  2:" );
      for ( int i = 130; i < 146; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 130; i < 146; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 146; i < 162; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 146; i < 162; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 162; i < 178; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 162; i < 178; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 178; i < 194; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 178; i < 194; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
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
   MasterDecoderDisable = 0 ;
   if ( digitalRead( MasterDecoderDisablePin ) == LOW )
   {
      MasterDecoderDisable = 1 ;
   }
   else
   {
      MasterDecoderDisable = Function0_value & 1 ;
   }

   #ifdef MOTORON

      SoftwareServo::refresh();

   #endif

   if ( MasterDecoderDisable == 1 )
   {
      for ( int i = 0; i < numfpins; ++i ) digitalWrite( fpins[ i ], 0 ); // All LEDs off
   }


   if ( MasterDecoderDisable == 0 )
   {
      for ( int i = 0; i < num_active_functions; ++i )
      {
         uint8_t cv_value = Dcc.getCV( 30 + i );

         switch ( cv_value )
         {
            case  0:   // Master Decoder Disable
            {
               MasterDecoderDisable = 0;
               if ( digitalRead( MasterDecoderDisablePin ) ==  LOW )
               {
                  MasterDecoderDisable = 1;
               }
               break;
            }

            case  1:   // 
            {
               if ( ( ( digitalRead(  8 ) ==  LOW ) || ( function_value[ cv_value ] == 1 ) ) && !run_switch_set[ cv_value ] )
               {
                  ss1[ 0 ] = 1;
                  run_switch_set[ cv_value ] = true;
               }

               #if defined( DEBUGCV )

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
               if ( ( ( digitalRead(  9 ) ==  LOW ) || ( function_value[ cv_value ] == 1 ) ) && !run_switch_set[ cv_value ] )
               {
                  ss2[ 0 ] = 1;
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

      //    ==========================   Large Switch Set  1 Start Run
   if ( ss1[  0 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV(  50 ) * MasterTimeConstant ) ); //  Wait1
      ss1[  0 ] = 0;
      ss1[  1 ] = 1;
   }

   if ( ( ss1[  1 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  52 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  51 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  53 ) * MasterTimeConstant ) ); //  Wait2
      ss1[  1 ] = 0;
      ss1[  2 ] = 1;
   }

   if ( ( ss1[  2 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  55 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  54 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  56 ) * MasterTimeConstant ) ); //  Wait3
      ss1[  2 ] = 0;
      ss1[  3 ] = 1;
   }

   if ( ( ss1[  3 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  58 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  57 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  59 ) * MasterTimeConstant ) ); //  Wait4
      ss1[  3 ] = 0;
      ss1[  4 ] = 1;
   }

   if ( ( ss1[  4 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  61 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  60 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  62 ) * MasterTimeConstant ) ); //  Wait5
      ss1[  4 ] = 0;
      ss1[  5 ] = 1;
   }

   if ( ( ss1[  5 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  64 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  63 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss1[  5 ] = 0;
      ss1[  6 ] = 1;

      if ( Dcc.getCV(  65 ) != 0 )
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss1[  6 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV(  66 ) * MasterTimeConstant ) ); //  Wait1
      ss1[  6 ] = 0;
      ss1[  7 ] = 1;
   }

   if ( ( ss1[  7 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  68 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  67 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  69 ) * MasterTimeConstant ) ); //  Wait2
      ss1[  7 ] = 0;
      ss1[  8 ] = 1;
   }

   if ( ( ss1[  8 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  71 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  70 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  72 ) * MasterTimeConstant ) ); //  Wait3
      ss1[  8 ] = 0;
      ss1[  9 ] = 1;
   }

   if ( ( ss1[  9 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  74 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  73 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  75 ) * MasterTimeConstant ) ); //  Wait4
      ss1[  9 ] = 0;
      ss1[ 10 ] = 1;
   }

   if ( ( ss1[ 10 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  77 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  76 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  78 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 10 ] = 0;
      ss1[ 11 ] = 1;
   }

   if ( ( ss1[ 11 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  80 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  79 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss1[ 11 ] = 0;
      ss1[ 12 ] = 1;

      if ( Dcc.getCV(  81 ) != 0 )
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss1[ 12 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV(  82 ) * MasterTimeConstant ) ); //  Wait1
      ss1[ 12 ] = 0;
      ss1[ 13 ] = 1;
   }

   if ( ( ss1[ 13 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  84 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  83 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  1
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  85 ) * MasterTimeConstant ) ); //  Wait2
      ss1[ 13 ] = 0;
      ss1[ 14 ] = 1;
   }

   if ( ( ss1[ 14 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  87 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  86 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  88 ) * MasterTimeConstant ) ); //  Wait3
      ss1[ 14 ] = 0;
      ss1[ 15 ] = 1;
   }

   if ( ( ss1[ 15 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  90 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  89 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  91 ) * MasterTimeConstant ) ); //  Wait3
      ss1[ 15 ] = 0;
      ss1[ 16 ] = 1;
   }

   if ( ( ss1[ 16 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  93 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  92 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV(  94 ) * MasterTimeConstant ) ); //  Wait3
      ss1[ 16 ] = 0;
      ss1[ 17 ] = 1;
   }

   if ( ( ss1[ 17 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV(  96 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  95 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1[ 17 ] = 0;
      ss1[ 18 ] = 1;

      if ( Dcc.getCV(  97 ) != 0 )
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss1[ 18 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV(  98 ) * MasterTimeConstant ) ); //  Wait1
      ss1[ 18 ] = 0;
      ss1[ 19 ] = 1;
   }

   if ( ( ss1[ 19 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 100 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV(  99 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  1
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 101 ) * MasterTimeConstant ) ); //  Wait2
      ss1[ 19 ] = 0;
      ss1[ 20 ] = 1;
   }

   if ( ( ss1[ 20 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 103 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 102 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 104 ) * MasterTimeConstant ) ); //  Wait3
      ss1[ 20 ] = 0;
      ss1[ 21 ] = 1;
   }

   if ( ( ss1[ 21 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 106 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 105 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 107 ) * MasterTimeConstant ) ); //  Wait4
      ss1[ 21 ] = 0;
      ss1[ 22 ] = 1;
   }

   if ( ( ss1[ 22 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 109 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 108 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 110 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 22 ] = 0;
      ss1[ 23 ] = 1;
   }

   if ( ( ss1[ 23 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 112 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 111 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss1[ 23 ] = 0;
      ss1[ 24 ] = 1;

      if ( Dcc.getCV( 113 ) != 0 )
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss1[ 24 ] == 1 )
   {
      ss1delay = _Mmillis + ( long( Dcc.getCV( 114 ) * MasterTimeConstant ) ); //  Wait1
      ss1[ 24 ] = 0;
      ss1[ 25 ] = 1;
   }

   if ( ( ss1[ 25 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 116 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 115 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 117 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 25 ] = 0;
      ss1[ 26 ] = 1;
   }

   if ( ( ss1[ 26 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 119 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 118 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 120 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 26 ] = 0;
      ss1[ 27 ] = 1;
   }

   if ( ( ss1[ 27 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 122 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 121 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 123 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 27 ] = 0;
      ss1[ 28 ] = 1;
   }

   if ( ( ss1[ 28 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 125 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 124 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss1delay = _Mmillis + ( long( Dcc.getCV( 126 ) * MasterTimeConstant ) ); //  Wait5
      ss1[ 28 ] = 0;
      ss1[ 29 ] = 1;
   }

   if ( ( ss1[ 29 ] == 1 ) && ( ss1delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 128 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 127 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss1[ 29 ] = 0;
      run_switch_set[  1 ] = false;

      if ( Dcc.getCV( 129 ) != 0 )
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
   }


      //    ==========================   Large Switch Set  2 Start Run
   if ( ss2[  0 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV(  130 ) * MasterTimeConstant ) ); //  Wait1
      ss2[  0 ] = 0;
      ss2[  1 ] = 1;
   }

   if ( ( ss2[  1 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 132 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 131 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 133 ) * MasterTimeConstant ) ); //  Wait2
      ss2[  1 ] = 0;
      ss2[  2 ] = 1;
   }

   if ( ( ss2[  2 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 135 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 134 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 136 ) * MasterTimeConstant ) ); //  Wait3
      ss2[  2 ] = 0;
      ss2[  3 ] = 1;
   }

   if ( ( ss2[  3 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 138 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 137 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 139 ) * MasterTimeConstant ) ); //  Wait4
      ss2[  3 ] = 0;
      ss2[  4 ] = 1;
   }

   if ( ( ss2[  4 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 141 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 140 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 142 ) * MasterTimeConstant ) ); //  Wait5
      ss2[  4 ] = 0;
      ss2[  5 ] = 1;
   }

   if ( ( ss2[  5 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 144 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 143 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss2[  5 ] = 0;
      ss2[  6 ] = 1;

      if ( Dcc.getCV( 145 ) != 0 )
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  2 Continues
   if ( ss2[  6 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV( 146 ) * MasterTimeConstant ) ); //  Wait1
      ss2[  6 ] = 0;
      ss2[  7 ] = 1;
   }

   if ( ( ss2[  7 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 148 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 147 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 149 ) * MasterTimeConstant ) ); //  Wait2
      ss2[  7 ] = 0;
      ss2[  8 ] = 1;
   }

   if ( ( ss2[  8 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 151 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 150 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 152 ) * MasterTimeConstant ) ); //  Wait3
      ss2[  8 ] = 0;
      ss2[  9 ] = 1;
   }

   if ( ( ss2[  9 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 154 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 153 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 155 ) * MasterTimeConstant ) ); //  Wait4
      ss2[  9 ] = 0;
      ss2[ 10 ] = 1;
   }

   if ( ( ss2[ 10 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 157 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 156 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 158 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 10 ] = 0;
      ss2[ 11 ] = 1;
   }

   if ( ( ss2[ 11 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 160 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 159 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss2[ 11 ] = 0;
      ss2[ 12 ] = 1;

      if ( Dcc.getCV( 161 ) != 0 )
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  2 Continues
   if ( ss2[ 12 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV( 162 ) * MasterTimeConstant ) ); //  Wait1
      ss2[ 12 ] = 0;
      ss2[ 13 ] = 1;
   }

   if ( ( ss2[ 13 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 164 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 163 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  1
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 165 ) * MasterTimeConstant ) ); //  Wait2
      ss2[ 13 ] = 0;
      ss2[ 14 ] = 1;
   }

   if ( ( ss2[ 14 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 167 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 166 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 168 ) * MasterTimeConstant ) ); //  Wait3
      ss2[ 14 ] = 0;
      ss2[ 15 ] = 1;
   }

   if ( ( ss2[ 15 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 170 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 169 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 171 ) * MasterTimeConstant ) ); //  Wait3
      ss2[ 15 ] = 0;
      ss2[ 16 ] = 1;
   }

   if ( ( ss2[ 16 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 173 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 172 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 174 ) * MasterTimeConstant ) ); //  Wait3
      ss2[ 16 ] = 0;
      ss2[ 17 ] = 1;
   }

   if ( ( ss2[ 17 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 176 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 175 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2[ 17 ] = 0;
      ss2[ 18 ] = 1;

      if ( Dcc.getCV( 177 ) != 0 )
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss2[ 18 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV( 178 ) * MasterTimeConstant ) ); //  Wait1
      ss2[ 18 ] = 0;
      ss2[ 19 ] = 1;
   }

   if ( ( ss2[ 19 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 180 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 179 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  1
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 181 ) * MasterTimeConstant ) ); //  Wait2
      ss2[ 19 ] = 0;
      ss2[ 20 ] = 1;
   }

   if ( ( ss2[ 20 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 183 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 182 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 184 ) * MasterTimeConstant ) ); //  Wait3
      ss2[ 20 ] = 0;
      ss2[ 21 ] = 1;
   }

   if ( ( ss2[ 21 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 186 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 185 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 187 ) * MasterTimeConstant ) ); //  Wait4
      ss2[ 21 ] = 0;
      ss2[ 22 ] = 1;
   }

   if ( ( ss2[ 22 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 189 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 188 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 190 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 22 ] = 0;
      ss2[ 23 ] = 1;
   }

   if ( ( ss2[ 23 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 192 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 191 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss2[ 23 ] = 0;
      ss2[ 24 ] = 1;

      if ( Dcc.getCV( 193 ) != 0 )
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
   }

      //    ==========================   Large Switch Set  1 Continues
   if ( ss2[ 24 ] == 1 )
   {
      ss2delay = _Mmillis + ( long( Dcc.getCV( 194 ) * MasterTimeConstant ) ); //  Wait1
      ss2[ 24 ] = 0;
      ss2[ 25 ] = 1;
   }

   if ( ( ss2[ 25 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 196 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 195 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 197 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 25 ] = 0;
      ss2[ 26 ] = 1;
   }

   if ( ( ss2[ 26 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 199 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 198 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 200 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 26 ] = 0;
      ss2[ 27 ] = 1;
   }

   if ( ( ss2[ 27 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 202 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 201 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 203 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 27 ] = 0;
      ss2[ 28 ] = 1;
   }

   if ( ( ss2[ 28 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 205 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 204 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss2delay = _Mmillis + ( long( Dcc.getCV( 206 ) * MasterTimeConstant ) ); //  Wait5
      ss2[ 28 ] = 0;
      ss2[ 29 ] = 1;
   }

   if ( ( ss2[ 29 ] == 1 ) && ( ss2delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 208 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 207 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss2[ 29 ] = 0;
      run_switch_set[  2 ] = false;

      if ( Dcc.getCV( 209 ) != 0 )
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
   }
}       // end  loop() 


/* ******************************************************************************* */


void exec_switch_function( byte switch_function, byte fpin, byte fbit )
{
   if ( MasterDecoderDisable == 1 ) return;

      // 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next 
   switch ( switch_function )  //  Find the switch function to execute
   {
      case  0:    //  0 == No function
      default:
      {
         break;
      }

      case  1:    //
      {
         digitalWrite( fpin, fbit );  //  Simple pin switch on/off
         break;
      }

      case  2:    //
      {
         if ( fbit != 0 )
         {
            digitalWrite ( fpin, random ( 0, 2 ) );  //  Random pin switch on/off
         }
         else
         {
            digitalWrite ( fpin, 0 );  //  Force the bit OFF
         }
         break;
      }

      case  3:    // 
      {
         if ( fbit != 0 )
         {
            digitalWrite ( fpin, Weighted_ON() );  //  Weighted Random pin switch on/off
         }
         else
         {
            digitalWrite ( fpin, 0 );  //  Force the bit OFF
         }
         break;
      }

      case  4:    // 
      {
         playTheclip( fpin, default_volume * fbit );
         break;
      }

      case  5:    // 
      {
         #ifdef SERVOON
            servo[ 0 ].write( fpin | ( fbit << 7 ) );      //  Position Servo
         #endif
         break;
      }

      case  6:    // 
      {
         digitalWrite ( fpin, fbit );                  //  Dual pin on/off  alternate blink
         digitalWrite ( fpin + 1, ( ~fbit ) & 0x01 );  //  Dual pin on/off  alternate blink
         break;
      }

      case  7:    //   Start up another switch set
      {
         switch ( fpin )    // Start another Switching set based on the fpin argument
         {
            case  0:    //  No action required
            default:
            {
               break;
            }

            case  1:    //   Start Switch Set  1
            {
               ss1[ 0 ] = 1;
               run_switch_set[ fpin ] = true;
               break;
            }

            case  2:    //    Start Switch Set  2
            {
               ss2[ 0 ] = 1;
               run_switch_set[ fpin ] = true;
               break;
            }
         }
         break;
      }

      case  8:    // Start Switching set if not already started
      {
         switch ( fpin )    // Start another Switching set based on the fpin argument
         {
            case  0:    //  
            default:
            {
               break;
            }

            case  1:    //   Start Switch Set  1
            {
               if ( run_switch_set[ fpin ] == false )
               {
                  ss1[ 0 ] = 1;
                  run_switch_set[ fpin ] = true;
               }
               break;
            }

            case  2:    //    Start Switch Set  2
            {
               if ( run_switch_set[ fpin ] == false )
               {
                  ss2[ 0 ] = 1;
                  run_switch_set[ fpin ] = true;
               }
               break;
            }
         }
         break;
      }
   }
}     //   end    exec_switch_function()


/* ******************************************************************************* */


boolean Weighted_ON()
{
   if ( random ( 0, 100 ) > 40 )
   {
      return true ;  //  This will return true 60% of the time
   }

   return false;
}    //   end    Weighted_ON()


/* ******************************************************************************* */


#if defined( SOUNDON )

   void playTrackOnChannel( byte dtrack )
   {
      if ( dtrack == 0 ) return;

      if ( dtrack != 127 )
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


/* ******************************************************************************* */


   void setVolumeOnChannel( byte dvolume )
   {
      if ( dvolume > default_volume )
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


/* ******************************************************************************* */


void playTheclip( byte fpin, byte volume )
{
   #if defined( SOUNDON )

      setVolumeOnChannel( volume );          //  Set volume

      playTrackOnChannel(  fpin  );          //  Play clips

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
      case FN_0_4:    //  Function Group 1 F0 F4 F3 F2 F1
      {
         exec_function(  0, FunctionPin0, ( FuncState & FN_BIT_00 ) >> 4 );
         exec_function(  1, FunctionPin0, ( FuncState & FN_BIT_01 ) >> 0 );
         exec_function(  2, FunctionPin0, ( FuncState & FN_BIT_02 ) >> 1 );
         exec_function(  3, FunctionPin0, ( FuncState & FN_BIT_03 ) >> 2 );
         exec_function(  4, FunctionPin0, ( FuncState & FN_BIT_04 ) >> 3 );
         break ;
      }

      case FN_5_8:    //  Function Group 1    F8 F7 F6 F5
      {
         exec_function(  5, FunctionPin0, ( FuncState & FN_BIT_05 ) >> 0 );
         exec_function(  6, FunctionPin0, ( FuncState & FN_BIT_06 ) >> 1 );
         exec_function(  7, FunctionPin0, ( FuncState & FN_BIT_07 ) >> 2 );
         exec_function(  8, FunctionPin0, ( FuncState & FN_BIT_08 ) >> 3 );
         break ;
      }

      case FN_9_12:   //  Function Group 1    F12 F11 F10 F9
      {
         exec_function(  9, FunctionPin0, ( FuncState & FN_BIT_09 ) >> 0 );
         exec_function( 10, FunctionPin0, ( FuncState & FN_BIT_10 ) >> 1 );
         exec_function( 11, FunctionPin0, ( FuncState & FN_BIT_11 ) >> 2 );
         exec_function( 12, FunctionPin0, ( FuncState & FN_BIT_12 ) >> 3 );
         break ;
      }

      case FN_13_20:  //  Function Group 2  ==  F20 - F13
      {
         exec_function( 13, FunctionPin0, ( FuncState & FN_BIT_13 ) >> 0 );
         exec_function( 14, FunctionPin0, ( FuncState & FN_BIT_14 ) >> 1 );
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
      case  0:  // Master Disable by Function 0
      {
         Function0_value = byte( FuncState );
         break ;
      }

      case  1:  //   run switch set [ function ]
      case  2:  //   run switch set [ function ]
      case  3:  //   run switch set [ function ]
      case  4:  //   run switch set [ function ]
      case  5:  //   run switch set [ function ]
      case  6:  //   run switch set [ function ]
      case  7:  //   run switch set [ function ]
      case  8:  //   run switch set [ function ]
      case  9:  //   run switch set [ function ]
      case 10:  //   run switch set [ function ]
      {
         function_value[ function ] = byte( FuncState );
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
