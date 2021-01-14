

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
char commandString[128]      ;   // Max length for a buffer.
boolean foundBom      = false;   // Found begin of messages.
boolean foundEom      = false;   // Founf  end  of messages.


///////////////////////////////////////*********************************************************

// Uncomment to use the PinChangeInterrupts iso External Interrupts
#define PIN_CHANGE_INT

///////////////////////////////////////*********************************************************
#include <NmraDcc.h>

NmraDcc     Dcc ;
DCC_MSG  Packet ;

// ******** UNLESS YOU WANT ALL CV'S RESET UPON EVERY POWER UP REMOVE THE "//" IN THE FOLLOWING LINE!!
#define DECODER_LOADED

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL MONITOR
#define _DEBUG_

#ifdef _DEBUG_
   #define _PP(a) Serial.print( a );
   #define _PL(a) Serial.println(a);
#else
   #define _PP(a)
   #define _PL(a)
#endif

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO USE YOUR SERIAL PORT FOR COMMANDS
#define _MONITOR_


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
//   {CV_ACCESSORY_DECODER_ADDRESS_LSB,  Accessory_Address       & 0xFF},
//   {CV_ACCESSORY_DECODER_ADDRESS_MSB, (Accessory_Address >> 8) & 0x07},

//   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},
//   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 0},

//  Speed Steps don't matter for this decoder, only for loc decoders
//  ONLY uncomment 1 CV_29_CONFIG line below as approprate DEFAULT IS SHORT ADDRESS
// {CV_29_CONFIG,          0},                                                                                 // Short Address 14 Speed Steps
// {CV_29_CONFIG, CV29_F0_LOCATION},                                                                           // Short Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},                                                     // Long  Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_F0_LOCATION},                       // Accesory Decoder Short Address
   {CV_29_CONFIG, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE | CV29_EXT_ADDRESSING | CV29_F0_LOCATION}, // Accesory Decoder  Long Address

   {CV_DECODER_MASTER_RESET, 0},

   {CV_To_Store_SET_CV_Address,      SET_CV_Address       & 0xFF },  // LSB Set CV Address
   {CV_To_Store_SET_CV_Address + 1, (SET_CV_Address >> 8) & 0x3F },  // MSB Set CV Address

   { 30,   4}, // GEN 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 31,  50}, //     Maximum level outputs (100 max)
   { 32, 100}, //     Waitmicros  (250 * 100,000 max)
   { 33,   4}, //     Waitmicros divider (standard 4)
   { 34, 100}, //     Standard blink interval  (* 10)
   { 35,   0}, // LS1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 36,   0}, //     Maximum level this output
   { 37,   0}, //     Waitmicros output highend
   { 38,   0}, //     Waitmicros output divider
   { 39,   0}, //     Blinkinterval this output
   { 40,   0}, // LS2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 41,   0}, //     Maximum level this output
   { 42,   0}, //     Waitmicros output highend
   { 43,   0}, //     Waitmicros output divider
   { 44,   0}, //     Blinkinterval this output
   { 45,   3}, // LS3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 46,  60}, //     Maximum level this output
   { 47, 100}, //     Waitmicros output highend
   { 48,   5}, //     Waitmicros output divider
   { 49,  30}, //     Blinkinterval this output
   { 50,   5}, // LS4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 51, 100}, //     Maximum level this output
   { 52, 100}, //     Waitmicros output highend
   { 53,   4}, //     Waitmicros output divider
   { 54,  75}, //     Blinkinterval this output
   { 55,   5}, // LB1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 56, 100}, //     Maximum level this output
   { 57, 100}, //     Waitmicros output highend
   { 58,   4}, //     Waitmicros output divider
   { 59,  75}, //     Blinkinterval this output
   { 60,   0}, // LB2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 61,   0}, //     Maximum level this output
   { 62,   0}, //     Waitmicros output highend
   { 63,   0}, //     Waitmicros output divider
   { 64,   0}, //     Blinkinterval this output
   { 65,   3}, // LB3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 66,  60}, //     Maximum level this output
   { 67, 100}, //     Waitmicros output highend
   { 68,   5}, //     Waitmicros output divider
   { 69,  30}, //     Blinkinterval this output
   { 70,   0}, // LB4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 71,   0}, //     Maximum level this output
   { 72,   0}, //     Waitmicros output highend
   { 73,   0}, //     Waitmicros output divider
   { 74,   0}, //     Blinkinterval this output
   { 75,   0}, // LB5 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 76,   0}, //     Maximum level this output
   { 77,   0}, //     Waitmicros output highend
   { 78,   0}, //     Waitmicros output divider
   { 79,   0}, //     Blinkinterval this output
   { 80,   0}, // LB6 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 81,   0}, //     Maximum level this output
   { 82,   0}, //     Waitmicros output highend
   { 83,   0}, //     Waitmicros output divider
   { 84,   0}, //     Blinkinterval this output
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
   {106,  30}, //     Maximum level this output
   {107,  10}, //     Waitmicros output highend
   {108,  10}, //     Waitmicros output divider
   {109,  30}, //     Blinkinterval this output
};


/* ******************************************************************************* */


uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);

void notifyCVResetFactoryDefault()
{
   // Make 'FactoryDefaultCVIndex' non-zero and equal to num CVs to be reset 
   // to flag to the loop() function that a reset to FacDef needs to be done.
   FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
}


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

   Dcc.init(MAN_ID_DIY, 201, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER, CV_To_Store_SET_CV_Address);

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

         // These are the absolute maximums allowed
         uint8_t M_maxLevel = Dcc.getCV( 31 );
         uint8_t M_highend  = Dcc.getCV( 32 );
         uint8_t M_divider  = Dcc.getCV( 33 );
         uint8_t M_interval = Dcc.getCV( 34 );

         // Loop through all the settings for checking and correcting
         for (int i = 0; i < numfpins; i++) 
         {
            uint8_t cv_value = Dcc.getCV(35 + (i * 5));

            // Print some values if requested
            #if defined(_DEBUG_) || defined(_TEST_)

               Serial.print(         "cv #: ");
               Serial.print(35 + (i * 5), DEC);
               Serial.print("\t"   " value: ");
               Serial.println(cv_value,   DEC);

            #endif

            // These are the default settings for counters and states
            ftn_queue[i].fadeCounter    =     0;
            ftn_queue[i].ledState       =   LOW;
            ftn_queue[i].previousMicros =     0;
            ftn_queue[i].previousMillis =     0;
            ftn_queue[i].upDown         = false;

            // Now we're going to do some calculations for the rest
            ftn_queue[i].inUse = Dcc.getCV(35 + (i * 5));

            uint8_t S_maxLevel = Dcc.getCV(36 + (i * 5));
            uint8_t S_highend  = Dcc.getCV(37 + (i * 5));
            uint8_t S_divider  = Dcc.getCV(38 + (i * 5));
            uint8_t S_interval = Dcc.getCV(39 + (i * 5));

            if (M_maxLevel > S_maxLevel)
            {
               ftn_queue[i].maxLevel = S_maxLevel;
            }
            else
            {
               ftn_queue[i].maxLevel = M_maxLevel;
            }

            if (M_interval > S_interval)
            {
               ftn_queue[i].blinkInterval = S_interval * 10;
            }
            else
            {
               ftn_queue[i].blinkInterval = M_interval * 10;
            }

            if (M_highend > S_highend)
            {
               ftn_queue[i].waitMicros = ( (S_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
            }
            else
            {
               ftn_queue[i].waitMicros = ( (M_highend * 100000) / Palatis::SoftPWM.PWMlevels() );
            }

            if (M_divider > S_divider)
            {
               ftn_queue[i].waitMicros = ( ftn_queue[i].waitMicros / S_divider );
            }
            else
            {
               ftn_queue[i].waitMicros = ( ftn_queue[i].waitMicros / M_divider );
            }
         }
   #if defined(DECODER_LOADED)
      }
   #endif

   #ifdef _DEBUG_
      Serial.println(""); // An extra empty line for clearness
      Serial.println("*************************************");

      #ifdef _MONITOR_
         displayText(); // Shows the standard explanation text
      #endif

      Serial.println("*************************************");
      Serial.println(""); // An extra empty line for clearness
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

   Dcc.process();

   for (int i = 0; i < numfpins; i++) 
   {
      if (ftn_queue[i].inUse >= 1)
      {

         switch (Dcc.getCV(35 + (i * 5)))
         {
            case 1:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
            {
               Palatis::SoftPWM.set( i, ftn_queue[i].maxLevel);
            }
               break;

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
            }
               break;

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
            }
               break;

            default:     // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
            {
               Palatis::SoftPWM.set( i,   0);
            }
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

   if( FactoryDefaultCVIndex && Dcc.isSetCVReady())
   {
      noInterrupts();
      FactoryDefaultCVIndex--;  // Decrement first as initially it is the size of the array (last element + 1)
      Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
      interrupts();
   }

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
 *                                  You must & FuncState with the appropriate
 *                                  FN_BIT_nn value to isolate a given bit.
 *
 *  Returns:
 *    None
 */
void  notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState)
{

   #if defined(_DEBUG_) || defined(_MONITOR_)

      Serial.print( "notifyDccFunc: Addr: " );
      Serial.print( Addr,  DEC );
      Serial.print( (AddrType == DCC_ADDR_SHORT) ? 'S' : 'L' );
      Serial.print( "\t"  " Function Group: " );
      Serial.print( FuncGrp,  DEC );
      Serial.print( "\t"  " State: " );
      Serial.print( FuncState,  DEC );
      Serial.println();

   #endif

   switch(FuncGrp)
   {
      case FN_0_4:    //Function Group 1 F0 F4 F3 F2 F1
         exec_function(  0, FunctionPin0 , (FuncState & FN_BIT_00)>>4 );
         exec_function(  1, FunctionPin1 , (FuncState & FN_BIT_01)    );
         exec_function(  2, FunctionPin2 , (FuncState & FN_BIT_02)>>1 );
         exec_function(  3, FunctionPin3 , (FuncState & FN_BIT_03)>>2 );
         exec_function(  4, FunctionPin4 , (FuncState & FN_BIT_04)>>3 );
         break;

      case FN_5_8:    //Function Group 1 S FFFF == 1 F8 F7 F6 F5  &  == 0  F12 F11 F10 F9 F8
         exec_function(  5, FunctionPin5 , (FuncState & FN_BIT_05)    );
         exec_function(  6, FunctionPin6 , (FuncState & FN_BIT_06)>>1 );
         exec_function(  7, FunctionPin7 , (FuncState & FN_BIT_07)>>2 );
         exec_function(  8, FunctionPin8 , (FuncState & FN_BIT_08)>>3 );
         break;

      case FN_9_12:
         exec_function(  9, FunctionPin9 , (FuncState & FN_BIT_09)    );
         exec_function( 10, FunctionPin10, (FuncState & FN_BIT_10)>>1 );
         exec_function( 11, FunctionPin11, (FuncState & FN_BIT_11)>>2 );
         exec_function( 12, FunctionPin12, (FuncState & FN_BIT_12)>>3 );
         break;

      case FN_13_20:   //Function Group 2 FuncState == F20-F13 Function Control
         exec_function( 13, FunctionPin13, (FuncState & FN_BIT_13)    );
         exec_function( 14, FunctionPin14, (FuncState & FN_BIT_14)>>1 );
         exec_function( 15, FunctionPin15, (FuncState & FN_BIT_15)>>2 );
         //ec_function( 16, FunctionPin16, (FuncState & FN_BIT_16)>>3 );
         break;

      case FN_21_28:
         break;	
   }
}

void exec_function (int function, int pin, int FuncState)
{

  switch ( Dcc.getCV( 35 + ( function * 5 ) ) )
  {

   // case 0:    // On - Off LED
   //    digitalWrite (pin, FuncState);
   //    ftn_queue[function].inuse = 0;
   //    break;

   //  case 1:    // Blinking LED
   //    if ((ftn_queue[function].inuse==0) && (FuncState==1))  {
   //      ftn_queue[function].inuse = 1;
   //      ftn_queue[function].start_value = 0;
   //      digitalWrite(pin, 0);
   //      ftn_queue[function].stop_value = int(Dcc.getCV( 33+(function*5)));
   //    } else {
   //        if ((ftn_queue[function].inuse==1) && (FuncState==0)) {
   //          ftn_queue[function].inuse = 0;
   //          digitalWrite(pin, 0);
   //        }
   //      }
   //    break;

   //  case 2:    // Servo
   //    if (ftn_queue[function].inuse == 0)  {
	//     ftn_queue[function].inuse = 1;
	// 	servo[function].attach(pin);
	//   }
   //    if (FuncState==1) ftn_queue[function].increment = char ( Dcc.getCV( 31+(function*5)));
   //      else ftn_queue[function].increment = - char(Dcc.getCV( 31+(function*5)));
   //    if (FuncState==1) ftn_queue[function].stop_value = Dcc.getCV( 33+(function*5));
   //      else ftn_queue[function].stop_value = Dcc.getCV( 32+(function*5));
   //    break;

   //  case 3:    // Blinking LED PAIR
   //    if ((ftn_queue[function].inuse==0) && (FuncState==1))  {
   //      ftn_queue[function].inuse = 1;
   //      ftn_queue[function].start_value = 0;
   //      digitalWrite(fpins[function], 0);
   //      digitalWrite(fpins[function+1], 1);
   //      ftn_queue[function].stop_value = int(Dcc.getCV( 33+(function*5)));
   //    } else {
   //        if (FuncState==0) {
   //          ftn_queue[function].inuse = 0;
   //          digitalWrite(fpins[function], 0);
   //          digitalWrite(fpins[function+1], 0);
   //        }
   //      }
   //    break;

   //  case 4:    // Pulse Output based on Rate*10 Milliseconds
   //    if ((ftn_queue[function].inuse==0) && (FuncState==1)) {  //First Turn On Detected
   //      digitalWrite(fpins[function], 1);
	// 	delay (10*ftn_queue[function].increment);
   //      digitalWrite(fpins[function], 0);
	// 	ftn_queue[function].inuse = 1;                    //inuse set to 1 says we already pulsed
   //    } else 
   //        if (FuncState==0)  ftn_queue[function].inuse = 0;
   //    break;

   //  case 5:    // Fade On
   //    #define fadedelay 24
   //    if ((ftn_queue[function].inuse==0) && (FuncState==1))  {
   //      ftn_queue[function].inuse = 1;
   //      for (t=0; t<ftn_queue[function].stop_value; t+=ftn_queue[function].increment) {
   //        digitalWrite( fpins[function], 1);
   //        delay(fadedelay*(t/(1.*ftn_queue[function].stop_value)));
   //        digitalWrite( fpins[function], 0);
   //        delay(fadedelay-(fadedelay*(t/(1.*ftn_queue[function].stop_value))));
   //      }
   //      digitalWrite( fpins[function],  1 );
   //    } else {
   //        if ((ftn_queue[function].inuse==1) && (FuncState==0)) {
   //          ftn_queue[function].inuse = 0;
   //          digitalWrite(fpins[function], 0);
   //        }
   //      }
   //    break;

  }
}


/*+
 *  notifyCVChange()  Called when a CV value is changed.
 *                    This is called whenever a CV's value is changed.
 *  notifyDccCVChange()  Called only when a CV value is changed by a Dcc packet or a internal lib function.
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
   #if defined(_DEBUG_) || defined(_MONITOR_)

      Serial.print( "notifyCVChange: CV: ");
      Serial.print( CV, DEC);
      Serial.print( " Value: " );
      Serial.print( Value, DEC);
      Serial.println();

   #else

      Value = Value;  // Silence Compiler Warnings...
      CV = CV;

   #endif  

// eerst checken of CV 30 is >> on/off outputs
// daarna checken CV 31 t/m 34 >> (her)bereken outputs

// hierna kijken welke groep (0 t/m 14) betrokken is
// dan kijken we welke groep herberekend moet worden

}


/* ******************************************************************************* */
// The next part will / can only be used for an mpu with enough memory

#ifdef _MONITOR_

/* ******************************************************************************* */
      // 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
/* **********************************************************************************

   <0>   all outputs: Off
   <1>   all outputs: On
   <2>   all outputs: Blink Off
   <3>   all outputs: Blink On
   <4>   all outputs: Fade Off
   <5>   all outputs: Fade On

   <R>   reads a configuration variable: <R x>
   <W>   write a configuration variable: <W x y>

   <C>   clear everything: Factory Default
   <D>   dumps everything: to Serial.Print

   <f>   controls mobile engine decoder functions F0-F12: <f x y>
   <F>   lists all funtions and settings for all outputs: <F>

   <M>   list the available SRAM on the chip


   <a>: controls stationary accessory decoders
   <b>: sets/clear a configuration variable bit in an engine decoder on the main operations track
   <B>: sets/clear a configuration variable bit in an engine decoder on the programming track

*/
void displayText()
{
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("Put in one of the following commands:"                                        ));
   Serial.println(F("-----------------------------------------------------------------------------"));
   Serial.println(F("<A>: controls single outputs ( 1 to 16), as defined by CVs (including timing)"));
   Serial.println(F("** Example:  <A 14 1>  or  <A 14 ON>  both switch ON output 14 for set timers"));
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("<T>: controls turnouts, 1 to 8, where [T1 = A1 and A2], [T2 = A3 and A4], etc"));
   Serial.println(F("** Example:  <T 4 1>  or  <T 4 ON>  both switch ON A8 (Turnout 4) incl timing"));
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("<R>: reads a configuration variable byte from the connected accessory decoder"));
   Serial.println(F("*** Example:  <R 100>  reads the setting of CV100 (output 14 config)"         ));
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("<W>: writes a configuration variable byte  to the connected accessory decoder"));
   Serial.println(F("*** Example: <W 100 0>  (re)sets CV100 (output 14 config) to On/Off"          ));
   Serial.println(""); // An extra empty line for clearness
   Serial.println(F("<C>: clears all the settings in Eeprom and CVs, goes back to Factory Defaults"));
   Serial.println(F("<E>: writes all the settings to Eeprom"                                       ));
   Serial.println(F("<F>: returns the amount of free SRAM memory on the Arduino as <f MEM> (bytes)"));
   Serial.println(F("<s>: returns status messages, including output states, and the sketch version"));
   Serial.println(F("-----------------------------------------------------------------------------"));
   Serial.println(F("!! Include < and > in your command !!  -and-  !! spaces are also mandatory !!"));
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


void parseCom(char *com)
{
   switch (com[1])  // com[0] = '<' or ' '
   {


/*****  SWITCH ALL THE OUTPUTS AT ONCE  ****/

      case '0':     // <0>   all outputs: Off
/*
 *    returns: <0>
 */

         Palatis::SoftPWM.allOff();  // Set the PWM value of all channels to 0.

         Serial.println("0");
         break;


      case '1':     // <1>   all outputs: On
/*
 *    returns: <1>
 */
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         Serial.println("1");
         break;


      case '2':     // <2>   all outputs: Blink Off
/*
 *    returns: <2>
 */
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         Serial.println("2");
         break;


      case '3':     // <3>   all outputs: Blink On
/*
 *    returns: <3>
 */
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         Serial.println("3");
         break;


      case '4':     // <4>   all outputs: Fade Off
/*
 *    returns: <4>
 */
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         Serial.println("4");
         break;


      case '5':     // <5>   all outputs: Fade On
/*
 *    returns: <5>
 */
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         Serial.println("5");
         break;


/***** OPERATE STATIONARY ACCESSORY DECODERS  ****/    

      case 'A':       // <A OUTPUT# ACTIVATE>
/*
 *    <A>: controls single outputs ( 1 to 16), as defined by CVs (including timing)
 *         ** Example:  <A 14 1>  or  <A 14 ON>  both switch ON output 14
 *
 *    OUTPUT#:  the address (ID) of the output (1 - 16)
 *    ACTIVATE: 1 = on (set), 0 = off (clear)
 *
 *    returns: NONE
 */

         {
            // uint16_t Addr = Dcc.getAddr();   // 40

// Config 0 = On/Off, 1 = Blink, 2 = Servo, 3 = DBL LED Blink, 4 = Pulsed, 5 = Fade

//           void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower )

            // int pin = 0;

            // pin = fpins[ 0 ]; // Config 0 = On/Off
            // exec_function( 0, pin, 1);

            // pin = fpins[ 1 ]; // Config 1 = Blink
            // exec_function( 1, pin, 1);

            // pin = fpins[ 2 ]; // Config 4 = Pulsed
            // exec_function( 2, pin, 1);

            // pin = fpins[ 3 ]; // Config 5 = Fade
            // exec_function( 3, pin, 1);

         }


         // Output::parse(com + 2);
         break;


/***** CREATE/EDIT/REMOVE/SHOW & OPERATE A TURN-OUT  ****/    

      case 'T':       // <T ID THROW>
/*
 *    <T ID THROW>:                sets turnout ID to either the "thrown" or "unthrown" position
 *
 *    ID: the numeric ID (0-32767) of the turnout to control
 *    THROW: 0 (unthrown) or 1 (thrown)
 *
 *    returns: <H ID THROW> or <X> if turnout ID does not exist
 *
 *    *** SEE ACCESSORIES.CPP FOR COMPLETE INFO ON THE DIFFERENT VARIATIONS OF THE "T" COMMAND
 *    USED TO CREATE/EDIT/REMOVE/SHOW TURNOUT DEFINITIONS
 */
         // Turnout::parse(com + 2);
         break;


/***** CREATE/EDIT/REMOVE/SHOW & OPERATE AN OUTPUT PIN  ****/    //////////////////////////////////////////////////////////////////////////

      case 'Z':       // <Z ID ACTIVATE>
/*
 *   <Z ID ACTIVATE>:          sets output ID to either the "active" or "inactive" state
 *
 *   ID: the numeric ID (0-32767) of the output to control
 *   ACTIVATE: 0 (active) or 1 (inactive)
 *
 *   returns: <Y ID ACTIVATE> or <X> if output ID does not exist
 *
 *   *** SEE OUTPUTS.CPP FOR COMPLETE INFO ON THE DIFFERENT VARIATIONS OF THE "O" COMMAND
 *   USED TO CREATE/EDIT/REMOVE/SHOW TURNOUT DEFINITIONS
 */
         // Output::parse(com + 2);
         break;      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
      // pRegs->writeCVByte(com + 2);

// void RegisterList::writeCVByte(char *s) volatile{
         // byte bWrite[ 4 ];
         int  bValue, cv;
         // int  cv, c, d, base;

         if( sscanf( com + 2, "%d %d", &cv, &bValue ) != 2 ) return;
         // cv--;                              // actual CV addresses are cv-1 (0-1023)

         // bWrite[ 0 ] = 0x7C + (highByte( cv ) & 0x03 );   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
         // bWrite[ 1 ] = lowByte( cv );
         // bWrite[ 2 ] = bValue;

         uint8_t cvCheck = Dcc.setCV( cv, bValue );

//   loadPacket(0,resetPacket,2,1);    // 1 reset packet
//   loadPacket(0,bWrite,3,4);         // 4 verify packets
//   loadPacket(0,resetPacket,2,1);    // 1 reset packet
//   loadPacket(0,idlePacket,2,10);    // 10 idle packets ???

// c=0;
// d=0;
// base=0;

//   for(int j=0;j<ACK_BASE_COUNT;j++)
//     base+=analogRead(CURRENT_MONITOR_PIN_PROG);
//   base/=ACK_BASE_COUNT;

      //   bWrite[ 0 ] = 0x74 + ( highByte( cv ) & 0x03 );   // set-up to re-verify entire byte

//   loadPacket(0,resetPacket,2,3);          // NMRA recommends starting with 3 reset packets
//   loadPacket(0,bWrite,3,5);               // NMRA recommends 5 verfy packets
//   loadPacket(0,resetPacket,2,1);          // forces code to wait until all repeats of bRead are completed (and decoder begins to respond)

//   for(int j=0;j<ACK_SAMPLE_COUNT;j++){
//     c=(analogRead(CURRENT_MONITOR_PIN_PROG)-base)*ACK_SAMPLE_SMOOTHING+c*(1.0-ACK_SAMPLE_SMOOTHING);
//     if(c>ACK_SAMPLE_THRESHOLD)
//       d=1;
//   }

         // if( d == 0 ) bValue = -1;    // verify unsuccessfull

         Serial.print( "<w " );
         Serial.print( cv + 0 , DEC );
         Serial.print( " "   );
         Serial.print( cvCheck, DEC );
         Serial.print( "> \n");

         break;


/***** READ CONFIGURATION VARIABLE BYTE FROM DECODER ****/

      case 'R':     // <R CV>
/*
 *    reads a Configuration Variable from the decoder
 *
 *    CV: the number of the Configuration Variable memory location in the decoder to read from (1-1024)
 *
 *    returns: <r CV VALUE)
 *    where VALUE is a number from 0-255 as read from the requested CV, or -1 if read could not be verified
 */


         {
            for (int i = 0; i < 20; i++) 
            {
               uint8_t cv_value = Dcc.getCV(30 + i);

               Serial.print(" cv_value: ");
               Serial.println(cv_value, DEC);
            }
         }

         // pRegs->readCV(com + 2);
         break;


/***** DUMPS CONFIGURATION VARIABLES FROM DECODER ****/

      case 'D':     // <D>
/*
 *    dumps all Configuration Variables from the decoder
 *
 *    returns a list of: <CV VALUE)
 *    where VALUE is a number from 0-255 as read from the CV, or -1 if read could not be verified
 */

         {
            for (int i = 0; i < FactoryDefaultCVIndex; i++)
            {
               uint8_t cvValue = Dcc.getCV( i );

               Serial.print( " cv: "          );
               Serial.print( i, DEC           );
               Serial.print( "\t"  " value: " );
               Serial.print( cvValue, DEC     );
               Serial.println();
            }
         }
         break;


/***** STORE SETTINGS IN EEPROM  ****/

      case 'E':     // <E>
/*
 *    stores settings for turnouts and sensors into EEPROM
 *
 *    returns: <e nTurnouts nSensors>
 */
         //  EEStore::store();
         //  Serial.print("<e ");
         //  Serial.print(EEStore::eeStore->data.nTurnouts);
         //  Serial.print(" ");
         //  Serial.print(EEStore::eeStore->data.nSensors);
         //  Serial.print(" ");
         //  Serial.print(EEStore::eeStore->data.nOutputs);
         //  Serial.print(">");
         break;


/***** CLEAR SETTINGS IN EEPROM  ****/

      case 'C':     // <C>
/*
 *    clears settings for Turnouts in EEPROM
 *
 *    returns: <O>
 */
      //  EEStore::clear();
         Serial.print("<O>");
         break;


/***** ATTEMPTS TO DETERMINE HOW MUCH FREE SRAM IS AVAILABLE IN ARDUINO  ****/

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
         Serial.print("<m ");
         Serial.print((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
         Serial.println(">");
      }
      break;


/***** PRINT CARRIAGE RETURN IN SERIAL MONITOR WINDOW  ****/

      case ' ':     // < >
/*
 *    simply prints a carriage return - useful when interacting with Ardiuno through serial monitor window
 *
 *    returns: a carriage return
 */
         Serial.println("");
         break;

   }
}

#endif


/* ******************************************************************************* */


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
   // if (Function13_value==1)
   // {
   //   Motor1Speed = (Speed & 0x7f );
   //   if (Motor1Speed == 1)  Motor1Speed=0;
   //   Motor1ForwardDir  = ForwardDir;
   // }

   // if (Function14_value==1)
   // {
   //   Motor2Speed = (Speed & 0x7f );
   //   if (Motor2Speed == 1)  Motor2Speed=0;
   //   Motor2ForwardDir  = ForwardDir;
   // }
}


//   
