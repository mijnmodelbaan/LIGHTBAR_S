
/*
   TODO: check for maximum when changing CVs 31 ... 34
   TODO: check 'notifyDccCVChange' and '<W>' command: they use the same functionality
   TODO:
*/


#include <avr/wdt.h> // Needed for automatic reset functions.

//  The next line is there to prevent large in-rush currents.
#define SOFTPWM_OUTPUT_DELAY

#include <SoftPWM.h>

SOFTPWM_DEFINE_CHANNEL( 0, DDRD, PORTD, PORTD7);  //Arduino pin D7
SOFTPWM_DEFINE_CHANNEL( 1, DDRC, PORTC, PORTC1);  //Arduino pin A1
SOFTPWM_DEFINE_CHANNEL( 2, DDRD, PORTD, PORTD5);  //Arduino pin D5
SOFTPWM_DEFINE_CHANNEL( 3, DDRD, PORTD, PORTD3);  //Arduino pin D3
SOFTPWM_DEFINE_CHANNEL( 4, DDRD, PORTD, PORTD6);  //Arduino pin D6
SOFTPWM_DEFINE_CHANNEL( 5, DDRC, PORTC, PORTC0);  //Arduino pin A0
SOFTPWM_DEFINE_CHANNEL( 6, DDRD, PORTD, PORTD4);  //Arduino pin D4
SOFTPWM_DEFINE_CHANNEL( 7, DDRC, PORTC, PORTC3);  //Arduino pin A3
SOFTPWM_DEFINE_CHANNEL( 8, DDRC, PORTC, PORTC4);  //Arduino pin A4
SOFTPWM_DEFINE_CHANNEL( 9, DDRC, PORTC, PORTC5);  //Arduino pin A5
SOFTPWM_DEFINE_CHANNEL(10, DDRB, PORTB, PORTB0);  //Arduino pin D8
SOFTPWM_DEFINE_CHANNEL(11, DDRB, PORTB, PORTB1);  //Arduino pin D9
SOFTPWM_DEFINE_CHANNEL(12, DDRB, PORTB, PORTB2);  //Arduino pin 10
SOFTPWM_DEFINE_CHANNEL(13, DDRC, PORTC, PORTC2);  //Arduino pin A2
SOFTPWM_DEFINE_CHANNEL(14, DDRB, PORTB, PORTB5);  //Arduino pin 13

SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS( 15, 100);


/* ******************************************************************************* */


extern int __heap_start, *__brkval;

char bomMarker        =   '<';   // Begin of message marker.
char eomMarker        =   '>';   // End   of message marker.
char commandString[ 64]      ;   // Max length for a buffer.
boolean foundBom      = false;   // Found begin of messages.
boolean foundEom      = false;   // Founf  end  of messages.


///////////////////////////////////////**********************************************

// Uncomment to use the PinChangeInterrupts iso External Interrupts
// #define PIN_CHANGE_INT

#include <NmraDcc.h>

NmraDcc     Dcc ;
DCC_MSG  Packet ;

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
   #define _3P(a,b) Serial.print( a, b );
   #define _3L(a,b) Serial.println(a, b);
#else
   #define _MP( a )
   #define _ML( a )
   #define _3P(a,b)
   #define _3L(a,b)
#endif


/* ******************************************************************************* */


int tim_delay =  500;
int numfpins  =   15;
byte fpins [] = {  7,  15,   5,   3,   6,  14,   4,  17,  18,  19,   8,   9,  10,  16,  13};
//  pinnames:  > PD7, PC1, PD5, PD3, PD6, PC0, PD4, PC3, PC4, PC5, PB0, PB1, PB2, PC2, PB5 <

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
#define Accessory_Address    40  // THIS ADDRESS IS THE START OF THE SOUTPUTS RANGE

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
};
QUEUE volatile *ftn_queue = new QUEUE[16];

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
// {CV_29_CONFIG,          0},                                                                                 // Short Address 14 Speed Steps
   {CV_29_CONFIG, CV29_F0_LOCATION},                                                                           // Short Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},                                                     // Long  Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_F0_LOCATION},                       // Accesory Decoder Short Address
// {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_EXT_ADDRESSING | CV29_F0_LOCATION}, // Accesory Decoder  Long Address

   {CV_DECODER_MASTER_RESET, 0},

   {CV_To_Store_SET_CV_Address,      SET_CV_Address       & 0xFF },  // LSB Set CV Address
   {CV_To_Store_SET_CV_Address + 1, (SET_CV_Address >> 8) & 0x3F },  // MSB Set CV Address

   { 30,   1}, // GEN 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 31,  50}, //     Maximum level outputs (100 max)
   { 32, 100}, //     Waitmicros  (250 * 100,000 max)
   { 33,   5}, //     Waitmicros divider (standard 5)
   { 34, 100}, //     Standard blink interval  (* 10)
   { 35,   1}, // LS1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 36,  50}, //     Maximum level this output
   { 37, 100}, //     Waitmicros output highend
   { 38,   5}, //     Waitmicros output divider
   { 39, 100}, //     Blinkinterval this output
   { 40,   1}, // LS2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 41,  50}, //     Maximum level this output
   { 42, 100}, //     Waitmicros output highend
   { 43,   5}, //     Waitmicros output divider
   { 44, 100}, //     Blinkinterval this output
   { 45,   1}, // LS3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 46,  50}, //     Maximum level this output
   { 47, 100}, //     Waitmicros output highend
   { 48,   5}, //     Waitmicros output divider
   { 49, 100}, //     Blinkinterval this output
   { 50,   1}, // LS4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 51,  50}, //     Maximum level this output
   { 52, 100}, //     Waitmicros output highend
   { 53,   5}, //     Waitmicros output divider
   { 54, 100}, //     Blinkinterval this output
   { 55,   1}, // LB1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 56,  50}, //     Maximum level this output
   { 57, 100}, //     Waitmicros output highend
   { 58,   5}, //     Waitmicros output divider
   { 59, 100}, //     Blinkinterval this output
   { 60,   1}, // LB2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 61,  50}, //     Maximum level this output
   { 62, 100}, //     Waitmicros output highend
   { 63,   5}, //     Waitmicros output divider
   { 64, 100}, //     Blinkinterval this output
   { 65,   1}, // LB3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 66,  50}, //     Maximum level this output
   { 67, 100}, //     Waitmicros output highend
   { 68,   5}, //     Waitmicros output divider
   { 69, 100}, //     Blinkinterval this output
   { 70,   1}, // LB4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 71,  50}, //     Maximum level this output
   { 72, 100}, //     Waitmicros output highend
   { 73,   5}, //     Waitmicros output divider
   { 74, 100}, //     Blinkinterval this output
   { 75,   1}, // LB5 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 76,  50}, //     Maximum level this output
   { 77, 100}, //     Waitmicros output highend
   { 78,   5}, //     Waitmicros output divider
   { 79, 100}, //     Blinkinterval this output
   { 80,   1}, // LB6 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
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
};

volatile uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);

// These are the absolute maximums allowed !!!
volatile uint8_t M_maxLevel = Dcc.getCV( 31 );
volatile uint8_t M_highend  = Dcc.getCV( 32 );
volatile uint8_t M_divider  = Dcc.getCV( 33 );
volatile uint8_t M_interval = Dcc.getCV( 34 );


/* ******************************************************************************* */


void setup()
{

   noInterrupts();

   // initialize the digital pins as outputs
   for (int i = 0; i < numfpins; i++) 
   {
      digitalWrite(fpins[i], 0);  // Switch the pin off first.
      pinMode(fpins[i], OUTPUT);  // Then set it as an output.
   }

   // Start SoftPWM with 50hz pwm frequency
   Palatis::SoftPWM.begin( 50);

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

   Dcc.pin(digitalPinToInterrupt(FunctionPinDcc), FunctionPinDcc, false);

   // Dcc.init(MAN_ID_DIY, 201, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, CV_To_Store_SET_CV_Address);
   Dcc.init(MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, CV_To_Store_SET_CV_Address);

   #if defined(DECODER_LOADED)

      if (Dcc.getCV(CV_DECODER_MASTER_RESET) == CV_DECODER_MASTER_RESET) 
      {
      #endif

         for (int i = 0; i < FactoryDefaultCVIndex; i++)
         {
            Dcc.setCV(FactoryDefaultCVs[i].CV, FactoryDefaultCVs[i].Value);

            digitalWrite(FunctionPinLed, 1);
            delay (tim_delay);
            digitalWrite(FunctionPinLed, 0);
         }

      #if defined(DECODER_LOADED)
      }
   #endif

   // These are the absolute maximums allowed
   M_maxLevel = Dcc.getCV( 31 ); //     Maximum level outputs (100 max)
   M_highend  = Dcc.getCV( 32 ); //     Waitmicros  (250 * 100,000 max)
   M_divider  = Dcc.getCV( 33 ); //     Waitmicros divider (standard 5)
   M_interval = Dcc.getCV( 34 ); //     Standard blink interval  (* 10)

   // Loop through all the settings for checking and correcting
   for (int i = 0; i < numfpins; i++) 
   {
      calculateFtnQueue( i );
   }

   _PL(""); // An extra empty line for clearness
   _PL("*************************************");

   #ifdef _MONITOR_
      displayText(); // Shows the standard explanation text
   #endif

   _PL("*************************************");
   _PL(""); // An extra empty line for clearness

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

   Dcc.process();

   for (int i = 0; i < numfpins; i++) 
   {
      switch (ftn_queue[ i ].inUse )
      {
         case 1:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         {
            Palatis::SoftPWM.set( i, ftn_queue[i].maxLevel);
            break;
         }

         case 3:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         { 
            if (currentMillis - ftn_queue[i].previousMillis >= ftn_queue[i].blinkInterval)
            {
               // Save the last time you blinked the LED
               ftn_queue[i].previousMillis = currentMillis;

               // If the LED is off turn it on and vice-versa
               if (ftn_queue[i].ledState == LOW)
               {
                  Palatis::SoftPWM.set( i, ftn_queue[i].maxLevel);
                  ftn_queue[i].ledState =  HIGH;
               }
               else
               {
                  ftn_queue[i].ledState =   LOW;
                  Palatis::SoftPWM.set( i,   0);
               }
            }
            break;
         }

         case 5:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
         { 
            if (currentMicros - ftn_queue[i].previousMicros > ftn_queue[i].waitMicros)
            {
               ftn_queue[i].previousMicros = currentMicros;

               if ((ftn_queue[i].fadeCounter > ftn_queue[i].maxLevel - 1) || (ftn_queue[i].fadeCounter < 1))
               {
                  ftn_queue[i].upDown = !ftn_queue[i].upDown;
               }

               if (ftn_queue[i].upDown == true) { ftn_queue[i].fadeCounter++; } else { ftn_queue[i].fadeCounter--; }

               Palatis::SoftPWM.set( i, ftn_queue[i].fadeCounter);

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

   #ifdef _MONITOR_

      if (foundEom)
      {
         foundBom = false;
         foundEom = false;
         parseCom(commandString);
      }

   #endif

}


/* ******************************************************************************* */


void softwareReset( uint8_t preScaler )
{
   wdt_enable( preScaler );

   while(1) {}  // Wait for the prescaler time to expire and do an auto-reset
}


/* ******************************************************************************* */


void calculateFtnQueue( int number )
{

   // Now we're going to do some calculations
   ftn_queue[ number ].inUse = Dcc.getCV(35 + ( number * 5 ));

   // Print some values if requested
   _PP( "cv #: ");
   _2P( 35 + ( number * 5 ), DEC);
   _PP( "\t" " value: ");
   _2L( ftn_queue[ number ].inUse, DEC);

   // Default settings for counters and states
   ftn_queue[ number ].fadeCounter    =     0;
   ftn_queue[ number ].ledState       =   LOW;
   ftn_queue[ number ].previousMicros =     0;
   ftn_queue[ number ].previousMillis =     0;
   ftn_queue[ number ].upDown         = false;

   // These values stored in the Configuration Variables
   uint8_t S_maxLevel = Dcc.getCV( 36 + ( number * 5 ));
   uint8_t S_highend  = Dcc.getCV( 37 + ( number * 5 ));
   uint8_t S_divider  = Dcc.getCV( 38 + ( number * 5 ));
   uint8_t S_interval = Dcc.getCV( 39 + ( number * 5 ));

   if (M_maxLevel > S_maxLevel)
   {
      ftn_queue[number].maxLevel = S_maxLevel;
   }
   else
   {
      ftn_queue[number].maxLevel = M_maxLevel;
   }

   if (M_interval > S_interval)
   {
      ftn_queue[number].blinkInterval = S_interval * 10;
   }
   else
   {
      ftn_queue[number].blinkInterval = M_interval * 10;
   }

   if (M_highend > S_highend)
   {
      ftn_queue[number].waitMicros = ( (S_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
   }
   else
   {
      ftn_queue[number].waitMicros = ( (M_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
   }

   if (M_divider > S_divider)
   {
      ftn_queue[number].waitMicros = ( ftn_queue[number].waitMicros / S_divider );
   }
   else
   {
      ftn_queue[number].waitMicros = ( ftn_queue[number].waitMicros / M_divider );
   }
}


/* ******************************************************************************* */


volatile int prevFuncState = 25;  // Previous state of light function.

void exec_function (int function, int FuncState)
{
   if ( function == -1 )
   {
      if ( prevFuncState != FuncState)
      {
         prevFuncState = FuncState; // Something changed state

         char recData[4] = " 0 ";
         recData[1] = 48 + FuncState;

         parseCom( recData );       // Action on CV value
      }
   }
   else
   {
      if ( ftn_queue[ function ].inUse != FuncState )
      {
         ftn_queue[ function ].inUse = FuncState;
      }
   }

   // _PP("\t" "function: " );
   // _2P(function,  DEC);
   // _PP("\t" "FuncState: ");
   // _2P(FuncState, DEC);
   // _PP("\t" "cvValue: "  );
   // _2P(cvValue,   DEC);
   // _PP("\t" "prevFuncState: ");
   // _2L(prevFuncState,     DEC);


// F0 alle lichten aan of uit  -->  FuncState 1 of 0  -->  CV 30
// F1 LS1 aan of uit  -->  FuncState 1 of 0  -->  CV 35  -->  ftn_queue[0]  -->  function


}


/* ******************************************************************************* */
// The next part will / can only be used for an mpu with enough memory

#ifdef _MONITOR_

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
   Serial.println(F("Put in one of the following commands: "                        ));
   Serial.println(F("--------------------------------------------------------------"));
   Serial.println(F("<0>   all outputs: Off"                                        ));
   Serial.println(F("<1>   all outputs:  On"                                        ));
   Serial.println(F("<2>   all outputs: Blink Off"                                  ));
   Serial.println(F("<3>   all outputs: Blink  On"                                  ));
   Serial.println(F("<4>   all outputs: Fade Off"                                   ));
   Serial.println(F("<5>   all outputs: Fade  On"                                   ));
   Serial.println("");
   Serial.println(F("<C>   clear everything: Factory Default"                       ));
   Serial.println(F("<D>   dumps everything: to your monitor"                       ));
   //rial.println("");
   //rial.println(F("<f>   controls mobile engine decoder functions F0-F12: <f x y>"));
   //rial.println(F("<F>   lists all funtions and settings for all outputs: <F>"  )  );
   Serial.println("");
   Serial.println(F("<M>   list the available SRAM on the chip"                     ));
   Serial.println("");
   Serial.println(F("<R>   reads a configuration variable: <R x>"                   ));
   Serial.println(F("<W>   write a configuration variable: <W x y>"                 ));
   Serial.println(F("--------------------------------------------------------------"));
   Serial.println(F("Include < and > in your command   -and-   spaces are mandatory"));
   Serial.println("");
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

      if (inChar == bomMarker) {         // start of new command
         sprintf(commandString, " ");
         foundBom = true;
      }
      else if (inChar == eomMarker) {    // end of new command
         foundEom = true;
         // parse(commandString) in the loop
      }
      else if (strlen(commandString) < 128) {
         sprintf(commandString, "%s%c", commandString,  inChar);
         // if comandString still has space, append character just read from serial line
         // otherwise, character is ignored, (but we'll continue to look for '<' or '>')
      }
   }
}


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
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 0 );
            ftn_queue[i].inUse = 0;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<0>" );
         interrupts();
         break;
      }


      case '1':     // <1>   all outputs: On
/*
 *    returns: <1>
 */
      {
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 1 );
            ftn_queue[i].inUse = 1;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<1>" );
         interrupts();
         break;
      }


      case '2':     // <2>   all outputs: Blink Off
/*
 *    returns: <2>
 */
      {
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 2 );
            ftn_queue[i].inUse = 2;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<2>" );
         interrupts();
         break;
      }


      case '3':     // <3>   all outputs: Blink On
/*
 *    returns: <3>
 */
      {
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 3 );
            ftn_queue[i].inUse = 3;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<3>" );
         interrupts();
         break;
      }


      case '4':     // <4>   all outputs: Fade Off
/*
 *    returns: <4>
 */
      {
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 4 );
            ftn_queue[i].inUse = 4;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<4>" );
         interrupts();
         break;
      }


      case '5':     // <5>   all outputs: Fade On
/*
 *    returns: <5>
 */
      {
         noInterrupts();   // Disable interrupts.

         for (int i = 0; i < numfpins - 1; i++) 
         {
            Dcc.setCV( 35 + ( i * 5 ), 5 );
            ftn_queue[i].inUse = 5;
            calculateFtnQueue( i );
         }

         // Send feedback and restore interrupts.
         _ML( "\t" "<5>" );
         interrupts();
         break;
      }


/****  CLEAR SETTINGS TO FACTORY DEFAULTS  ****/

      case 'C':     // <C>
/*
 *    clears settings to Factory Defaults
 *
 *    returns: <FD done check>
 */
      {
         uint8_t cvCheck = Dcc.setCV( 120, 120 );

         _MP( "\t"  "<FD done>"  "\t" );
         _3L( cvCheck, DEC );

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
         FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);

         for (int i = 0; i < FactoryDefaultCVIndex; i++)
         {
            uint8_t cvValue = Dcc.getCV( FactoryDefaultCVs[i].CV );

            _MP( " cv: "                      );
            _3P( FactoryDefaultCVs[i].CV, DEC );
            _MP( "\t"              " value: " );
            _3L( cvValue, DEC                 );
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

         _MP( "<r " );
         _3P( cv + 0 , DEC );
         _MP( " "   );
         _3P( cvCheck, DEC );
         _ML( ">"   );
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

         _MP( "<w " );
         _3P( cv + 0 , DEC );
         _MP( " "   );
         _3P( cvCheck, DEC );
         _ML( ">"   );

         switch ( cv )
         {
            case  30:
            {
               char recData[4] = " 1 ";
               recData[1] = com[6];

               parseCom( recData ); //  Recursive action on CV value

               _PL( "30" );
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
               for (int i = 0; i < numfpins; i++)
               {
                  calculateFtnQueue( i );
               }

               _PL( "31 ... 34" );
               break;
            }

            case  35 ... 104:
            {
               calculateFtnQueue( (cv - 35) / 5 );

               _PL( "35 ... 104" );
               break;
            }

            case 105 ... 109:
            {
               calculateFtnQueue( 14 );

               _PL( "105 ... 109" );
               break;
            }

            default:
            {
               _PL( "default" );
               break;
            }
         }
      }


/****  PRINT CARRIAGE RETURN IN SERIAL MONITOR WINDOW  ****/

      case ' ':     // < >
/*
 *    simply prints a carriage return - useful when interacting with Ardiuno through serial monitor window
 *
 *    returns: a carriage return
 */
      {
         _ML("");
         break;
      }


/****  DEFAULT FOR THE SWITCH FUNCTION = NO ACTION  ****/

      default:
         break;

   }
}

#endif   // _MONITOR_


/************************************************************************************
            Call-back functions
************************************************************************************/

#if defined (__cplusplus)
	extern "C" {
#endif


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
void    notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION Dir, DCC_SPEED_STEPS SpeedSteps )
{
   // _PP("\t" "notifyDccSpeed Addr: ");
   // _2P(Addr, DEC);
   // _PP(" " + (AddrType == DCC_ADDR_SHORT) ? 'S' : 'L' );
   // _PP("\t" "Speed: ");
   // _2P(Speed, DEC);
   // _PP("\t" "Dir: ");
   // _PP(Dir);
   // _PP("\t" "SpeedSteps: ");
   // _2L(SpeedSteps, DEC);
}


/* **********************************************************************************
 *  notifyDccReset(uint8_t hardReset) Callback for a DCC reset command.
 *
 *  Inputs:
 *    hardReset - 0 normal reset command.
 *                1 hard reset command.
 *
 *  Returns:
 *    None
 */
// void    notifyDccReset(uint8_t hardReset )
// {
//    _PP("\t" "notifyDccReset: ");
//    _2L(hardReset, DEC);
// }


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
void    notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState)
{
   // _PP( "\t" "notifyDccFunc Addr: " );
   // _2P( Addr,  DEC );
   // _PP( (AddrType == DCC_ADDR_SHORT) ? 'S' : 'L' );
   // _PP( "\t"  " Function Group: " );
   // _2P( FuncGrp,  DEC );
   // _PP( "\t"  " State: " );
   // _2L( FuncState,  DEC );

   switch(FuncGrp)
   {
      case FN_0_4:    //Function Group 1 F0 F4 F3 F2 F1
         exec_function( -1, (FuncState & FN_BIT_00)>>4 );
         exec_function(  0, (FuncState & FN_BIT_01)    );
         exec_function(  1, (FuncState & FN_BIT_02)>>1 );
         exec_function(  2, (FuncState & FN_BIT_03)>>2 );
         exec_function(  3, (FuncState & FN_BIT_04)>>3 );
         break;

      case FN_5_8:    //Function Group 1 S FFFF == 1 F8 F7 F6 F5  &  == 0  F12 F11 F10 F9
         exec_function(  4, (FuncState & FN_BIT_05)    );
         exec_function(  5, (FuncState & FN_BIT_06)>>1 );
         exec_function(  6, (FuncState & FN_BIT_07)>>2 );
         exec_function(  7, (FuncState & FN_BIT_08)>>3 );
         break;

      case FN_9_12:
         exec_function(  8, (FuncState & FN_BIT_09)    );
         exec_function(  9, (FuncState & FN_BIT_10)>>1 );
         exec_function( 10, (FuncState & FN_BIT_11)>>2 );
         exec_function( 11, (FuncState & FN_BIT_12)>>3 );
         break;

      case FN_13_20:   //Function Group 2 FuncState == F20-F13 Function Control
         exec_function( 12, (FuncState & FN_BIT_13)    );
         exec_function( 13, (FuncState & FN_BIT_14)>>1 );
         // c_function( 15, (FuncState & FN_BIT_15)>>2 );
         // c_function( 16, (FuncState & FN_BIT_16)>>3 );
         break;

      case FN_21_28:
         break;	
   }
}


/* **********************************************************************************
 *  notifyDccAccBoardAddrSet() Board oriented callback for a turnout accessory decoder.
 *                             This notification is when a new Board Address is set to the
 *                             address of the next DCC Turnout Packet that is received
 *
 *                             This is enabled via the setAccDecDCCAddrNextReceived() method above
 *
 *  Inputs:
 *    BoardAddr   - Per board address. Equivalent to CV 1 LSB & CV 9 MSB.
 *                  per board for a standard accessory decoder with 4 output pairs.
 *
 *  Returns:
 *    None
 */
void    notifyDccAccBoardAddrSet( uint16_t BoardAddr)
{
   _PL("\t" "notifyDccAccBoardAddrSet");
}


/* **********************************************************************************
 *  notifyDccAccOutputAddrSet() Output oriented callback for a turnout accessory decoder.
 *                              This notification is when a new Output Address is set to the
 *                              address of the next DCC Turnout Packet that is received
 *
 *                             This is enabled via the setAccDecDCCAddrNextReceived() method above
 *
 *  Inputs:
 *    Addr        - Per output address. There will be 4 Addr addresses
 *                  per board for a standard accessory decoder with 4 output pairs.
 *
 *  Returns:
 *    None
 */
void    notifyDccAccOutputAddrSet( uint16_t Addr)
{
   _PL("\t" "notifyDccAccOutputAddrSet");
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
void notifyCVChange(uint16_t CV, uint8_t Value)
{
   _PP( "\t" "notifyCVChange: CV: ");
   _2P( CV, DEC    );
   _PP( " Value: " );
   _2L( Value, DEC );
}

void    notifyDccCVChange( uint16_t CV, uint8_t Value)
{
   _PP( "\t" "notifyDccCVChange: CV: ");
   _2P( CV,     DEC);
   _PP( " Value: " );
   _2L( Value,  DEC);


//  THIS PART is almost the same as in the 'W' command....

         switch ( CV )
         {
            case  30:
            {
               char recData[4] = " 0 ";

               if (Value == 1) {recData[1] = 49;}
               if (Value == 2) {recData[1] = 50;}
               if (Value == 3) {recData[1] = 51;}
               if (Value == 4) {recData[1] = 52;}
               if (Value == 5) {recData[1] = 53;}

               parseCom( recData ); //  Recursive action on CV value

               _PL( "30" );
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
               for (int i = 0; i < numfpins; i++)
               {
                  calculateFtnQueue( i );
               }

               _PL( "31 ... 34" );
               break;
            }

            case  35 ... 104:
            {
               calculateFtnQueue(( CV - 35 ) / 5 );

               _PL( "35 ... 104" );
               break;
            }

            case 105 ... 109:
            {
               calculateFtnQueue( 14 );

               _PL( "105 ... 109" );
               break;
            }

            default:
            {
               _PL( "default" );
               break;
            }
         }

}


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
 *                                                                                                        *
 *  Returns:
 *    None
 */
void notifyCVResetFactoryDefault()
{
   // Make 'FactoryDefaultCVIndex' non-zero and equal to num CVs to be reset 
   // to flag to the loop() function that a reset to FacDef needs to be done.
   FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
}


#if defined (__cplusplus)
}
#endif

//   
