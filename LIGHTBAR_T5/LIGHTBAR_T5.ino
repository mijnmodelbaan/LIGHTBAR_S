
/* 
   Sketch demonstrating switching lights on / off with F0 (FL)
   Basic / standard address of decoder is 140 after first run.
   All lights switch simultaneously.

 */


#include "DCC_Decoder.h"
#define kDCC_INTERRUPT      0


#include <EEPROM.h>
#define EEPROM_LIGHTMODE    0  // we store the last "light mode"
#define EEPROM_SWITCHSTATUS 1  // we store the last switchstatus
#define EEPROM_ADDRESS      2  // we store our dcc address (140)


#define PUSHBUTTON         11
#define LEARNINGBUTTON     12
#define LEDCONTROL         13


#define MAXLIGHTMODE        2   // mode0 = continuous, mode1 = flicketing, mode2 = roundrobin
int lightMode    =          0;  // mode0 = continuous
int switchStatus =        LOW;
int learningMode =        LOW;


    // Function button masks - F0 - F4
#define FN_BIT_00	0x10
#define FN_BIT_01	0x01
#define FN_BIT_02	0x02
#define FN_BIT_03	0x04
#define FN_BIT_04	0x08


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
   int   count;                  // Number of packets received
   byte  validBytes;             // Number of bytes in package
   byte  data[ 6 ];              // Valid packets are received

} DCCAccessoryAddress;

DCCAccessoryAddress gAddresses[ 15 ];


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * here we check the gAdresses[] caroussel and act accordingly
 */
void readAndResetTable()
{
      // Walk table and look for a matching packet
   for( int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); ++i )
   {
      if( gAddresses[ i ].validBytes > 0 )
      {
            // check for F0 - F4
         if ( gAddresses[ i ].data[ 1 ] >= 128 && gAddresses[ i ].data[ 1 ] <= 144 )
         {
            switchStatus = ( gAddresses[ i ].data[ 1 ] & FN_BIT_00 ? HIGH : LOW );
            EEPROM.write( EEPROM_SWITCHSTATUS, switchStatus );

            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for F5 - F8
         if ( gAddresses[ i ].data[ 1 ] >= 176 && gAddresses[ i ].data[ 1 ] <= 184 )
         {
            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for F9 - F12
         if ( gAddresses[ i ].data[ 1 ] >= 160 && gAddresses[ i ].data[ 1 ] <= 168 )
         {
            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for F13 - F20
         if ( gAddresses[ i ].data[ 1 ] == 222 )
         {
            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for F21 - F28
         if ( gAddresses[ i ].data[ 1 ] == 223 )
         {
            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for remainders
         if ( gAddresses[ i ].data[ 1 ] > 0x00 )
         {
            gAddresses[ i ].data[ 1 ] = 0x00;
         }

            // check for remainders
         if ( gAddresses[ i ].data[ 1 ] ==  0 )
         {
            gAddresses[ i ].validBytes = 0;
            gAddresses[ i ].count = 0;
         }
      }
   }
}


/**
 * this is just a function to show the state of the decoder via the onboard PCB led
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
void treatLight( int switchStatus, int lightMode )
{
      // switch all outputs at once on / off
   for ( int i = 1; i < 15; ++i ) 
   {
      digitalWrite( fpins[ i ], switchStatus );  // Switch the pin
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet handlers
//

// ALL packets are sent to the RawPacket handler. Returning true indicates that packet was handled.
// DCC_Decoder library starts watching for the next preamble.
// Returning false and the library will continue parsing packets and finds another handler to call.
//
boolean RawPacket_Handler(byte byteCount, byte* packetBytes)
{

   int myaddress = EEPROM.read( EEPROM_ADDRESS ) + ( EEPROM.read( EEPROM_ADDRESS + 1 ) * 256 );

   if ( myaddress == packetBytes[ 0 ] )
   {
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
      myaddress = 140;
      EEPROM.write( EEPROM_ADDRESS    , myaddress % 256 );
      EEPROM.write( EEPROM_ADDRESS + 1, myaddress / 256 );
      showAcknowledge( 5 );
   }

   lightMode = EEPROM.read(  EEPROM_LIGHTMODE  );
   if ( lightMode > MAXLIGHTMODE ) lightMode = 0;

   switchStatus = EEPROM.read( EEPROM_SWITCHSTATUS );
   if ( switchStatus != HIGH && switchStatus != LOW) switchStatus = LOW;

   showAcknowledge( lightMode + 1 );
   randomSeed ( analogRead ( A7 ) );

   DCC.SetRawPacketHandler(  RawPacket_Handler  );

   DCC.SetBasicAccessoryDecoderPacketHandler( BasicAccDecoderPacket_Handler,  true );

   DCC.SetBaselineControlPacketHandler( BaselineDecoderControlPacket_Handler, true );

   DCC.SetupDecoder( 0x01, 0x01, kDCC_INTERRUPT );

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
   static unsigned long timerOne   = 0;
   static unsigned long timerTwo   = 0;

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
   // check if we have new info in our caroussel
   if ( millis() - timerTwo > 100 )
   {
      readAndResetTable();
      timerTwo = millis();
   }

   ////////////////////////////////////////////////////////////////
   // check if we have light to change from state
   if ( millis() - timerOne >  50 ) 
   {
      treatLight( switchStatus, lightMode );
      timerOne = millis();
   }
}


//
