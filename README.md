# LIGHTBAR_S
DCC controlled lights for model railroads.

This will be the first installment, designed for small carriages and vans.

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
