
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


//  The next line is there to prevent large in-rush currents during PWM cycly.
#define SOFTPWM_OUTPUT_DELAY

#include <SoftPWM.h>

SOFTPWM_DEFINE_CHANNEL( 1, DDRD, PORTD, PORTD3);  //Arduino pin D3 --> LS4
SOFTPWM_DEFINE_CHANNEL( 2, DDRD, PORTD, PORTD4);  //Arduino pin D4 --> LB3
SOFTPWM_DEFINE_CHANNEL( 3, DDRD, PORTD, PORTD5);  //Arduino pin D5 --> LS3
SOFTPWM_DEFINE_CHANNEL( 4, DDRD, PORTD, PORTD6);  //Arduino pin D6 --> LB1
SOFTPWM_DEFINE_CHANNEL( 5, DDRD, PORTD, PORTD7);  //Arduino pin D7 --> LS1
SOFTPWM_DEFINE_CHANNEL( 6, DDRB, PORTB, PORTB0);  //Arduino pin D8 --> AX1
SOFTPWM_DEFINE_CHANNEL( 7, DDRB, PORTB, PORTB1);  //Arduino pin D9 --> AX2
SOFTPWM_DEFINE_CHANNEL( 8, DDRB, PORTB, PORTB2);  //Arduino pin 10 --> AX3
SOFTPWM_DEFINE_CHANNEL( 9, DDRB, PORTB, PORTB3);  //Arduino pin 11 --> DNU
SOFTPWM_DEFINE_CHANNEL(10, DDRB, PORTB, PORTB4);  //Arduino pin 12 --> DNU
SOFTPWM_DEFINE_CHANNEL(11, DDRB, PORTB, PORTB5);  //Arduino pin 13 --> LED
SOFTPWM_DEFINE_CHANNEL(12, DDRC, PORTC, PORTC0);  //Arduino pin A0 --> LB2
SOFTPWM_DEFINE_CHANNEL(13, DDRC, PORTC, PORTC1);  //Arduino pin A1 --> LS2
SOFTPWM_DEFINE_CHANNEL(14, DDRC, PORTC, PORTC2);  //Arduino pin A2 --> AX4
SOFTPWM_DEFINE_CHANNEL(15, DDRC, PORTC, PORTC3);  //Arduino pin A3 --> LB4
SOFTPWM_DEFINE_CHANNEL(16, DDRC, PORTC, PORTC4);  //Arduino pin A4 --> LB5
SOFTPWM_DEFINE_CHANNEL(17, DDRC, PORTC, PORTC5);  //Arduino pin A5 --> LB6

SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS( 17, 100);  // Set 17 pulsed outputs

volatile unsigned int PWM_level = 0; // We start at the beginning


/* ******************************************************************************* */


#include "TaskScheduler.h"

Scheduler  taskTimer;


// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL MONITOR
// #define XX_DEBUGGR

#ifdef XX_DEBUGGR
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


/* **********************************************************************************


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


********************************************************************************** */


/* task used to broadcast data via websockets once a second - runs forever */
void taskT01Callback();
Task taskT01( 1000, TASK_FOREVER, &taskT01Callback, NULL, NULL, NULL, NULL );

/* this task starts the blinking of the LEDs */
void taskTL0Callback();
Task taskT02(  500, TASK_FOREVER, &taskTL0Callback, NULL, NULL, NULL, NULL );

/* task used to (re)connect MQTT - runs every 5 seconds, if WiFi connected */
void taskT03Callback();
Task taskT03( 5000, TASK_FOREVER, &taskT03Callback, NULL, NULL, NULL, NULL );


void taskL01Callback();
void taskL02Callback();
void taskL03Callback();
void taskL04Callback();
void taskL05Callback();
void taskL06Callback();
void taskL07Callback();
void taskL08Callback();
void taskL09Callback();
void taskL10Callback();
void taskL11Callback();
void taskL12Callback();
void taskL13Callback();
void taskL14Callback();
void taskL15Callback();
void taskL16Callback();
void taskLEDCallback();


/* ******************************************************************************* */


void setup()
{
   noInterrupts();

   DDRB = DDRB | 0b00111111;  // Set bits B[5-0] as outputs, leave the rest as-is.
   DDRC = DDRC | 0b00111111;  // Set bits C[5-0] as outputs, leave the rest as-is.
   DDRD = DDRD | 0b11111000;  // Set bits D[7-3] as outputs, leave the rest as-is.

   // Start SoftPWM with 60hz pwm frequency
   Palatis::SoftPWM.begin( 60);

   #if defined(XX_DEBUGGR) || defined(_MONITOR_)

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
      Serial.println("-------------------------------------");

   #endif

   /* initialize (and enable) all necessary schedules */
   taskTimer.init();  // Not really necessary - security

   taskTimer.addTask(taskT01);
   taskTimer.addTask(taskT02);
   taskTimer.addTask(taskT03);

   taskT01.enableDelayed(1500);
   taskT02.enableDelayed(1000);
   taskT03.enableDelayed(1500);

   interrupts();  // Ready to rumble....
}


/* ******************************************************************************* */


void loop()
{

   taskTimer.execute(); // Keep all the timers running...

}


/* ******************************************************************************* */


void taskT01Callback()
{
   ;
}


void taskTL0Callback()
{
   /* switch OFF all the LEDs */

   Palatis::SoftPWM.allOff(); // Set the PWM value of all channels to 0

   PWM_level += 10;  // Increase the PWM level by 10
   if ( PWM_level > Palatis::SoftPWM.PWMlevels() )
   {
      PWM_level = 10;
   }

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskTL0Callback");

#endif

   taskT02.setCallback(&taskL01Callback);
}


void taskL01Callback()
{
   /* switch ON the first LED */

   Palatis::SoftPWM.set(   1, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL01Callback");

#endif

   taskT02.setCallback(&taskL02Callback);
}


void taskL02Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   2, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL02Callback");

#endif

   taskT02.setCallback(&taskL03Callback);
}


void taskL03Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   3, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL03Callback");

#endif

   taskT02.setCallback(&taskL04Callback);
}


void taskL04Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   4, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL04Callback");

#endif

   taskT02.setCallback(&taskL05Callback);
}


void taskL05Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   5, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL05Callback");

#endif

   taskT02.setCallback(&taskL06Callback);
}


void taskL06Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   6, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL06Callback");

#endif

   taskT02.setCallback(&taskL07Callback);
}


void taskL07Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   7, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL07Callback");

#endif

   taskT02.setCallback(&taskL08Callback);
}


void taskL08Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   8, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL08Callback");

#endif

   taskT02.setCallback(&taskL09Callback);
}


void taskL09Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(   9, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL09Callback");

#endif

   taskT02.setCallback(&taskL10Callback);
}


void taskL10Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  10, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL10Callback");

#endif

   taskT02.setCallback(&taskL11Callback);
}


void taskL11Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  11, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL11Callback");

#endif

   taskT02.setCallback(&taskL12Callback);
}


void taskL12Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  12, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL12Callback");

#endif

   taskT02.setCallback(&taskL13Callback);
}


void taskL13Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  13, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL13Callback");

#endif

   taskT02.setCallback(&taskL14Callback);
}


void taskL14Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  14, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL14Callback");

#endif

   taskT02.setCallback(&taskL15Callback);
}


void taskL15Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  15, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL15Callback");

#endif

   taskT02.setCallback(&taskL16Callback);
}


void taskL16Callback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  16, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskL16Callback");

#endif

   taskT02.setCallback(&taskLEDCallback);
}

void taskLEDCallback()
{
   /* switch ON the next  LED */

   Palatis::SoftPWM.set(  17, PWM_level);

#if defined(XX_DEBUGGR) || defined(_TEST_)

   _PL("TaskScheduler taskLEDCallback");

#endif

   taskT02.setCallback(&taskTL0Callback);
}



void taskT03Callback()
{
   ;
}


/* ---   END   --- */
