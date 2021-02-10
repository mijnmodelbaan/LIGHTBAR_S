
#include <DCC_Decoder.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL MONITOR OR CHANGE PLATFORMIO.INI
//
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO USE AS A '6LAMP PCB WITH AUX PORTS' ELSE IT'S A '3LAMP W/O AUX PORTS'
//
#define _6LAMP_


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Defines and structures
//
#define kDCC_INTERRUPT  0

typedef struct
{
    int               address;                // Address to respond to
    byte              output;                 // State of output 1=on, 0=off
    int               outputPin;              // Arduino output pin to drive
    boolean           isDigital;              // true=digital, false=analog. If analog must also set analogValue field
    boolean           isFlasher;              // true=flash output, false=no time, no flash.
    byte              analogValue;            // Value to use with analog type.
    int               durationMilli;          // Milliseconds to leave output on for.  0 means don't auto off

    unsigned long     onMilli;                // Used internally for timing
    unsigned long     offMilli;               // Used internally for timing

    #ifdef _DEBUG_
        int               count;              // Used internally for debugging
        byte              validBytes;         // Used internally for debugging
        byte              data[ 6 ];          // Used internally for debugging
    #endif

} DCCAccessoryAddress;

#if defined(_6LAMP)
    DCCAccessoryAddress gAddresses[ 14 ];
#else
    DCCAccessoryAddress gAddresses[  7 ];
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The dcc decoder object and global data
//
int gPacketCount     = 0;
int gIdlePacketCount = 0;
int gLongestPreamble = 0;

#ifdef _DEBUG_
    static unsigned long lastMillis = millis();
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decoder Init 
//
//  pinnumber   7,  15,   5,   3,   6,  14,   4,  17,  18,  19,   8,   9,  10,  16
//  pinnames: PD7, PC1, PD5, PD3, PD6, PC0, PD4, PC3, PC4, PC5, PB0, PB1, PB2, PC2
//  output:   LS1, LS2, LS3, LS4, LB1, LB2, LB3, LB4, LB5, LB6, AX1, AX2, AX3, AX4
//
//    AD0 = LB2    PD0 = Rx1    PB0 = AX1
//    AD1 = LS2    PD1 = Tx0    PB1 = AX2
//    AD2 = AX4    PD2 = DCC    PB2 = AX3
//    AD3 = LB4    PD3 = LS4    PB3 = ICSP
//    AD4 = LB5    PD4 = LB3    PB4 = ICSP
//    AD5 = LB6    PD5 = LS3    PB5 = ICSP
//    AD6 = DNU    PD6 = LB1    PB6 = Xtal1
//    AD7 = DNU    PD7 = LS1    PB7 = Xtal2
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void ConfigureDecoder()
{
    gAddresses[ 0].address       =    40;  // PD7  LS1
    gAddresses[ 0].output        =     0;
    gAddresses[ 0].outputPin     =     7;
    gAddresses[ 0].isDigital     =  true;
    gAddresses[ 0].isFlasher     = false;
    gAddresses[ 0].analogValue   =     0;
    gAddresses[ 0].durationMilli =     0;

    gAddresses[ 1].address       =    40;  // PC1  LS2
    gAddresses[ 1].output        =     0;
    gAddresses[ 1].outputPin     =    15;
    gAddresses[ 1].isDigital     = false;
    gAddresses[ 1].isFlasher     = false;
    gAddresses[ 1].analogValue   =   250;
    gAddresses[ 1].durationMilli =  1000;

    gAddresses[ 2].address       =    40;  // PD5  LS3
    gAddresses[ 2].output        =     0;
    gAddresses[ 2].outputPin     =     5;
    gAddresses[ 2].isDigital     =  true;
    gAddresses[ 2].isFlasher     =  true;
    gAddresses[ 2].analogValue   =     0;
    gAddresses[ 2].durationMilli =     0;

    gAddresses[ 3].address       =    40;  // PD3  LS4
    gAddresses[ 3].output        =     0;
    gAddresses[ 3].outputPin     =     3;
    gAddresses[ 3].isDigital     =  true;
    gAddresses[ 3].isFlasher     =  true;
    gAddresses[ 3].analogValue   =     0;
    gAddresses[ 3].durationMilli =  1000;

    gAddresses[ 4].address       =    41;  // PD6  LB1
    gAddresses[ 4].output        =     0;
    gAddresses[ 4].outputPin     =     6;
    gAddresses[ 4].isDigital     =  true;
    gAddresses[ 4].isFlasher     = false;
    gAddresses[ 4].analogValue   =     0;
    gAddresses[ 4].durationMilli =     0;

    gAddresses[ 5].address       =    41;  // PC0  LB2
    gAddresses[ 5].output        =     0;
    gAddresses[ 5].outputPin     =    14;
    gAddresses[ 5].isDigital     = false;
    gAddresses[ 5].isFlasher     = false;
    gAddresses[ 5].analogValue   =   250;
    gAddresses[ 5].durationMilli =     0;

    gAddresses[ 6].address       =    41;  // PD4  LB3
    gAddresses[ 6].output        =     0;
    gAddresses[ 6].outputPin     =     4;
    gAddresses[ 6].isDigital     =  true;
    gAddresses[ 6].isFlasher     = false;
    gAddresses[ 6].analogValue   =     0;
    gAddresses[ 6].durationMilli =     0;

#ifdef _6LAMP_ 

    gAddresses[ 7].address       =    41;  // PC3  LB4
    gAddresses[ 7].output        =     0;
    gAddresses[ 7].outputPin     =    17;
    gAddresses[ 7].isDigital     = false;
    gAddresses[ 7].isFlasher     = false;
    gAddresses[ 7].analogValue   =   250;
    gAddresses[ 7].durationMilli =     0;

    gAddresses[ 8].address       =    41;  // PC4  LB5
    gAddresses[ 8].output        =     0;
    gAddresses[ 8].outputPin     =    18;
    gAddresses[ 8].isDigital     = false;
    gAddresses[ 8].isFlasher     = false;
    gAddresses[ 8].analogValue   =   250;
    gAddresses[ 8].durationMilli =     0;

    gAddresses[ 9].address       =    41;  // PC5  LB6
    gAddresses[ 9].output        =     0;
    gAddresses[ 9].outputPin     =    19;
    gAddresses[ 9].isDigital     = false;
    gAddresses[ 9].isFlasher     = false;
    gAddresses[ 9].analogValue   =   250;
    gAddresses[ 9].durationMilli =     0;

    gAddresses[10].address       =    42;  // PB0  AX1
    gAddresses[10].output        =     0;
    gAddresses[10].outputPin     =     8;
    gAddresses[10].isDigital     =  true;
    gAddresses[10].isFlasher     = false;
    gAddresses[10].analogValue   =     0;
    gAddresses[10].durationMilli =     0;

    gAddresses[11].address       =    42;  // PB1  AX2
    gAddresses[11].output        =     0;
    gAddresses[11].outputPin     =     9;
    gAddresses[11].isDigital     =  true;
    gAddresses[11].isFlasher     = false;
    gAddresses[11].analogValue   =     0;
    gAddresses[11].durationMilli =     0;

    gAddresses[12].address       =    42;  // PB2  AX3
    gAddresses[12].output        =     0;
    gAddresses[12].outputPin     =    10;
    gAddresses[12].isDigital     =  true;
    gAddresses[12].isFlasher     = false;
    gAddresses[12].analogValue   =     0;
    gAddresses[12].durationMilli =     0;

    gAddresses[13].address       =    42;  // PC2  AX4
    gAddresses[13].output        =     0;
    gAddresses[13].outputPin     =    16;
    gAddresses[13].isDigital     = false;
    gAddresses[13].isFlasher     = false;
    gAddresses[13].analogValue   =   250;
    gAddresses[13].durationMilli =     0;

#endif

    // Setup output pins
    for(int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); i++ )
    {
        if( gAddresses[ i ].outputPin )
        {
            pinMode( gAddresses[ i ].outputPin, OUTPUT );
        }
        gAddresses[ i ].onMilli  =  0;
        gAddresses[ i ].offMilli =  0;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Packet handlers
//

// ALL packets are sent to the RawPacket handler. Returning true indicates that packet was handled. DCC library starts watching for 
// next preamble. Returning false and the library continues parsing packets and finds another handler to call.
#ifdef _DEBUG_

    boolean RawPacket_Handler(byte byteCount, byte* packetBytes)
    {
            // Bump global packet count
        ++gPacketCount;

        for( int j = 0; j < byteCount; j++ )
        {
            Serial.print( packetBytes[j], DEC );
            Serial.print( "\t");
        }
        Serial.println();

    //   Serial.print( byteCount, DEC );
    //   Serial.print( "\t");
    //   Serial.println( packetBytes[1], DEC );


        int thisPreamble = DCC.LastPreambleBitCount();
        if( thisPreamble > gLongestPreamble )
        {
            gLongestPreamble =  thisPreamble;
        }
        
            // Walk table and look for a matching packet
        for( int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); ++i )
        {
            if( gAddresses[i].validBytes )
            {
                    // Not an empty slot. Does this slot match this packet? If so, bump count.
                if( gAddresses[i].validBytes == byteCount )
                {
                    char isPacket = true;
                    for( int j = 0; j < byteCount; j++ )
                    {
                        if( gAddresses[i].data[j] != packetBytes[j] )
                        {
                            isPacket = false;
                            break;
                        } 
                    }
                    if( isPacket )
                    {
                        gAddresses[i].count++;
                        return false;
                    }
                }
            } 
            else 
            {
                    // Empty slot, just copy over data
                gAddresses[i].count++;
                gAddresses[i].validBytes = byteCount;
                for( int j = 0; j < byteCount; j++ )
                {
                    gAddresses[i].data[j] = packetBytes[j];
                }
                return false;
            }
        }

        return false;
    }

    // Idle packets are sent here (unless handled in rawpacket handler). 
    void IdlePacket_Handler( byte byteCount, byte* packetBytes )
    {
        ++gIdlePacketCount;
    }

#endif

//
// Basic accessory packet handler 
//
void BasicAccDecoderPacket_Handler( int address, boolean activate, byte data )
{
        // Convert NMRA packet address format to human address
    address -= 1;
    address *= 4;
    address += 1;
    address += ( data & 0x06 ) >> 1;

    boolean enable = ( data & 0x01 ) ? 1 : 0;

    for(int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); i++ )
    {
        if( address == gAddresses[i].address )
        {
            _PP("Basic addr: " );
            _2P( address,  DEC );
            _PP("   activate: ");
            _2L( enable,   DEC );

            if( enable )
            {
                gAddresses[i].output   =        1;
                gAddresses[i].onMilli  = millis();
                gAddresses[i].offMilli =        0;
            } else {
                gAddresses[i].output   =        0;
                gAddresses[i].onMilli  =        0;
                gAddresses[i].offMilli = millis();
            }

        } else {
            _PP("Basic addr: " );
            _2P( address,  DEC );
            _PP("   activate: ");
            _2L( enable,   DEC );
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG_

    void DumpAndResetTable()
    {
        char buffer60Bytes[ 60 ];

        if (gPacketCount > 0 && gIdlePacketCount > 0 && gLongestPreamble > 0)
        {
            Serial.print("Total Packet Count:  " );
            Serial.println(gPacketCount,     DEC );

            Serial.print("Idle Packet Count:   " );
            Serial.println(gIdlePacketCount, DEC );

            Serial.print("Longest Preamble:    " );
            Serial.println(gLongestPreamble, DEC );

            Serial.println("Count    Packet_Data");
            for( int i = 0; i < (int)(sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ); ++i )
            {
                if( gAddresses[i].validBytes > 0 )
                {
                    Serial.print( gAddresses[i].count, DEC );
                    if( gAddresses[i].count < 10 )
                    {
                        Serial.print("        ");
                    }else{
                        if( gAddresses[i].count < 100 )
                        {
                            Serial.print("       ");
                        }else{
                            Serial.print("      ");
                        }
                    }
                    Serial.println( DCC.MakePacketString(buffer60Bytes, gAddresses[i].validBytes, &gAddresses[i].data[0]) );
                }
                gAddresses[i].validBytes = 0;
                gAddresses[i].count = 0;
            }
            Serial.println("============================================");
        }

        gPacketCount     = 0;
        gIdlePacketCount = 0;
        gLongestPreamble = 0;
    }

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Setup
//
void setup() 
{ 
    Serial.begin( 115200 );

    DCC.SetBasicAccessoryDecoderPacketHandler( BasicAccDecoderPacket_Handler, true );

    ConfigureDecoder();

    #ifdef _DEBUG_
        DCC.SetRawPacketHandler(  RawPacket_Handler  );
        DCC.SetIdlePacketHandler( IdlePacket_Handler );
    #endif

    DCC.SetupDecoder( 0x01, 0x01, kDCC_INTERRUPT );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Main loop
//
void loop()
{
    static int addr = 0;

        ////////////////////////////////////////////////////////////////
        // Loop DCC library
    DCC.loop();

        ////////////////////////////////////////////////////////////////
        // Show the action if asked for
    #ifdef _DEBUG_

        if( millis() - lastMillis > 2000 )
        {
            DumpAndResetTable();
            lastMillis = millis();
        }

    #endif

        ////////////////////////////////////////////////////////////////
        // Bump to next address to test
    if( ++addr >= (int)( sizeof( gAddresses ) / sizeof( gAddresses[ 0 ] ) ) )
    {
        addr = 0;
    }

        ////////////////////////////////////////////////////////////////
        // Turn off output?
    if( gAddresses[addr].offMilli && gAddresses[addr].offMilli < millis() )
    {
            // Clear off time
        gAddresses[addr].offMilli = 0;

            // Disable output
        if( gAddresses[addr].isDigital )
        {
            digitalWrite( gAddresses[addr].outputPin, LOW);
        }else{
            analogWrite(  gAddresses[addr].outputPin,   0);
        }

            ////////////////////////////////////////////////////////////////
            // If still enabled and a flash type, set on time
        if( gAddresses[addr].output && gAddresses[addr].isFlasher )
        {
            gAddresses[addr].onMilli = millis() + gAddresses[addr].durationMilli;
        }else{
            gAddresses[addr].output  = 0;
        }

        return;
    }

        ////////////////////////////////////////////////////////////////
        // Turn on output?
    if( gAddresses[addr].onMilli && gAddresses[addr].onMilli <= millis() )
    {
            // Clear off time
        gAddresses[addr].onMilli = 0;

            // Enable output
        if( gAddresses[addr].isDigital )
        {
            digitalWrite( gAddresses[addr].outputPin, HIGH);
        }else{
            analogWrite( gAddresses[addr].outputPin, gAddresses[addr].analogValue );
        }

            ////////////////////////////////////////////////////////////////
            // If still enabled and a flash type, set off time
        if( gAddresses[addr].durationMilli )
        {
            gAddresses[addr].offMilli = millis() + gAddresses[addr].durationMilli;
        }

        return;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
