/*

Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Abstract:

Application to demonstrate gradient function of EVE.

Author : FTDI

Revision History:

*/

#include "FT_Platform.h"
#include "FT_App_Lift.h"
#ifdef FT900_PLATFORM
#include "ff.h"
#endif
#include "math.h"

#define F16(s)        ((ft_int32_t)((s) * 65536))
#define WRITE2CMD(a) Ft_Gpu_Hal_WrCmdBuf(phost,a,sizeof(a))

/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
ft_int16_t FT_DispWidth = 480;
ft_int16_t FT_DispHeight = 272;
ft_int16_t FT_DispHCycle =  548;
ft_int16_t FT_DispHOffset = 43;
ft_int16_t FT_DispHSync0 = 0;
ft_int16_t FT_DispHSync1 = 41;
ft_int16_t FT_DispVCycle = 292;
ft_int16_t FT_DispVOffset = 12;
ft_int16_t FT_DispVSync0 = 0;
ft_int16_t FT_DispVSync1 = 10;
ft_uint8_t FT_DispPCLK = 5;
ft_char8_t FT_DispSwizzle = 0;
ft_char8_t FT_DispPCLKPol = 1;
ft_char8_t FT_DispCSpread = 1;
ft_char8_t FT_DispDither = 1;
/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */
const ft_uint8_t FT_DLCODE_BOOTUP[12] =
{
    0,255,0,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};

#ifdef FT900_PLATFORM
	FATFS FatFs;
	ft_int16_t xy=0;
	ft_int32_t curr_rtc_clock;

typedef struct _localtime
    {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    } 	localtime;
	localtime st,lt;
	 DWORD get_fattime (void)
	 {
	 	/* Returns current time packed into a DWORD variable */
	 	return 0;
	 }
#else
	SYSTEMTIME st, lt;
#endif


	 ///////////////////////////////////////////////////////////////////////
	 FT_PROGMEM ft_prog_uint16_t sintab[] = {
	 0, 402, 804, 1206, 1607, 2009, 2410, 2811, 3211, 3611, 4011, 4409, 4807, 5205, 5601, 5997, 6392,
	 6786, 7179, 7571, 7961, 8351, 8739, 9126, 9511, 9895, 10278, 10659, 11038, 11416, 11792, 12166, 12539,
	 12909, 13278, 13645, 14009, 14372, 14732, 15090, 15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868,
	 18204, 18537, 18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705, 22004, 22301, 22594,
	 22883, 23169, 23452, 23731, 24006, 24278, 24546, 24811, 25072, 25329, 25582, 25831, 26077, 26318, 26556, 26789,
	 27019, 27244, 27466, 27683, 27896, 28105, 28309, 28510, 28706, 28897, 29085, 29268, 29446, 29621, 29790, 29955,
	 30116, 30272, 30424, 30571, 30713, 30851, 30984, 31113, 31236, 31356, 31470, 31580, 31684, 31785, 31880, 31970,
	 32056, 32137, 32213, 32284, 32350, 32412, 32468, 32520, 32567, 32609, 32646, 32678, 32705, 32727, 32744, 32757,
	 32764, 32767, 32764};


	 ft_int16_t qsin(ft_uint16_t a)
	 {
	   ft_uint8_t f;
	   ft_int16_t s0,s1;

	   if (a & 32768)
	     return -qsin(a & 32767);
	   if (a & 16384)
	       a = 32768 - a;
	   f = a & 127;
	   s0 = ft_pgm_read_word(sintab + (a >> 7));
	   s1 = ft_pgm_read_word(sintab + (a >> 7) + 1);
	   return (s0 + ((ft_int32_t)f * (s1 - s0) >> 7));
	 }

	 /* cos funtion */
	 ft_int16_t qcos(ft_uint16_t a)
	 {
	   return (qsin(a + 16384));
	 }




#define STARTUP_ADDRESS	100*1024L

Ft_Gpu_Hal_Context_t host,*phost;
#ifdef FT_81X_ENABLE
FT_Gpu_Fonts_t g_Gpu_Fonts[] = {
/* VC1 Hardware Fonts index 16*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
8,
1047548},
/* VC1 Hardware Fonts index 17*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
8,
1046524},
/* VC1 Hardware Fonts index 18*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
16,
1044476},
/* VC1 Hardware Fonts index 19*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
16,
1042428},
/* VC1 Hardware Fonts index 20*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,4,6,6,9,8,2,4,4,4,6,3,4,3,3,6,6,6,6,6,6,6,6,6,6,3,3,6,5,6,6,11,7,7,8,8,7,6,8,8,3,5,7,6,9,8,8,7,8,7,7,5,8,7,9,7,7,7,3,3,3,6,6,3,5,6,5,6,5,4,6,6,2,2,5,2,8,6,6,6,6,4,5,4,5,6,8,6,5,5,3,3,3,7,0,
1,
2,
10,
13,
1039100},
/* VC1 Hardware Fonts index 21*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,5,8,8,12,10,3,5,5,7,9,3,4,3,4,8,8,8,8,8,8,8,8,8,8,3,4,8,9,8,8,13,9,9,10,10,9,8,11,10,4,7,9,8,12,10,11,9,11,10,9,9,10,9,13,9,9,9,4,4,4,7,8,5,8,7,7,8,8,4,8,8,3,3,7,3,11,8,8,8,8,5,7,4,7,7,10,7,7,7,5,3,5,8,0,
1,
2,
13,
17,
1035580},
/* VC1 Hardware Fonts index 22*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,6,9,9,14,11,3,6,6,6,10,4,5,4,5,9,9,9,9,9,9,9,9,9,9,4,4,10,10,10,9,17,11,11,12,12,11,10,13,12,4,8,11,9,13,12,13,11,13,12,11,10,12,11,15,11,11,10,5,5,5,8,9,6,9,9,8,9,9,5,9,9,3,4,8,3,14,9,9,9,9,5,8,5,9,8,12,8,8,8,6,4,6,10,0,
1,
2,
14,
20,
1031548},
/* VC1 Hardware Fonts index 23*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,5,10,10,16,13,3,6,6,7,10,5,6,5,5,10,10,10,10,10,10,10,10,10,10,5,5,10,11,10,10,18,13,13,14,14,13,12,15,14,6,10,13,11,16,14,15,13,15,14,13,12,14,13,18,13,13,12,5,5,5,9,11,4,11,11,10,11,10,6,11,10,4,4,9,4,16,10,11,11,11,6,9,6,10,10,14,10,10,9,6,5,6,10,0,
1,
3,
17,
22,
1024380},
/* VC1 Hardware Fonts index 24*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,8,14,13,22,17,6,8,8,10,14,6,8,6,7,13,13,13,13,13,13,13,13,13,13,6,6,15,15,15,12,25,17,17,18,18,16,14,19,18,8,13,18,14,21,18,18,16,18,17,16,16,18,17,22,17,16,15,7,7,7,12,14,7,13,14,12,14,13,8,14,13,6,6,12,6,20,14,13,14,14,9,12,8,14,13,18,12,13,12,8,6,8,14,0,
1,
3,
24,
29,
0x2F7E3C},
/* VC1 Hardware Fonts index 25*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,12,19,18,29,22,6,11,11,13,19,9,11,9,9,18,18,18,18,18,18,18,18,18,18,9,9,19,19,19,18,34,22,22,24,24,22,20,25,24,9,16,22,18,27,24,25,22,26,24,22,20,24,22,31,22,22,20,9,9,9,16,18,11,18,18,16,18,18,9,18,18,7,7,16,7,27,18,18,18,18,11,16,9,18,16,23,16,16,16,11,9,11,19,0,
1,
4,
30,
38,
998684},
/* VC1 Hardware Fonts index 26*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,4,5,9,8,10,9,3,5,5,6,8,3,6,4,6,8,8,8,8,8,8,8,8,8,8,4,4,7,8,7,7,13,9,9,9,9,8,8,9,10,4,8,9,8,12,10,10,9,10,9,9,9,9,9,12,9,9,8,4,6,4,6,7,4,8,8,7,8,7,5,8,8,4,4,8,4,12,8,8,8,8,5,7,5,8,7,11,7,7,7,5,3,5,10,3,
2,
6,
12,
16,
991260},
/* VC1 Hardware Fonts index 27*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,6,11,10,12,11,4,6,6,7,10,4,8,5,7,10,10,10,10,10,10,10,10,10,10,4,4,9,10,9,8,15,11,11,11,12,9,9,12,12,5,9,11,9,15,12,12,11,12,11,10,10,12,11,15,11,11,10,5,7,5,7,8,5,9,10,9,10,9,6,10,10,4,4,9,4,15,10,10,10,10,6,9,6,10,9,13,9,9,9,6,4,6,12,4,
2,
8,
16,
20,
973852},
/* VC1 Hardware Fonts index 28*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,8,13,12,15,13,5,7,7,9,12,5,9,6,9,12,12,12,12,12,12,12,12,12,12,5,5,11,12,11,10,19,13,13,13,14,12,12,14,15,6,12,14,12,18,15,14,13,15,13,13,13,14,14,18,13,13,13,6,9,6,9,10,7,12,12,11,12,11,8,12,12,5,5,11,5,18,12,12,12,12,7,11,7,12,11,16,11,11,11,7,5,7,14,5,
2,
9,
18,
25,
950172},
/* VC1 Hardware Fonts index 29*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,9,15,14,18,15,5,8,8,10,14,5,11,6,10,14,14,14,14,14,14,14,14,14,14,6,6,12,14,13,11,21,15,15,15,16,13,13,16,17,7,13,15,13,21,17,16,15,17,15,15,14,16,15,21,15,15,14,7,10,6,10,11,8,13,14,13,14,13,9,14,14,6,6,13,6,21,14,14,14,14,8,13,8,14,12,18,12,12,12,8,6,8,16,6,
2,
11,
22,
28,
917948},
/* VC1 Hardware Fonts index 30*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,8,11,19,18,23,19,7,11,10,13,18,7,14,8,13,17,17,17,17,17,17,17,17,17,17,8,8,16,17,16,15,28,20,20,20,21,17,17,21,22,9,17,20,17,27,22,21,20,22,20,19,19,21,20,27,20,20,19,8,13,8,13,15,10,17,18,16,18,16,11,18,18,8,8,16,8,27,18,18,18,18,11,16,10,18,16,23,16,16,16,11,8,11,21,8,
2,
14,
28,
36,
863292},
/* VC1 Hardware Fonts index 31*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,11,15,26,24,31,26,9,14,14,18,24,9,19,11,17,24,24,24,24,24,24,24,24,24,24,11,11,21,24,22,20,38,27,27,27,28,23,23,28,30,12,23,27,23,36,30,29,27,29,27,26,25,28,27,36,27,27,25,11,18,11,18,20,13,23,24,22,24,22,15,24,24,11,11,22,11,37,24,24,24,24,15,22,13,24,21,32,21,21,21,14,10,14,29,10,
2,
18,
36,
49,
766524},
};


#else
FT_Gpu_Fonts_t g_Gpu_Fonts[] = {
/* VC1 Hardware Fonts index 16*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
8,
1047548},
/* VC1 Hardware Fonts index 17*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
8,
1046524},
/* VC1 Hardware Fonts index 18*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
16,
1044476},
/* VC1 Hardware Fonts index 19*/
{8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
1,
1,
8,
16,
1042428},
/* VC1 Hardware Fonts index 20*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,4,6,6,9,8,2,4,4,4,6,3,4,3,3,6,6,6,6,6,6,6,6,6,6,3,3,6,5,6,6,11,7,7,8,8,7,6,8,8,3,5,7,6,9,8,8,7,8,7,7,5,8,7,9,7,7,7,3,3,3,6,6,3,5,6,5,6,5,4,6,6,2,2,5,2,8,6,6,6,6,4,5,4,5,6,8,6,5,5,3,3,3,7,0,
1,
2,
10,
13,
1039100},
/* VC1 Hardware Fonts index 21*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,5,8,8,12,10,3,5,5,7,9,3,4,3,4,8,8,8,8,8,8,8,8,8,8,3,4,8,9,8,8,13,9,9,10,10,9,8,11,10,4,7,9,8,12,10,11,9,11,10,9,9,10,9,13,9,9,9,4,4,4,7,8,5,8,7,7,8,8,4,8,8,3,3,7,3,11,8,8,8,8,5,7,4,7,7,10,7,7,7,5,3,5,8,0,
1,
2,
13,
17,
1035580},
/* VC1 Hardware Fonts index 22*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,6,9,9,14,11,3,6,6,6,10,4,5,4,5,9,9,9,9,9,9,9,9,9,9,4,4,10,10,10,9,17,11,11,12,12,11,10,13,12,4,8,11,9,13,12,13,11,13,12,11,10,12,11,15,11,11,10,5,5,5,8,9,6,9,9,8,9,9,5,9,9,3,4,8,3,14,9,9,9,9,5,8,5,9,8,12,8,8,8,6,4,6,10,0,
1,
2,
14,
20,
1031548},
/* VC1 Hardware Fonts index 23*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,5,10,10,16,13,3,6,6,7,10,5,6,5,5,10,10,10,10,10,10,10,10,10,10,5,5,10,11,10,10,18,13,13,14,14,13,12,15,14,6,10,13,11,16,14,15,13,15,14,13,12,14,13,18,13,13,12,5,5,5,9,11,4,11,11,10,11,10,6,11,10,4,4,9,4,16,10,11,11,11,6,9,6,10,10,14,10,10,9,6,5,6,10,0,
1,
3,
17,
22,
1024380},
/* VC1 Hardware Fonts index 24*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,8,14,13,22,17,6,8,8,10,14,6,8,6,7,13,13,13,13,13,13,13,13,13,13,6,6,15,15,15,12,25,17,17,18,18,16,14,19,18,8,13,18,14,21,18,18,16,18,17,16,16,18,17,22,17,16,15,7,7,7,12,14,7,13,14,12,14,13,8,14,13,6,6,12,6,20,14,13,14,14,9,12,8,14,13,18,12,13,12,8,6,8,14,0,
1,
3,
24,
29,
1015356},
/* VC1 Hardware Fonts index 25*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,12,19,18,29,22,6,11,11,13,19,9,11,9,9,18,18,18,18,18,18,18,18,18,18,9,9,19,19,19,18,34,22,22,24,24,22,20,25,24,9,16,22,18,27,24,25,22,26,24,22,20,24,22,31,22,22,20,9,9,9,16,18,11,18,18,16,18,18,9,18,18,7,7,16,7,27,18,18,18,18,11,16,9,18,16,23,16,16,16,11,9,11,19,0,
1,
4,
30,
38,
998684},
/* VC1 Hardware Fonts index 26*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,4,5,9,8,10,9,3,5,5,6,8,3,6,4,6,8,8,8,8,8,8,8,8,8,8,4,4,7,8,7,7,13,9,9,9,9,8,8,9,10,4,8,9,8,12,10,10,9,10,9,9,9,9,9,12,9,9,8,4,6,4,6,7,4,8,8,7,8,7,5,8,8,4,4,8,4,12,8,8,8,8,5,7,5,8,7,11,7,7,7,5,3,5,10,3,
2,
6,
12,
16,
991260},
/* VC1 Hardware Fonts index 27*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,6,11,10,12,11,4,6,6,7,10,4,8,5,7,10,10,10,10,10,10,10,10,10,10,4,4,9,10,9,8,15,11,11,11,12,9,9,12,12,5,9,11,9,15,12,12,11,12,11,10,10,12,11,15,11,11,10,5,7,5,7,8,5,9,10,9,10,9,6,10,10,4,4,9,4,15,10,10,10,10,6,9,6,10,9,13,9,9,9,6,4,6,12,4,
2,
8,
16,
20,
973852},
/* VC1 Hardware Fonts index 28*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,8,13,12,15,13,5,7,7,9,12,5,9,6,9,12,12,12,12,12,12,12,12,12,12,5,5,11,12,11,10,19,13,13,13,14,12,12,14,15,6,12,14,12,18,15,14,13,15,13,13,13,14,14,18,13,13,13,6,9,6,9,10,7,12,12,11,12,11,8,12,12,5,5,11,5,18,12,12,12,12,7,11,7,12,11,16,11,11,11,7,5,7,14,5,
2,
9,
18,
25,
950172},
/* VC1 Hardware Fonts index 29*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,9,15,14,18,15,5,8,8,10,14,5,11,6,10,14,14,14,14,14,14,14,14,14,14,6,6,12,14,13,11,21,15,15,15,16,13,13,16,17,7,13,15,13,21,17,16,15,17,15,15,14,16,15,21,15,15,14,7,10,6,10,11,8,13,14,13,14,13,9,14,14,6,6,13,6,21,14,14,14,14,8,13,8,14,12,18,12,12,12,8,6,8,16,6,
2,
11,
22,
28,
917948},
/* VC1 Hardware Fonts index 30*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,8,11,19,18,23,19,7,11,10,13,18,7,14,8,13,17,17,17,17,17,17,17,17,17,17,8,8,16,17,16,15,28,20,20,20,21,17,17,21,22,9,17,20,17,27,22,21,20,22,20,19,19,21,20,27,20,20,19,8,13,8,13,15,10,17,18,16,18,16,11,18,18,8,8,16,8,27,18,18,18,18,11,16,10,18,16,23,16,16,16,11,8,11,21,8,
2,
14,
28,
36,
863292},
/* VC1 Hardware Fonts index 31*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,11,15,26,24,31,26,9,14,14,18,24,9,19,11,17,24,24,24,24,24,24,24,24,24,24,11,11,21,24,22,20,38,27,27,27,28,23,23,28,30,12,23,27,23,36,30,29,27,29,27,26,25,28,27,36,27,27,25,11,18,11,18,20,13,23,24,22,24,22,15,24,24,11,11,22,11,37,24,24,24,24,15,22,13,24,21,32,21,21,21,14,10,14,29,10,
2,
18,
36,
49,
766524},
};
#endif

ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;
ft_uint32_t music_playing = 0,fileszsave = 0;


#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif


ft_void_t Ft_App_WrCoCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef  BUFFER_OPTIMIZATION
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_CmdBuffer[Ft_CmdBuffer_Index];
   *pBuffcmd = cmd;
#endif
#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_WrCmd32(phost,cmd);
#endif
#ifdef FT900_PLATFORM
   Ft_Gpu_Hal_WrCmd32(phost,cmd);
#endif
   /* Increment the command index */
   Ft_CmdBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrDlCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef BUFFER_OPTIMIZATION  
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_DlBuffer[Ft_DlBuffer_Index];
   *pBuffcmd = cmd;
#endif

#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_Wr32(phost,(RAM_DL+Ft_DlBuffer_Index),cmd);
#endif
#ifdef FT900_PLATFORM
   Ft_Gpu_Hal_Wr32(phost,(RAM_DL+Ft_DlBuffer_Index),cmd);
#endif
   /* Increment the command index */
   Ft_DlBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrCoStr_Buffer(Ft_Gpu_Hal_Context_t *phost,const ft_char8_t *s)
{
#ifdef  BUFFER_OPTIMIZATION  
  ft_uint16_t length = 0;
  length = strlen(s) + 1;//last for the null termination
  
  strcpy(&Ft_CmdBuffer[Ft_CmdBuffer_Index],s);  

  /* increment the length and align it by 4 bytes */
  Ft_CmdBuffer_Index += ((length + 3) & ~3);  
#endif  
}

ft_void_t Ft_App_Flush_DL_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_DlBuffer_Index > 0)
     Ft_Gpu_Hal_WrMem(phost,RAM_DL,Ft_DlBuffer,Ft_DlBuffer_Index);
#endif     
   Ft_DlBuffer_Index = 0;
   
}

ft_void_t Ft_App_Flush_Co_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_CmdBuffer_Index > 0)
     Ft_Gpu_Hal_WrCmdBuf(phost,Ft_CmdBuffer,Ft_CmdBuffer_Index);
#endif     
   Ft_CmdBuffer_Index = 0;
}


/* API to give fadeout effect by changing the display PWM from 100 till 0 */
ft_void_t Ft_App_fadeout()
{
   ft_int32_t i;
	
	for (i = 100; i >= 0; i -= 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);

		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
}

/* API to perform display fadein effect by changing the display PWM from 0 till 100 and finally 128 */
ft_void_t Ft_App_fadein()
{
	ft_int32_t i;
	
	for (i = 0; i <=100 ; i += 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
	/* Finally make the PWM 100% */
	i = 128;
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
}


/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
ft_void_t Ft_App_GPU_DLSwap(ft_uint8_t DL_Swap_Type)
{
	ft_uint8_t Swap_Type = DLSWAP_FRAME,Swap_Done = DLSWAP_FRAME;

	if(DL_Swap_Type == DLSWAP_LINE)
	{
		Swap_Type = DLSWAP_LINE;
	}

	/* Perform a new DL swap */
	Ft_Gpu_Hal_Wr8(phost,REG_DLSWAP,Swap_Type);

	/* Wait till the swap is done */
	while(Swap_Done)
	{
		Swap_Done = Ft_Gpu_Hal_Rd8(phost,REG_DLSWAP);

		if(DLSWAP_DONE != Swap_Done)
		{
			Ft_Gpu_Hal_Sleep(10);//wait for 10ms
		}
	}	
}


/* API to demonstrate calibrate widget/functionality */
ft_void_t Ft_App_CoPro_Widget_Calibrate()
{
	ft_uint8_t *pbuff;
	ft_uint32_t NumBytesGen = 0,TransMatrix[6];
	ft_uint16_t CurrWriteOffset = 0;

	/*************************************************************************/
	/* Below code demonstrates the usage of calibrate function. Calibrate    */
	/* function will wait untill user presses all the three dots. Only way to*/
	/* come out of this api is to reset the coprocessor bit.                 */
	/*************************************************************************/
	{
	
	Ft_Gpu_CoCmd_Dlstart(phost);

	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw number at 0,0 location */
	//Ft_App_WrCoCmd_Buffer(phost,COLOR_A(30));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), (FT_DispHeight/2), 27, OPT_CENTER, "Please Tap on the dot");
	Ft_Gpu_CoCmd_Calibrate(phost,0);

	/* Download the commands into FIFIO */
	Ft_App_Flush_Co_Buffer(phost);
	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	/* Print the configured values */
	Ft_Gpu_Hal_RdMem(phost,REG_TOUCH_TRANSFORM_A,(ft_uint8_t *)TransMatrix,4*6);//read all the 6 coefficients
#ifdef MSVC_PLATFORM
	printf("Touch screen transform values are A 0x%x,B 0x%x,C 0x%x,D 0x%x,E 0x%x, F 0x%x",
		TransMatrix[0],TransMatrix[1],TransMatrix[2],TransMatrix[3],TransMatrix[4],TransMatrix[5]);
#endif
	}

}

float Ft_App_lerp(float t, float a, float b)
{
	return (float)((1 - t) * a + t * b);
}
float Ft_App_smoothlerp(float t, float a, float b)
{
	
    float lt = 3 * t * t - 2 * t * t * t;

    return Ft_App_lerp(lt, a, b);
}


ft_void_t Ft_BootupConfig()
{
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

		/* Access address 0 to wake up the FT800 */
		Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);
		Ft_Gpu_Hal_Sleep(20);

		/* Set the clk to external clock */
#if (!defined(ME800A_HV35R) && !defined(ME810A_HV35R) && !defined(ME812AU_WH50R) && !defined(ME813AU_WH50C) && !defined(ME810AU_WH70R) && !defined(ME811AU_WH70C) )
		Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);
		Ft_Gpu_Hal_Sleep(10);
#endif

		{
			ft_uint8_t chipid;
			//Read Register ID to check if FT800 is ready.
			chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
			while(chipid != 0x7C)
			{
				chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
				ft_delay(100);
			}
	#if defined(MSVC_PLATFORM) || defined (FT900_PLATFORM)
			printf("VC1 register ID after wake up %x\n",chipid);
	#endif
	}
	/* Configuration of LCD display */
#ifdef DISPLAY_RESOLUTION_QVGA
	/* Values specific to QVGA LCD display */
	FT_DispWidth = 320;
	FT_DispHeight = 240;
	FT_DispHCycle =  408;
	FT_DispHOffset = 70;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 10;
	FT_DispVCycle = 263;
	FT_DispVOffset = 13;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 2;
	FT_DispPCLK = 8;
	FT_DispSwizzle = 2;
	FT_DispPCLKPol = 0;
	FT_DispCSpread = 1;
	FT_DispDither = 1;

#endif
#ifdef DISPLAY_RESOLUTION_WVGA
	/* Values specific to QVGA LCD display */
	FT_DispWidth = 800;
	FT_DispHeight = 480;
	FT_DispHCycle =  928;
	FT_DispHOffset = 88;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 48;
	FT_DispVCycle = 525;
	FT_DispVOffset = 32;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 3;
	FT_DispPCLK = 2;
	FT_DispSwizzle = 0;
	FT_DispPCLKPol = 1;
	FT_DispCSpread = 0;
	FT_DispDither = 1;
#endif
#ifdef DISPLAY_RESOLUTION_HVGA_PORTRAIT
	/* Values specific to HVGA LCD display */

	FT_DispWidth = 320;
	FT_DispHeight = 480;
	FT_DispHCycle =  400;
	FT_DispHOffset = 40;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 10;
	FT_DispVCycle = 500;
	FT_DispVOffset = 10;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 5;
	FT_DispPCLK = 4;
	FT_DispSwizzle = 2;
	FT_DispPCLKPol = 1;
	FT_DispCSpread = 1;
	FT_DispDither = 1;

#ifdef ME810A_HV35R
	FT_DispPCLK = 5;
#endif

#endif

#if defined(ME800A_HV35R)
	/* After recognizing the type of chip, perform the trimming if necessary */
    Ft_Gpu_ClockTrimming(phost,LOW_FREQ_BOUND);
#endif

	Ft_Gpu_Hal_Wr16(phost, REG_HCYCLE, FT_DispHCycle);
	Ft_Gpu_Hal_Wr16(phost, REG_HOFFSET, FT_DispHOffset);
	Ft_Gpu_Hal_Wr16(phost, REG_HSYNC0, FT_DispHSync0);
	Ft_Gpu_Hal_Wr16(phost, REG_HSYNC1, FT_DispHSync1);
	Ft_Gpu_Hal_Wr16(phost, REG_VCYCLE, FT_DispVCycle);
	Ft_Gpu_Hal_Wr16(phost, REG_VOFFSET, FT_DispVOffset);
	Ft_Gpu_Hal_Wr16(phost, REG_VSYNC0, FT_DispVSync0);
	Ft_Gpu_Hal_Wr16(phost, REG_VSYNC1, FT_DispVSync1);
	Ft_Gpu_Hal_Wr8(phost, REG_SWIZZLE, FT_DispSwizzle);
	Ft_Gpu_Hal_Wr8(phost, REG_PCLK_POL, FT_DispPCLKPol);
	Ft_Gpu_Hal_Wr16(phost, REG_HSIZE, FT_DispWidth);
	Ft_Gpu_Hal_Wr16(phost, REG_VSIZE, FT_DispHeight);
	Ft_Gpu_Hal_Wr16(phost, REG_CSPREAD, FT_DispCSpread);
	Ft_Gpu_Hal_Wr16(phost, REG_DITHER, FT_DispDither);

#if (defined(FT_800_ENABLE) || defined(FT_810_ENABLE) ||defined(FT_812_ENABLE))
    /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
    Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,RESISTANCE_THRESHOLD);
#endif
    #if defined(FT_81X_ENABLE)
        Ft_Gpu_Hal_Wr16(phost, REG_GPIOX_DIR, 0xffff);
        Ft_Gpu_Hal_Wr16(phost, REG_GPIOX, 0xffff);
    #else
        Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
        Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);
    #endif


    /*It is optional to clear the screen here*/
    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);


    Ft_Gpu_Hal_Wr8(phost, REG_PCLK,FT_DispPCLK);//after this display is visible on the LCD


#ifdef ENABLE_ILI9488_HVGA_PORTRAIT
	/* to cross check reset pin */
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);
	ft_delay(120);
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x7f);
	ft_delay(120);
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);

	ILI9488_Bootup();

	/* Reconfigure the SPI */
#ifdef FT900_PLATFORM
	printf("after ILI9488 bootup \n");
	//spi
	// Initialize SPIM HW
	sys_enable(sys_device_spi_master);
	gpio_function(27, pad_spim_sck); /* GPIO27 to SPIM_CLK */
	gpio_function(28, pad_spim_ss0); /* GPIO28 as CS */
	gpio_function(29, pad_spim_mosi); /* GPIO29 to SPIM_MOSI */
	gpio_function(30, pad_spim_miso); /* GPIO30 to SPIM_MISO */

	gpio_write(28, 1);
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
#endif

#endif




	/* make the spi to quad mode - addition 2 bytes for silicon */
#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#ifdef ENABLE_SPI_QUAD
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_QUAD_CHANNEL,FT_GPU_SPI_TWODUMMY);
#elif ENABLE_SPI_DUAL
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_DUAL_CHANNEL,FT_GPU_SPI_TWODUMMY);
#else
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);
#endif

#endif

#ifdef FT900_PLATFORM
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);

#if (defined(ENABLE_SPI_QUAD))
    /* Initialize IO2 and IO3 pad/pin for dual and quad settings */
    gpio_function(31, pad_spim_io2);
    gpio_function(32, pad_spim_io3);
    gpio_write(31, 1);
    gpio_write(32, 1);
#endif

	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);

#ifdef ENABLE_SPI_QUAD
	spi_option(SPIM,spi_option_bus_width,4);	
#elif ENABLE_SPI_DUAL	
	spi_option(SPIM,spi_option_bus_width,2);	
#else	
	spi_option(SPIM,spi_option_bus_width,1);
#endif

	
#endif


phost->ft_cmd_fifo_wp = Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE);

}

/* API to load raw data from file into perticular locatio of ft800 */
ft_int32_t Ft_App_LoadRawFromFile(ft_char8_t *pFileName,ft_uint32_t DstAddr)
{
	ft_int32_t FileLen = 0,Ft800_addr = RAM_G;
	ft_uint8_t *pbuff = NULL;
#ifdef FT900_PLATFORM
	FIL FilSrc;
	unsigned int numBytesRead;
#endif

#if  defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU) 
	FILE *pFile = fopen(pFileName,"rb");//TBD - make platform specific
#endif

#ifdef FT900_PLATFORM

	if(f_open(&FilSrc, pFileName, FA_READ) != FR_OK)
	printf("Error in opening file");
	else
	{

#endif



#if  defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU) 

	/* inflate the data read from binary file */
	if(NULL == pFile)
	{
		printf("Error in opening file %s \n",pFileName);
	}
	else
	{
#endif
		Ft800_addr = DstAddr;

#if  defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU) 
		fseek(pFile,0,SEEK_END);
		FileLen = ftell(pFile);
		fseek(pFile,0,SEEK_SET);
		pbuff = (ft_uint8_t *)malloc(8192);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */			
			fread(pbuff,1,blocklen,pFile);
			FileLen -= blocklen;
			/* copy data continuously into FT800 memory */
			Ft_Gpu_Hal_WrMem(phost,Ft800_addr,pbuff,blocklen);
			Ft800_addr += blocklen;
		}
		/* close the opened binary zlib file */
		fclose(pFile);
		free(pbuff);

#endif


#ifdef FT900_PLATFORM

		FileLen = f_size(&FilSrc);
		pbuff = (ft_uint8_t *)malloc(8192);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */
		//	f_read(pbuff,1,blocklen,pFile);
			f_read(&FilSrc,pbuff,blocklen,&numBytesRead);
			FileLen -= blocklen;
			/* copy data continuously into FT800 memory */
			Ft_Gpu_Hal_WrMem(phost,Ft800_addr,pbuff,blocklen);
			Ft800_addr += blocklen;
		}
		/* close the opened binary zlib file */
		f_close(&FilSrc);
		free(pbuff);
		printf("\nload image finished");
#endif

	}

	return 0;
}

/* APIs useful for bitmap postprocessing */
/* Identity transform matrix */
ft_int16_t Ft_App_TrsMtrxLoadIdentity(Ft_App_GraphicsMatrix_t *pMatrix)
{
	/* Load the identity matrix into the input pointer */
	pMatrix->a = 1*FT_APP_MATIRX_PRECITION;
	pMatrix->b = 0*FT_APP_MATIRX_PRECITION;
	pMatrix->c = 0*FT_APP_MATIRX_PRECITION;
	pMatrix->d = 0*FT_APP_MATIRX_PRECITION;
	pMatrix->e = 1*FT_APP_MATIRX_PRECITION;
	pMatrix->f = 0*FT_APP_MATIRX_PRECITION;
	return 0;
}
ft_int16_t Ft_App_UpdateTrsMtrx(Ft_App_GraphicsMatrix_t *pMatrix,Ft_App_PostProcess_Transform_t *ptrnsmtrx)
{
	/* perform the translation from float to FT800 specific transform units */
	ptrnsmtrx->Transforma = pMatrix->a*FT_APP_MATRIX_GPU_PRECITION;
	ptrnsmtrx->Transformd = pMatrix->d*FT_APP_MATRIX_GPU_PRECITION;
	ptrnsmtrx->Transformb = pMatrix->b*FT_APP_MATRIX_GPU_PRECITION;
	ptrnsmtrx->Transforme = pMatrix->e*FT_APP_MATRIX_GPU_PRECITION;
	ptrnsmtrx->Transformc = pMatrix->c*FT_APP_MATRIX_GPU_PRECITION;
	ptrnsmtrx->Transformf = pMatrix->f*FT_APP_MATRIX_GPU_PRECITION;

	return 0;
}

/* give out the translated values. */
ft_int16_t Ft_App_TrsMtrxTranslate(Ft_App_GraphicsMatrix_t *pMatrix,float x,float y,float *pxres,float *pyres)
{
	float a,b,c,d,e,f;

	/* matrix multiplication */
	a = pMatrix->a;	b = pMatrix->b;	c = pMatrix->c;	d = pMatrix->d;	e = pMatrix->e;	f = pMatrix->f;

	*pxres = x*a + y*b + c;
	*pyres = x*d + y*e + f;
	pMatrix->c = *pxres;
	pMatrix->f = *pyres;
	
	return 0;
}
/* API for rotation of angle degree - using vc++ math library - if not then use array of sine tables to compule both sine and cosine */
/* +ve degree for anti clock wise and -ve for clock wise */
/* note not checking the overflow cases */
ft_int16_t Ft_App_TrsMtrxRotate(Ft_App_GraphicsMatrix_t *pMatrix,ft_int32_t th)
{
	double pi = 3.1415926535;
	double angle = -th*pi/180;
	Ft_App_GraphicsMatrix_t tempmtx,tempinput;


	tempinput.a = pMatrix->a;
	tempinput.b = pMatrix->b;
	tempinput.c = pMatrix->c;
	tempinput.d = pMatrix->d;
	tempinput.e = pMatrix->e;
	tempinput.f = pMatrix->f;

	Ft_App_TrsMtrxLoadIdentity(&tempmtx);
	tempmtx.a = cos(angle)*FT_APP_MATIRX_PRECITION;
	tempmtx.b = sin(angle)*FT_APP_MATIRX_PRECITION;
	tempmtx.d = -sin(angle)*FT_APP_MATIRX_PRECITION;
	tempmtx.e = cos(angle)*FT_APP_MATIRX_PRECITION;

	/* perform matrix multiplecation and store in the input */
	pMatrix->a = tempinput.a*tempmtx.a + tempinput.b*tempmtx.d;
	pMatrix->b = tempinput.a*tempmtx.b + tempinput.b*tempmtx.e;
	pMatrix->c = tempinput.a*tempmtx.c + tempinput.b*tempmtx.f + tempinput.c*1;
	pMatrix->d = tempinput.d*tempmtx.a + tempinput.e*tempmtx.d;
	pMatrix->e = tempinput.d*tempmtx.b + tempinput.e*tempmtx.e;
	pMatrix->f = tempinput.d*tempmtx.c + tempinput.e*tempmtx.f + tempinput.f*1;
	
	return 0;
}
/* Scaling done for x and y factors - from 1 to 255 */
/* Input units are in terms of 65536 */
ft_int16_t Ft_App_TrsMtrxScale(Ft_App_GraphicsMatrix_t *pMatrix,float xfactor,float yfactor)
{

	pMatrix->a /= xfactor;
	pMatrix->d /= xfactor;

	pMatrix->b /= yfactor;
	pMatrix->e /= yfactor;

	return 0;
}
/* flip the image - 1 for right flip, 2 for bottom flip */
ft_int16_t Ft_App_TrsMtrxFlip(Ft_App_GraphicsMatrix_t *pMatrix,ft_int32_t Option)
{
	/* need to verify both */
	if(FT_APP_GRAPHICS_FLIP_RIGHT == (Option & FT_APP_GRAPHICS_FLIP_RIGHT))
	{
		pMatrix->a = -pMatrix->a;
		pMatrix->d = -pMatrix->d;
	}
	if(FT_APP_GRAPHICS_FLIP_BOTTOM == (Option & FT_APP_GRAPHICS_FLIP_BOTTOM))
	{
		pMatrix->b = -pMatrix->b;
		pMatrix->e = -pMatrix->e;
	}

	return 0;
}
/* Arrawy used for custom fonts */
S_LiftAppFont_t G_LiftAppFontArrayNumbers[1] = 
{
	//font structure
	{
	/* Max Width */
	80,
	/* Max Height */
	156,
	/* Max Stride */
	80,
	/* format */
	L8,
	/* Each character width */
	80,80,80,80,80,80,80,80,80,80,80,80,
	}
};
S_LiftAppFont_t G_LiftAppFontArrayArrow[1] = 
{	//arrow structure
	{
	/* Max Width */
	80,
	/* Max Height */
	85,
	/* Max Stride */
	80,
	/* format */
	L8,
	/* Each character width */
	80,
	}
};
/* rate is applied for all the below transitions */

/*	
	lift floor traversal 
	Lift floor stagnant
	Arrow resize
	Floor number resize - resize rate can be same for both arrow and digits
*/

/*	
	control of floor numbers being traversed
	start from a particular fllor and end in the particular floor
*/
//api for control path of the lift application
ft_int32_t LiftAppControlFlow()
{

}

/* API to compute the bitmap offset wrt input characters and scale 
   ResizeVal is in terms of 8 bits precision - ex 1 will be equal to 256
   xOffset and  yOffset are in terms of 4 bits precision - 1/16th pixel format and are the center of the display character
   Even the FT800 display list is generated in this API
*/
ft_int32_t FT_LiftAppComputeBitmap(S_LiftAppFont_t *pLAFont, ft_int32_t FloorNum,ft_uint8_t TotalNumChar,ft_int32_t ResizeVal,ft_int16_t xOffset,ft_int16_t yOffset)
{
	/* compute the total number of digits in the input */
	ft_uint8_t TotalChar = 1;
	ft_int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;
	ft_char8_t FontValArray[8];

	TotalChar = 1;
	FlNum = FloorNum;
	TotHzSz = pLAFont->MaxWidth;
	FontValArray[0] = '\0';//requirement from dec2ascii api

	Ft_Gpu_Hal_Dec2Ascii(FontValArray,FloorNum);
	TotalChar = strlen(FontValArray);
	if(0 != TotalNumChar)
	{
		TotalChar = TotalNumChar;
	}
	TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	
	//change the x and y offset to center of the characters
	xoff = xOffset*16 - TotHzSz*TotalChar/2;
	yoff = yOffset*16 - TotVtSz/2;
	/* since resize value is same for both the sides the resize is same for all characters */
	for(i=0;i<TotalChar;i++)
	{
		/* Calculation of cell number from  */
		if('-' == FontValArray[i])
		{
			Ft_App_WrCoCmd_Buffer(phost,CELL(0));
		}
		else
		{

			Ft_App_WrCoCmd_Buffer(phost,CELL(FontValArray[i]-'0' + 1));
		}
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
		//increment the character after every ittiration
		xoff += TotHzSz;
	}
	return 0;
}
//API for colomn bitmap computation
ft_int32_t FT_LiftAppComputeBitmapColomn(S_LiftAppFont_t *pLAFont, ft_int32_t FloorNum,ft_uint8_t TotalNumChar,ft_int32_t ResizeVal,ft_int16_t xOffset,ft_int16_t yOffset)
{
	/* compute the total number of digits in the input */
	ft_uint8_t TotalChar = 1;
	ft_int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;
	ft_char8_t FontValArray[8];

	TotalChar = 1;
	FlNum = FloorNum;
	TotHzSz = pLAFont->MaxWidth;
	FontValArray[0] = '\0';//requirement from dec2ascii api

	Ft_Gpu_Hal_Dec2Ascii(FontValArray,FloorNum);
	TotalChar = strlen(FontValArray);
	if(0 != TotalNumChar)
	{
		TotalChar = TotalNumChar;
	}
	TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	
	//change the x and y offset to center of the characters
	xoff = xOffset*16 - TotHzSz/2;
	yoff = yOffset*16 - TotVtSz*TotalChar/2;
	/* since resize value is same for both the sides the resize is same for all characters */
	for(i=0;i<TotalChar;i++)
	{
		/* Calculation of cell number from  */
		if('-' == FontValArray[i])
		{
			Ft_App_WrCoCmd_Buffer(phost,CELL(0));
		}
		else
		{
			Ft_App_WrCoCmd_Buffer(phost,CELL(FontValArray[i]-'0' + 1));
		}
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
		//increment the character after every ittiration
		//xoff += TotHzSz;
		yoff += TotVtSz;
	}
	return 0;
}

ft_int32_t FT_LiftAppComputeBitmapRowRotate(S_LiftAppFont_t *pLAFont, ft_int32_t FloorNum,ft_uint8_t TotalNumChar,ft_int32_t ResizeVal,ft_int16_t xOffset,ft_int16_t yOffset)
{
	/* compute the total number of digits in the input */
	ft_uint8_t TotalChar = 1;
	ft_int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;
	ft_char8_t FontValArray[8];

	TotalChar = 1;
	FlNum = FloorNum;
	TotHzSz = pLAFont->MaxWidth;
	FontValArray[0] = '\0';//requirement from dec2ascii api

	Ft_Gpu_Hal_Dec2Ascii(FontValArray,FloorNum);
	TotalChar = strlen(FontValArray);
	if(0 != TotalNumChar)
	{
		TotalChar = TotalNumChar;
	}
	TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	
	//change the x and y offset to center of the characters
	xoff = xOffset*16 - TotVtSz/2;
	yoff = yOffset*16 - (TotHzSz*TotalChar/2) + (TotHzSz*(TotalChar-1));
	/* since resize value is same for both the sides the resize is same for all characters */
	for(i=0;i<TotalChar;i++)
	{
		/* Calculation of cell number from  */
		if('-' == FontValArray[i])
		{
			Ft_App_WrCoCmd_Buffer(phost,CELL(0));
		}
		else
		{

			Ft_App_WrCoCmd_Buffer(phost,CELL(FontValArray[i]-'0' + 1));
		}
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
		//increment the character after every ittiration
		yoff -= TotHzSz;
	}
	return 0;
}

//API for single bitmap computation - follow the above api for more details
ft_int32_t FT_LiftAppComputeBitmap_Single(S_LiftAppFont_t *pLAFont, ft_int32_t BitmapIdx,ft_int32_t ResizeVal,ft_int16_t xOffset,ft_int16_t yOffset)
{
	/* compute the total number of digits in the input */
	ft_int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;

	TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
	
	//change the x and y offset to center of the characters
	xoff = xOffset*16 - TotHzSz/2;
	yoff = yOffset*16 - TotVtSz/2;

	Ft_App_WrCoCmd_Buffer(phost,CELL(BitmapIdx));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));

	return 0;
}

/* API to play the music files */
ft_uint32_t play_music(ft_char8_t *pFileName,ft_uint32_t dstaddr,ft_uint8_t Option,ft_uint32_t Buffersz)
{
#ifdef FT900_PLATFORM
		ft_uint32_t filesz = 0,chunksize = 16*1024,totalbufflen = 64*1024,currreadlen = 0;
		ft_uint8_t *pBuff = NULL;
		ft_uint32_t wrptr = dstaddr,return_val = 0;
		ft_uint32_t rdptr,freebuffspace;
		SDHOST_STATUS    SDHostStatus;
				FATFS            FatFs;				// FatFs work area needed for each volume
				FIL              FilSrc;			// File object needed for each open file
				FIL              FilDes;			// File object needed for each open file
				FRESULT          fResult;			// Return value of FatFs APIs
				unsigned int     numBytesWritten, numBytesRead;
				unsigned char    buffer[32];
				unsigned long    crcSrc, crcDes;
					SDHostStatus = sdhost_card_detect();

					if (sdhost_card_detect()== SDHOST_CARD_INSERTED)
					{
						printf("\nCard inserted!\n");
					}

					if (f_open(&FilSrc, "SerenadeChopin.raw", FA_READ) != FR_OK)
								{
						;
								}

#else
	FILE *pFile = NULL;
	ft_uint32_t filesz = 0,chunksize = 16*1024,totalbufflen = 64*1024,currreadlen = 0;
	ft_uint8_t *pBuff = NULL;
	ft_uint32_t wrptr = dstaddr,return_val = 0;
	ft_uint32_t rdptr,freebuffspace;
//	pFile = fopen("..\\..\\..\\..\\Test\\SerenadeChopin.raw","rb+");
#endif
	if(2 == Option)
	{
		Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,dstaddr);//Audio playback start address 
		//Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,totalbufflen);//Length of raw data buffer in bytes		
		Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,Buffersz);
		Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
		Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);				
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);		
		return;
	}
#ifdef FT900_PLATFORM
	if(0 == music_playing)
		{
		if (f_open(&FilSrc, *pFileName, FA_READ) != FR_OK)
										{
			printf("ho1ho \n");}
										}
		filesz=f_size(&FilSrc);

		f_lseek(&FilSrc,42);
			filesz=0;
		fileszsave = filesz;
		printf("\nfilesz %d\n",filesz);
		pBuff = (ft_uint8_t *)malloc(totalbufflen);

		while(filesz > 0)
		{
			currreadlen = filesz;

			if(currreadlen > chunksize)
			{
				currreadlen = chunksize;
			}
			//fread(pBuff,1,currreadlen,pFile);
			f_read(&FilSrc,pBuff,currreadlen,&numBytesRead);
			Ft_Gpu_Hal_WrMem(phost, wrptr, (ft_uint8_t *)pBuff,currreadlen);
			wrptr +=  currreadlen;
			if(wrptr > (dstaddr + totalbufflen))
			{
				wrptr = dstaddr;
			}


			filesz -= currreadlen;
		}
			if((0 == music_playing) && (0 == Option))
			{
				Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,dstaddr);//Audio playback start address
				Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,fileszsave);
				Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
				Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
				Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
				Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);
				Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);
			    music_playing = 1;
			}

			{
				//Check the frees0ace if the file has more
				rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR);
				while(totalbufflen < fileszsave)
				{
					Ft_Gpu_CoCmd_MemSet(phost,wrptr,0,chunksize);
				}

			}


		//if read pointer is already passed over write pointer
		if (Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr) {
			//wait till the read pointer will be wrapped over
			while(Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr);
		}
			f_close(&FilSrc);
			free(pBuff);
			if(filesz == 0)
			{
				dstaddr = dstaddr + fileszsave;
				music_playing = 0;
				return dstaddr;
			}

else if(0 == Option)
		{
			rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) - dstaddr;
			if((fileszsave <= rdptr) || (0 == rdptr))
			{
				music_playing = 0;
			}

		}

#else


	if(0 == music_playing)
	{
	pFile = fopen(pFileName,"rb+");
	
	fseek(pFile,0,SEEK_END);
	filesz = ftell(pFile);

	
	fseek(pFile,42,SEEK_SET);
	filesz = 0;
	fread(&filesz,1,4,pFile);

	fileszsave = filesz;
	
	pBuff = (ft_uint8_t *)malloc(totalbufflen);

	while(filesz > 0)
	{
		currreadlen = filesz;

		if(currreadlen > chunksize)
		{
			currreadlen = chunksize;
		}
		fread(pBuff,1,currreadlen,pFile);
		
		Ft_Gpu_Hal_WrMemFromFlash(phost, wrptr, (ft_uint8_t *)pBuff,currreadlen);
		wrptr +=  currreadlen;
		if(wrptr > (dstaddr + totalbufflen))
		{
			wrptr = dstaddr;
		}
		
			
		filesz -= currreadlen;
	     
		if((0 == music_playing) && (0 == Option))
		{
			Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,dstaddr);//Audio playback start address 
			Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,fileszsave);
			Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
			Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);				
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);		
		    music_playing = 1;
		}

		{
			//Check the frees0ace if the file has more
			rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR);
			while(totalbufflen < fileszsave)
			{
				Ft_Gpu_CoCmd_MemSet(phost,wrptr,0,chunksize);
			}

		}
	}

	//if read pointer is already passed over write pointer
	if (Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr) {
		//wait till the read pointer will be wrapped over 
		while(Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr);
	}
		fclose(pFile);
		free(pBuff);
		if(filesz == 0)
		{
			dstaddr = dstaddr + fileszsave;
			music_playing = 0;
			return dstaddr;
		}
	}
	else if(0 == Option)
	{
		rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) - dstaddr;
		if((fileszsave <= rdptr) || (0 == rdptr))
		{
			music_playing = 0;
		}
		
	}
#endif
	/* return the current write pointer in any case */
	return wrptr;
}
#ifdef FT900_PLATFORM
ft_int32_t FT_LiftApp_FTransition(S_LiftAppCtxt_t *pLACtxt,ft_uint32_t dstaddr)
{

	ft_char8_t StringArray[100],StringArray_mid[100],audio_array[100];
	ft_int32_t dst_addr_st_file;
	StringArray[0] = '\0';
	StringArray_mid[0] = '\0';
	/* check for the current floor number logic */
	if(pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum)
	{

		/* first time only entry */
		if(LIFTAPP_DIR_NONE != pLACtxt->ArrowDir)
		{
			pLACtxt->CurrFloorNumStagnantRate = pLACtxt->SLATransPrms.FloorNumStagnantRate;
			pLACtxt->CurrFloorNumResizeRate = pLACtxt->SLATransPrms.ResizeRate;
		}
		/* Make the direction to 0 */
		pLACtxt->ArrowDir = LIFTAPP_DIR_NONE;//arrow is disabled in none direction
		{
			if((0 == pLACtxt->AudioPlaying) && (1 == pLACtxt->Orientation)){

				/* load the ring tone */

				dst_addr_st_file = play_music("bl.wav",dstaddr,1,0);/* load the floor number audio */

				/* load the floor number audio */


				/* load the floor number audio */
			//	strcat(StringArray_mid,"..\\..\\..\\..\\Test\\");
				if(pLACtxt->CurrFloorNum){
					Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
					//strcat(StringArray_mid, StringArray);
				//	strcat(StringArray_mid, ".wav");
					dst_addr_st_file = play_music("bl.wav",dst_addr_st_file,1,0);
					play_music("bl.wav",dstaddr,2,dst_addr_st_file - dstaddr);


				}

				/*else if(pLACtxt->CurrFloorNum == -1)
				{
					dst_addr_st_file = play_music("..\\..\\..\\..\\Test\\-1.wav",dst_addr_st_file,1,0);
					play_music("..\\..\\..\\..\\Test\\base1.wav",dstaddr,2,dst_addr_st_file - dstaddr);
					
					
				}
				else if(pLACtxt->CurrFloorNum == -2)
				{
					dst_addr_st_file = play_music("..\\..\\..\\..\\Test\\-2.wav",dst_addr_st_file,1,0);
					play_music("..\\..\\..\\..\\Test\\base2.wav",dstaddr,2,dst_addr_st_file - dstaddr);
					
					
				}*/
				pLACtxt->AudioPlaying = 1;

				
			}

			else if((0 == pLACtxt->AudioPlaying) && (0 == pLACtxt->Orientation)) {



						/* load the ring tone */
							dst_addr_st_file = play_music("bf.wav",dstaddr,1,0);

				/* load the floor number audio */

				Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
			//	strcat(StringArray_mid, StringArray);
				//strcat(StringArray_mid, ".wav");
				dst_addr_st_file = play_music("bf.wav",dst_addr_st_file,1,0);
				play_music("bf.wav",dstaddr,2,dst_addr_st_file - dstaddr);
				pLACtxt->AudioPlaying = 1;

		}

			
		}
		
		/* resizing of floor number */
		if(pLACtxt->CurrFloorNumResizeRate > 0)
		{
			pLACtxt->CurrFloorNumResizeRate--;
			return 0;
		}
		pLACtxt->CurrFloorNumResizeRate = 0;
		
		/* rate implmenetation */
		if(pLACtxt->CurrFloorNumStagnantRate > 0)
		{
			pLACtxt->CurrFloorNumStagnantRate--;//rate based logic
			return 0;//do not perform any thing
		}
		pLACtxt->CurrFloorNumStagnantRate = 0;
		pLACtxt->AudioPlaying = 0;
		/* delay the below code till rate is completed - modify the rate if application over rights */
		//limitation - make sure the max is +ve
		while((pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum) || (0 == pLACtxt->NextFloorNum))//to make sure the same floor number is not assigned
		{
			pLACtxt->NextFloorNum = pLACtxt->SLATransPrms.MinFloorNum + ft_random(pLACtxt->SLATransPrms.MaxFloorNum - pLACtxt->SLATransPrms.MinFloorNum);
		}

		pLACtxt->ArrowDir = LIFTAPP_DIR_DOWN;
		/* generate a new random number and change the direction as well */
		if(pLACtxt->NextFloorNum > pLACtxt->CurrFloorNum)
		{
			pLACtxt->ArrowDir = LIFTAPP_DIR_UP;
			play_music("gu.wav",dstaddr,0,0);
		}
		else
		{
			play_music("gd.wav",dstaddr,0,0);
			printf("going downing");
		}
		
		/* set the starting of the arrow resize */
		pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
		//set the curr floor number change 
		pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
		return 0;
	}

	/* moving up or moving down logic */
	/* rate based on count */
	pLACtxt->CurrArrowResizeRate--;
	if(pLACtxt->CurrArrowResizeRate <= 0)
	{
		pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
	}
	//changing floor numbers
	pLACtxt->CurrFloorNumChangeRate--;
	if(pLACtxt->CurrFloorNumChangeRate <= 0)
	{
		pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
		//change the floor number wrt direction
		if(LIFTAPP_DIR_DOWN == pLACtxt->ArrowDir)
		{
			pLACtxt->CurrFloorNum -=1;
			if(0 == pLACtxt->CurrFloorNum)
			{
				pLACtxt->CurrFloorNum -=1;
			}
		}
		else
		{
			pLACtxt->CurrFloorNum +=1;
			if(0 == pLACtxt->CurrFloorNum)
			{
				pLACtxt->CurrFloorNum +=1;
			}
		}
	}
	return 0;
}

#else
/* API for floor number logic */
ft_int32_t FT_LiftApp_FTransition(S_LiftAppCtxt_t *pLACtxt,ft_uint32_t dstaddr)
{

	ft_char8_t StringArray[100],StringArray_mid[100],audio_array[100];
	ft_int32_t dst_addr_st_file;
	StringArray[0] = '\0';
	StringArray_mid[0] = '\0';
	/* check for the current floor number logic */
	if(pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum)
	{

		/* first time only entry */
		if(LIFTAPP_DIR_NONE != pLACtxt->ArrowDir)
		{
			pLACtxt->CurrFloorNumStagnantRate = pLACtxt->SLATransPrms.FloorNumStagnantRate;
			pLACtxt->CurrFloorNumResizeRate = pLACtxt->SLATransPrms.ResizeRate;
		}
		/* Make the direction to 0 */
		pLACtxt->ArrowDir = LIFTAPP_DIR_NONE;//arrow is disabled in none direction
		{
			if((0 == pLACtxt->AudioPlaying) && (1 == pLACtxt->Orientation)){

				/* load the ring tone */

				dst_addr_st_file = play_music("..\\..\\..\\Test\\bl.wav",dstaddr,1,0);/* load the floor number audio */

				/* load the floor number audio */


				/* load the floor number audio */
				strcat(StringArray_mid,"..\\..\\..\\Test\\");
				if(pLACtxt->CurrFloorNum){
					Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
					strcat(StringArray_mid, StringArray);
					strcat(StringArray_mid, ".wav");
					dst_addr_st_file = play_music(StringArray_mid,dst_addr_st_file,1,0);
					play_music(StringArray_mid,dstaddr,2,dst_addr_st_file - dstaddr);


				}

				/*else if(pLACtxt->CurrFloorNum == -1)
				{
					dst_addr_st_file = play_music("..\\..\\..\\..\\Test\\-1.wav",dst_addr_st_file,1,0);
					play_music("..\\..\\..\\..\\Test\\base1.wav",dstaddr,2,dst_addr_st_file - dstaddr);


				}
				else if(pLACtxt->CurrFloorNum == -2)
				{
					dst_addr_st_file = play_music("..\\..\\..\\..\\Test\\-2.wav",dst_addr_st_file,1,0);
					play_music("..\\..\\..\\..\\Test\\base2.wav",dstaddr,2,dst_addr_st_file - dstaddr);


				}*/
				pLACtxt->AudioPlaying = 1;


			}


			else if((0 == pLACtxt->AudioPlaying) && (0 == pLACtxt->Orientation)) {



						/* load the ring tone */
							dst_addr_st_file = play_music("..\\..\\..\\Test\\bf.wav",dstaddr,1,0);

				/* load the floor number audio */
				strcat(StringArray_mid,"..\\..\\..\\Test\\");
				Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
				strcat(StringArray_mid, StringArray);
				strcat(StringArray_mid, ".wav");
				dst_addr_st_file = play_music(StringArray_mid,dst_addr_st_file,1,0);
				play_music(StringArray_mid,dstaddr,2,dst_addr_st_file - dstaddr);
				pLACtxt->AudioPlaying = 1;

		}


		}

		/* resizing of floor number */
		if(pLACtxt->CurrFloorNumResizeRate > 0)
		{
			pLACtxt->CurrFloorNumResizeRate--;
			return 0;
		}
		pLACtxt->CurrFloorNumResizeRate = 0;

		/* rate implmenetation */
		if(pLACtxt->CurrFloorNumStagnantRate > 0)
		{
			pLACtxt->CurrFloorNumStagnantRate--;//rate based logic
			return 0;//do not perform any thing
		}
		pLACtxt->CurrFloorNumStagnantRate = 0;
		pLACtxt->AudioPlaying = 0;
		/* delay the below code till rate is completed - modify the rate if application over rights */
		//limitation - make sure the max is +ve
		while((pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum) || (0 == pLACtxt->NextFloorNum))//to make sure the same floor number is not assigned
		{
			pLACtxt->NextFloorNum = pLACtxt->SLATransPrms.MinFloorNum + ft_random(pLACtxt->SLATransPrms.MaxFloorNum - pLACtxt->SLATransPrms.MinFloorNum);
		}

		pLACtxt->ArrowDir = LIFTAPP_DIR_DOWN;
		/* generate a new random number and change the direction as well */
		if(pLACtxt->NextFloorNum > pLACtxt->CurrFloorNum)
		{
			pLACtxt->ArrowDir = LIFTAPP_DIR_UP;
			play_music("..\\..\\..\\Test\\gu.wav",dstaddr,0,0);
		}
		else
		{
			play_music("..\\..\\..\\Test\\gd.wav",dstaddr,0,0);
		}

		/* set the starting of the arrow resize */
		pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
		//set the curr floor number change
		pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
		return 0;
	}

	/* moving up or moving down logic */
	/* rate based on count */

	pLACtxt->CurrArrowResizeRate--;
	if(pLACtxt->CurrArrowResizeRate <= 0)
	{
		pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
	}
	//changing floor numbers
	pLACtxt->CurrFloorNumChangeRate--;
	if(pLACtxt->CurrFloorNumChangeRate <= 0)
	{
		pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
		//change the floor number wrt direction
		if(LIFTAPP_DIR_DOWN == pLACtxt->ArrowDir)
		{
			pLACtxt->CurrFloorNum -=1;
			if(0 == pLACtxt->CurrFloorNum)
			{
				pLACtxt->CurrFloorNum -=1;
			}
		}
		else
		{
			pLACtxt->CurrFloorNum +=1;
			if(0 == pLACtxt->CurrFloorNum)
			{
				pLACtxt->CurrFloorNum +=1;
			}
		}
	}
	return 0;
}
#endif
/* API to get the rate of the running text*/
float linear(float p1,float p2,ft_uint16_t t,ft_uint16_t rate)
{
       float st  = (float)t/rate;
       return p1+(st*(p2-p1));
}

/*API to display the fonts in both portrait and landscape orientations*/
ft_void_t font_display(ft_int16_t BMoffsetx,ft_int16_t BMoffsety,ft_uint32_t stringlenghth,ft_char8_t *string_display,ft_uint8_t opt_landscape)
{
	ft_uint16_t k;
	if(0 == opt_landscape)
	{
	for(k=0;k<stringlenghth;k++)
		{
#ifdef DISPLAY_RESOLUTION_WVGA
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(8));
		Ft_App_WrCoCmd_Buffer(phost, CELL(string_display[k]));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(BMoffsetx*16,BMoffsety*16));
#else
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,8,string_display[k]));
#endif
			BMoffsety -= g_Gpu_Fonts[8].FontWidth[string_display[k]];
		}
	}
	else if(1 == opt_landscape)
	{
			for(k=0;k<stringlenghth;k++)
		{
#ifdef DISPLAY_RESOLUTION_WVGA
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(28));
		Ft_App_WrCoCmd_Buffer(phost, CELL(string_display[k]));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(BMoffsetx*16,BMoffsety*16));
#else
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,28,string_display[k]));

#endif
				BMoffsetx += g_Gpu_Fonts[8].FontWidth[string_display[k]];
		}
	
	}
}

/* Lift demo application demonstrates background animation and foreground digits and arrow direction in landscape orientation */
ft_void_t FT_LiftApp_Landscape()
{
	/* Download the raw data of custom fonts into memory */
	ft_int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
	ft_int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16,xoffset,yoffset;
	ft_int32_t Baddr0,Baddr1,Baddr2,i,SzInc = 16,SzFlag = 0;
	ft_uint8_t fontr = 255,fontg = 255,fontb = 255,minute_temp,day_temp;
	S_LiftAppRate_t *pLARate;
	S_LiftAppTrasParams_t *pLATParams;
	S_LiftAppBallsLinear_t S_LiftBallsArray[8*10],*pLiftBalls = NULL;//as of now 80 balls are been plotted
	Ft_App_GraphicsMatrix_t S_GPUMatrix;
	Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
	float temptransx,temptransy,running_rate;
	S_LiftAppCtxt_t S_LACtxt;
	
#ifndef FT900_PLATFORM

	SYSTEMTIME st, lt;
#else
    static unsigned int curr_rtc_clock;
	static ft_uint32_t  counter,time_curr,tt,count_t;
	localtime st,lt;
#endif

	ft_uint16_t hour,hour_st,hour_nd,min,min_st,min_nd,sec,year,month,day,year_st_nd_dig,year_st_dig,year_nd_dig,year_rd_frth_dig,year_rd_dig,year_frth_dig;
	ft_uint16_t day_st, day_nd;
	ft_char8_t *day_of_week=" Sun, ",nd_line[100];
	ft_char8_t text_run_disp[] = "The running text is displayed here.";
	ft_char8_t *hour_st_str, *hour_nd_str, *min_st_str, *min_nd_str,*day_st_str,*day_nd_str,*year_st_str, *year_nd_str, *year_rd_str, *year_frth_str;
	ft_char8_t* month_of_year;
	ft_uint32_t stringlen,FontTableAddress,k,j,l,stringlen_week,stringlen_day,stringlen_text_run_disp,stringlen_st;
	ft_int32_t FontWid = 0;

	ft_uint8_t *pbuff;
	ft_int32_t FileLen=0;
	FT_Gpu_Fonts_t Display_fontstruct;
	float t=0;
	#ifdef FT900_PLATFORM
	float ttt ;
	#endif
	ft_uint16_t m;

	nd_line[0] = '0';
#ifdef FT900_PLATFORM
	 curr_rtc_clock++;
								  time_curr=curr_rtc_clock%1024;
								  t=curr_rtc_clock/32768*1000%1000;
								  if (time_curr<tt)
								  {
								       count_t++;
								  }
#endif
	/* load the bitmap raw data */

	/* Initial setup code to setup all the required bitmap handles globally */
	Ft_DlBuffer_Index = 0;
	
	#ifdef FT900_PLATFORM
	pLARate->IttrCount = &S_LACtxt.SLARate.IttrCount;
	#else
	pLARate = &S_LACtxt.SLARate;
	#endif
	pLATParams = &S_LACtxt.SLATransPrms;


	//initialize all the rate parameters
	pLARate->CurrTime = 0;
	pLARate->IttrCount = 0;

	/* Initialize all the transition parameters - all the below are in terms of basic units of rate
	either they can be based on itterations or based on time giffies */
	pLATParams->FloorNumChangeRate = 64;
	pLATParams->MaxFloorNum = 7;
	pLATParams->MinFloorNum = -2;
	pLATParams->ResizeRate = 32;
	pLATParams->ResizeDimMax = 2*256;
	pLATParams->ResizeDimMin = 1*256;
	pLATParams->FloorNumStagnantRate = 128;

	/* Initialization of lift context parameters */
	S_LACtxt.ArrowDir = LIFTAPP_DIR_DOWN;//going down direction
	S_LACtxt.CurrFloorNum = 5;//current floor number to start with
	S_LACtxt.NextFloorNum = -2;//destination floor number as of now
	S_LACtxt.CurrArrowResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumStagnantRate = 0;
	S_LACtxt.CurrFloorNumChangeRate = S_LACtxt.SLATransPrms.FloorNumChangeRate;
	S_LACtxt.AudioPlaying = 0;
	S_LACtxt.Orientation = 1;

	
	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen

	Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS));
	Baddr0 = RAM_G;

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("font.raw",RAM_G);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\font.raw",RAM_G);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr0));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BWd*2, BHt*2));

	Baddr1 = ((Baddr0 + BSt*BHt*11 + 15)&~15);
//	Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\arr.raw",Baddr1);

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("arr.raw",Baddr1);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\arr.raw",Baddr1);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr1));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowWd*2, BArrowHt*2));//make sure the bitmap is displayed when rotation happens

	Baddr2 = Baddr1 + 80*85;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs6.raw",Baddr2);

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs6.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs6.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 60, 60));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));
	Baddr2 += 60*55;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs5.raw",Baddr2);

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs5.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs5.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));
	Baddr2 += 50*46;

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs4.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs4.raw",Baddr2);
#endif
//	Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs4.raw",Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 40, 37));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 40, 37));
	Baddr2 += 40*37;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs3.raw",Baddr2);

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs3.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs3.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));
	Baddr2 += 30*27;
//	Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs2.raw",Baddr2);

#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs2.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs2.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));
	Baddr2 += 20*18;
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs1.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs1.raw",Baddr2);
#endif
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs1.raw",Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0,0,7,0));
	Baddr2 += 10*10;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\logo.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("logo.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\logo.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 129*2, 50));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 129*2, 50));//make sure whole bitmap is displayed after rotation - as height is greater than width
	Baddr2 += 129*2*50;

	// Read the font address from 0xFFFFC location 
	
	FontTableAddress = Ft_Gpu_Hal_Rd32(phost,0xFFFFC);
	
	Ft_CmdBuffer_Index = 0;
	
	/* Download the commands into fifo */	
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Baddr2 += 10*10;
	Baddr2 =  ((Baddr2 + 7)&~7);//audio engine's requirement
	
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY());
	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	Ft_App_GPU_DLSwap(DLSWAP_FRAME);
	Ft_Gpu_Hal_Sleep(30);

	/* compute the random values at the starting */
	pLiftBalls = S_LiftBallsArray;
	for(i=0;i<(NumBallsRange*NumBallsEach);i++)
	{
		pLiftBalls->xOffset = ft_random(FT_DispWidth*16);
		pLiftBalls->yOffset = ft_random(FT_DispHeight*16);
		pLiftBalls->dx = ft_random(RandomVal*8) - RandomVal*4;
		pLiftBalls->dy = -1*ft_random(RandomVal*8);
		pLiftBalls++;
	}
#ifdef FT900_PLATFORM
								st.wHour=15;
								st.wMinute=59;
								st.wDay=5;
								lt.wDayOfWeek=3;
								st.wYear=2014;
								st.wMilliseconds=0;
								st.wSecond=0;
								st.wMonth=11;
#endif
	while(1)
	{		
		/* Logic of user touch - change background or skin wrt use touch */
		/* Logic of transition - change of floor numbers and direction */
		FT_LiftApp_FTransition(&S_LACtxt,Baddr2);		

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		/* Background gradient */
		Ft_Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,0,FT_DispHeight,0x132B3B);

		/* Draw background bitmaps */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		pLiftBalls = S_LiftBallsArray;
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(64));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		for(i=0;i<(NumBallsRange*NumBallsEach);i++)
		{
			/* handle inst insertion - check for the index */
			if(0 == (i%NumBallsEach))
			{
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2 + (i/NumBallsEach)));
			}
			/* recalculate the background balls offset and respective rate when ball moves out of the diaply area */
			if( (pLiftBalls->xOffset > ((FT_DispWidth + 60)*16)) || 
				(pLiftBalls->yOffset > ((FT_DispHeight + 60) *16)) ||
				(pLiftBalls->xOffset < (-60*16)) || 
				(pLiftBalls->yOffset < (-60*16)) )
			{
				/* always offset starts from below the screen and moves upwards */
				pLiftBalls->xOffset = ft_random(FT_DispWidth*16);
				pLiftBalls->yOffset = FT_DispHeight*16 + ft_random(60*16);

				pLiftBalls->dx = ft_random(RandomVal*8) - RandomVal*4;
				pLiftBalls->dy = -1*ft_random(RandomVal*8);
			}
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(pLiftBalls->xOffset, pLiftBalls->yOffset));
			pLiftBalls->xOffset += pLiftBalls->dx;
			pLiftBalls->yOffset += pLiftBalls->dy;
			pLiftBalls++;
		}
		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost,ALPHA_FUNC(ALWAYS,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));

		if(LIFTAPP_DIR_NONE != S_LACtxt.ArrowDir)//do not display the arrow in case of no direction, meaning stagnant
		{

			//calculation of size value based on the rate
			//the bitmaps are scaled from original resolution till 2 times the resolution on both x and y axis
			SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrArrowResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
			Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
			/* Draw the arrow first */
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1)); 
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
			//BMoffsetx = ((FT_DispWidth/4) - (BArrowWd*SzInc/32));
			BMoffsetx = (BArrowWd - (BArrowWd*SzInc/32));
			BMoffsety = ((FT_DispHeight/2) - (BArrowHt*SzInc/32));

			/* perform inplace flip and scale of bitmap in case of direction is up */
			if(LIFTAPP_DIR_UP == S_LACtxt.ArrowDir)
			{
				Ft_App_TrsMtrxLoadIdentity(&S_GPUMatrix);
				Ft_App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
				Ft_App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
				Ft_App_TrsMtrxFlip(&S_GPUMatrix,FT_APP_GRAPHICS_FLIP_BOTTOM);		
				Ft_App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
		
				//Ft_App_TrsMtrxTranslate(&S_GPUMatrix,-80/2.0,-85/2.0,&temptransx,&temptransy);

				Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
			}
			else if(LIFTAPP_DIR_DOWN == S_LACtxt.ArrowDir)
			{
				//perform only scaling as rotation is not required
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256*16/SzInc));
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256*16/SzInc));			
			}
/////////////////////////%%%%%%%%%%%%%%%        adjust the location  of arrow
		//	FT_LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd,(FT_DispHeight/2)+);
#ifdef DISPLAY_RESOLUTION_WVGA
 	FT_LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd*2+40,(FT_DispHeight*2/4));

#elif defined DISPLAY_RESOLUTION_HVGA_PORTRAIT
			FT_LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd*2,(FT_DispHeight*2/8));
#else
			FT_LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd,(FT_DispHeight/2));		
#endif
			Ft_Gpu_CoCmd_LoadIdentity(phost);
			Ft_Gpu_CoCmd_SetMatrix(phost);
		}
		/* Draw the font bitmaps */
		/* algorithm for animation - floor numbers will change in dimensions when stagnant at a perticular floor */
		/* arrow will change in case of movement of lift */
		/* display the bitmap with increased size */
		SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrFloorNumResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
		//BMoffsetx = ((FT_DispWidth*3/4) - (BWd/2));
		//BMoffsetx = ((FT_DispWidth*3/4) - (BWd*SzInc/32));
		BMoffsetx = FT_DispWidth - BWd*2 - (BWd*SzInc/32);
		
		//BMoffsety = ((FT_DispHeight/2) - (BHt/2));
		BMoffsety = ((FT_DispHeight/2) - (BHt*SzInc/32));
		/* calculate the resolution change based on the number of characters used as well as */
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256*16/SzInc));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256*16/SzInc));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0)); 
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
		//FT_LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BWd*2,(FT_DispHeight/2));
////&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&adjust the number location
#ifdef DISPLAY_RESOLUTION_WVGA
	FT_LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth*3/4 ,(FT_DispHeight/2));

#elif defined DISPLAY_RESOLUTION_HVGA_PORTRAIT
		FT_LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth/2 ,(FT_DispHeight*3/4));
#else
		FT_LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BWd*2,(FT_DispHeight/2));
#endif	
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256));

#ifndef FT900_PLATFORM
	/*	lt.wMonth=9;
		lt.wHour=15;
		lt.wMinute=45;
		lt.wDay=25;
		lt.wYear=2014;
		lt.wMilliseconds=0;
		lt.wSecond=0;
		lt.wMonth=9;
				st.wHour=15;
				st.wMinute=45;
				st.wDay=25;
				st.wYear=2014;
				st.wMilliseconds=0;
				st.wSecond=0;
*/
		GetSystemTime(&st);
		GetLocalTime(&lt);
#else
			/*	lt.wMonth=9;
						lt.wHour=9;
						lt.wMinute=45;
						lt.wDay=25;
					    lt.wYear=2014;
					  	lt.wMilliseconds=0;
						lt.wSecond=0;
				   		lt.wMonth=9;*/


								/* curr_rtc_clock = rtcGetCurrentCounterVal();
								 printf("  \n curr_rtc_clock %d \n",curr_rtc_clock);
								  time_curr=curr_rtc_clock%1024;
								  t=curr_rtc_clock/32768*1000%1000;
								  if (time_curr<tt)
								  {
								       count_t++;
								  }

								  lt.wSecond=curr_rtc_clock/32768%60;

								       lt.wMinute=(curr_rtc_clock/32768/60+st.wMinute)%60;
								       lt.wHour=(curr_rtc_clock/32768/3600+st.wHour)%12;
								       lt.wMonth=st.wMonth;
								       lt.wYear=2014;
								       lt.wDay=28;*/


										// curr_rtc_clock = rtcGetCurrentCounterVal();
		curr_rtc_clock++;
																// printf("  \n curr_rtc_clock %d \n",curr_rtc_clock);
																  time_curr=curr_rtc_clock%32;
																  t=curr_rtc_clock/32*1000%1000;
																  if (time_curr<tt)
																  {
																       count_t++;
																  }

																  	   lt.wSecond=curr_rtc_clock/32%60;
																       lt.wMinute=(curr_rtc_clock/32/60+st.wMinute)%60;

																       if(lt.wMinute<minute_temp)
																       {st.wHour++;}
																       minute_temp=lt.wMinute;
																       lt.wHour=(curr_rtc_clock/32/3600+st.wHour)%12;

																       if(lt.wDay<day_temp)
																       {st.wDay++;}
																       lt.wDay=(curr_rtc_clock/32/3600/24+st.wDay)%30;
																       day_temp=lt.wDay;
																       lt.wMonth=st.wMonth;
																       lt.wYear=2014;

#endif
	hour = (ft_uint16_t )lt.wHour;
	min = (ft_uint16_t )lt.wMinute;
	year = (ft_uint16_t )lt.wYear;
	month = (ft_uint16_t )lt.wMonth;
	day = (ft_uint16_t )lt.wDay;
	

		if(lt.wMonth == 1)
		{
			month_of_year = " January ";
		}
		else if(lt.wMonth == 2)
		{
			month_of_year = " February ";
		}
		else if(lt.wMonth == 3)
		{
			month_of_year = " March ";
		}
		else if(lt.wMonth == 4)
		{
			month_of_year = " April ";
		}
		else if(lt.wMonth == 5)
		{
			month_of_year = " May ";
		}
		else if(lt.wMonth == 6)
		{
			month_of_year = " June ";
		}
		else if(lt.wMonth == 7)
		{
			month_of_year = " July ";
		}
		else if(lt.wMonth == 8)
		{
			month_of_year = " August ";
		}
		else if(lt.wMonth == 9)
		{
			month_of_year = " September ";
		}
		else if(lt.wMonth == 10)
		{
			month_of_year = " October ";
		}
		else if(lt.wMonth == 11)
		{
			month_of_year = " November ";
		}
		else if(lt.wMonth == 12)
		{
			month_of_year = " December ";
		}
		if(lt.wDayOfWeek == 0)
		{
			day_of_week = " Sun, ";
		}
		else if(lt.wDayOfWeek == 1)
		{
			day_of_week = "Mon, ";
		}
		else if(lt.wDayOfWeek == 2)
		{
			day_of_week = "Tue, ";
		}
		else if(lt.wDayOfWeek == 3)
		{
			day_of_week = "Wed, ";
		}
		else if(lt.wDayOfWeek == 4)
		{
			day_of_week = "Thur, ";
		}
		else if(lt.wDayOfWeek == 5)
		{
			day_of_week = "Fri, ";
		}
		else if(lt.wDayOfWeek == 6)
		{
			day_of_week = "Sat, ";
		}

		
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		BMoffsetx = 10;
		BMoffsety = 200;



		if(hour > 9)
		{
			hour_st = (hour/10)+'0';
			hour_nd = (hour%10)+'0';
			hour_st_str = &hour_st;
			hour_nd_str = &hour_nd;
			strcat(hour_st_str,hour_nd_str);
		}
		else
		{
			hour = hour + '0';
			hour_st_str =&hour;
			strcpy(hour_st_str,hour_st_str);
		}
		
		
		strcat(hour_st_str,":");

		min_st = min/10 + '0';
		min_nd = min%10 + '0';
		min_st_str = &min_st;
		min_nd_str = &min_nd;
		strcat(min_st_str,min_nd_str);
		strcat(hour_st_str,min_st_str);
		stringlen_st = strlen(hour_st_str);
	#ifdef FT900_PLATFORM
		BMoffsety = 420;
		#endif
		font_display(BMoffsetx,BMoffsety,stringlen_st,hour_st_str,1);


		
		BMoffsety = 220;

		
		strcpy(&nd_line,day_of_week);


		day_st = (day/10) + '0';
		day_nd = (day%10) + '0';
		day_st_str = &day_st;
		day_nd_str = &day_nd;

		strcat(day_st_str,day_nd_str);
		//strcat(day_st_str," ");

		strcat(&nd_line,day_st_str);
		
		strcat(&nd_line,month_of_year);
		

		year_st_nd_dig = year/100;
		year_st_dig = (year_st_nd_dig/10) + '0';
		year_nd_dig = year_st_nd_dig%10 + '0';
		year_rd_frth_dig = year%100;
		year_rd_dig = (year_rd_frth_dig/10) + '0';
		year_frth_dig = (year_rd_frth_dig%10) + '0';


		year_st_str = &year_st_dig;
		year_nd_str = &year_nd_dig;
		year_rd_str = &year_rd_dig;
		year_frth_str  = &year_frth_dig;


		strcat(year_st_str,year_nd_str);
		strcat(&nd_line,year_st_str);
		strcat(year_rd_str,year_frth_str);
		strcat(&nd_line,year_rd_str);
		stringlen_st = strlen(nd_line);
		#ifdef FT900_PLATFORM
		BMoffsety = 440;
		#endif
		font_display(BMoffsetx,BMoffsety,stringlen_st,nd_line,1);
		
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

		stringlen_text_run_disp = strlen(text_run_disp);
#ifdef FT900_PLATFORM
		BMoffsety = 455;
		if(t < 200)
		{t++;
		ttt++;}
			BMoffsetx = linear(800,0,ttt,200);
		#else
			BMoffsety = 240;
			if(t < 200)
		t++;
			BMoffsetx = linear(480,0,t,200);
		#endif
		
		//else{t=0;}
	
	//	printf("\nBMOffsetx      %d\n",BMoffsetx);
	//	printf("\nttt    %d\n",ttt);adjust the location of runing text
		font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp,1);
		//running_rate = linear((BMoffsetx + 130),(BMoffsetx +150),0,200);

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(11));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,11,0));


		Ft_App_WrCoCmd_Buffer(phost, DISPLAY() );
		Ft_Gpu_CoCmd_Swap(phost);
		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);
		t++;
		/* Wait till coprocessor completes the operation
		TBD - for maximum throughput we can check for only the leftout space in fifo and cotinue constructing the display lists */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		//rate count increment logic
		pLARate->IttrCount++;
	}

}

/* Lift demo application demonstrates background animation and foreground digits and arrow direction in portrait orientation */
ft_void_t FT_LiftApp_Portrait()
{
	/* Download the raw data of custom fonts into memory */
	ft_int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
	ft_int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16;
	ft_int32_t Baddr0,Baddr1,Baddr2,Baddr3,i,SzInc = 16,SzFlag = 0;
	ft_uint8_t fontr = 255,fontg = 255,fontb = 255,minute_temp,day_temp;
	S_LiftAppRate_t *pLARate;
	S_LiftAppTrasParams_t *pLATParams;
	S_LiftAppBallsLinear_t S_LiftBallsArray[8*10],*pLiftBalls = NULL;//as of now 80 balls are been plotted
	Ft_App_GraphicsMatrix_t S_GPUMatrix;
	Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
	float temptransx,temptransy,running_rate;
	S_LiftAppCtxt_t S_LACtxt;
	//S_LiftAppRate_t *pLARate;
	//S_LiftAppTrasParams_t *pLATParams;

#ifdef FT900_PLATFORM
	localtime st,lt;
	  static ft_uint32_t  counter,time_curr,tt,count_t;
	  st.wHour=14;
	  								st.wMinute=26;
	  								st.wDay=14;
	  								lt.wDayOfWeek=3;
	  								st.wYear=2015;
	  								st.wMilliseconds=0;
	  								st.wSecond=0;
	  								st.wMonth=07;
#else
	SYSTEMTIME st, lt;
#endif
	ft_uint16_t hour,hour_st,hour_nd,min,min_st,min_nd,sec,year,month,day,year_st_nd_dig,year_st_dig,year_nd_dig,year_rd_frth_dig,year_rd_dig,year_frth_dig;
	ft_uint16_t day_st, day_nd;
	ft_char8_t *day_of_week=" Sun, ",nd_line[100];
	ft_char8_t text_run_disp[] = "The running text is displayed here.";
	ft_char8_t *hour_st_str, *hour_nd_str, *min_st_str, *min_nd_str,*day_st_str,*day_nd_str,*year_st_str, *year_nd_str, *year_rd_str, *year_frth_str;
	ft_char8_t* month_of_year;
	ft_uint32_t stringlen,FontTableAddress,k,j,l,stringlen_week,stringlen_day,stringlen_text_run_disp,stringlen_st;
	ft_int32_t FontWid = 0;
#ifdef FT900_PLATFORM
	float tttt;
#endif
	ft_uint8_t *pbuff;
	ft_int32_t FileLen=0;
	FT_Gpu_Fonts_t Display_fontstruct;
	float t = 0;
	ft_uint16_t m;

	nd_line[0] = '0';

	

	/* Initial setup code to setup all the required bitmap handles globally */
	Ft_DlBuffer_Index = 0;
	pLARate = &S_LACtxt.SLARate;
	pLATParams = &S_LACtxt.SLATransPrms;

	//initialize all the rate parameters
	pLARate->CurrTime = 0;
	pLARate->IttrCount = 0;

	/* Initialize all the transition parameters - all the below are in terms of basic units of rate
	either they can be based on itterations or based on time giffies */
	pLATParams->FloorNumChangeRate = 64;
	pLATParams->MaxFloorNum = 90;
	pLATParams->MinFloorNum = 70;
	pLATParams->ResizeRate = 32;
	pLATParams->ResizeDimMax = 2*256;
	pLATParams->ResizeDimMin = 1*256;
	pLATParams->FloorNumStagnantRate = 128;

	/* Initialization of lift context parameters */
	S_LACtxt.ArrowDir = LIFTAPP_DIR_DOWN;//going down direction
	S_LACtxt.CurrFloorNum = 87;//current floor number to start with
	S_LACtxt.NextFloorNum = 80;//destination floor number as of now
	S_LACtxt.CurrArrowResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumStagnantRate = 0;
	S_LACtxt.CurrFloorNumChangeRate = S_LACtxt.SLATransPrms.FloorNumChangeRate;
	S_LACtxt.AudioPlaying = 0;
	S_LACtxt.Orientation = 0;

	
	/* load the bitmap raw data */
	Baddr0 = RAM_G;
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("font.raw",RAM_G);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\font.raw",RAM_G);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr0));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BHt*2, BHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width

	Baddr1 = ((Baddr0 + BSt*BHt*11 + 15)&~15);
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\arr.raw",Baddr1);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("arr.raw",Baddr1);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\arr.raw",Baddr1);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr1));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowHt*2, BArrowHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width

	Baddr2 = Baddr1 + 80*85;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs6.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs6.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs6.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 60, 55));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));
	Baddr2 += 60*55;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs5.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs5.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs5.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));
	Baddr2 += 50*46;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs4.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs4.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs4.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8,40,37));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER,40,37));
	Baddr2 += 40*37;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs3.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs3.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs3.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));
	Baddr2 += 30*27;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs2.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs2.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs2.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));
	Baddr2 += 20*18;
	//Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\bs1.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("bs1.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\bs1.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));

	Baddr2 += 10*10;
//	Ft_App_LoadRawFromFile("..\\..\\..\\..\\Test\\logo.raw",Baddr2);
#ifdef FT900_PLATFORM
	Ft_App_LoadRawFromFile("logo.raw",Baddr2);
#else
	Ft_App_LoadRawFromFile("..\\..\\..\\Test\\logo.raw",Baddr2);
#endif
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 129*2, 50));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 129, 129));//make sure whole bitmap is displayed after rotation - as height is greater than width
	Baddr2 += 129*2*50;

	// Read the font address from 0xFFFFC location 
	FontTableAddress = Ft_Gpu_Hal_Rd32(phost,0xFFFFC);
	
	Ft_CmdBuffer_Index = 0;
	
	/* Download the commands into fifo */	
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE(8));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L4,9,25));
	#ifdef FT_81X_ENABLE
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(0x2E799C));
	#else
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(950172));
	#endif
	
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 50));
	
	Baddr2 += 10*10;

	Baddr2 =  ((Baddr2 + 7)&~7);//audio engine's requirement
	
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY());
	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	Ft_App_GPU_DLSwap(DLSWAP_FRAME);
	Ft_Gpu_Hal_Sleep(30);

	/* compute the random values at the starting */
	pLiftBalls = S_LiftBallsArray;
	for(i=0;i<(NumBallsRange*NumBallsEach);i++)
	{
		//always start from the right and move towards left
		pLiftBalls->xOffset = ft_random(FT_DispWidth*16);
		pLiftBalls->yOffset = ft_random(FT_DispHeight*16);
		pLiftBalls->dx = -1*ft_random(RandomVal*8);//always -ve
		pLiftBalls->dy = ft_random(RandomVal*8) - RandomVal*4;
		pLiftBalls++;
	}


	while(1)
	{		
		/* Logic of user touch - change background or skin wrt use touch */
		/* Logic of transition - change of floor numbers and direction */
		FT_LiftApp_FTransition(&S_LACtxt,Baddr2);		
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		/* Background gradient */
		Ft_Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,FT_DispWidth,0,0x132B3B);


		/* Draw background bitmaps */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		pLiftBalls = S_LiftBallsArray;
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(64));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		for(i=0;i<(NumBallsRange*NumBallsEach);i++)
		{			
			if(0 == i%NumBallsEach)
			{				
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2 + (i/NumBallsEach)));
			}
			if( ( (pLiftBalls->xOffset > ((FT_DispWidth + 60)*16)) || (pLiftBalls->yOffset > ((FT_DispHeight + 60) *16)) ) ||
				( (pLiftBalls->xOffset < (-60*16)) || (pLiftBalls->yOffset < (-60*16)) ))
			{
				pLiftBalls->xOffset = FT_DispWidth*16 + ft_random(80*16);
				pLiftBalls->yOffset = ft_random(FT_DispHeight*16);
				pLiftBalls->dx = -1*ft_random(RandomVal*8);
				pLiftBalls->dy = ft_random(RandomVal*8) - RandomVal*4;

			}
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(pLiftBalls->xOffset, pLiftBalls->yOffset));
			pLiftBalls->xOffset += pLiftBalls->dx;
			pLiftBalls->yOffset += pLiftBalls->dy;
			pLiftBalls++;
		}
		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost,ALPHA_FUNC(ALWAYS,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));

		if(LIFTAPP_DIR_NONE != S_LACtxt.ArrowDir)//do not display the arrow in case of no direction, meaning stagnant
		{

			//calculation of size value based on the rate
			//the bitmaps are scaled from original resolution till 2 times the resolution on both x and y axis
			SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrArrowResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
			Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
			/* Draw the arrow first */
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1)); 
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
			BMoffsetx = (BArrowWd - (BArrowWd*SzInc/32));
			BMoffsety = ((FT_DispHeight/2) - (BArrowHt*SzInc/32));

			/* perform inplace flip and scale of bitmap in case of direction is up */
			Ft_App_TrsMtrxLoadIdentity(&S_GPUMatrix);
			Ft_App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
			Ft_App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
			Ft_App_TrsMtrxRotate(&S_GPUMatrix,90);
			if(LIFTAPP_DIR_UP == S_LACtxt.ArrowDir)
			{
				Ft_App_TrsMtrxFlip(&S_GPUMatrix,FT_APP_GRAPHICS_FLIP_RIGHT);		
			}

			Ft_App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
			Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
#ifdef DISPLAY_RESOLUTION_WVGA
			FT_LiftAppComputeBitmap_Single(G_LiftAppFontArrayArrow,0,SzInc*16,BArrowWd*2,(FT_DispHeight/2));

#elif defined DISPLAY_RESOLUTION_HVGA_PORTRAIT
			FT_LiftAppComputeBitmap_Single(G_LiftAppFontArrayArrow,0,SzInc*16,BArrowWd*1.8,(FT_DispHeight/4));
#else
			FT_LiftAppComputeBitmap_Single(G_LiftAppFontArrayArrow,0,SzInc*16,BArrowWd,(FT_DispHeight/2));
#endif
			Ft_Gpu_CoCmd_LoadIdentity(phost);
			Ft_Gpu_CoCmd_SetMatrix(phost);
		}

		
		/* algorithm for animation - floor numbers will change in dimensions when stagnant at a perticular floor */
		/* arrow will change in case of movement of lift */
		/* display the bitmap with increased size */
		SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrFloorNumResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
		BMoffsetx = FT_DispWidth - BWd*2 - (BWd*SzInc/32);
		BMoffsety = ((FT_DispHeight/2) - (BHt*SzInc/32));

		/* calculate the resolution change based on the number of characters used as well as */
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
		/* perform inplace flip and scale of bitmap in case of direction is up */
		Ft_App_TrsMtrxLoadIdentity(&S_GPUMatrix);
		Ft_App_TrsMtrxTranslate(&S_GPUMatrix,BWd/2.0,BHt/2.0,&temptransx,&temptransy);
		Ft_App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
		Ft_App_TrsMtrxRotate(&S_GPUMatrix,90);
		Ft_App_TrsMtrxTranslate(&S_GPUMatrix,(-BHt*SzInc)/32.0,(-BWd*SzInc)/32.0,&temptransx,&temptransy);
		Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));

		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0)); 
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps

		///////////////adjust the  position of num
#ifdef DISPLAY_RESOLUTION_WVGA
		FT_LiftAppComputeBitmapRowRotate(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BHt-100,(FT_DispHeight/2));

#elif defined DISPLAY_RESOLUTION_HVGA_PORTRAIT
		FT_LiftAppComputeBitmapRowRotate(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BHt,(FT_DispHeight*3/4-50));
#else
		FT_LiftAppComputeBitmapRowRotate(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BHt,(FT_DispHeight/2));
#endif
#ifndef FT900_PLATFORM
		GetSystemTime(&st);
		GetLocalTime(&lt);


#else

		curr_rtc_clock++;
		 time_curr=curr_rtc_clock%32;
																		  t=curr_rtc_clock/32*1000%1000;
																		  if (time_curr<tt)
																		  {
																		       count_t++;
																		  }

																		  lt.wSecond=curr_rtc_clock/32%60;
																		       lt.wMinute=(curr_rtc_clock/32/60+st.wMinute)%60;

																		       if(lt.wMinute<minute_temp)
																		       {st.wHour++;}
																		       minute_temp=lt.wMinute;
																		       lt.wHour=(curr_rtc_clock/32/3600+st.wHour)%12;

																		       if(lt.wDay<day_temp)
																		       {st.wDay++;}
																		       lt.wDay=(curr_rtc_clock/32/3600/24+st.wDay)%30;
																		       day_temp=lt.wDay;
																		       lt.wMonth=st.wMonth;
																		       lt.wYear=2015;

#endif
	hour = (ft_uint16_t )lt.wHour;
	min = (ft_uint16_t )lt.wMinute;
	year = (ft_uint16_t )lt.wYear;
	day = (ft_uint16_t )lt.wDay;
#ifdef FT900_PLATFORM
	lt.wSecond++;
	#endif
		if(lt.wMonth == 1)
		{
			month_of_year = " January";
		}
		else if(lt.wMonth == 2)
		{
			month_of_year = " February";
		}
		else if(lt.wMonth == 3)
		{
			month_of_year = " March ";
		}
		else if(lt.wMonth == 4)
		{
			month_of_year = " April ";
		}
		else if(lt.wMonth == 5)
		{
			month_of_year = " May ";
		}
		else if(lt.wMonth == 6)
		{
			month_of_year = " June ";
		}
		else if(lt.wMonth == 7)
		{
			month_of_year = " July ";
		}
		else if(lt.wMonth == 8)
		{
			month_of_year = " August ";
		}
		else if(lt.wMonth == 9)
		{
			month_of_year = " September";
		}
		else if(lt.wMonth == 10)
		{
			month_of_year = " October ";
		}
		else if(lt.wMonth == 11)
		{
			month_of_year = " November";
		}
		else if(lt.wMonth == 12)
		{
			month_of_year = " December";
		}
		if(lt.wDayOfWeek == 0)
		{
			day_of_week = "Sun,";
		}
		else if(lt.wDayOfWeek == 1)
		{
			day_of_week = "Mon,";
		}
		else if(lt.wDayOfWeek == 2)
		{
			day_of_week = "Tue,";
		}
		else if(lt.wDayOfWeek == 3)
		{
			day_of_week = "Wed,";
		}
		else if(lt.wDayOfWeek == 4)
		{
			day_of_week = "Thur,";
		}
		else if(lt.wDayOfWeek == 5)
		{
			day_of_week = "Fri,";
		}
		else if(lt.wDayOfWeek == 6)
		{
			day_of_week = "Sat,";
		}

		
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		BMoffsetx = ((FT_DispWidth /2) + 170);
		BMoffsety = ((FT_DispHeight /2) + 105);

#ifdef DISPLAY_RESOLUTION_WVGA//adjust for time display
		BMoffsetx = (700);
		BMoffsety = (430);

#endif
#ifdef DISPLAY_RESOLUTION_HVGA_PORTRAIT
		BMoffsetx = ((FT_DispWidth /2) + 100);
		BMoffsety = ((FT_DispHeight /2) + 180);
#endif
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(28));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST,BORDER,BORDER,25,25));

		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,18*65536/2,25*65536/2);
		Ft_Gpu_CoCmd_Rotate(phost,-90*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,-25*65536/2,-18*65536/2);
		Ft_Gpu_CoCmd_SetMatrix(phost);


		if(hour > 9)
		{
			hour_st = (hour/10)+'0';
			hour_nd = (hour%10)+'0';
			hour_st_str = &hour_st;
			hour_nd_str = &hour_nd;
			strcat(hour_st_str,hour_nd_str);
		}
		else
		{
			hour = hour + '0';
			hour_st_str =&hour;
			strcpy(hour_st_str,hour_st_str);
		}
		
		
		strcat(hour_st_str,":");

		min_st = min/10 + '0';
		min_nd = min%10 + '0';
		min_st_str = &min_st;
		min_nd_str = &min_nd;
		strcat(min_st_str,min_nd_str);
		strcat(hour_st_str,min_st_str);
		stringlen_st = strlen(hour_st_str);

#ifdef DISPLAY_RESOLUTION_WVGA
		font_display(BMoffsetx,BMoffsety,stringlen_st,hour_st_str,0);
#else
		font_display(BMoffsetx,BMoffsety,stringlen_st,hour_st_str,0);
#endif

		BMoffsetx = ((FT_DispWidth /2) + 190);
		BMoffsety = ((FT_DispHeight /2) + 105);


#ifdef DISPLAY_RESOLUTION_WVGA
		BMoffsety = (370);
		BMoffsetx = 700;
#endif
#ifdef DISPLAY_RESOLUTION_HVGA_PORTRAIT
		BMoffsetx = ((FT_DispWidth /2) + 100);
		BMoffsety = ((FT_DispHeight /2) + 80);
#endif
		strcpy(&nd_line,day_of_week);


		day_st = (day/10) + '0';
		day_nd = (day%10) + '0';
		day_st_str = &day_st;
		day_nd_str = &day_nd;

		strcat(day_st_str,day_nd_str);
		//strcat(day_st_str," ");

		strcat(&nd_line,day_st_str);
		
		strcat(&nd_line,month_of_year);
		

		year_st_nd_dig = year/100;
		year_st_dig = (year_st_nd_dig/10) + '0';
		year_nd_dig = year_st_nd_dig%10 + '0';
		year_rd_frth_dig = year%100;
		year_rd_dig = (year_rd_frth_dig/10) + '0';
		year_frth_dig = (year_rd_frth_dig%10) + '0';


		year_st_str = &year_st_dig;
		year_nd_str = &year_nd_dig;
		year_rd_str = &year_rd_dig;
		year_frth_str  = &year_frth_dig;


		strcat(year_st_str,year_nd_str);
		strcat(&nd_line,year_st_str);
		strcat(year_rd_str,year_frth_str);
		strcat(&nd_line,year_rd_str);
		stringlen_st = strlen(nd_line);

		font_display(BMoffsetx,BMoffsety,stringlen_st,nd_line,0);

		BMoffsetx = ((FT_DispWidth /2) + 210);
		BMoffsety = ((FT_DispHeight /2) + 130);
#ifdef DISPLAY_RESOLUTION_WVGA
		BMoffsetx = (750);
#endif
#ifdef DISPLAY_RESOLUTION_HVGA_PORTRAIT
		BMoffsetx = ((FT_DispWidth /2) + 115);
#endif
		stringlen_text_run_disp = strlen(text_run_disp);
	//	if(t < 200)
	//		t++;
	//	BMoffsety = linear(0,480,t,200);
#ifdef FT900_PLATFORM
		if(t < 200)
			{t++;
			tttt++;}
			else{t=0;}
			BMoffsety = linear(0,480,tttt,200);
#else
	if(t < 200)
			t++;
		BMoffsety = linear(0,272,t,200);

#endif
		font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp,0);

		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,129*65536,50*65536);
		Ft_Gpu_CoCmd_Rotate(phost,270*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,-80*65536,-50*65536);
		Ft_Gpu_CoCmd_SetMatrix(phost);

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(11));
		//Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-100*16,120*16));
#ifdef DISPLAY_RESOLUTION_WVGA
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-80*16,20*16));

#elif defined DISPLAY_RESOLUTION_HVGA_PORTRAIT
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-80*16,10*16));
#else
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-80*16,150*16));
#endif
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY() );
		Ft_Gpu_CoCmd_Swap(phost);
		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);
		t++;
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		//rate count increment - 
		pLARate->IttrCount++;
	}

}

/* Startup Screen Information*/ 

char *info[] = { "FT800 Lift Application",
                          "API to demonstrate lift application,",
                          "with FTDI logo",
                          "& date and time display"
                       }; 

/********API to return the assigned TAG value when penup,for the primitives/widgets******/

static ft_uint8_t sk=0;
ft_uint8_t Read_Keys()
{
  static ft_uint8_t Read_tag=0,temp_tag=0,ret_tag=0;	
  Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(Read_tag!=NULL)		
  {
    if(temp_tag!=Read_tag)
    {
      temp_tag = Read_tag;	
      sk = Read_tag;										
    }  
  }
  else
  {
    if(temp_tag!=0)
    {
      ret_tag = temp_tag;
    }  
    sk = 0;
  }
  return ret_tag;
}
/***********************API used to SET the ICON******************************************/
/*Refer the code flow in the flowchart availble in the Application Note */



ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
// Touch Screen Calibration
  Ft_CmdBuffer_Index = 0;  
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,26,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
// Ftdi Logo animation  
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ));
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -=4;
#ifdef FT_81X_ENABLE
  dloffset -= 2*4;//remove two more instructions in case of 81x
#endif
  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,100000L);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
//Enter into Info Screen
  do
  {	Ft_CmdBuffer_Index = 0;
    Ft_Gpu_CoCmd_Dlstart(phost);
    Ft_Gpu_CoCmd_Append(phost,100000L,dloffset);
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(0));
    Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(219,180,150));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(220));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(EDGE_STRIP_A));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));

    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,20,28,OPT_CENTERX|OPT_CENTERY,info[0]);
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,60,26,OPT_CENTERX|OPT_CENTERY,info[1]);
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,90,26,OPT_CENTERX|OPT_CENTERY,info[2]);  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,120,26,OPT_CENTERX|OPT_CENTERY,info[3]);  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight-30,26,OPT_CENTERX|OPT_CENTERY,"Click to play");

	
//	Check if the Play key and change the color
    if(sk!='P')
    {
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));}
    else
   // {Ft_Gpu_CoCmd_Dlstart(phost);}
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(20*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG('P'));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth/2)*16,(FT_DispHeight-60)*16));
	 Ft_App_WrCoCmd_Buffer(phost,CLEAR(0,1,0));

	  Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 0, 0) );
	Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(INCR,INCR) );
	Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(0,0,0,0) );//mask all the colors
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));

	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-2+15),(FT_DispHeight-67+8),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-57+11),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
	Ft_App_WrCoCmd_Buffer(phost, END());
	Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(1,1,1,1) );//enable all the colors
	Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(EQUAL,1,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth,0,0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth,FT_DispHeight,0,0));
	//Ft_App_WrCoCmd_Buffer(phost, END());
	
	Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
	Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(ALWAYS,0,255));
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
	//Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0) );
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-2+15),(FT_DispHeight-67+8),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-57+11),0,0));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
	//Ft_App_WrCoCmd_Buffer(phost, END());



    //Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-14,(FT_DispHeight-75),14,4));
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  }while(Read_Keys()!='P');
/* wait until Play key is not pressed*/ 
  /* To cover up some distortion while demo is being loaded */
    Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
  Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}


#ifdef FT900_PLATFORM
ft_void_t FT900_Config()
{
	sys_enable(sys_device_uart0);
    gpio_function(48, pad_uart0_txd); /* UART0 TXD */
    gpio_function(49, pad_uart0_rxd); /* UART0 RXD */
    uart_open(UART0,                    /* Device */
              1,                        /* Prescaler = 1 */
              UART_DIVIDER_115200_BAUD,  /* Divider = 1302 */
              uart_data_bits_8,         /* No. Data Bits */
              uart_parity_none,         /* Parity */
              uart_stop_bits_1);        /* No. Stop Bits */

    /* Print out a welcome message... */
    uart_puts(UART0,

        "(C) Copyright 2014-2015, Future Technology Devices International Ltd. \r\n"
        "--------------------------------------------------------------------- \r\n"
        "Welcome to Lift Example ... \r\n"
        "\r\n"
        "--------------------------------------------------------------------- \r\n"
        );

#ifdef ENABLE_ILI9488_HVGA_PORTRAIT
/* asign all the respective pins to gpio and set them to default values */
gpio_function(34, pad_gpio34);
gpio_dir(34, pad_dir_output);
gpio_write(34,1);

gpio_function(27, pad_gpio27);
gpio_dir(27, pad_dir_output);
gpio_write(27,1);

gpio_function(29, pad_gpio29);
gpio_dir(29, pad_dir_output);
gpio_write(29,1);

gpio_function(33, pad_gpio33);
gpio_dir(33, pad_dir_output);
gpio_write(33,1);


gpio_function(30, pad_gpio30);
gpio_dir(30, pad_dir_output);
gpio_write(30,1);

gpio_function(28, pad_gpio28);
gpio_dir(28, pad_dir_output);
gpio_write(28,1);


	gpio_function(43, pad_gpio43);
	gpio_dir(43, pad_dir_output);
gpio_write(43,1);
gpio_write(34,1);
gpio_write(28,1);
gpio_write(43,1);
gpio_write(33,1);
gpio_write(33,1);

#endif


/* useful for timer */
ft_millis_init();
interrupt_enable_globally();
//printf("ft900 config done \n");
   SDHOST_STATUS sd_status;
	  //

		if (sdhost_card_detect() != SDHOST_CARD_INSERTED)
		{
		printf("Please Insert SD Card\r\n");
		}
		else
		{ printf("\n SD Card inserted!");}


		if (f_mount(&FatFs, "", 0) != FR_OK)
		{
			printf("\n Mounted failed.");;
		}

			printf("\n Mounted succesfully.");
}
#endif






#if defined MSVC_PLATFORM | defined FT900_PLATFORM
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{

	ft_uint8_t chipid;
	#ifdef FT900_PLATFORM
		FT900_Config();

		sys_enable(sys_device_sd_card);
		sdhost_init();

			#define GPIO_SD_CLK  (19)
			#define GPIO_SD_CMD  (20)
			#define GPIO_SD_DAT3 (21)
			#define GPIO_SD_DAT2 (22)
			#define GPIO_SD_DAT1 (23)
			#define GPIO_SD_DAT0 (24)
			#define GPIO_SD_CD   (25)
			#define GPIO_SD_WP   (26)
		    gpio_function(GPIO_SD_CLK, pad_sd_clk); gpio_pull(GPIO_SD_CLK, pad_pull_none);//pad_pull_none
		    gpio_function(GPIO_SD_CMD, pad_sd_cmd); gpio_pull(GPIO_SD_CMD, pad_pull_pullup);
		    gpio_function(GPIO_SD_DAT3, pad_sd_data3); gpio_pull(GPIO_SD_DAT3, pad_pull_pullup);
		    gpio_function(GPIO_SD_DAT2, pad_sd_data2); gpio_pull(GPIO_SD_DAT2, pad_pull_pullup);
		    gpio_function(GPIO_SD_DAT1, pad_sd_data1); gpio_pull(GPIO_SD_DAT1, pad_pull_pullup);
		    gpio_function(GPIO_SD_DAT0, pad_sd_data0); gpio_pull(GPIO_SD_DAT0, pad_pull_pullup);
		    gpio_function(GPIO_SD_CD, pad_sd_cd); gpio_pull(GPIO_SD_CD, pad_pull_pullup);
		    gpio_function(GPIO_SD_WP, pad_sd_wp); gpio_pull(GPIO_SD_WP, pad_pull_pullup);

		    /* Check to see if a card is inserted */
	#endif
		    Ft_Gpu_HalInit_t halinit;

		    	halinit.TotalChannelNum = 1;


		    	Ft_Gpu_Hal_Init(&halinit);
		    	host.hal_config.channel_no = 0;
		    	host.hal_config.pdn_pin_no = FT800_PD_N;
		    	host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;
                #if defined(MSVC_PLATFORM) && defined(MSVC_PLATFORM_SPI_LIBMPSSE)
		    	    host.hal_config.spi_clockrate_khz = 12000; //in KHz
                #elif defined(MSVC_PLATFORM) && defined(MSVC_PLATFORM_SPI_LIBFT4222)
                    host.hal_config.spi_clockrate_khz = 20000; //in KHz
		        #endif
		    #ifdef ARDUINO_PLATFORM_SPI
		    	host.hal_config.spi_clockrate_khz = 4000; //in KHz
		    #endif
		        Ft_Gpu_Hal_Open(&host);


		    	phost = &host;

		        Ft_BootupConfig();

	
	#if ((defined FT900_PLATFORM) || defined(MSVC_PLATFORM))
		printf("\n reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
		printf("\n reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
	    printf("\n reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
		printf("\n reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));
	#endif


    /*It is optional to clear the screen here*/	
    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
    
    Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen. 



	Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);		
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,0xFF);
	

	Info();
	/* APP to demonstrate lift functionality */
#ifdef ORIENTATION_PORTRAIT

	FT_LiftApp_Portrait();

#endif

#ifdef ORIENTATION_LANDSCAPE

	FT_LiftApp_Landscape();

#endif	
	//
	/* Close all the opened handles */
    Ft_Gpu_Hal_Close(phost);
    Ft_Gpu_Hal_DeInit();
#ifdef MSVC_PLATFORM
	return 0;
#endif
}

void loop()
{
}



/* Nothing beyond this */













