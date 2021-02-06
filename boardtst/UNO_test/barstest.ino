

/* -------------   This sketch pulses 17 outputs in a continuous loop:   -------------
 * 
 * PD3, PD4, PD5, PD6, PD7, PB0, PB1, PB2, PB3, PB4, PC0, PC1, PC2, PC3, PC4, PC5, PB5
 * ----------------------------------------------------------------------------------- */


#include "avr\io.h"
#include "TaskScheduler.h"


Scheduler  taskTimer;

// Debug and Test options
#define _DEBUG_
//#define _TEST_

#ifdef _DEBUG_
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
Task taskT02( 1000, TASK_FOREVER, &taskTL0Callback, NULL, NULL, NULL, NULL );

/* task used to (re)connect MQTT - runs every 5 seconds, if WiFi connected */
void taskT03Callback();
Task taskT03( 5000, TASK_FOREVER, &taskT03Callback, NULL, NULL, NULL, NULL );


void setup()
{

   DDRB = DDRB | 0b00111111;  // Set bits B[5-0] as outputs, leave the rest as-is.
   DDRC = DDRC | 0b00111111;  // Set bits C[5-0] as outputs, leave the rest as-is.
   DDRD = DDRD | 0b11111000;  // Set bits D[7-3] as outputs, leave the rest as-is.

   Serial.begin(115200);

   while (!Serial)
   {
      ; // wait for Serial port to connect. Needed for native USB port.
   }
   Serial.flush();    // Wait for all the rubbish to finish displaying.
   while (Serial.available() > 0) {
      Serial.read();
   }

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler Starting");

#endif


   /* initialize (and enable) all necessary schedules */
   taskTimer.init();  // Not really necessary - security

   taskTimer.addTask(taskT01);
   taskTimer.addTask(taskT02);
   taskTimer.addTask(taskT03);

   taskT01.enableDelayed(1000);
   taskT02.enableDelayed(1500);
   taskT03.enableDelayed( 500);
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

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskTL0Callback");

#endif

   taskT02.setCallback(&taskL01Callback);
}


void taskL01Callback()
{
   /* switch ON the first LED */

   PORTD = PORTD | 0b00001000;   // PD3

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL01Callback");

#endif

   taskT02.setCallback(&taskL02Callback);
}


void taskL02Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b00010000;   // PD4

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL02Callback");

#endif

   taskT02.setCallback(&taskL03Callback);
}


void taskL03Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b00100000;   // PD5

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL03Callback");

#endif

   taskT02.setCallback(&taskL04Callback);
}


void taskL04Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b01000000;   // PD6

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL04Callback");

#endif

   taskT02.setCallback(&taskL05Callback);
}


void taskL05Callback()
{
   /* switch ON the next  LED */

   PORTD = PORTD | 0b10000000;   // PD7

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL05Callback");

#endif

   taskT02.setCallback(&taskL06Callback);
}


void taskL06Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000001;   // PB0

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL06Callback");

#endif

   taskT02.setCallback(&taskL07Callback);
}


void taskL07Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000010;   // PB1

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL07Callback");

#endif

   taskT02.setCallback(&taskL08Callback);
}


void taskL08Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00000100;   // PB2

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL08Callback");

#endif

   taskT02.setCallback(&taskL09Callback);
}


void taskL09Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00001000;   // PB3

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL09Callback");

#endif

   taskT02.setCallback(&taskL10Callback);
}


void taskL10Callback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00010000;   // PB4

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL10Callback");

#endif

   taskT02.setCallback(&taskL11Callback);
}


void taskL11Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000001;   // PC0

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL11Callback");

#endif

   taskT02.setCallback(&taskL12Callback);
}


void taskL12Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000010;   // PC1

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL12Callback");

#endif

   taskT02.setCallback(&taskL13Callback);
}


void taskL13Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00000100;   // PC2

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL13Callback");

#endif

   taskT02.setCallback(&taskL14Callback);
}


void taskL14Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00001000;   // PC3

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL14Callback");

#endif

   taskT02.setCallback(&taskL15Callback);
}


void taskL15Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00010000;   // PC4

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL15Callback");

#endif

   taskT02.setCallback(&taskL16Callback);
}


void taskL16Callback()
{
   /* switch ON the next  LED */

   PORTC = PORTC | 0b00100000;   // PC5

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskL16Callback");

#endif

   taskT02.setCallback(&taskLEDCallback);
}

void taskLEDCallback()
{
   /* switch ON the next  LED */

   PORTB = PORTB | 0b00100000;   // PB5

#if defined(_DEBUG_) || defined(_TEST_)

   _PL("TaskScheduler taskLEDCallback");

#endif

   taskT02.setCallback(&taskTL0Callback);
}



void taskT03Callback()
{
   ;
}
