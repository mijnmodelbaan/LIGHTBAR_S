# LIGHTBAR_S
DCC controlled lights for model railroads.

Here one can find several sketches for the Arduino framework on the Atmel platform.
The sketches are merely based on other persons work, with some changes for my need.

Some sketches are created for my version of Geoff Bunza's IDEC ideas.
Some are created for my LightBar system, designed for small carriages and vans.

AVR_TRAFFIC:
LIGHTBAR_TR:

Sequential traffic light system, based on Geoff's 'IDEC2_5_LargeFunctionSets'
AVR_TRAFFIC has the whole sequence written out, like the original.
LIGHTBAR_TR has some modifications (functions) which saves 6K in the sketch.


IDEC_5L2M1S
IDEC_BMULTI
IDEC_MULTIF
IDEC_WELDER

LIGHTBAR_36
LIGHTBAR_T1
LIGHTBAR_T2
LIGHTBAR_T3
LIGHTBAR_T4
LIGHTBAR_T5




The module (pcb) will be app. 50 x 10mm.


/***** HOW TO (DE)CODE ENGINE DECODER FUNCTIONS F0-F28 ****/

/*
 *    turns on and off engine decoder functions F0-F28 (F0 is sometimes called FL)  
 * 
 *    To set functions F0-F4 on (=1) or off (=0):
 * 
 *    BYTE1:  128 + F1*1 + F2*2 + F3*4 + F4*8 + F0*16
 *    BYTE2:  omitted
 * 
 *    To set functions F5-F8 on (=1) or off (=0):
 * 
 *    BYTE1:  176 + F5*1 + F6*2 + F7*4 + F8*8
 *    BYTE2:  omitted
 * 
 *    To set functions F9-F12 on (=1) or off (=0):
 * 
 *    BYTE1:  160 + F9*1 +F10*2 + F11*4 + F12*8
 *    BYTE2:  omitted
 * 
 *    To set functions F13-F20 on (=1) or off (=0):
 * 
 *    BYTE1: 222 
 *    BYTE2: F13*1 + F14*2 + F15*4 + F16*8 + F17*16 + F18*32 + F19*64 + F20*128
 * 
 *    To set functions F21-F28 on (=1) of off (=0):
 * 
 *    BYTE1: 223
 *    BYTE2: F21*1 + F22*2 + F23*4 + F24*8 + F25*16 + F26*32 + F27*64 + F28*128
 * 
 */
