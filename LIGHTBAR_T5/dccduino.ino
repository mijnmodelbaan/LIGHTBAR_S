
#include "DCC_Decoder.h"
#define kDCC_INTERRUPT      0


#include <EEPROM.h>
#define EEPROM_LIGHTMODE    0  // we store the last "light mode" selected by the user (via the push button)
#define EEPROM_SWITCHSTATUS 1  // we store the last switch status (send via dcc)
#define EEPROM_ADDRESS      2  // we store our dcc address


#define PUSHBUTTON         11
#define LEARNINGBUTTON     12
#define LEDCONTROL         13

#define MAXLIGHTMODE        2   // mode0 = continuous, mode1 = flicketing, mode2 = roundrobin
int lightMode    =          0;
int switchStatus =       HIGH;
int learningMode =        LOW;


byte fpins[] = { 11,   7,  15,   5,   3,   6,  14,   4,  17,  18,  19,   8,   9,  10,  16,  13,  12};
//  pinnames =  DNU, PD7, PC1, PD5, PD3, PD6, PC0, PD4, PC3, PC4, PC5, PB0, PB1, PB2, PC2, PB5, DNU <
//  used as  =       LS1, LS2, LS3, LS4, LB1, LB2, LB3, LB4, LB5, LB6, AX1, AX2, AX3, AX4, LED      <
//
//    AD0 = LB2    PD0 = Rx1    PB0 =   AX1
//    AD1 = LS2    PD1 = Tx0    PB1 =   AX2
//    AD2 = AX4    PD2 = DCC    PB2 =   AX3
//    AD3 = LB4    PD3 = LS4    PB3 =  ICSP
//    AD4 = LB5    PD4 = LB3    PB4 =  ICSP
//    AD5 = LB6    PD5 = LS3    PB5 =  ICSP
//    AD6 = DNU    PD6 = LB1    PB6 = Xtal1
//    AD7 = DNU    PD7 = LS1    PB7 = Xtal2
//
///////////////////////////////////////////////////////////////////


typedef struct
{
   int               count;                  // Used internally for debugging
   byte              validBytes;             // Used internally for debugging
   byte              data[ 6 ];              // Used internally for debugging

} DCCAccessoryAddress;

DCCAccessoryAddress gAddresses[ 15 ];


/**
 * this is just a function to show via the onboard PCB led, the state of the decoder
 */
void showAcknowledge( int nb ) 
{
   for (int i = 0; i < nb; ++i)
   {
      digitalWrite( LEDCONTROL, HIGH );   // turn the LED on (HIGH is the voltage level)
      delay( 250 );                       // wait for 250 milliseconds
      digitalWrite( LEDCONTROL,  LOW );   // turn the LED off by making the voltage  LOW
      delay( 250 );                       // wait for 250 milliseconds
  }
}


/**
 * work done on connected lights depending on their state
 */
void treatLight(int switchStatus,int lightMode) 
{
   static int valLight[ 3 ] = {0, 0, 0};
   static int blink = -1;
   static int chain =  0;

   int tmp;

   switch( lightMode ) 
   {
      /*
      * mode 0:
      * - switch status = HIGH: we switch  on the light in a fading  on way.
      * - switch status =  LOW: we switch off the light in a fading off way.
      */
      case 0:
         if ( switchStatus == HIGH) 
         {
            for ( int i = 0; i < 3; ++i ) if ( valLight[ i ] < 255 ) valLight[ i ] += 5;
         } else {
            for ( int i = 0; i < 3; ++i ) if ( valLight[ i ] >   0 ) valLight[ i ] -= 5;
         }
         break;
      /*
      * mode 1:
      * - switch status = HIGH: we switch  on the light in a fading  on way.
      * - switch status =  LOW: we switch off the light in a fading off way.
      * BUT randomly (1% of the time), we switch off a light and switch it on just after, to flicker it
      */
      case 1:
         if ( switchStatus == HIGH ) 
         {
            for ( int i = 0; i < 3; ++i ) if ( valLight[ i ] < 255) valLight[ i ] += 5;
         } else {
            for ( int i = 0; i < 3; ++i ) if ( valLight[ i ] >   0) valLight[ i ] -= 5;
         }
         /* 1% chance to flicker 1 of the 3 lights */
         if (( blink = random( 100 )) < 3) 
         {
            tmp = valLight[ blink ];
            valLight[ blink ] = 0;
            // analogWrite( LIGHT0, valLight[ 0 ]);
            // analogWrite( LIGHT1, valLight[ 1 ]);
            // analogWrite( LIGHT2, valLight[ 2 ]);
            delay( 50 );
            valLight[ blink ] = tmp;
         }
         break;
      /*
      * mode 2: we switch on the first light, then the second light, then the 3rd light
      */
      case 2:
         chain++;
         if ( chain == 20 ) 
         {
            valLight[ 0 ] =   0;
            valLight[ 1 ] = 255;
         } else if ( chain == 40 ) 
         {
            valLight[ 1 ] =   0;
            valLight[ 2 ] = 255;
         } else if ( chain == 60 ) 
         {
            valLight[ 2 ] =   0;
            valLight[ 0 ] = 255;
            chain = 0;
         }
         break;
   }
   // analogWrite( LIGHT0, valLight[ 0 ] );
   // analogWrite( LIGHT1, valLight[ 1 ] );
   // analogWrite( LIGHT2, valLight[ 2 ] );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet handlers
//

// ALL packets are sent to the RawPacket handler. Returning true indicates that packet was handled. DCC library starts watching for 
// next preamble. Returning false and the library continues parsing packets and finds another handler to call.
boolean RawPacket_Handler(byte byteCount, byte* packetBytes)
{

   int myaddress = EEPROM.read( EEPROM_ADDRESS ) + ( EEPROM.read( EEPROM_ADDRESS + 1 ) * 256 );

   if ( myaddress == packetBytes[ 0 ] )
   {

      for( int j = 0; j < byteCount; j++ )
      {
         Serial.print( packetBytes[j], DEC );
         Serial.print( "\t");
      }
      Serial.println();

         // Walk table and look for a matching packet
      for( int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); ++i )
      {
         if( gAddresses[ i ].validBytes )
         {
               // Not an empty slot. Does this slot match this packet? If so, bump count.
            if( gAddresses[ i ].validBytes == byteCount )
            {
               char isPacket = true;
               for( int j = 0; j < byteCount; ++j )
               {
                  if( gAddresses[ i ].data[ j ] != packetBytes[ j ] )
                  {
                     isPacket = false;
                     break;
                  } 
               }
               if( isPacket )
               {
                  gAddresses[ i ].count++;
                  return false;
               }
            }
         } 
         else 
         {
               // Empty slot, just copy over data
            gAddresses[ i ].count++;
            gAddresses[ i ].validBytes = byteCount;
            for( int j = 0; j < byteCount; ++j )
            {
               gAddresses[ i ].data[ j ] = packetBytes[ j ];
            }
            return false;
         }
      }
   } 

   return false;
}


/**
 * when a DCC packet is received, this function is called
 * for our needs, we decode the address:
 * - if we are in learning mode, we store this address as our new address
 * - if we are in normal mode, we switch on, or off light and relay, depending on the "enable" state command
 */
void BasicAccDecoderPacket_Handler( int address, boolean activate, byte data )
{
   // Convert NMRA packet address format to human address
   address -= 1;
   address *= 4;
   address += 1;
   address += ( data & 0x06 ) >> 1;

   int enable = ( data & 0x01 ) ? HIGH : LOW;

   // if we are in learning mode, we store our dcc address
   if ( learningMode == HIGH ) 
   {
//       EEPROM.write( EEPROM_ADDRESS    , address % 256 );
//       EEPROM.write( EEPROM_ADDRESS + 1, address / 256 );
      showAcknowledge( 3 );
   } 
   else 
   {
      // if we are in normal mode, then... we act accordingly
      int myaddress = EEPROM.read( EEPROM_ADDRESS ) + ( EEPROM.read( EEPROM_ADDRESS + 1 ) * 256 );

      if ( myaddress == address ) 
      {
//          switchStatus = enable;
//          EEPROM.write( EEPROM_SWITCHSTATUS, switchStatus );
//          // digitalWrite( RELAY, switchStatus );
      }
   }
}


/**
 * when a DCC packet is received, this function is called
 */
void BaselineDecoderControlPacket_Handler( int address, int speed, int direction )
{
   // if we are in normal mode, then... we act accordingly
   // this can be used later (for a directional lightning)
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// standard arduino initialisation function
//
void setup() {

   pinMode( PUSHBUTTON,     INPUT );
   pinMode( LEARNINGBUTTON, INPUT );

   // initialize the digital pins as outputs
   for ( int i = 1; i < 15; ++i ) 
   {
      digitalWrite( fpins[ i ], 0 );  // Switch the pin off first.
      pinMode( fpins[ i ], OUTPUT );  // Then set it as an output.
   }

   int myaddress = EEPROM.read( EEPROM_ADDRESS ) + ( EEPROM.read( EEPROM_ADDRESS + 1 ) * 256 );
   if ( myaddress <= 0 or myaddress >= 0x27FF )
   {
      myaddress = 40;
      EEPROM.write( EEPROM_ADDRESS    , myaddress % 256 );
      EEPROM.write( EEPROM_ADDRESS + 1, myaddress / 256 );
      showAcknowledge( 5 );
   }

   lightMode = EEPROM.read(  EEPROM_LIGHTMODE  );
   if ( lightMode > MAXLIGHTMODE ) lightMode = 0;

   showAcknowledge( lightMode + 1 );
   randomSeed( analogRead( A3  )  );

   DCC.SetRawPacketHandler(  RawPacket_Handler  );

   DCC.SetBasicAccessoryDecoderPacketHandler( BasicAccDecoderPacket_Handler,  true );

   DCC.SetBaselineControlPacketHandler( BaselineDecoderControlPacket_Handler, true );

   DCC.SetupDecoder( 0x01, 0x01, kDCC_INTERRUPT );

   Serial.begin(115200);
   Serial.println( myaddress, DEC );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void loop() 
{
   static int pushbuttonOldval     = 0;
   static int pushbuttonVal        = 0;
   static int learningbuttonOldval = 0;
   static int learningbuttonVal    = 0;
   static unsigned long timer      = 0;
   
   ////////////////////////////////////////////////////////////////
   // Loop DCC library
   DCC.loop();

   ////////////////////////////////////////////////////////////////
   // check if the push button has been pressed
   pushbuttonVal = digitalRead( PUSHBUTTON );
   if ( pushbuttonVal == LOW && pushbuttonOldval != pushbuttonVal) 
   {
   //    lightMode++;
   //    if ( lightMode > MAXLIGHTMODE ) lightMode = 0;
   //    EEPROM.write( EEPROM_LIGHTMODE, lightMode );
   //    showAcknowledge( lightMode + 1);
   }
   pushbuttonOldval = pushbuttonVal;

   ////////////////////////////////////////////////////////////////
   // check if the learning button has been enabled
   learningbuttonVal = digitalRead( LEARNINGBUTTON );
   if ( learningbuttonOldval != learningbuttonVal) 
   {
   //    learningMode = learningbuttonVal;
   //    if ( learningMode == HIGH ) showAcknowledge( 3 );
   }
   learningbuttonOldval = learningbuttonVal;
   
   ////////////////////////////////////////////////////////////////
   // check if we have light to change from state
   if ( millis() - timer > 50 ) 
   {
      // treatLight( switchStatus, lightMode );
      timer = millis();
   }
}


//
