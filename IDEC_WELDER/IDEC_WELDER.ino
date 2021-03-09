
/* 
 * 
 * Interactive Decoder                             IDEC2_3_Building2Wldrs.ino
 * Version 1.08 Geoff Bunza 2020
 * Works with both short and long DCC Addesses
 * 
 * This decoder will control Random Building Lighting
 * F0 = Master Function - OFF = Function - ON DISABLES the decoder
 * Input Pin for Decoder Disable = Pin 3 Active  LOW
 * 
 * F0 == Master Decoder Disable == ON
 * F1 == Welder 1       Disable == ON
 * F2 == Welder 2       Disable == ON
 * 
 * PRO MINI PIN ASSIGNMENT:
2 - DCC Input
3 - Input Pin for MasterDecoderDisable Active LOW
4 - LED Blue Welder1
5 - LED White Welder1
6 - LED Blue Welder2
7 - LED White Welder2
8 - LED
9 - LED
10 - LED
11 - LED
12 - LED 
13 - LED
14 A0 - LED
15 A1 - LED
16 A2 - LED
17 A3 - LED
18 A4 - LED
19 A5 - LED
 * 
 */


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ***** Uncomment to use the PinChangeInterrupts iso External Interrupts *****
// #define PIN_CHANGE_INT

#include <NmraDcc.h>

NmraDcc     Dcc ;
DCC_MSG  Packet ;


#include <avr/wdt.h>  //  Needed for automatic reset functions.


#define This_Decoder_Address       40

uint8_t CV_DECODER_JUST_A_RESET = 251;
uint8_t CV_DECODER_MASTER_RESET = 252;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ******** REMOVE THE "//" IN THE NEXT LINE UNLESS YOU WANT ALL CV'S RESET ON EVERY POWER-UP
#define DECODER_LOADED

// ******** REMOVE THE "//" IN THE NEXT LINE IF YOU WANT TO RUN A TEST DURING SETUP SEQUENCES
// #define TESTRUN

// ******** REMOVE THE "//" IN THE NEXT LINE TO SEE DETAILED INFO ABOUT INCOMING C.V MESSAGES
// #define DEBUGCV

// ******** REMOVE THE "//" IN THE FOLLOWING LINE TO SEND DEBUGGING INFO TO THE SERIAL OUTPUT
// #define DEBUGID

#if defined( DEBUGID ) || defined( DEBUGCV )
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
//  *****  small macro used to create all the timed events  *****
#define runEvery( n ) for ( static unsigned long lasttime; millis() - lasttime > ( unsigned long )( n ); lasttime = millis() )


// unsigned long estop                =    1;
int building_tim_delay             =    0;
int welder1_tim_delay              =    0;
int welder2_tim_delay              =    0;
int welder1_delta                  =    0;
int welder2_delta                  =    0;
byte welder1_on                    =    0;
byte welder2_on                    =    0;

const int MasterDecoderDisablePin  =   14;  //  A0 Master Decoder Disable Input Pin | Active  LOW
const int FunctionPin0             =   20;  //  Place holders ONLY
const int FunctionPinDcc           =    2;  //  Inputpin DCCsignal

const int Welder1BluePin           =    3;  //  Blue  LED simulation Welder 1
const int Welder1WhitePin          =    4;  //  White LED simulation Welder 1
const int Welder2BluePin           =    5;  //  Blue  LED simulation Welder 2
const int Welder2WhitePin          =    6;  //  White LED simulation Welder 2

const int num_active_functions     =    3;  //  Number # functions starting with F0
const int numleds                  =   16;  //  Number of Output pins to initialize

byte fpins []                      =  { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19 };
                                            //  This are all the Output Pins available for a house

int MasterDecoderDisable           =    0;
int MasterDisable_value            =    0;
int Disable_welder1                =    0;
int Disable_welder2                =    0;


struct QUEUE
{
   int            inuse;   // pin in use or not
   int current_position;
   int        increment;
   int       stop_value;
   int      start_value;
};
QUEUE *ftn_queue = new QUEUE[  3 ];


struct CVPair
{
   uint16_t    CV;
   uint8_t  Value;
};


CVPair FactoryDefaultCVs [] =
{
   {CV_MULTIFUNCTION_PRIMARY_ADDRESS, This_Decoder_Address & 0x7F },

   // These two CVs define the Long DCC Address
   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, (( This_Decoder_Address >> 8 ) & 0x7F ) + 192 },
   {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB,    This_Decoder_Address & 0xFF },

   // ONLY uncomment 1 CV_29_CONFIG line below as approprate DEFAULT IS SHORT ADDRESS
// {CV_29_CONFIG,                0},                        // Short Address 14 Speed Steps
   {CV_29_CONFIG, CV29_F0_LOCATION},                        // Short Address 28/128 Speed Steps
// {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION},  // Long  Address 28/128 Speed Steps

   {CV_DECODER_MASTER_RESET,     0},

   { 30,   0},  //  F0 Config 0=DISABLE On/Off, 1=Disable Welder 1, 2=Disable Welder2
   { 31,   1},  //  F0 Config 0=DISABLE On/Off, 1=Disable Welder 1, 2=Disable Welder2
   { 32,   2},  //  F0 Config 0=DISABLE On/Off, 1=Disable Welder 1, 2=Disable Welder2

   { 50,  90},  //  Master Building Time Delay 0-255
   { 51, 127},  //  Welder1 Time Constant
   { 52, 147},  //  Welder2 Time Constant

   {251,   0},  //  CV_DECODER_JUST_A_RESET */
/* {252,   0},  //  CV_DECODER_MASTER_RESET */
   {253,   0},  //  Extra
};


/* ******************************************************************************* */


uint8_t FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
void notifyCVResetFactoryDefault()
{
   // Make FactoryDefaultCVIndex non-zero and equal to # CV's to be reset.
   FactoryDefaultCVIndex = sizeof( FactoryDefaultCVs ) / sizeof( CVPair );
};


/* ******************************************************************************* */


void setup()
{
   noInterrupts();

      // initialize the digital pins as outputs
   for ( int i = 0; i < numleds; ++i )
   {
      digitalWrite( fpins[ i ], 0 );  // Switch the pin off first.
      pinMode( fpins[ i ], OUTPUT );  // Then set it as an output.
   }

   pinMode( MasterDecoderDisablePin,  INPUT_PULLUP );  //  Master Decoder Disable Input Pin   - Active LOW

   #if defined( DEBUGID ) || defined( DEBUGCV )

      Serial.begin( 115200 );

      while ( !Serial )
      {
         ; // wait for Serial port to connect. Needed for native USB port.
      }
      Serial.flush();    // Wait for all the rubbish to finish displaying.
      while ( Serial.available() > 0 )
      {
         Serial.read(); // Clear the input buffer to get 'real' inputdata.
      }

   #endif

      //  Call the DCC 'pin' and 'init' functions to enable the DCC Receiver.
   Dcc.pin( digitalPinToInterrupt( FunctionPinDcc ), FunctionPinDcc, false );
   Dcc.init( MAN_ID_DIY, 201, FLAGS_MY_ADDRESS_ONLY, 0 );      delay( 1000 );

   #if defined( DECODER_LOADED )

      if ( Dcc.getCV( CV_DECODER_MASTER_RESET ) == CV_DECODER_MASTER_RESET ) 
      {
      #endif

         FactoryDefaultCVIndex =  sizeof( FactoryDefaultCVs ) / sizeof( CVPair );

         for ( int i = 0; i < FactoryDefaultCVIndex; ++i )
         {
            Dcc.setCV( FactoryDefaultCVs[ i ].CV, FactoryDefaultCVs[ i ].Value );
         }

      #if defined(DECODER_LOADED)
      }
   #endif

   Dcc.setCV( CV_DECODER_JUST_A_RESET, 0 );  //  Reset the just_a_reset CV

   #if defined( TESTRUN )

      for ( int i = 0; i < numleds; ++i )   //  Turn all LEDs  ON in sequence
      {
         digitalWrite( fpins[ i ], HIGH );
         delay( 60 );
      }

      delay( 400 );

      for ( int i = 0; i < numleds; ++i )   //  Turn all LEDs OFF in sequence
      {
         digitalWrite( fpins[ i ],  LOW );
         delay( 60 );
      }

   #endif

   for ( int i = 0; i < num_active_functions; ++i )
   {
      uint8_t  cv_value = Dcc.getCV( 30 + i );
      switch ( cv_value )
      {
         case 0:   // Master Decoder Disable
         {
            MasterDecoderDisable = 0 ;
            if ( digitalRead( MasterDecoderDisablePin ) ==  LOW )
            {
               MasterDecoderDisable = 1 ;
            }
            break ;
         }

         case 1:   //  F1 Disables Welder 1
         {
            Disable_welder1 = 0 ;
            break ;
         }

         case 2:   //  F2 Disables Welder 2
         {
            Disable_welder2 = 0 ;
            break ;
         }

         default:
         {
            break ;
         }
      }
   }

   building_tim_delay = int( Dcc.getCV( 50 ) ) * 11 ;

   welder1_tim_delay  = int( Dcc.getCV( 51 ) ) * 21 ;

   welder2_tim_delay  = int( Dcc.getCV( 52 ) ) * 31 ;

   #if defined( DEBUGID ) || defined( DEBUGCV )

      _PL( "\n" "CV Dump: " );
      for ( int i = 30; i < 33; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "Timer setting:" );
      for ( int i = 50; i < 53; ++i ) { _2P( Dcc.getCV( i ), DEC ); _PP( "\t" ); }
      _PL( "" );

      _PL( "My Address" );
      _2P( Dcc.getCV( CV_MULTIFUNCTION_PRIMARY_ADDRESS ), DEC ); _PP( "\t \n" );
      _PL( "" );

      _2L( building_tim_delay, DEC );
      _2L( welder1_tim_delay,  DEC );
      _2L( welder2_tim_delay,  DEC );

   #endif

   interrupts();  // Ready to rumble....
}


/* ******************************************************************************** */


void loop()
{
   //  MUST call the NmraDcc.process() method frequently for correct library operation
   Dcc.process();

   //  Check Master Input Over ride
   MasterDecoderDisable = 0 ;
   if ( digitalRead( MasterDecoderDisablePin ) == LOW )
   {
      MasterDecoderDisable = 1 ;
   }
   else
   {
      MasterDecoderDisable = MasterDisable_value & 1 ;
   }

   // ========  Random Building Lights  ========================
   runEvery( building_tim_delay )  digitalWrite( fpins[ random( 4, numleds ) ], lightsw( ) );

   // ========  Random Welder1  Lights  ========================
   if ( ( MasterDecoderDisable == 0 ) && ( Disable_welder1 == 0 ) )
   {
      runEvery( welder1_tim_delay ) { welder1_on = random( 20, 50 ); welder1_delta = random( 23, 133 ); }
   }
   runEvery( welder1_delta )  digitalWrite( Welder1WhitePin, run_welder1_wsw( ) );
   runEvery( welder1_delta )  digitalWrite( Welder1BluePin,  run_welder1_bsw( ) );

   // ========  Random Welder2  Lights  ========================
   if ( ( MasterDecoderDisable == 0 ) && ( Disable_welder2 == 0 ) )
   {
      runEvery( welder2_tim_delay )  { welder2_on = random( 25, 47 ); welder2_delta = random( 22, 125 ); }
   }
   runEvery( welder2_delta )  digitalWrite( Welder2WhitePin, run_welder2_wsw( ) );
   runEvery( welder2_delta )  digitalWrite( Welder2BluePin,  run_welder2_bsw( ) );

}  // end loop()


/* ******************************************************************************* */


boolean run_welder1_wsw( )
{
   if ( ( MasterDecoderDisable == 1 ) || ( welder1_on <= 0 ) )
   {
      return  LOW ;   //  Eventually turn all lights OFF
   }

   --welder1_on ;

   if ( random( 0, 100 ) > 48 )
   {
      return HIGH ;   //  48 represents a 52% ON time
   }
   else
   {
      return  LOW ;
   }
}                     //  End run_welder1_wsw


/* ******************************************************************************** */


boolean run_welder1_bsw( )
{
   if ( ( MasterDecoderDisable == 1 ) || ( welder1_on <= 0 ) )
   {
      return  LOW ;   //  Eventually turn all lights OFF
   }

   --welder1_on ;

   if ( random( 0, 100 ) > 35 )
   {
      return HIGH ;   //  35 represents a 65% ON time
   }
   else
   {
      return  LOW ;
   }
}                     //  end run_welder1_bsw


/* ******************************************************************************** */


boolean run_welder2_wsw( )
{
   if ( ( MasterDecoderDisable == 1 ) || ( welder2_on <= 0 ) )
   {
      return  LOW ;   //  Eventually turn all lights OFF
   }

   --welder2_on ;

   if ( random( 0, 100 ) > 48 )
   {
      return HIGH ;   //  48 represents a 52% ON time
   }
   else
   {
      return  LOW ;
   }
}                     //  end run_welder2_wsw


/* ******************************************************************************** */


boolean run_welder2_bsw( )
{
   if ( ( MasterDecoderDisable == 1 ) || ( welder2_on <= 0 ) )
   {
      return  LOW ;   //  Eventually turn all lights OFF
   }

   --welder2_on ;

   if ( random( 0, 100 ) > 35 )
   {
      return HIGH ;   //  35 represents a 65% ON time
   }
   else
   {
      return  LOW ;
   }
}                     //  end run_welder2_bsw


/* ******************************************************************************** */


boolean lightsw( )
{
   if ( MasterDecoderDisable == 1 )
   {
      return  LOW ;   //  Eventually turn all lights OFF
   }

   if ( random( 0, 100 ) > 40 )
   {
      return HIGH ;   //  40 represents a 60% ON time
   }
   else
   {
      return  LOW ;
   }
}                     //  End lightsw


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
void    notifyCVChange( uint16_t CV, uint8_t Value )
{
   #if defined( DEBUGCV )

      _PP( "notifyCVChange: CV: " );
      _2P( CV,                DEC );
      _PP( "\t" " Value: "        );
      _2L( Value,             DEC );

   #endif

   if ( ( ( CV == 251 ) && ( Value == 251 ) ) || ( ( CV == 252 ) && ( Value == 252 ) ) )
   {
      wdt_enable( WDTO_15MS );  //  Resets after 15 milliSecs

      while( 1 ) {}  // Wait for the prescaler time to expire
   }
}       //   end notifyCVChange()


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
void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState )
{
   #if defined( DEBUGCV )

      _PP( "Address = "   );
      _2L( Addr,       DEC);
      _PP( "AddrType = "  );
      _2L( AddrType,   DEC);
      _PP( "FuncGrp = "   );
      _2L( FuncGrp,    DEC);
      _PP( "FuncState = " );
      _2L( FuncState, DEC );

   #endif

   switch( FuncGrp )
   {
      case FN_0_4:    //  Function Group 1 F0 F4 F3 F2 F1
      {
         exec_function( 0, FunctionPin0, ( FuncState & FN_BIT_00 ) >> 4 );
         exec_function( 1, FunctionPin0, ( FuncState & FN_BIT_01 ) >> 0 );
         exec_function( 2, FunctionPin0, ( FuncState & FN_BIT_02 ) >> 1 );
         break ;
      }

      case FN_5_8:    //  Function Group 1    F8 F7 F6 F5
      {
         break ;
      }

      case FN_9_12:   //  Function Group 1 F12 F11 F10 F9
      {
         break ;
      }

      case FN_13_20:  //  Function Group 2  ==  F20 - F13
      {
         break ;
      }

      case FN_21_28:  //  Function Group 2  ==  F28 - F21
      {
         break ;
      }
   }
}                     //  End notifyDccFunc()


/* ******************************************************************************* */


void exec_function ( int function, int pin, int FuncState )
{
   #if defined( DEBUGCV )

      _PP( "exec_function = " );
      _2L( function,      DEC );
      _PP( "pin = "           );
      _2L( pin,           DEC );
      _PP( "FuncState = "     );
      _2L( FuncState,     DEC );

   #endif

   switch ( Dcc.getCV( 30 + function ) )
   {
      case 0:    // Master Disable by Function 0
      {
         MasterDisable_value = byte( FuncState );
         break ;
      }

      case 1:    // Welderer Disable by Function 1
      {
         Disable_welder1 = byte( FuncState );
         break ;
      }

      case 2:    // Welder Disable by Function 2
      {
         Disable_welder2 = byte( FuncState );
         break ;
      }

      default:
      {
         break ;
      }
   }
}                //  End exec_function()


/* ******************************************************************************* */


//
