
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
SOFTPWM_DEFINE_CHANNEL( 8, DDRC, PORTC, PORTC3);  //Arduino pin A3 --> LB4
SOFTPWM_DEFINE_CHANNEL( 9, DDRC, PORTC, PORTC4);  //Arduino pin A4 --> LB5
SOFTPWM_DEFINE_CHANNEL(10, DDRC, PORTC, PORTC5);  //Arduino pin A5 --> LB6
// TPWM_DEFINE_CHANNEL(11, DDRB, PORTB, PORTB0);  //Arduino pin D8 --> AX1
// TPWM_DEFINE_CHANNEL(12, DDRB, PORTB, PORTB1);  //Arduino pin D9 --> AX2
// TPWM_DEFINE_CHANNEL(13, DDRB, PORTB, PORTB2);  //Arduino pin 10 --> AX3
// TPWM_DEFINE_CHANNEL(14, DDRC, PORTC, PORTC2);  //Arduino pin A2 --> AX4
// TPWM_DEFINE_CHANNEL(15, DDRB, PORTB, PORTB5);  //Arduino pin 13 --> LED

SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS( 15, 100);  // Set 10 pulsed outputs


// ******** UNLESS YOU WANT ALL CV'S RESET UPON EVERY POWER UP REMOVE THE "//" IN THE FOLLOWING LINE!!
#define DECODER_LOADED


// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL MONITOR
#define _DEBUG_

#ifdef _DEBUG_
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


// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE YOUR SERIAL PORT FOR COMMANDS
#define _MONITOR_

#ifdef _MONITOR_
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


/* ******************************************************************************* */


extern int __heap_start, *__brkval;

#ifdef _MONITOR_

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
#define Accessory_Address    40  // THIS ADDRESS IS THE ADDRESS OF THIS DCC DECODER

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
QUEUE volatile *ftn_queue = new QUEUE[17];

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

   #if defined(_DEBUG_) || defined(_MONITOR_)

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

   // Send feedback if requested.
   _MP( "\t" "<" );
   _3P( cvvalue, DEC );
   _ML( ">" );
}


/* ******************************************************************************* */


void setFtnQueue( uint16_t cvNumber, uint8_t cvValue )
{

   _PP( "setFTNQueue: ");
   _2P( cvNumber,   DEC);
   _PP( "   Value: "   );
   _2L( cvValue,    DEC);

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

   // Print some values  -  if requested
   _PP( "\t" "calculateFtnQueue  cv: ");
   _2P( 30 + ( number * 5 ),       DEC);
   _PP( "\t"                " value: ");
   _2L( ftn_queue[ number ].inUse, DEC);

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


//
