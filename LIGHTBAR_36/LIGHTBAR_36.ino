
/*
   TODO: check for maximum when changing CVs 31 ... 34
   TODO: implement change of direction --> forward / backward
*/

/* **********************************************************************************
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************** */

#include <Arduino.h> // Needed for C++ conversion of INOfile.

#include <avr/wdt.h> // Needed for automatic reset functions.


/* ******************************************************************************* */

//  The next line is there to prevent large in-rush currents during PWM cycly.
#define SOFTPWM_OUTPUT_DELAY

#include <SoftPWM.h>

SOFTPWM_DEFINE_CHANNEL( 1, DDRD, PORTD, PORTD7);  //Arduino pin D7 --> LS1
SOFTPWM_DEFINE_CHANNEL( 2, DDRC, PORTC, PORTC1);  //Arduino pin A1 --> LS2
SOFTPWM_DEFINE_CHANNEL( 3, DDRD, PORTD, PORTD5);  //Arduino pin D5 --> LS3
SOFTPWM_DEFINE_CHANNEL( 4, DDRD, PORTD, PORTD3);  //Arduino pin D3 --> LS4
SOFTPWM_DEFINE_CHANNEL( 5, DDRD, PORTD, PORTD6);  //Arduino pin D6 --> LB1
SOFTPWM_DEFINE_CHANNEL( 6, DDRC, PORTC, PORTC0);  //Arduino pin A0 --> LB2
SOFTPWM_DEFINE_CHANNEL( 7, DDRD, PORTD, PORTD4);  //Arduino pin D4 --> LB3

#if defined( LAMPS36 )

   SOFTPWM_DEFINE_CHANNEL( 8, DDRC, PORTC, PORTC3);  //Arduino pin A3 --> LB4
   SOFTPWM_DEFINE_CHANNEL( 9, DDRC, PORTC, PORTC4);  //Arduino pin A4 --> LB5
   SOFTPWM_DEFINE_CHANNEL(10, DDRC, PORTC, PORTC5);  //Arduino pin A5 --> LB6
   SOFTPWM_DEFINE_CHANNEL(11, DDRB, PORTB, PORTB0);  //Arduino pin D8 --> AX1
   SOFTPWM_DEFINE_CHANNEL(12, DDRB, PORTB, PORTB1);  //Arduino pin D9 --> AX2
   SOFTPWM_DEFINE_CHANNEL(13, DDRB, PORTB, PORTB2);  //Arduino pin 10 --> AX3
   SOFTPWM_DEFINE_CHANNEL(14, DDRC, PORTC, PORTC2);  //Arduino pin A2 --> AX4

#endif

SOFTPWM_DEFINE_CHANNEL(15, DDRB, PORTB, PORTB5);  //Arduino pin 13 --> LED

SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS( 15, 100);  // Set 15 pulsed outputs


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ***** Uncomment to use the PinChangeInterrupts iso External Interrupts *****
// #define PIN_CHANGE_INT

#include <NmraDcc.h>

NmraDcc     Dcc ;
DCC_MSG  Packet ;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** UNLESS YOU WANT ALL CV'S RESET UPON EVERY POWER UP REMOVE THE "//" IN THE FOLLOWING LINE!!
#define DECODER_LOADED

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL OUTPUT
// #define DEBUG36

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE YOUR SERIAL PORT FOR COMMANDS
// #define MONITOR

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO USE AS A '6LAMP PCB WITH AUX PORTS' ELSE IT'S A '3LAMP W/O AUX PORTS'
// #define LAMPS36


#ifdef DEBUG36
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


#ifdef MONITOR
   #define _MP( a ) Serial.print(    a );
   #define _ML( a ) Serial.println(  a );
   #define _3P(a,b) Serial.print(  a, b);
   #define _3L(a,b) Serial.println(a, b);
#else
   #define _MP( a )
   #define _ML( a )
   #define _3P(a,b)
   #define _3L(a,b)
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern int __heap_start, *__brkval;

#ifdef MONITOR

   char bomMarker        =   '<';   // Begin of message marker.
   char eomMarker        =   '>';   // End   of message marker.
   char commandString[ 32]      ;   // Max length for a buffer.
   char sprintfBuffer[ 32]      ;   // Max length for a buffer.
   boolean foundBom      = false;   // Found begin of messages.
   boolean foundEom      = false;   // Founf  end  of messages.

#endif


/* ******************************************************************************* */


int tim_delay =  500;
int numfpins  =   17;
byte fpins [] = { 11,   7,  15,   5,   3,   6,  14,   4,  17,  18,  19,   8,   9,  10,  16,  13,  12};
//  pinnames:  > DNU, PD7, PC1, PD5, PD3, PD6, PC0, PD4, PC3, PC4, PC5, PB0, PB1, PB2, PC2, PB5, DNU <

const int FunctionPinRx  =  0;  // PD0  Tx0
const int FunctionPinTx  =  1;  // PD1  Rx1
const int FunctionPinDcc =  2;  // PD2  DCC

const int FunctionPin0   =  3;  // PD3  LS4
const int FunctionPin1   =  4;  // PD4  LB3
const int FunctionPin2   =  5;  // PD5  LS3
const int FunctionPin3   =  6;  // PD6  LB1
const int FunctionPin4   =  7;  // PD7  LS1
const int FunctionPin5   =  8;  // PB0  AX1
const int FunctionPin6   =  9;  // PB1  AX2
const int FunctionPin7   = 10;  // PB2  AX3
const int FunctionPin8   = 11;  // PB3  DNU
const int FunctionPin9   = 12;  // PB4  DNU
const int FunctionPinLed = 13;  // PB5  LED
const int FunctionPin10  = 14;  // PC0  LB2
const int FunctionPin11  = 15;  // PC1  LS2
const int FunctionPin12  = 16;  // PC2  AX4
const int FunctionPin13  = 17;  // PC3  LB4
const int FunctionPin14  = 18;  // PC4  LB5
const int FunctionPin15  = 19;  // PC5  LB6

//    AD0 = LB2    PD0 = Rx1    PB0 = AX1
//    AD1 = LS2    PD1 = Tx0    PB1 = AX2
//    AD2 = AX4    PD2 = DCC    PB2 = AX3
//    AD3 = LB4    PD3 = LS4    PB3 = ICSP
//    AD4 = LB5    PD4 = LB3    PB4 = ICSP
//    AD5 = LB6    PD5 = LS3    PB5 = ICSP
//    AD6 = DNU    PD6 = LB1    PB6 = Xtal1
//    AD7 = DNU    PD7 = LS1    PB7 = Xtal2


/* ******************************************************************************* */


#define SET_CV_Address       24  // THIS ADDRESS IS FOR SETTING CV'S  (LIKE A LOCO)
#define Accessory_Address   140  // THIS ADDRESS IS THE ADDRESS OF THIS DCC DECODER

uint8_t CV_DECODER_MASTER_RESET  = 120; // THIS IS THE CV ADDRESS OF THE FULL RESET
#define CV_To_Store_SET_CV_Address 121
#define CV_Accessory_Address       CV_ACCESSORY_DECODER_ADDRESS_LSB

struct QUEUE
{
   int                        inUse;   // output in use or not
   unsigned long     previousMicros;   // last time for fading
   unsigned long         waitMicros;   // interval in micros()
   int                  fadeCounter;   // count of fadinglevel
   int                     maxLevel;   // maximum output level
   bool                      upDown;   // travel direction led
   unsigned long     previousMillis;   // last time we blinked
   unsigned long      blinkInterval;   // interval in millis()
   int                     ledState;   // state of this output
   int                previousState;   // previous outputstate
};
QUEUE volatile *ftn_queue = new QUEUE[ 17 ];

struct CVPair
{
   uint16_t     CV;
   uint8_t   Value;
};

CVPair FactoryDefaultCVs [] =
{
   // These two CVs define the Long Accessory Address
   {CV_ACCESSORY_DECODER_ADDRESS_LSB,  Accessory_Address       & 0xFF},
   {CV_ACCESSORY_DECODER_ADDRESS_MSB, (Accessory_Address >> 8) & 0x07},

   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},
   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 0},

//  Speed Steps don't matter for this decoder, only for loc decoders
//  ONLY uncomment 1 CV_29_CONFIG line below as approprate DEFAULT IS SHORT ADDRESS
// {CV_29_CONFIG,                0},                                                                           // Short Address 14 Speed Steps
   {CV_29_CONFIG, CV29_F0_LOCATION},                                                                           // Short Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_EXT_ADDRESSING    | CV29_F0_LOCATION},                                                  // Long  Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_F0_LOCATION},                       // Accesory Decoder Short Address
// {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_EXT_ADDRESSING | CV29_F0_LOCATION}, // Accesory Decoder  Long Address

   {CV_DECODER_MASTER_RESET, 0},

   {CV_To_Store_SET_CV_Address,      SET_CV_Address       & 0xFF },  // LSB Set CV Address
   {CV_To_Store_SET_CV_Address + 1, (SET_CV_Address >> 8) & 0x3F },  // MSB Set CV Address

   { 30,   0}, // GEN 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 31,  50}, //     Maximum level outputs (100 max)
   { 32, 100}, //     Waitmicros  (250 * 100,000 max)
   { 33,   5}, //     Waitmicros divider (standard 5)
   { 34, 100}, //     Standard blink interval  (* 10)
   { 35,   0}, // LS1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 36,  50}, //     Maximum level this output
   { 37, 100}, //     Waitmicros output highend
   { 38,   5}, //     Waitmicros output divider
   { 39, 100}, //     Blinkinterval this output
   { 40,   0}, // LS2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 41,  50}, //     Maximum level this output
   { 42, 100}, //     Waitmicros output highend
   { 43,   5}, //     Waitmicros output divider
   { 44, 100}, //     Blinkinterval this output
   { 45,   0}, // LS3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 46,  50}, //     Maximum level this output
   { 47, 100}, //     Waitmicros output highend
   { 48,   5}, //     Waitmicros output divider
   { 49, 100}, //     Blinkinterval this output
   { 50,   0}, // LS4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 51,  50}, //     Maximum level this output
   { 52, 100}, //     Waitmicros output highend
   { 53,   5}, //     Waitmicros output divider
   { 54, 100}, //     Blinkinterval this output
   { 55,   0}, // LB1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 56,  50}, //     Maximum level this output
   { 57, 100}, //     Waitmicros output highend
   { 58,   5}, //     Waitmicros output divider
   { 59, 100}, //     Blinkinterval this output
   { 60,   0}, // LB2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 61,  50}, //     Maximum level this output
   { 62, 100}, //     Waitmicros output highend
   { 63,   5}, //     Waitmicros output divider
   { 64, 100}, //     Blinkinterval this output
   { 65,   0}, // LB3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 66,  50}, //     Maximum level this output
   { 67, 100}, //     Waitmicros output highend
   { 68,   5}, //     Waitmicros output divider
   { 69, 100}, //     Blinkinterval this output

#if defined( LAMPS36 )

   { 70,   0}, // LB4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 71,  50}, //     Maximum level this output
   { 72, 100}, //     Waitmicros output highend
   { 73,   5}, //     Waitmicros output divider
   { 74, 100}, //     Blinkinterval this output
   { 75,   0}, // LB5 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 76,  50}, //     Maximum level this output
   { 77, 100}, //     Waitmicros output highend
   { 78,   5}, //     Waitmicros output divider
   { 79, 100}, //     Blinkinterval this output
   { 80,   0}, // LB6 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 81,  50}, //     Maximum level this output
   { 82, 100}, //     Waitmicros output highend
   { 83,   5}, //     Waitmicros output divider
   { 84, 100}, //     Blinkinterval this output
   { 85,   0}, // AX1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 86,   0}, //     Maximum level this output
   { 87,   0}, //     Waitmicros output highend
   { 88,   0}, //     Waitmicros output divider
   { 89,   0}, //     Blinkinterval this output
   { 90,   0}, // AX2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 91,   0}, //     Maximum level this output
   { 92,   0}, //     Waitmicros output highend
   { 93,   0}, //     Waitmicros output divider
   { 94,   0}, //     Blinkinterval this output
   { 95,   0}, // AX3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 96,   0}, //     Maximum level this output
   { 97,   0}, //     Waitmicros output highend
   { 98,   0}, //     Waitmicros output divider
   { 99,   0}, //     Blinkinterval this output
   {100,   0}, // AX4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   {101,   0}, //     Maximum level this output
   {102,   0}, //     Waitmicros output highend
   {103,   0}, //     Waitmicros output divider
   {104,   0}, //     Blinkinterval this output

#endif // LAMPS36

   {105,   1}, // LED 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   {106,  50}, //     Maximum level this output
   {107, 100}, //     Waitmicros output highend
   {108,   5}, //     Waitmicros output divider
   {109, 100}, //     Blinkinterval this output
   {110,   0}, // DNU 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   {111,   0}, //     Maximum level this output
   {112,   0}, //     Waitmicros output highend
   {113,   0}, //     Waitmicros output divider
   {114,   0}, //     Blinkinterval this output
};

volatile uint8_t FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

// These are the absolute maximums allowed !!!
volatile uint8_t M_maxLevel = Dcc.getCV( 31 );
volatile uint8_t M_highend  = Dcc.getCV( 32 );
volatile uint8_t M_divider  = Dcc.getCV( 33 );
volatile uint8_t M_interval = Dcc.getCV( 34 );

// This is the switch between F0  or  F1 - F14
volatile uint8_t C_between  = Dcc.getCV(111 );


/* ******************************************************************************* */


void setup()
{
   noInterrupts();

   // initialize the digital pins as outputs
   for (int i = 1; i < numfpins - 1; i++) 
   {
      digitalWrite( fpins[ i ], 0 );  // Switch the pin off first.
      pinMode( fpins[ i ], OUTPUT );  // Then set it as an output.
   }

   // Start SoftPWM with 60hz pwm frequency
   Palatis::SoftPWM.begin( 60);

   #if defined(DEBUG36) || defined(MONITOR)

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

      Serial.println("-------------------------------------");
      Serial.println(""); // An extra empty line for clearness

      // SoftPWM  print interrupt load for diagnostic purposes
      Palatis::SoftPWM.printInterruptLoad();
      Serial.println(""); // An extra empty line for clearness

   #endif

   Dcc.pin( digitalPinToInterrupt( FunctionPinDcc ), FunctionPinDcc, false );

   Dcc.init( MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, CV_To_Store_SET_CV_Address );

   #if defined(DECODER_LOADED)

      if ( Dcc.getCV( CV_DECODER_MASTER_RESET ) == CV_DECODER_MASTER_RESET ) 
      {
      #endif

         FactoryDefaultCVIndex =  sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

         for (int i = 0; i < FactoryDefaultCVIndex; i++)
         {
            Dcc.setCV( FactoryDefaultCVs[ i ].CV, FactoryDefaultCVs[ i ].Value );

            digitalWrite( FunctionPinLed, 1 );
            delay ( tim_delay );
            digitalWrite( FunctionPinLed, 0 );
         }

      #if defined(DECODER_LOADED)
      }
   #endif

   // These are the absolute maximums allowed
   M_maxLevel = Dcc.getCV( 31 ); //     Maximum level outputs (100 max)
   M_highend  = Dcc.getCV( 32 ); //     Waitmicros  (250 * 100,000 max)
   M_divider  = Dcc.getCV( 33 ); //     Waitmicros divider (standard 5)
   M_interval = Dcc.getCV( 34 ); //     Standard blink interval  (* 10)

   // This is the switch between F0  or  F1 - F14
   C_between  = Dcc.getCV( 111 );

   // Loop through all the settings for checking and correcting
   for (int i = 1; i < numfpins - 1; i++)
   {
      calculateFtnQueue( i );
   }

   #ifdef MONITOR
      displayText(); // Shows the standard text.
   #endif

   // Switch on the LED to signal we're ready.
   digitalWrite(FunctionPinLed, 0);
   delay (tim_delay);
   digitalWrite(FunctionPinLed, 1);
   delay (tim_delay);
   digitalWrite(FunctionPinLed, 0);
   delay (tim_delay);
   digitalWrite(FunctionPinLed, 1);

   interrupts();  // Ready to rumble....
}


/* ******************************************************************************* */


void loop()
{
   unsigned long currentMillis = millis();
   unsigned long currentMicros = micros();

   Dcc.process(); // Loop through the DCC process.

// Check the number of outputs, not the number of numfpins!!!
   for (unsigned int i = 1; i < Palatis::SoftPWM.size(); ++i)
   {
      switch ( ftn_queue[ i ].inUse )
      {
         case 1:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         {
            Palatis::SoftPWM.set( i, ftn_queue[ i ].maxLevel );
            break;
         }

         case 3:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         { 
            if (currentMillis - ftn_queue[ i ].previousMillis >= ftn_queue[ i ].blinkInterval)
            {
               // Save the last time you blinked the LED
               ftn_queue[ i ].previousMillis = currentMillis;

               // If the LED is off turn it on and vice-versa
               if (ftn_queue[ i ].ledState == LOW)
               {
                  Palatis::SoftPWM.set( i, ftn_queue[ i ].maxLevel);
                  ftn_queue[ i ].ledState = HIGH;
               }
               else
               {
                  ftn_queue[ i ].ledState =  LOW;
                  Palatis::SoftPWM.set( i,    0);
               }
            }
            break;
         }

         case 5:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         { 
            if (currentMicros - ftn_queue[ i ].previousMicros > ftn_queue[ i ].waitMicros)
            {
               ftn_queue[ i ].previousMicros = currentMicros;

               if ((ftn_queue[ i ].fadeCounter > ftn_queue[ i ].maxLevel - 1) || (ftn_queue[ i ].fadeCounter < 1))
               {
                  ftn_queue[ i ].upDown = !ftn_queue[ i ].upDown;
               }

               if (ftn_queue[ i ].upDown == true) { ftn_queue[ i ].fadeCounter++; } else { ftn_queue[ i ].fadeCounter--; }

               Palatis::SoftPWM.set( i, ftn_queue[ i ].fadeCounter);
            }
            break;
         }

         default:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         {
            Palatis::SoftPWM.set( i,   0);
            break;
         }
      }
   }

   #ifdef MONITOR

      if (foundEom)
      {
         foundBom = false;
         foundEom = false;
         parseCom( commandString );
      }

   #endif
}


/* ******************************************************************************* */


void softwareReset( uint8_t preScaler )
{
   wdt_enable( preScaler );

   while( 1 ) {}  // Wait for the prescaler time to expire and do an auto-reset
}


/* ******************************************************************************* */


void setAllOutputCVs( uint8_t cvvalue )
{
   noInterrupts();   // Disable interrupts.

   for (int i = 1; i < numfpins - 2; i++) 
   {
      Dcc.setCV( 30 + ( i * 5 ), cvvalue );
      ftn_queue[ i ].inUse = cvvalue;
   }

   interrupts();  // Enable the interrupts.

   #if defined(DEBUG36) || defined(MONITOR)

      // Send feedback if requested.
      _MP( "\t" "<" );
      _3P( cvvalue, DEC );
      _ML( ">" );

   #endif
}


/* ******************************************************************************* */


void setFtnQueue( uint16_t cvNumber, uint8_t cvValue )
{

   #if defined(DEBUG36) || defined(MONITOR)

      _PP( "setFTNQueue: ");
      _2P( cvNumber,   DEC);
      _PP( "   Value: "   );
      _2L( cvValue,    DEC);

   #endif

   switch ( cvNumber )
   {
      case  30:
      {
         char recData[ 4 ] = " 0 ";

         recData[ 1 ] = 48 + cvValue;  // Make it an ASCII code
         parseCom( recData ); //   Recursive action on CV value

         break;
      }

      case  31 ...  34:
      {
         // These are the absolute maximums allowed
         M_maxLevel = Dcc.getCV( 31 ); //     Maximum level outputs (100 max)
         M_highend  = Dcc.getCV( 32 ); //     Waitmicros  (250 * 100,000 max)
         M_divider  = Dcc.getCV( 33 ); //     Waitmicros divider (standard 5)
         M_interval = Dcc.getCV( 34 ); //     Standard blink interval  (* 10)

         // Loop through all the settings for checking, correcting the values
         for (int i = 1; i < numfpins - 1; i++)
         {
            calculateFtnQueue( i );
         }
         break;
      }

      case  35 ... 104:
      {
         calculateFtnQueue( ( cvNumber - 30 ) / 5 );   // Group of 5 values

         break;
      }

      case 105 ... 109:
      {
         break;
      }

      case 110 ... 114:
      {
         // This is the switch between F0  or  F1 - F14
         C_between  = Dcc.getCV( 111 );

         break;
      }

      default:
      {
         break;
      }
   }
}


/* ******************************************************************************* */


void calculateFtnQueue( int number )
{
   // Now we're going to do some calculations
   ftn_queue[ number ].inUse = Dcc.getCV( 30 + ( number * 5 ) );

   #if defined(DEBUG36) || defined(MONITOR)

      // Print some values  -  if requested
      _PP( "\t" "calculateFtnQueue  cv: ");
      _2P( 30 + ( number * 5 ),       DEC);
      _PP( "\t"                " value: ");
      _2L( ftn_queue[ number ].inUse, DEC);

   #endif

   // Default settings for counters and states
   ftn_queue[ number ].fadeCounter    =     0;
   ftn_queue[ number ].ledState       =   LOW;
   ftn_queue[ number ].previousMicros =     0;
   ftn_queue[ number ].previousMillis =     0;
   ftn_queue[ number ].upDown         = false;
   ftn_queue[ number ].waitMicros     =     0;

   // These values stored in the Configuration Variables
   uint8_t S_maxLevel = Dcc.getCV( 31 + ( number * 5 ));
   uint8_t S_highend  = Dcc.getCV( 32 + ( number * 5 ));
   uint8_t S_divider  = Dcc.getCV( 33 + ( number * 5 ));
   uint8_t S_interval = Dcc.getCV( 34 + ( number * 5 ));

   if (M_maxLevel > S_maxLevel)
   {
      ftn_queue[ number ].maxLevel = S_maxLevel;
   }
   else
   {
      ftn_queue[ number ].maxLevel = M_maxLevel;
   }

   if (M_interval > S_interval)
   {
      ftn_queue[ number ].blinkInterval = S_interval * 10;
   }
   else
   {
      ftn_queue[ number ].blinkInterval = M_interval * 10;
   }

   if (M_highend > S_highend)
   {
      ftn_queue[ number ].waitMicros = ( (S_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
   }
   else
   {
      ftn_queue[ number ].waitMicros = ( (M_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
   }

   if (M_divider > S_divider)
   {
      ftn_queue[ number ].waitMicros = ( ftn_queue[ number ].waitMicros / S_divider );
   }
   else
   {
      ftn_queue[ number ].waitMicros = ( ftn_queue[ number ].waitMicros / M_divider );
   }
}


/* ******************************************************************************* */
// The next part will / can only be used for an mpu with enough memory

#if defined(MONITOR)


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

   <f>   controls mobile engine decoder functions F0-F12: <f x y>
   <F>   lists all funtions and settings for all outputs: <F>

   <M>   list the available SRAM on the chip

   <R>   reads a configuration variable: <R x>
   <W>   write a configuration variable: <W x y>

*/
void displayText()
{
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("Put in one of the following commands: "          ));
   Serial.println(F("------------------------------------------------"));
   Serial.println(F("<0>   all outputs: Power Off"                    ));
   Serial.println(F("<1>   all outputs: Power  On"                    ));
   Serial.println(F("<2>   all outputs: Blink Off"                    ));
   Serial.println(F("<3>   all outputs: Blink  On"                    ));
   Serial.println(F("<4>   all outputs: Fader Off"                    ));
   Serial.println(F("<5>   all outputs: Fader  On"                    ));
   Serial.println("");
   Serial.println(F("<C>   clear everything: Factory Default"         ));
   Serial.println(F("<D>   prints CV values: to your monitor"         ));
   Serial.println("");
   Serial.println(F("<f>   control decoder functions F0-F14: <f x y> "));
   Serial.println(F("<F>   dump the functionqueue and settings: <F>  "));
   Serial.println("");
   Serial.println(F("<M>   list the available SRAM on the chip"       ));
   Serial.println("");
   Serial.println(F("<R>   reads a configuration variable: <R x>"     ));
   Serial.println(F("<W>   write a configuration variable: <W x y>"   ));
   Serial.println(F("----------------------------------------------- "));
   Serial.println(F("* include '<', '>' and spaces in your command * "));
   Serial.println(""); // An extra empty line for clearness
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
         sprintf(commandString, " ");
         foundBom = true;
      }
      else if (inChar == eomMarker)    // end of new command
      {
      // sprintf(commandString, " ");
         foundEom = true;
         // parse(commandString) in the loop
      }
      else if (strlen(commandString) <  64) // put it all in
      {
         sprintf(commandString, "%s%c", commandString,  inChar);
         // if comandString still has space, append character just read from serial line
         // otherwise, character is ignored, (but we'll continue to look for '<' or '>')
      }
   }
}

#endif   // MONITOR


/* ******************************************************************************* */


void parseCom( char *com )
{
   switch ( com[1] )  // com[0] = '<' or ' '
   {

/*****  SWITCH ALL THE OUTPUTS AT ONCE  ****/

      case '0':     // <0>   all outputs: Off
/*
 *    returns: <0>
 */
      {
         setAllOutputCVs( 0 );
         break;
      }


      case '1':     // <1>   all outputs: On
/*
 *    returns: <1>
 */
      {
         setAllOutputCVs( 1 );
         break;
      }


      case '2':     // <2>   all outputs: Blink Off
/*
 *    returns: <2>
 */
      {
         setAllOutputCVs( 2 );
         break;
      }


      case '3':     // <3>   all outputs: Blink On
/*
 *    returns: <3>
 */
      {
         setAllOutputCVs( 3 );
         break;
      }


      case '4':     // <4>   all outputs: Fade Off
/*
 *    returns: <4>
 */
      {
         setAllOutputCVs( 4 );
         break;
      }


      case '5':     // <5>   all outputs: Fade On
/*
 *    returns: <5>
 */
      {
         setAllOutputCVs( 5 );
         break;
      }

      #if defined(DEBUG36) || defined(MONITOR)

/****  CLEAR SETTINGS TO FACTORY DEFAULTS  ****/

      case 'C':     // <C>
/*
 *    clears settings to Factory Defaults
 *
 *    returns: <FD done check>
 */
      {
         uint8_t cvCheck = Dcc.setCV( 120, 120 );

         #if defined(DEBUG36) || defined(MONITOR)

            _MP( "\t"  "<FD done>"  "\t" );
            _3L( cvCheck, DEC );

         #endif

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

            #if defined(DEBUG36) || defined(MONITOR)

               _MP( " cv: "                      );
               _3P( FactoryDefaultCVs[i].CV, DEC );
               _MP( "\t"              " value: " );
               _3L( cvValue, DEC                 );

            #endif

         }
         break;
      }


/***** DUMPS ALL FTNQUEUE SETTINGS FOR ALL OUTPUTS ****/

      case 'F':     // <F>
/*
 *    dumps all FTNqueue settings from memory
 *
 *    returns a list of: <settings>
 */
      {
         #if defined(DEBUG36) || defined(MONITOR)

            _ML("\n\npin  iU     prevMic     waitMic  fC  mL  uD     prevMil     blnkInt  lS  pS \n");
            for (int i = 1; i < numfpins - 2; i++)
            {
               sprintf( sprintfBuffer, " %2i %3i %11lu"  , i, ftn_queue[ i ].inUse, ftn_queue[ i ].previousMicros );
               _MP( sprintfBuffer );
               sprintf( sprintfBuffer, " %11lu %3i %3i"  , ftn_queue[ i ].waitMicros, ftn_queue[ i ].fadeCounter, ftn_queue[ i ].maxLevel );
               _MP( sprintfBuffer );
               sprintf( sprintfBuffer, " %3i %11lu %11lu", ftn_queue[ i ].upDown, ftn_queue[ i ].previousMillis, ftn_queue[ i ].blinkInterval );
               _MP( sprintfBuffer );
               sprintf( sprintfBuffer, " %3i %3i "       , ftn_queue[ i ].ledState, ftn_queue[ i ].previousState );
               _ML( sprintfBuffer );
            }
            _ML( "\n" );

         #endif

         break;
      }


/***** DUMPS CONFIGURATION VARIABLES FROM DECODER ****/

      case 'f':     // <f>
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

            #if defined(DEBUG36) || defined(MONITOR)

               _MP( " cv: "                      );
               _3P( FactoryDefaultCVs[i].CV, DEC );
               _MP( "\t"              " value: " );
               _3L( cvValue, DEC                 );

            #endif

         }
         break;
      }


/****  ATTEMPTS TO DETERMINE HOW MUCH FREE SRAM IS AVAILABLE IN ARDUINO  ****/

      case 'M':     // <M>
/*
 *    measure amount of free SRAM memory left on the Arduino based on trick found on the internet.
 *    Useful when setting dynamic array sizes, considering the Uno only has 2048 bytes of dynamic SRAM.
 *    Unfortunately not very reliable --- would be great to find a better method
 *
 *    returns: <m MEM>
 *    where MEM is the number of free bytes remaining in the Arduino's SRAM
 */
      {
         int v; 
         _MP( "<m ");
         _MP( (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval) );
         _ML( ">"  );
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
         int  cv;

         if( sscanf( com + 2, "%d", &cv ) != 1 ) { return; }

         uint8_t cvCheck = Dcc.getCV( cv );

         #if defined(DEBUG36) || defined(MONITOR)

            _MP( "<r " );
            _3P( cv + 0 , DEC );
            _MP( " "   );
            _3P( cvCheck, DEC );
            _ML( ">"   );

         #endif

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

         uint8_t cvCheck = Dcc.setCV( cv, bValue );

         #if defined(DEBUG36) || defined(MONITOR)

            _MP( "<w " );
            _3P( cv + 0 , DEC );
            _MP( " "   );
            _3P( cvCheck, DEC );
            _ML( ">"   );

         #endif

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
         _ML("");

         #ifdef MONITOR
            displayText(); // Shows the standard explanation text
         #endif

         break;
      }

      #endif

/****  DEFAULT FOR THE SWITCH FUNCTION = NO ACTION  ****/

      default:
         break;

   }
}


/************************************************************************************
            Call-back functions
************************************************************************************/

#if defined (__cplusplus)
	extern "C" {
#endif


//  Some functions below are left, which might be needed in some future expansions.


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
 */   /*
void    notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION Dir, DCC_SPEED_STEPS SpeedSteps )
{
   _PL("notifyDccSpeed");
}
   */

/* **********************************************************************************
 *  notifyDccReset(uint8_t hardReset) Callback for a DCC reset command.
 *
 *  Inputs:
 *    hardReset - 0 normal reset command.
 *                1 hard reset command.
 *
 *  Returns:
 *    None
 */   /*
void    notifyDccReset(uint8_t hardReset )
{
   _PL( "notifyDccReset" );
}
   */

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
void    notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState ) 
{
   static bool B_between = ( FuncState & FN_BIT_00 ? true : false ); // Keeps track of F0 function.

   if ( C_between == 0 )   //  Use the F0 function to switch lights.
   {
      if ( B_between != ( FuncState & FN_BIT_00 ? true : false ) && ( FuncGrp == FN_0_4 ) )
      {
         B_between = ( FuncState & FN_BIT_00 ? true : false );

         Dcc.setCV( 30, FuncState & FN_BIT_00 ? 1 : 0 );
      }
   }
   else
   {
      switch( FuncGrp )
      {
         case FN_0_4:    //Function Group 1 F0 F4 F3 F2 F1
         {
            Dcc.setCV( 35, FuncState & FN_BIT_01 ? 1 : 0 );
            Dcc.setCV( 40, FuncState & FN_BIT_02 ? 1 : 0 );
            Dcc.setCV( 45, FuncState & FN_BIT_03 ? 1 : 0 );
            Dcc.setCV( 50, FuncState & FN_BIT_04 ? 1 : 0 );
            break;
         }

         case FN_5_8:    //Function Group 1    F8 F7 F6 F5
         {
            Dcc.setCV( 55, FuncState & FN_BIT_05 ? 1 : 0 );
            Dcc.setCV( 60, FuncState & FN_BIT_06 ? 1 : 0 );
            Dcc.setCV( 65, FuncState & FN_BIT_07 ? 1 : 0 );
            Dcc.setCV( 70, FuncState & FN_BIT_08 ? 1 : 0 );
            break;
         }

         case FN_9_12:   //Function Group 1 F12 F11 F10 F9
         {
            Dcc.setCV( 75, FuncState & FN_BIT_09 ? 1 : 0 );
            Dcc.setCV( 80, FuncState & FN_BIT_10 ? 1 : 0 );
            Dcc.setCV( 85, FuncState & FN_BIT_11 ? 1 : 0 );
            Dcc.setCV( 90, FuncState & FN_BIT_12 ? 1 : 0 );
            break;
         }

         case FN_13_20:  //Function Group 2  ==  F20 - F13
         {
            Dcc.setCV( 95, FuncState & FN_BIT_13 ? 1 : 0 );
            Dcc.setCV(100, FuncState & FN_BIT_14 ? 1 : 0 );
            break;
         }

         case FN_21_28:
         {
            break;
         }
      }
   }
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

   #if defined(DEBUG36) || defined(MONITOR)

      _PP( " notifyCVChange: CV: ");
      _2P( CV, DEC    );
      _PP( " Value: " );
      _2L( Value, DEC );

   #endif

   setFtnQueue( CV, Value );
}

// void    notifyDccCVChange( uint16_t CV, uint8_t Value )
// {
//    _PP( " notifyDccCVChange: CV: ");
//    _2P( CV,     DEC);
//    _PP( " Value: " );
//    _2L( Value,  DEC);

//    setFtnQueue( CV, Value );
// }


/* **********************************************************************************
 *  notifyCVResetFactoryDefault() Called when CVs must be reset.
 *                                This is called when CVs must be reset
 *                                to their factory defaults. This callback
 *                                should write the factory default value of
 *                                relevent CVs using the setCV() method.
 *                                setCV() must not block whens this is called.
 *                                Test with isSetCVReady() prior to calling setCV()
 *
 *  Inputs:
 *    None
 *
 *  Returns:
 *    None
 */
void    notifyCVResetFactoryDefault()
{
   // Make 'FactoryDefaultCVIndex' non-zero and equal to num CVs to be reset 
   // to flag to the loop() function that a reset to FacDef needs to be done.
   FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
}


#if defined (__cplusplus)
}
#endif

//
