
/* 
 * 
 * Interactive Decoder For Traffic Lights
 * Version 1.01 Willem van Bergen (2021)
 * Works with both short and long DCC Addesses
 * This decoder will control Switch Sequences for LEDs
 * Based on Geoff Bunza's great work.
 * 
 * F0 = Master Function | OFF = Function | ON DISABLES the decoder
 * F0 is configured as Master Decoder Disable Override | ON == Disable Decoder
 * Input Pin for Decoder Disable | Pin 14 / A0 Active LOW == Disable Decoder
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
 */
/* 
 * PRO MINI PIN ASSIGNMENT:
 *   2 D2 - DCC Input Pin
 *   3 D3 - Output Pin  3  Red    P
 *   4 D4 - Output Pin  4  Red    1
 *   5 D5 - Output Pin  5  Red    2
 *   6 D6 - Output Pin  6  Red    3
 *   7 D7 - Output Pin  7  Red    4
 *   8 B0 - Input  Pin  Switch    1  Blinking Yellow | Active  LOW
 *   9 B1 - Output Pin  9  Yellow P
 *  10 B2 - Output Pin 10  Yellow 1
 *  11 B3 - Output Pin 11  Yellow 2
 *  12 B4 - Output Pin 12  Yellow 3
 *  13 B5 - Output Pin 13  Yellow 4
 *  14 A0 - Input  Pin 14  sets MasterDecoderDisable | Active  LOW
 *  15 A1 - Output Pin 15  Green  P
 *  16 A2 - Output Pin 16  Green  1
 *  17 A3 - Output Pin 17  Green  2
 *  18 A4 - Output Pin 18  Green  3
 *  19 A5 - Output Pin 19  Green  4
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
// #define DECODER_LOADED

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
byte ss1[ ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned long ss2delay = 0;  //  Switch Set  2
byte ss2[ ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned long ss3delay = 0;  //  Switch Set  3
byte ss3[ ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int MasterDecoderDisablePin  =   14;  //  A0  Master Decoder Disable Input Pin | Active  LOW
const int MasterFunctionSwitchPin  =    8;  //  B0  Master Function Switch Input Pin | Active  LOW
const int num_active_functions     =    4;  //  Number of Functions starting with F0
const int FunctionPin0             =   20;  //  Dummy number
const int FunctionPinDcc           =    2;  //  Inputpin DCCsignal


uint8_t ttemp             =  0;  //  Temporary variable used in the loop function
int MasterDecoderDisable  =  0;  //  Standard value decoder is Enabled by default
int MasterFunctionSwitch  =  0;  //  Standard value | default is Blinking Yellows

int Function0_value       =  0;
byte function_value[ ]    =  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int numINpins       =  2;                                //  Number of INput pins to initialize
byte inputpins[ ]         =  {  8, 14 };                       //  These are all the used  Input Pins

const int numfpins        = 15;                                // Number of Output pins to initialize
byte fpins[ ]             =  {  3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19 };  //   Outputs

bool run_switch_set[ ]    =  { false, false, false, false };
byte switchset_channel[ ] =  { 0, 0, 0, 0 };


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
   { 32,  10},  //  F2  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 33,  10},  //  F3  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 34,  10},  //  F4  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 35,  10},  //  F5  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 36,  10},  //  F6  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 37,  10},  //  F7  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 38,  10},  //  F8  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 39,  10},  //  F9  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 40,  10},  // F10  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 41,  10},  // F11  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 42,  10},  // F12  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 43,  10},  // F13  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 44,  10},  // F14  Config 0=DISABLE On/Off, 1-8=Switch Control 1-10, 11=LED On/Off
   { 45, 255},  // F15  not used


   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 50,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 1
   { 51,   1},  //
   { 52, 131},  //  Output Pin  3  Red    P  On
   { 53,   0},  //
   { 54,   1},  //
   { 55, 132},  //  Output Pin  4  Red    1  On
   { 56,   0},  //
   { 57,   1},  //
   { 58, 133},  //  Output Pin  5  Red    2  On
   { 59,   0},  //
   { 60,   1},  //
   { 61, 134},  //  Output Pin  6  Red    3  On
   { 62,   0},  //
   { 63,   1},  //
   { 64, 135},  //  Output Pin  7  Red    4  On
   { 65,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 66,  10},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 2
   { 67,   1},  //
   { 68,   3},  //  Output Pin  3  Red    P Off
   { 69,   0},  //
   { 70,   1},  //
   { 71, 143},  //  Output Pin 15  Green  P  On
   { 72,  20},  //
   { 73,   1},  //
   { 74,  15},  //  Output Pin 15  Green  P Off
   { 75,   0},  //
   { 76,   1},  //
   { 77, 137},  //  Output Pin  9  Yellow P  On
   { 78,  10},  //
   { 79,   1},  //
   { 80,   9},  //  Output Pin  9  Yellow P Off
   { 81,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 82,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 3
   { 83,   1},  //
   { 84, 131},  //  Output Pin  3  Red    P  On
   { 85,  10},  //
   { 86,   1},  //
   { 87, 138},  //  Output Pin 10  Yellow 1  On
   { 88,   5},  //
   { 89,   1},  //
   { 90,  10},  //  Output Pin 10  Yellow 1 Off
   { 91,   0},  //
   { 92,   1},  //
   { 93,   4},  //  Output Pin  4  Red    1 Off
   { 94,   0},  //
   { 95,   1},  //
   { 96, 144},  //  Output Pin 16  Green  1  On
   { 97,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   { 98,  30},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 4
   { 99,   1},  //
   {100,  16},  //  Output Pin 16  Green  1 Off
   {101,   0},  //
   {102,   1},  //
   {103, 138},  //  Output Pin 10  Yellow 1  On
   {104,  15},  //
   {105,   1},  //
   {106,  10},  //  Output Pin 10  Yellow 1 Off
   {107,   0},  //
   {108,   1},  //
   {109, 132},  //  Output Pin  4  Red    1  On
   {110,  10},  //
   {111,   1},  //
   {112, 139},  //  Output Pin 11  Yellow 2  On
   {113,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {114,  10},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 1 / 5
   {115,   1},  //
   {116,  11},  //  Output Pin 11  Yellow 2 Off
   {117,   0},  //
   {118,   1},  //
   {119,   5},  //  Output Pin  5  Red    2 Off
   {120,   0},  //
   {121,   1},  //
   {122, 145},  //  Output Pin 17  Green  2  On
   {123,  30},  //
   {124,   1},  //
   {125,  17},  //  Output Pin 17  Green  2 Off
   {126,   0},  //
   {127,   0},  //
   {128,   0},  //  Check Start Switch Set 2 On
   {129,   0},  //


   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {130,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 1
   {131,   1},  //
   {132, 139},  //  Output Pin 11  Yellow 2  On
   {133,  15},  //
   {134,   1},  //
   {135,  11},  //  Output Pin 11  Yellow 2 Off
   {136,   0},  //
   {137,   1},  //
   {138, 133},  //  Output Pin  5  Red    2  On
   {139,  10},  //
   {140,   1},  //
   {141, 140},  //  Output Pin 12  Yellow 3  On
   {142,  10},  //
   {143,   1},  //
   {144,  12},  //  Output Pin 12  Yellow 3 Off
   {145,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {146,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 2
   {147,   1},  //
   {148,   6},  //  Output Pin  6  Red    3 Off
   {149,   0},  //
   {150,   1},  //
   {151, 146},  //  Output Pin 18  Green  3  On
   {152,  30},  //
   {153,   1},  //
   {154,  18},  //  Output Pin 18  Green  3 Off
   {155,   0},  //
   {156,   1},  //
   {157, 140},  //  Output Pin 12  Yellow 3  On
   {158,  15},  //
   {159,   1},  //
   {160,  12},  //  Output Pin 12  Yellow 3 Off
   {161,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {162,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 3
   {163,   1},  //
   {164, 134},  //  Output Pin  6  Red    3  On
   {165,  10},  //
   {166,   1},  //
   {167, 141},  //  Output Pin 13  Yellow 4  On
   {168,  10},  //
   {169,   1},  //
   {170,  13},  //  Output Pin 13  Yellow 4 Off
   {171,   0},  //
   {172,   1},  //
   {173,   7},  //  Output Pin  7  Red    4 Off
   {174,   0},  //
   {175,   1},  //
   {176, 147},  //  Output Pin 19  Green  4  On
   {177,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {178,  30},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 4
   {179,   1},  //
   {180,  19},  //  Output Pin 19  Green  4 Off
   {181,   0},  //
   {182,   1},  //
   {183, 141},  //  Output Pin 13  Yellow 4  On
   {184,  15},  //
   {185,   1},  //
   {186,  13},  //  Output Pin 13  Yellow 4 Off
   {187,   0},  //
   {188,   1},  //
   {189, 135},  //  Output Pin  7  Red    4  On
   {190,   0},  //
   {191,   0},  //
   {192,   0},  //  Check Start Switch Set 3 On
   {193,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {194,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 2 / 5
   {195,   0},  //
   {196,   0},  //
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


   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {210,   1},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 3 / 1
   {211,   1},  //
   {212, 137},  //  Output Pin  9  Yellow P  On
   {213,   1},  //
   {214,   1},  //
   {215, 138},  //  Output Pin 10  Yellow 1  On
   {216,   1},  //
   {217,   1},  //
   {218, 139},  //  Output Pin 11  Yellow 2  On
   {219,   1},  //
   {220,   1},  //
   {221, 140},  //  Output Pin 12  Yellow 3  On
   {222,   1},  //
   {223,   1},  //
   {224, 141},  //  Output Pin 13  Yellow 4  On
   {225,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {226,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 3 / 2
   {227,   1},  //
   {228,   9},  //  Output Pin  9  Yellow P Off
   {229,   1},  //
   {230,   1},  //
   {231,  10},  //  Output Pin 10  Yellow 1 Off
   {232,   1},  //
   {233,   1},  //
   {234,  11},  //  Output Pin 11  Yellow 2 Off
   {235,   1},  //
   {236,   1},  //
   {237,  12},  //  Output Pin 12  Yellow 3 Off
   {238,   1},  //
   {239,   1},  //
   {240,  13},  //  Output Pin 13  Yellow 4 Off
   {241,   0},  //

   //  Switch Mode | 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7=Next SS, 8=Next SS Conditional
   {242,   0},  //  Wait1 = 0 - 254 * MasterTimeConstant Seconds  |  Switch SET 3 / 3
   {243,   0},  //
   {244,   0},  //  Start Switch Set  1
   {245,   0},  //
   {246,   0},  //
   {247,   0},  //
   {248,   0},  //  Not 0 if the set has to be kept running after starting it


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

      // Function Switch Disable
   MasterFunctionSwitch = 0;
   if ( digitalRead( MasterFunctionSwitchPin ) == LOW )
   {
      MasterFunctionSwitch = 1;
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

      _PL( "Small Switch Set  3:" );
      for ( int i = 210; i < 226; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 210; i < 226; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 226; i < 242; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 226; i < 242; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL("\n");
      for ( int i = 242; i < 249; ++i ) { _2P(            i  , DEC ); _PP( "\t" ); }
      _PL("");
      for ( int i = 242; i < 249; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
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

            case  1:   //  Function F1
            {
               MasterFunctionSwitch = 0;
               if ( digitalRead( MasterFunctionSwitchPin ) ==  LOW )
               {
                  MasterFunctionSwitch = 1;
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

      if ( Dcc.getCV(  65 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         ss1[  6 ] = 1;
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

      if ( Dcc.getCV(  81 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         ss1[ 12 ] = 1;
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

      if ( Dcc.getCV(  97 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         ss1[ 18 ] = 1;
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

      if ( Dcc.getCV( 113 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         ss1[ 24 ] = 1;
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

      if ( Dcc.getCV( 129 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         ss2[  0 ] = 1;
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

      if ( Dcc.getCV( 145 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
      else
      {
         ss2[  6 ] = 1;
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

      if ( Dcc.getCV( 161 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
      else
      {
         ss2[ 12 ] = 1;
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

      if ( Dcc.getCV( 177 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
      else
      {
         ss2[ 18 ] = 1;
      }
   }

      //    ==========================   Large Switch Set  2 Continues
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

      if ( Dcc.getCV( 193 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss2[  0 ] = 1;
         run_switch_set[  2 ] =  true;
      }
      else
      {
         ss2[ 24 ] = 1;
      }
   }

      //    ==========================   Large Switch Set  2 Continues
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

      if ( Dcc.getCV( 209 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss1[  0 ] = 1;
         run_switch_set[  1 ] =  true;
      }
      else
      {
         run_switch_set[  1 ] = false;
      }
   }


      //    ==========================   Small Switch Set  3 Start Run
   if ( ss3[  0 ] == 1 )
   {
      ss3delay = _Mmillis + ( long( Dcc.getCV( 210 ) * MasterTimeConstant ) ); //  Wait1
      ss3[  0 ] = 0;
      ss3[  1 ] = 1;
   }

   if ( ( ss3[  1 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 212 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 211 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 213 ) * MasterTimeConstant ) ); //  Wait2
      ss3[  1 ] = 0;
      ss3[  2 ] = 1;
   }

   if ( ( ss3[  2 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 215 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 214 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 216 ) * MasterTimeConstant ) ); //  Wait3
      ss3[  2 ] = 0;
      ss3[  3 ] = 1;
   }

   if ( ( ss3[  3 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 218 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 217 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 219 ) * MasterTimeConstant ) ); //  Wait4
      ss3[  3 ] = 0;
      ss3[  4 ] = 1;
   }

   if ( ( ss3[  4 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 221 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 220 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 222 ) * MasterTimeConstant ) ); //  Wait5
      ss3[  4 ] = 0;
      ss3[  5 ] = 1;
   }

   if ( ( ss3[  5 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 224 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 223 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss3[  5 ] = 0;

      if ( Dcc.getCV( 225 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss3[  0 ] = 1;
         run_switch_set[  3 ] =  true;
      }
      else
      {
         ss3[  6 ] = 1;
      }
   }

      //    ==========================   Small Switch Set  3 Continues
   if ( ss3[  6 ] == 1 )
   {
      ss3delay = _Mmillis + ( long( Dcc.getCV( 226 ) * MasterTimeConstant ) ); //  Wait1
      ss3[  6 ] = 0;
      ss3[  7 ] = 1;
   }

   if ( ( ss3[  7 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 228 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 227 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function 1
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 229 ) * MasterTimeConstant ) ); //  Wait2
      ss3[  7 ] = 0;
      ss3[  8 ] = 1;
   }

   if ( ( ss3[  8 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 231 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 230 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 232 ) * MasterTimeConstant ) ); //  Wait3
      ss3[  8 ] = 0;
      ss3[  9 ] = 1;
   }

   if ( ( ss3[  9 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 234 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 233 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  3
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 235 ) * MasterTimeConstant ) ); //  Wait4
      ss3[  9 ] = 0;
      ss3[ 10 ] = 1;
   }

   if ( ( ss3[ 10 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 237 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 236 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  4
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 238 ) * MasterTimeConstant ) ); //  Wait5
      ss3[ 10 ] = 0;
      ss3[ 11 ] = 1;
   }

   if ( ( ss3[ 11 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 240 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 239 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  5
      }
      ss3[ 11 ] = 0;

      if ( Dcc.getCV( 241 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss3[  0 ] = 1;
         run_switch_set[  3 ] =  true;
      }
      else
      {
         ss3[ 12 ] = 1;
      }
   }

      //    ==========================   Small Switch Set  3 Continues
   if ( ss3[ 12 ] == 1 )
   {
      ss3delay = _Mmillis + ( long( Dcc.getCV( 242 ) * MasterTimeConstant ) ); //  Wait1
      ss3[ 12 ] = 0;
      ss3[ 13 ] = 1;
   }

   if ( ( ss3[ 13 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 244 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 243 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  1
      }
      ss3delay = _Mmillis + ( long( Dcc.getCV( 245 ) * MasterTimeConstant ) ); //  Wait2
      ss3[ 13 ] = 0;
      ss3[ 14 ] = 1;
   }

   if ( ( ss3[ 14 ] == 1 ) && ( ss3delay <= _Mmillis ) )
   {
      ttemp = ( Dcc.getCV( 247 ) );
      if ( ttemp != 0 )
      {
         exec_switch_function( Dcc.getCV( 246 ), ttemp & 0x3F, ttemp >> 7 );   //  execute switch function  2
      }
      ss3[ 14 ] = 0;

      if ( Dcc.getCV( 248 ) != 0 )  //  Do we jump back to the beginning or move on...
      {
         ss3[  0 ] = 1;
         run_switch_set[  3 ] =  true;
      }
      else
      {
         run_switch_set[  3 ] = false;
      }
   }


   if ( ( MasterFunctionSwitch == 1 ) || ( function_value[ 1 ] == 1 ) )  //  0 = Blinking Yellow | 1 = Working Normally
   {
      if ( run_switch_set[  3 ] ==  true ) { return; }
      if ( run_switch_set[  1 ] ==  true ) { return; }

      ss1[ 0 ] = 1;
      run_switch_set[ 1 ] =  true;  //  Start Switch Set 1
   }
   else
   {
      if ( run_switch_set[  1 ] ==  true ) { return; }  //  Wait for Switch Set 1 to Stop
      if ( run_switch_set[  3 ] ==  true ) { return; }  //  Wait for Switch Set 3 to Stop

      for ( int i = 0; i < numfpins; ++i )
      {
         digitalWrite( fpins[ i ],  LOW );              //  Turn all LEDs OFF in sequence
      }

      ss3[ 0 ] = 1;
      run_switch_set[ 3 ] =  true;                      //  Start Switch Set 3 as Default
   }
}       // end  loop() 


/* ******************************************************************************* */


void exec_switch_function( byte switch_function, byte fpin, byte fbit )
{
   if ( MasterDecoderDisable == 1 ) return;

      // 0=NOP, 1=0/1, 2=RND, 3=WRND, 4=SND, 5=SRVO, 6=Dual Pin, 7/8=Next 
   switch ( switch_function )  //  Find the switch function to execute
   {
      case  0:    //  0 = No function
      case  2:    //  2 = Randow
      case  3:    //  3 = Weighted Randowm
      case  4:    //  4 = Sound
      case  5:    //  5 = Servo
      case  6:    //  6 = Dual Pin
      default:
      {
         break;
      }

      case  1:    //
      {
         digitalWrite( fpin, fbit );  //  Simple pin switch on/off
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

            case  2:    //   Start Switch Set  2
            {
               ss2[ 0 ] = 1;
               run_switch_set[ fpin ] = true;
               break;
            }

            case  3:    //    Start Switch Set  3
            {
               ss3[ 0 ] = 1;
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
            case  0:    //  No action required
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

            case  2:    //   Start Switch Set  2
            {
               if ( run_switch_set[ fpin ] == false )
               {
                  ss2[ 0 ] = 1;
                  run_switch_set[ fpin ] = true;
               }
               break;
            }

            case  3:    //    Start Switch Set  3
            {
               if ( run_switch_set[ fpin ] == false )
               {
                  ss3[ 0 ] = 1;
                  run_switch_set[ fpin ] = true;
               }
               break;
            }
         }
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
      case FN_0_4:    //  Function Group 1    F0 F4 F3 F2 F1
      {
         exec_function(  0, FunctionPin0, ( FuncState & FN_BIT_00 ) >> 4 );
         exec_function(  1, FunctionPin0, ( FuncState & FN_BIT_01 ) >> 0 );
         //ec_function(  2, FunctionPin0, ( FuncState & FN_BIT_02 ) >> 1 );
         exec_function(  3, FunctionPin0, ( FuncState & FN_BIT_03 ) >> 2 );
         //ec_function(  4, FunctionPin0, ( FuncState & FN_BIT_04 ) >> 3 );
         break ;
      }

      case FN_5_8:    //  Function Group 1    F8  F7  F6  F5
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
   #if defined( DEBUGCV )

      _PP( "exec_function = " );
      _2L( function,      DEC );
      _PP( "pin (n/u) = "     );
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
      case  3:  //   run switch set [ function ]
      {
         function_value[ function ] = byte( FuncState );
         break ;
      }

      case  2:  //   run switch set [ function ]
      case  4:  //   run switch set [ function ]
      case  5:  //   run switch set [ function ]
      case  6:  //   run switch set [ function ]
      case  7:  //   run switch set [ function ]
      case  8:  //   run switch set [ function ]
      case  9:  //   run switch set [ function ]
      case 10:  //   run switch set [ function ]
      default:
      {
         break ;
      }
   }
}                //  End exec_function()


/* ******************************************************************************* */


//
