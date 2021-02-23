

/* -------------   This sketch pulses 17 outputs in a continuous loop:   -------------
 * 
 * PD3, PD4, PD5, PD6, PD7, PB0, PB1, PB2, PB3, PB4, PC0, PC1, PC2, PC3, PC4, PC5, PB5
 * ----------------------------------------------------------------------------------- */


#include "avr\io.h"
#include "TaskScheduler.h"


Scheduler  taskTimer;

// Debug and Test options
#define _DEBUG_

#if defined(_DEBUG_) || defined(_TEST_)
   #define _PP(a) Serial.print(a);
   #define _PL(a) Serial.println(a);
#else
   #define _PP(a)
   #define _PL(a)
#endif


/* task used to broadcast data via websockets once a second - runs forever */
void taskT01Callback();
Task taskT01( 1000, TASK_FOREVER, &taskT01Callback, NULL, NULL, NULL, NULL );

/* this task starts the blinking of the LEDs */
void taskTL0Callback();
Task taskT02(  100, TASK_FOREVER, &taskTL0Callback, NULL, NULL, NULL, NULL );

/* task used to (re)connect MQTT - runs every 5 seconds, if WiFi connected */
void taskT03Callback();
Task taskT03( 5000, TASK_FOREVER, &taskT03Callback, NULL, NULL, NULL, NULL );


void setup()
{

   DDRB = DDRB | 0b00111111;  // Set bits B[5-0] as outputs, leave the rest as-is.
   DDRC = DDRC | 0b00111111;  // Set bits C[5-0] as outputs, leave the rest as-is.
   DDRD = DDRD | 0b11111000;  // Set bits D[7-3] as outputs, leave the rest as-is.

   #if defined(_DEBUG_) || defined(_TEST_)
  
      Serial.begin(115200);
  
      while (!Serial)
      {
         ; // wait for Serial port to connect. Needed for native USB port.
      }
      Serial.flush();    // Wait for all the rubbish to finish displaying.
      while (Serial.available() > 0) {
         Serial.read();
      }
  
   #endif
  
   _PL("TaskScheduler Starting");


   /* initialize (and enable) all necessary schedules */
   taskTimer.init();  // Not really necessary - security

   taskTimer.addTask( taskT01 );
   taskTimer.addTask( taskT02 );
   taskTimer.addTask( taskT03 );

   taskT01.enableDelayed( 1500);
   taskT02.enableDelayed( 1250);
   taskT03.enableDelayed( 1500);
}


void loop()
{

   taskTimer.execute(); // Keep all the timers running...
}


void taskT01Callback()
{
   ;
}


void taskTL0Callback()
{
   /* switch OFF all the LEDs */

   PORTB = PORTB & 0b11000000;  // Reset bits B[5-0], leave the rest as-is.
   PORTC = PORTC & 0b11000000;  // Reset bits C[5-0], leave the rest as-is.
   PORTD = PORTD & 0b00000111;  // Reset bits D[7-3], leave the rest as-is.

   _PL("TaskScheduler taskTL0Callback");

   taskT02.setCallback(&taskL01Callback);
}


void taskL01Callback()
{
   /* switch ON the first LED */

   PORTD = PORTD | 0b00001000;   // PD3

   _PL("TaskScheduler taskL01Callback");

   taskT02.setCallback(&taskL02Callback);
}


void taskL02Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b00010000;   // PD4

      _PL("TaskScheduler taskL02Callback");

   taskT02.setCallback(&taskL03Callback);
}


void taskL03Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b00100000;   // PD5

   _PL("TaskScheduler taskL03Callback");

   taskT02.setCallback(&taskL04Callback);
}


void taskL04Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b01000000;   // PD6

   _PL("TaskScheduler taskL04Callback");

   taskT02.setCallback(&taskL05Callback);
}


void taskL05Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b10000000;   // PD7

   _PL("TaskScheduler taskL05Callback");

   taskT02.setCallback(&taskL06Callback);
}


void taskL06Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000001;   // PB0

   _PL("TaskScheduler taskL06Callback");

   taskT02.setCallback(&taskL07Callback);
}


void taskL07Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000010;   // PB1

   _PL("TaskScheduler taskL07Callback");

   taskT02.setCallback(&taskL08Callback);
}


void taskL08Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000100;   // PB2

   _PL("TaskScheduler taskL08Callback");

   taskT02.setCallback(&taskL09Callback);
}


void taskL09Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00001000;   // PB3

   _PL("TaskScheduler taskL09Callback");

   taskT02.setCallback(&taskL10Callback);
}


void taskL10Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00010000;   // PB4

   _PL("TaskScheduler taskL10Callback");

   taskT02.setCallback(&taskL11Callback);
}


void taskL11Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000001;   // PC0

   _PL("TaskScheduler taskL11Callback");

   taskT02.setCallback(&taskL12Callback);
}


void taskL12Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000010;   // PC1

   _PL("TaskScheduler taskL12Callback");

   taskT02.setCallback(&taskL13Callback);
}


void taskL13Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000100;   // PC2

   _PL("TaskScheduler taskL13Callback");

   taskT02.setCallback(&taskL14Callback);
}


void taskL14Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00001000;   // PC3

   _PL("TaskScheduler taskL14Callback");

   taskT02.setCallback(&taskL15Callback);
}


void taskL15Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00010000;   // PC4

   _PL("TaskScheduler taskL15Callback");

   taskT02.setCallback(&taskL16Callback);
}


void taskL16Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00100000;   // PC5

   _PL("TaskScheduler taskL16Callback");

   taskT02.setCallback(&taskLEDCallback);
}

void taskLEDCallback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00100000;   // PB5

   _PL("TaskScheduler taskLEDCallback");

   taskT02.setCallback(&taskTL0Callback);
}



void taskT03Callback()
{
   ;
}
