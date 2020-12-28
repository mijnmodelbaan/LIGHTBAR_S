
//  TODO: declare all variables

//  TODO: create the Setup() loop

//  TODO: create the main Loop().


#include <SoftPWM.h>

// SOFTPWM_DEFINE_CHANNEL( 0, DDRD, PORTD, PORTD0);  //Arduino pin  0
// SOFTPWM_DEFINE_CHANNEL( 1, DDRD, PORTD, PORTD1);  //Arduino pin  1
// SOFTPWM_DEFINE_CHANNEL( 2, DDRD, PORTD, PORTD2);  //Arduino pin  2
// SOFTPWM_DEFINE_CHANNEL( 3, DDRD, PORTD, PORTD3);  //Arduino pin  3
// SOFTPWM_DEFINE_CHANNEL( 4, DDRD, PORTD, PORTD4);  //Arduino pin  4
// SOFTPWM_DEFINE_CHANNEL( 5, DDRD, PORTD, PORTD5);  //Arduino pin  5
// SOFTPWM_DEFINE_CHANNEL( 6, DDRD, PORTD, PORTD6);  //Arduino pin  6
// SOFTPWM_DEFINE_CHANNEL( 7, DDRD, PORTD, PORTD7);  //Arduino pin  7
// SOFTPWM_DEFINE_CHANNEL( 8, DDRB, PORTB, PORTB0);  //Arduino pin  8
// SOFTPWM_DEFINE_CHANNEL( 9, DDRB, PORTB, PORTB1);  //Arduino pin  9
// SOFTPWM_DEFINE_CHANNEL(10, DDRB, PORTB, PORTB2);  //Arduino pin 10
// SOFTPWM_DEFINE_CHANNEL(11, DDRB, PORTB, PORTB3);  //Arduino pin 11
// SOFTPWM_DEFINE_CHANNEL(12, DDRB, PORTB, PORTB4);  //Arduino pin 12
// SOFTPWM_DEFINE_CHANNEL(13, DDRB, PORTB, PORTB5);  //Arduino pin 13
// SOFTPWM_DEFINE_CHANNEL(14, DDRC, PORTC, PORTC0);  //Arduino pin A0
// SOFTPWM_DEFINE_CHANNEL(15, DDRC, PORTC, PORTC1);  //Arduino pin A1
// SOFTPWM_DEFINE_CHANNEL(16, DDRC, PORTC, PORTC2);  //Arduino pin A2
// SOFTPWM_DEFINE_CHANNEL(17, DDRC, PORTC, PORTC3);  //Arduino pin A3
// SOFTPWM_DEFINE_CHANNEL(18, DDRC, PORTC, PORTC4);  //Arduino pin A4
// SOFTPWM_DEFINE_CHANNEL(19, DDRC, PORTC, PORTC5);  //Arduino pin A5


#include <NmraDcc.h>


// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL MONITOR
#define _DEBUG_


#ifdef _DEBUG_
   #define _PP(a) Serial.print( a );
   #define _PL(a) Serial.println(a);
#else
   #define _PP(a)
   #define _PL(a)
#endif


/* ******************************************************************************* */


int tim_delay = 500;
int numfpins  =  12;
byte fpins [] = { 14,  15,  16,  17,  18,  19,   5,   6,   9,  10,  13, 4};
//  pinnames:  > PC0, PC1, PC2, PC3, PC4, PC5, PD5, PD6, PB1, PB2, PB5 <

const int FunctionPinRx  =  0;  // PD0
const int FunctionPinTx  =  1;  // PD1
const int FunctionPinDcc =  2;  // PD2
const int FunctionPinLed = 13;  // PB5

const int FunctionPin0   =  3;  // PD3
const int FunctionPin1   =  4;  // PD4
const int FunctionPin2   =  5;  // PD5
const int FunctionPin3   =  6;  // PD6
const int FunctionPin4   =  7;  // PD7
const int FunctionPin5   =  8;  // PB0
const int FunctionPin6   =  9;  // PB1
const int FunctionPin7   = 10;  // PB2
const int FunctionPin8   = 11;  // PB3
const int FunctionPin9   = 12;  // PB4
const int FunctionPin10  = 14;  // PC0 - LS1
const int FunctionPin11  = 15;  // PC1 - LS2
const int FunctionPin12  = 16;  // PC2 - LS3
const int FunctionPin13  = 17;  // PC3 - LS4
const int FunctionPin14  = 18;  // PC4 - LS5
const int FunctionPin15  = 19;  // PC5 - LS6

/* ******************************************************************************* */


#define SET_CV_Address       24  // THIS ADDRESS IS FOR SETTING CV'S  (LIKE A LOCO)
#define Accessory_Address    40  // THIS ADDRESS IS THE START OF THE SWITCHES RANGE

uint8_t CV_DECODER_MASTER_RESET  = 120; // THIS IS THE CV ADDRESS OF THE FULL RESET
#define CV_To_Store_SET_CV_Address 121
#define CV_Accessory_Address       CV_ACCESSORY_DECODER_ADDRESS_LSB

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

   { 30,   4}, // GEN Number of outputs  (16 maximum)
   { 31,  50}, //     Maximum level outputs (100 max)
   { 32, 100}, //     Waitmicros  (250 * 100,000 max)
   { 33,   4}, //     Waitmicros divider (standard 4)
   { 34, 999}, //     Standard blink interval  (1000)
   { 35,   1}, // LS1 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 36, 100}, //     Maximum level this output
   { 37, 100}, //     Waitmicros output highend
   { 38,   4}, //     Waitmicros output divider
   { 39, 750}, //     Blinkinterval this output
   { 40,   3}, // LS2 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 41, 100}, //     Maximum level this output
   { 42, 100}, //     Waitmicros output highend
   { 43,   4}, //     Waitmicros output divider
   { 44, 750}, //     Blinkinterval this output
   { 45,   1}, // LS3 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 46, 100}, //     Maximum level this output
   { 47, 100}, //     Waitmicros output highend
   { 48,   4}, //     Waitmicros output divider
   { 49, 750}, //     Blinkinterval this output
   { 50,   5}, // LS4 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 51, 100}, //     Maximum level this output
   { 52, 100}, //     Waitmicros output highend
   { 53,   4}, //     Waitmicros output divider
   { 54, 750}, //     Blinkinterval this output
   { 55,   0}, // LS5 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 56,   0}, //     Maximum level this output
   { 57,   0}, //     Waitmicros output highend
   { 58,   0}, //     Waitmicros output divider
   { 59,   0}, //     Blinkinterval this output
   { 60,   0}, // LS6 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 61,   0}, //     Maximum level this output
   { 62,   0}, //     Waitmicros output highend
   { 63,   0}, //     Waitmicros output divider
   { 64,   0}, //     Blinkinterval this output
   { 65,   0}, // LS7 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 66,   0}, //     Maximum level this output
   { 67,   0}, //     Waitmicros output highend
   { 68,   0}, //     Waitmicros output divider
   { 69,   0}, //     Blinkinterval this output
   { 70,   0}, // LS8 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 71,   0}, //     Maximum level this output
   { 72,   0}, //     Waitmicros output highend
   { 73,   0}, //     Waitmicros output divider
   { 74,   0}, //     Blinkinterval this output
   { 75,   0}, // LS9 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 76,   0}, //     Maximum level this output
   { 77,   0}, //     Waitmicros output highend
   { 78,   0}, //     Waitmicros output divider
   { 79,   0}, //     Blinkinterval this output
   { 80,   0}, // L10 0 = Off, 1 = On, 2 = Blink Off, 3 = Blink On, 4 = Fade Off, 5 = Fade On
   { 81,   0}, //     Maximum level this output
   { 82,   0}, //     Waitmicros output highend
   { 83,   0}, //     Waitmicros output divider
   { 84,   0}, //     Blinkinterval this output
};

uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);

int vol = 0;
int vul = 0;
volatile unsigned short rxInterruptCount = 0;

/* ******************************************************************************* */

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

volatile bool counting;
volatile unsigned long events;

unsigned long startTime;
const unsigned long INTERVAL = 1000;  // 1 second

void showResults ()
{
   Serial.print ("I counted: ");
   Serial.println (events, DEC);
}  // end of showResults

void setup()
{

//   noInterrupts();
//   PCMSK0 |= B00000110;    // turn on PCINT1 en PCINT2
//   PCIFR  |= B00000001;    // clear outstanding interrupts
//   PCICR  |= B00000001;    // turn on PCIE0 PCINT[7:0]

//   PCICR  |= (1<<PCIE0);    //enable group interrupts on PORTB - PCINT[7:0]
//   PCMSK0 |= (1<<PCINT1);   //enable interrupt pin  D9 (PB1 or OC1A/PCINT1)
//   PCMSK0 |= (1<<PCINT2);   //enable interrupt pin D10 (PB2 or OC1B/PCINT2)

//   DDRB = DDRB | 0b00000110;
//   DDRC = DDRC | 0b00001111;
//   DDRD = DDRD | 0b01111100;
//   interrupts();

//PORTB = (0<<PB2)|(0<<PB1);
//DDRB = (1<<DDB2)|(1<<DDB1);

//PORTD = (0<<PD6)|(0<<PD5)|(0<<PD4)|(0<<PD3);
//DDRD = (1<<DDD6)|(1<<DDD5)|(1<<DDD4)|(1<<DDD3);

//   noInterrupts();
//   PCICR  |= 0b00000100;
//   PCMSK2 |= 0b00001000;

//   PORTB = PORTB & 0b11111001;
//   DDRB  = DDRB  | 0b00000110;

//   DDRC = DDRC | 0b00001111;

//   PORTD = PORTD & 0b10000111;
//   DDRD  = DDRD  | 0b01111000;
   interrupts();



   #if defined(_DEBUG_) || defined(_TEST_)

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
      _PL("-------------------------------------");
      _PL(""); // An extra empty line for clearness

   #endif

   // initialize the digital pins as outputs
   for (int i = 0; i < numfpins; i++) 
   {
      digitalWrite(fpins[i], 0);  // Switch the pin off first.
      pinMode(fpins[i], OUTPUT);  // Then make it an output.
   }


   // Switch on the LED to signal we're ready.
   digitalWrite(FunctionPinLed, 0);
   delay (tim_delay);
   digitalWrite(FunctionPinLed, 1);

   _PL(""); // An extra empty line for clearness
   _PL("*************************************");

   analogWrite(FunctionPin6, 127);  // Start PWM
   analogWrite(FunctionPin0, 227);  // Start PWM

}

/* ******************************************************************************* */


void loop()
{

//   // set the brightness of FunctionPinX:
//   analogWrite(FunctionPin6, brightness);
//   analogWrite(FunctionPin7, brightness);

   // change the brightness for next time through the loop:
   brightness = brightness + fadeAmount;

   // reverse the direction of the fading at the ends of the fade:
   if (brightness <= 0 || brightness >= 255) {
     fadeAmount = -fadeAmount;
   }

   // wait for some milliseconds to see the dimming effect.
   delay(100);

   if (counting)
   {
      if (millis () - startTime < INTERVAL)
         return;

      counting = false;
      showResults ();
   }  // end of if

   noInterrupts ();
   events = 0;
   startTime = millis ();
//   PCIFR  |= 0b00000001;    // clear outstanding interrupts
//   EIFR = bit (INTF0);  // clear flag for interrupt 0
   counting = true;
   interrupts ();

}

/* ******************************************************************************* */


ISR(PCINT0_vect)
{

   if (counting) events++;

//   if (PINB & B00000010)
//   {
//      vol = digitalRead(FunctionPin6);
//      digitalWrite(FunctionPin0, vol);
//      digitalWrite(FunctionPin1, vol);
//   }

//   if (PINB & B00000100)
//   {
//      vul = digitalRead(FunctionPin7);
//      digitalWrite(FunctionPin2, vul);
//      digitalWrite(FunctionPin3, vul);
//   }

}

ISR(PCINT1_vect)
{

   if (counting) events++;

//   if (PINB & B00000010)
//   {
//      vol = digitalRead(FunctionPin6);
//      digitalWrite(FunctionPin0, vol);
//      digitalWrite(FunctionPin1, vol);
//   }

//   if (PINB & B00000100)
//   {
//      vul = digitalRead(FunctionPin7);
//      digitalWrite(FunctionPin2, vul);
//      digitalWrite(FunctionPin3, vul);
//   }

}

ISR(PCINT2_vect)
{

   if (counting) events++;

//   if (PINB & B00000010)
//   {
//      vol = digitalRead(FunctionPin6);
//      digitalWrite(FunctionPin0, vol);
//      digitalWrite(FunctionPin1, vol);
//   }

//   if (PINB & B00000100)
//   {
//      vul = digitalRead(FunctionPin7);
//      digitalWrite(FunctionPin2, vul);
//      digitalWrite(FunctionPin3, vul);
//   }

}

//
