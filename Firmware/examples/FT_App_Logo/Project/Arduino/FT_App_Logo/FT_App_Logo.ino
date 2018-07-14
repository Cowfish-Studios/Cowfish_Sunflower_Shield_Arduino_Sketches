/*+++

* Copyright (c) Future Technology Devices International 2014

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
 */


#include "FT_Platform.h"

#if defined(ARDUINO_PLATFORM)
#include <SPI.h>
#include "sdcard.h"
#endif
#ifdef FT900_PLATFORM
#include "ff.h"
#include <math.h>
#endif
#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)

#if defined(ARDUINO_PLATFORM)
#define MEMCMD(a) Ft_Gpu_Hal_WrCmdBufFromFlash(phost,(ft_prog_uchar8_t*)(a), sizeof(a))
#else
#define MEMCMD(a) Ft_Gpu_Hal_WrCmdBuf(phost,a,sizeof(a))
#endif
#define F16(s)        ((ft_int32_t)((s) * 65536))
#define SQ(v) (v*v)
#define AUDIO_ADDRESS 102400L
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
    255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};

#ifdef FT900_PLATFORM
	FATFS FatFs;

FATFS            FatFs;				// FatFs work area needed for each volume
FIL 			 CurFile;
FRESULT          fResult;
SDHOST_STATUS    SDHostStatus;
#endif

/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;



ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;

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
   if (Ft_DlBuffer_Index> 0)
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
ft_void_t SAMAPP_fadeout()
{
   ft_int32_t i;
	
	for (i = 100; i >= 0; i -= 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);

		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
}

/* API to perform display fadein effect by changing the display PWM from 0 till 100 and finally 128 */
ft_void_t SAMAPP_fadein()
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
#define SAMAPP_ENABLE_APIS_SET0
#ifdef SAMAPP_ENABLE_APIS_SET0

/* Optimized implementation of sin and cos table - precision is 16 bit */
FT_PROGMEM ft_prog_uint16_t sintab[] = {  0, 402, 804, 1206, 1607, 2009, 2410, 2811, 3211, 3611, 4011, 4409, 4807, 5205, 5601, 5997, 6392, 6786, 7179, 7571, 7961, 8351, 8739, 9126, 9511, 9895, 10278, 10659, 11038, 11416, 11792, 12166, 12539,
                                          12909, 13278, 13645, 14009, 14372, 14732, 15090, 15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868,  18204, 18537, 18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705, 22004,
                                          22301, 22594, 22883, 23169, 23452, 23731, 24006, 24278, 24546, 24811, 25072, 25329, 25582, 25831, 26077, 26318, 26556, 26789,27019, 27244, 27466, 27683, 27896, 28105, 28309, 28510, 28706, 28897, 29085, 
                                          29268, 29446, 29621, 29790, 29955, 30116, 30272, 30424, 30571, 30713, 30851, 30984, 31113, 31236, 31356, 31470, 31580, 31684, 31785, 31880, 31970, 32056, 32137, 32213, 32284, 32350, 32412, 32468, 32520, 
                                          32567, 32609, 32646, 32678, 32705, 32727, 32744, 32757,  32764, 32767, 32764};


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
#endif

/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms 

and check again */
ft_void_t Ft_Gpu_DLSwap(ft_uint8_t DL_Swap_Type)
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


/*****************************************************************************/


static ft_uint8_t home_star_icon[] = {0x78,0x9C,0xE5,0x94,0xBF,0x4E,0xC2,0x40,0x1C,0xC7,0x7F,0x2D,0x04,0x8B,0x20,0x45,0x76,0x14,0x67,0xA3,0xF1,0x0D,0x64,0x75,0xD2,0xD5,0x09,0x27,0x17,0x13,0xE1,0x0D,0xE4,0x0D,0x78,0x04,0x98,0x5D,0x30,0x26,0x0E,0x4A,0xA2,0x3E,0x82,0x0E,0x8E,0x82,0xC1,0x38,0x62,0x51,0x0C,0x0A,0x42,0x7F,0xDE,0xB5,0x77,0xB4,0x77,0x17,0x28,0x21,0x26,0x46,0xFD,0x26,0xCD,0xE5,0xD3,0x7C,0xFB,0xBB,0xFB,0xFD,0xB9,0x02,0xCC,0xA4,0xE8,0x99,0x80,0x61,0xC4,0x8A,0x9F,0xCB,0x6F,0x31,0x3B,0xE3,0x61,0x7A,0x98,0x84,0x7C,0x37,0xF6,0xFC,0xC8,0xDD,0x45,0x00,0xDD,0xBA,0xC4,0x77,0xE6,0xEE,0x40,0xEC,0x0E,0xE6,0x91,0xF1,0xD2,0x00,0x42,0x34,0x5E,0xCE,0xE5,0x08,0x16,0xA0,0x84,0x68,0x67,0xB4,0x86,0xC3,0xD5,0x26,0x2C,0x20,0x51,0x17,0xA2,0xB8,0x03,0xB0,0xFE,0x49,0xDD,0x54,0x15,0xD8,0xEE,0x73,0x37,0x95,0x9D,0xD4,0x1A,0xB7,0xA5,0x26,0xC4,0x91,0xA9,0x0B,0x06,0xEE,0x72,0xB7,0xFB,0xC5,0x16,0x80,0xE9,0xF1,0x07,0x8D,0x3F,0x15,0x5F,0x1C,0x0B,0xFC,0x0A,0x90,0xF0,0xF3,0x09,0xA9,0x90,0xC4,0xC6,0x37,0xB0,0x93,0xBF,0xE1,0x71,0xDB,0xA9,0xD7,0x41,0xAD,0x46,0xEA,0x19,0xA9,0xD5,0xCE,0x93,0xB3,0x35,0x73,0x0A,0x69,0x59,0x91,0xC3,0x0F,0x22,0x1B,0x1D,0x91,0x13,0x3D,0x91,0x73,0x43,0xF1,0x6C,0x55,0xDA,0x3A,0x4F,0xBA,0x25,0xCE,0x4F,0x04,0xF1,0xC5,0xCF,0x71,0xDA,0x3C,0xD7,0xB9,0xB2,0x48,0xB4,0x89,0x38,0x20,0x4B,0x2A,0x95,0x0C,0xD5,0xEF,0x5B,0xAD,0x96,0x45,0x8A,0x41,0x96,0x7A,0x1F,0x60,0x0D,0x7D,0x22,0x75,0x82,0x2B,0x0F,0xFB,0xCE,0x51,0x3D,0x2E,0x3A,0x21,0xF3,0x1C,0xD9,0x38,0x86,0x2C,0xC6,0x05,0xB6,0x7B,0x9A,0x8F,0x0F,0x97,0x1B,0x72,0x6F,0x1C,0xEB,0xAE,0xFF,0xDA,0x97,0x0D,0xBA,0x43,0x32,0xCA,0x66,0x34,0x3D,0x54,0xCB,0x24,0x9B,0x43,0xF2,0x70,0x3E,0x42,0xBB,0xA0,0x95,0x11,0x37,0x46,0xE1,0x4F,0x49,0xC5,0x1B,0xFC,0x3C,0x3A,0x3E,0xD1,0x65,0x0E,0x6F,0x58,0xF8,0x9E,0x5B,0xDB,0x55,0xB6,0x41,0x34,0xCB,0xBE,0xDB,0x87,0x5F,0xA9,0xD1,0x85,0x6B,0xB3,0x17,0x9C,0x61,0x0C,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xED,0xC9,0xFC,0xDF,0x14,0x54,0x8F,0x80,0x7A,0x06,0xF5,0x23,0xA0,0x9F,0x41,0xF3,0x10,0x30,0x4F,0x41,0xF3,0x18,0x30,0xCF,0xCA,0xFC,0xFF,0x35,0xC9,0x79,0xC9,0x89,0xFA,0x33,0xD7,0x1D,0xF6,0x5E,0x84,0x5C,0x56,0x6E,0xA7,0xDA,0x1E,0xF9,0xFA,0xAB,0xF5,0x97,0xFF,0x2F,0xED,0x89,0x7E,0x29,0x9E,0xB4,0x9F,0x74,0x1E,0x69,0xDA,0xA4,0x9F,0x81,0x94,0xEF,0x4F,0xF6,0xF9,0x0B,0xF4,0x65,0x51,0x08};


typedef struct
{
  ft_uint8_t sd_present:1;
}ft_bitfield;

ft_bitfield ft_flags;

/* Logo Setups*/

typedef struct logoim
{
  char name[7];
  ft_uint16_t image_height;
  ft_uint8_t image_format:5; 
  ft_uint8_t filter:1;
  ft_uint16_t sizex;
  ft_uint16_t sizey;
  ft_uint16_t linestride:10;
  ft_uint8_t gram_address;
}t_imageprp;
t_imageprp sym_prp[6] = {
                          {"S1.BIN",136,RGB565,BILINEAR, 512, 512,480,0},      // handle 0
              	          {"S2.BIN", 77,    L8,BILINEAR,2*77,2*77, 77,65},      // handle 1
                          {"S3.BIN", 28,    L8, NEAREST,3*50,3*28, 50,77},      // handle 2
                          {"S4.BIN", 50,    L8, NEAREST,  50,  50, 50,80},
                          {"S5.BIN", 77,    L8,BILINEAR,2*77,2*77, 77,83},
                          {"S6.BIN", 77,    L8, NEAREST,  84,  77, 84,95}
			},
          target_prp[4] = {  
                            {"T1.BIN", 68,    L8,BILINEAR, 136,136,68,64},      // handle 0
                  	    {"T2.BIN", 55,    L4,BILINEAR, 120, 55,60,70},      // handle 1
                            {"T3.BIN", 60,    L4,BILINEAR, 240,180,60,78},      // handle 2
                            {"T4.BIN", 150,   L8,BILINEAR, 480,300,240,82},
                          },
          hbo_prp[7]= {
                          {"H1.BIN",136,RGB565,BILINEAR,512,512,480,95},
                          {"H2.BIN",120,    L8,BILINEAR,512,512,120,32},
                          {"H3.BIN",3,    L8, NEAREST,360,3,360,55},
                          {"H4.BIN",136,   L8, NEAREST,512,512,30,57},
                          {"H5.BIN",72,   L8, NEAREST,512,512,72,65},
                          {"H6.BIN",64,   L8, NEAREST,512,512,60,71},
                          {"H7.BIN",63,   L8, NEAREST,512,512,258,79}, 
                        },
          fuse_prp[2] = { {"F1.BIN",128,    L8, NEAREST,512,512, 128,1},
                          {"F2.BIN", 94,    L4, NEAREST, 88, 94, 44,20}
                         },
          music_prp[3]= {
                          {"M1.BIN", 57,RGB565,BILINEAR,110,114,110,0},
                          {"M2.BIN",127,    L8, NEAREST,180,127,180,30},
                          {"M3.BIN",66,    L8, NEAREST,149,66,149,100},
                        },
         menu_prp[3]=   {
                          {"ML.BIN", 120,L8,BILINEAR,120,240,60,180},
                          {"MG.BIN", 272,L8,NEAREST,480,272,1,177},
                          {"ME.BIN", 136,L8,BILINEAR,480,272,240,217},
                        }; 
                        
FT_PROGMEM ft_prog_uint16_t  sx[20] = {0x017d,0x0011,0x00f8,0x001b,231,0x01a1,0x01ad,0x00ee,0x0004,0x011f,0x003b,0x000e,0x00dc,0x016d,0x0152,0x01da,0x00f7,0x0091,0x0045,0x0162};
FT_PROGMEM ft_prog_uchar8_t  sy[20] = {0x3d,0x1d,0x18,0x7f,101,0x60,0x18,0x1e,0x5f,0x9f,0x43,0x77,0x8a,0x6d,0x60,0x25,0x38,0x17,0x5b,0x94};
FT_PROGMEM ft_prog_uchar8_t  ss[20] = {0x04,0x05,0x06,0x07,0x05,0x05,0x08,0x08,0x0A,0x05,0x07,0x05,0x06,0x05,0x04,0x05,0x07,0x07,0x05,0x04};

FT_PROGMEM ft_prog_uint16_t hyp[48] = {72,144,216,102,161,228,161,204,260,228,260,305,72,144,216,72,144,216,102,161,228,161,204,260,228,260,305,102,161,228,161,204,260,228,260,305,72,144,216,102,161,228,161,204,260,228,260,305};
FT_PROGMEM float const t_can[48] = {1.0,1.0,1.0,0.707388,0.891115,0.951106,0.454487,0.707388,0.829206,0.309623,0.559604,0.707388,0.000796,0.000796,0.000796,-0.999999,-0.999999,-0.999999,-0.706262,-0.453068,-0.308108,-0.890391,-0.706262,-0.558283,-0.950613,-0.828314,-0.706262,-0.708513,-0.891837,-0.951596,-0.455905,-0.708513,-0.830095,-0.311137,-0.560923,-0.575278,-0.002389,-0.002389,-0.002389,0.705133,0.451647,0.306592,0.889665,0.705133,0.556961,0.950117,0.827421,-0.708513};
FT_PROGMEM float const t_san[48] = {0.0,0.0,0.0,0.706825,0.453778,0.308866,0.890753,0.706825,0.558943,0.950859,0.828760,0.706825,1.000000,1.000000,1.000000,0.001593,0.001593,0.001593,0.707951,0.891476,0.951351,0.455196,0.707951,0.829651,0.310380,0.560263,0.707951,-0.705698,-0.452358,-0.307350,-0.890028,-0.705698,-0.557622,-0.950365,-0.827868,-0.817958,-0.999997,-0.999997,-0.999997,-0.709075,-0.892196,-0.951841,-0.456614,-0.709075,-0.830539,-0.311894,-0.561582,-0.705698};


/*************************************************************************/
float linear(float p1,float p2,ft_uint16_t t,ft_uint16_t rate)
{
	float st  = (float)t/rate;
	return p1+(st*(p2-p1));
}

ft_int16_t smoothstep(ft_int16_t p1,ft_int16_t p2,ft_uint16_t t,ft_uint16_t rate)
{
	float dst  = (float)t/rate;
	float st = SQ(dst)*(3-2*dst);
	return p1+(st*(p2-p1));
}

ft_int16_t acceleration(ft_int16_t p1,ft_int16_t p2,ft_uint16_t t,ft_uint16_t rate)
{
	float dst  = (float)t/rate;
	float st = SQ(dst);
	return p1+(st*(p2-p1));
}

float deceleration(ft_int16_t p1,ft_int16_t p2,ft_uint16_t t,ft_uint16_t rate)
{
	float st,dst  = (float)t/rate;
	dst = 1-dst;
	st = 1-SQ(dst);
	return p1+(st*(p2-p1));
}

    

void Use_Rgb()
{
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));	
}
void Use_Alpha()
{
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
}

void Clear_Alpha()
{
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
}

    
void load_inflate_image(ft_uint8_t address,char* name)
{
    ft_uint8_t ibuff[512];
    Reader imfile;
    byte file = imfile.openfile(name);
    ft_int32_t fsize = imfile.size; 
    Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
    Ft_Gpu_Hal_WrCmd32(phost,address*1024L);
    while(fsize > 0)
    {
      uint16_t n = fsize > 512L ? 512L :fsize;
      imfile.readsector(ibuff);     
      Ft_Gpu_Hal_WrCmdBuf(phost,ibuff,n);				/* copy data continuously into command memory */
      fsize -= n;
    }
}

void Load_file2gram(ft_uint32_t add,ft_uint8_t sectors,Reader &r)
{
  ft_uint8_t pbuff[512];
  for(ft_uint8_t z=0;z<sectors;z++)
  {
    r.readsector(pbuff);
    Ft_Gpu_Hal_WrMem(phost,add,pbuff,512L);
    add+=512;
  }  
}

static ft_uint8_t istouch()
{
  return !(Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}


ft_void_t Play_Sound(ft_uint8_t sound,ft_uint8_t volume)
{
  Ft_Gpu_Hal_Wr8(phost,REG_VOL_SOUND,volume);
  Ft_Gpu_Hal_Wr8(phost,REG_SOUND,sound);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAY,1);

}	

ft_uint8_t Menu_prp = 0;
void Logo_Intial_setup(struct logoim sptr[],ft_uint8_t num)
{
   for(ft_uint8_t z=0;z<num;z++)
   {
     load_inflate_image(sptr[z].gram_address,sptr[z].name);
   }     
   
   Ft_Gpu_CoCmd_Dlstart(phost);        // start
   Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
   for(ft_uint8_t z=0;z<num;z++)
   {
     Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(z));					// bg             
     Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(sptr[z].gram_address*1024L));      
     Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(sptr[z].image_format,sptr[z].linestride,sptr[z].image_height));      
     if(Menu_prp==1 && z==1)          
     Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(sptr[z].filter,REPEAT,BORDER,sptr[z].sizex,sptr[z].sizey)); 
     else
     Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(sptr[z].filter,BORDER, BORDER, sptr[z].sizex,sptr[z].sizey)); 
   }
   Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
   Ft_Gpu_CoCmd_Swap(phost);
   Ft_App_Flush_Co_Buffer(phost);
   Ft_Gpu_Hal_WaitCmdfifo_empty(phost);    
}




void end_frame()
{
  PROGMEM prog_uint32_t std2[] = {
    DISPLAY(),
    CMD_SWAP
  };  MEMCMD(std2); 
}

ft_uint8_t Logo_Example5()
{
    ft_uint16_t zt=0,zt_s=0,zt1=0,zt_s1=0;
    ft_uint16_t ft_iteration_cts = 0;
    ft_int16_t ft_xoffset=0,ft_yoffset=0; 
    ft_int16_t x,y,r;
    ft_uint16_t zin;
    float inout = 0.01;
    ft_uint8_t hcell=2,f=25;
    ft_xoffset = 0; 
    ft_yoffset = 0; 
    ft_iteration_cts = 0;
    zt = 0; zt1= 0;
    Logo_Intial_setup(music_prp,3);
    do
    {
      if(istouch())
      return 1;
     
      
      x = smoothstep(480,90,zt,200);
      y = smoothstep(0,124,zt,200);
		
      if(inout<1.2)
      inout = linear(0.1,1.2,zt,200);		
      zin = (ft_int16_t)(256/inout);	
      r = smoothstep(50,8,zt,200);
      ft_yoffset = y;
      ft_xoffset = x-(r*sin(2*3.14*f*y));
      
      PROGMEM prog_uint32_t std[] = {
      CMD_DLSTART,
      CLEAR_COLOR_RGB(255,255,255),
      CLEAR(1,1,1),
      COLOR_RGB(255,255,255),
 //----------------------------------use alpha-----------------------------------------        
      COLOR_MASK(0,0,0,1),      
      COLOR_A(255),
 //------------------------------------------------------------------------------------        
      BEGIN(FTPOINTS),
      SAVE_CONTEXT(),
      CMD_LOADIDENTITY,
    };
    MEMCMD(std);
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((ft_uint16_t)(25*inout)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    Use_Rgb();
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(zin));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(zin));
    ft_yoffset -=(25*inout);
    ft_xoffset -=(27*inout); 
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,0,1));
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    Clear_Alpha();
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    ft_yoffset +=(26*inout);
    ft_xoffset +=(27*inout);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    Use_Alpha();
    x = smoothstep(480,100,zt,200);
    y = smoothstep(0,130,zt,200);
    r = smoothstep(150,75,zt,200);
    ft_xoffset = x+(r*sin(2*3.14*f*y));   
    if(r<=75)
    {
      ft_xoffset = smoothstep(ft_xoffset,120,zt1,100);
    }
    ft_yoffset = y;	
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    Ft_App_WrCoCmd_Buffer(phost,END());
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    Use_Rgb();
    ft_yoffset -=(25*inout);
    ft_xoffset -=(27*inout);
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,0,2));
    	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    Clear_Alpha();
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    ft_yoffset +=(26*inout);
    ft_xoffset +=(27*inout);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    Use_Alpha();
    r = smoothstep(250,75,zt,200);
    ft_yoffset = y+(r*sin(2*3.14*f*x));	
    if(r<=75)
    {
      ft_yoffset = smoothstep(ft_yoffset,90,zt1,100);
      if(zt1<100)zt1+=5;
    }
    ft_xoffset = x;			
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    Ft_App_WrCoCmd_Buffer(phost,END());
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    Use_Rgb();
    ft_yoffset -=(26*inout);
    ft_xoffset -=(27*inout);
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,0,0));
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    Clear_Alpha();
    Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
    ft_yoffset +=(26*inout);
    ft_xoffset +=(27*inout);
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_xoffset)*16,(ft_yoffset)*16));
    Ft_App_WrCoCmd_Buffer(phost,END());
    	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
    if(r<=75)	
    {
      Use_Rgb();
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));
      ft_yoffset = smoothstep(-127,43,zt1,100);
      if(zt1<=100)
      {
	Ft_App_WrCoCmd_Buffer(phost,CELL(hcell));
	ft_xoffset = 15;
      }
      else
      {	
	ft_xoffset = 11;
	if(hcell>0)hcell--;
	Ft_App_WrCoCmd_Buffer(phost,CELL(hcell));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));			
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ft_xoffset*16,ft_yoffset*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ft_xoffset*16,ft_yoffset*16));
      }
      Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));			
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ft_xoffset*16,ft_yoffset*16));			
      inout = linear(0.1,1,zt_s1,100);		
      zin = (ft_int16_t)(256/inout);	
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(zin));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(zin));
      r = smoothstep(100,8,zt_s1,100);
      x = smoothstep(512,200,zt_s1,100);
      y = smoothstep(-50,43,zt_s1,100);
      ft_xoffset = x;
      ft_yoffset = y-(r*sin(94.2*x));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,2,0));			
      ft_yoffset = 145-ft_yoffset;
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,2,1));	
      if(zt_s1<100) zt_s1+=2;		
      if(zt1<100)zt1+=2;
    }
    if(zt<200)zt+=2;
    end_frame();
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);  	
    ft_iteration_cts++;
  }  while(ft_iteration_cts<300);
  return 0;
}

ft_uint8_t Logo_Example4()
{
  ft_uint16_t zt=0,zt_s=0,zt1=0,zt_s1=0;
  ft_uint16_t ft_iteration_cts = 0;
 ft_int16_t ft_xoffset=0,ft_yoffset=0; 
 ft_uint8_t max_dots = 15;
 float zinout = 4;
 ft_uint8_t exp_p = 0;
 ft_uint16_t sts_rad=0;  

 ft_uint8_t   as[20];
 ft_uint8_t alpha = 255,rad=30; 
 Logo_Intial_setup(sym_prp,6);
 ft_iteration_cts = 0;
 ft_xoffset = 0;
 ft_yoffset = 0;
 zt = zt1 = 0;
 exp_p = 0;
 zt_s = zt_s1 = 0;
 do
 {
    if(istouch())     return 1;
     
    PROGMEM prog_uint32_t std[] = {
     CMD_DLSTART,
     CLEAR(1,1,1),
     BEGIN(BITMAPS),
     SAVE_CONTEXT(),
     CMD_LOADIDENTITY,
   };MEMCMD(std);
    Ft_Gpu_CoCmd_Scale(phost,zinout*65536L,zinout*65536L);
    Ft_Gpu_CoCmd_SetMatrix(phost);
    
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,0,0));	 
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    if(!exp_p)
    {
      zinout = linear(rad,2,zt,200);
      zinout = (rad<=1)?linear(2,1,zt_s,200):(zinout);
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A((ft_uint16_t)(256*zinout)));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E((ft_uint16_t)(256*zinout)));  
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(alpha));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(80,60,1,0));
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255-alpha));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(80,60,1,1));      
    }    
    else
    {
    //    ft_xoffset = acceleration(80,190,zt1,100);
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(190,60,5,0));			
	ft_xoffset = acceleration(80,108,zt1,100);
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,60,4,0));
	ft_xoffset = acceleration(80,52,zt1,100);
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,60,4,1));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2));
	for(ft_uint8_t r=1;r<3;r++)
	{
	  ft_int16_t radius = r*10;
	  if(r==1) zinout = deceleration(1,10,zt_s1,300);		
	  if(r==2) zinout = deceleration(1,.3,zt_s1,300);
	  Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A((ft_uint16_t)(256*zinout)));
	  Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E((ft_uint16_t)(256*zinout)));
	  for(ft_uint8_t r1=1;r1<10;r1++)
	  {
            ft_uint16_t angle = r1*(r*30);
	    zinout = (radius+sts_rad)*cos(angle*0.01744);  //3.14/180=0.01744
	    ft_xoffset = 90+zinout;
	    zinout = (radius+sts_rad)*sin(angle*0.01744);
	    ft_yoffset = 90+zinout;
	    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ft_xoffset*16,ft_yoffset*16));
	  }			
	}
  	if(sts_rad<512)sts_rad+=4;	//if(rad<=0) rad = 480;
	if(zt_s1<300) zt_s1+=1;
	if(zt1<100) zt1+=5;
      }
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(alpha));	
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));	
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    for(ft_uint8_t z=0;z<20;z++)
    {
      if(ft_random(20)==z)
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));	
      else
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));	
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256*ft_pgm_read_byte(ss+z)));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256*ft_pgm_read_byte(ss+z)));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_pgm_read_word(sx+z),ft_pgm_read_byte(sy+z),3,0));
    }    
    end_frame();
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

    rad = linear(30,1,zt,200);
    alpha = linear(255,0,zt,200);
    zinout = linear(4,2,zt,200);  
    if(zt_s<200)zt_s +=2; else exp_p = 1;  
    if(zt<200){zt+=2; zt_s=0;}     
    ft_iteration_cts++; 
 }while(ft_iteration_cts<300);  
 return 0;
}
ft_uint8_t  Logo_Example3()
{
  ft_uint16_t ft_iteration_cts = 0;
  ft_int16_t ft_xoffset=0,ft_yoffset=0,temp_xoffset,temp_yoffset,temp1_xoffset,temp2_xoffset; 
  ft_uint8_t alpha=255;
  ft_uint8_t rate=100,dst=0,src=210;
  ft_uint16_t resize = 1;
  ft_int16_t zt=0,zt1=0,r=30,cts=0,zt2=0,zt3=0,zt4=0,zt5=0;
  float zinout = .5,r2=0.01,r3=0;
  Logo_Intial_setup(hbo_prp,7);
  do
  { 
     if(istouch())
      return 1;
     
    PROGMEM prog_uint32_t std[] =
    {
      CMD_DLSTART,
      CLEAR(1,1,1),
      SAVE_CONTEXT(),
      BEGIN(BITMAPS),
      BITMAP_HANDLE(0),
      COLOR_A(180),
    };MEMCMD(std);
    zinout = linear(5,0.5,zt,rate);	      // 
    ft_xoffset = 210;
    ft_yoffset = 104;
    temp2_xoffset = 0; 
    temp1_xoffset =  temp_xoffset = ft_xoffset;
    temp_yoffset = ft_yoffset;
    Ft_Gpu_CoCmd_LoadIdentity(phost);
    if(zinout<=1.3 && ft_iteration_cts>150)
    {
      ft_xoffset = acceleration(480,269,zt2,50);
      if(zt2<50)zt2+=2;   
      if(ft_xoffset<=269)
      {
        ft_xoffset = smoothstep(269,155,zt3,100);
        if(zt3<100)zt3+=2; 
        if(zt4<50)zt4+=2; 
      }
      temp_xoffset = ft_xoffset;
      temp_yoffset = ft_yoffset;  
      ft_xoffset = acceleration(210,0,zt4,50);//  ball
      temp1_xoffset = ft_xoffset;
      if(ft_xoffset<=0)
      {
        ft_xoffset = deceleration(0,90,zt5,100); // ball
        temp2_xoffset = ft_xoffset;
        if(zt5<100) zt5+=2;
      }
  
    }
    ft_xoffset =min(210,ft_xoffset);
    Ft_Gpu_CoCmd_Translate(phost,-1*ft_xoffset*65536,0);
    Ft_Gpu_CoCmd_Scale(phost,3*65536,3*65536);
    Ft_Gpu_CoCmd_SetMatrix(phost);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,0));
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    alpha = linear(255,0,zt,100);
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(alpha));
    ft_xoffset = 60; //(FT_DispWidth - 360)/2;
    ft_yoffset = 134+2;//(FT_DispHeight- 4)/2;
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,2,0));  // Center line
    r2 = linear(0.01,1,zt,rate);	
    ft_xoffset = smoothstep(239,400,zt,rate);	
    ft_yoffset = (FT_DispHeight- 136*r2)/2;
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A((ft_uint16_t)(256/r2)));  
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E((ft_uint16_t)(256/r2)));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,3,0));   // side curvature
    ft_xoffset = smoothstep(239,60,zt,rate);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,3,1));   // side curvature
    alpha = linear(255,0,zt1,300);		
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(alpha));
    ft_xoffset = 168;//(FT_DispWidth - 72*2)/2;
    ft_yoffset = 64;//(FT_DispHeight- 72*2)/2;
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A((ft_uint16_t)(128)));    // center  lighting
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E((ft_uint16_t)(128)));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,4,0)); 	
    if(zinout>0.5)
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A((ft_uint16_t)(256/zinout)));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E((ft_uint16_t)(256/zinout)));
    ft_xoffset = (ft_uint16_t)(FT_DispWidth - 120*zinout)/2;
    ft_yoffset = (ft_uint16_t)(FT_DispHeight- 120*zinout)/2;
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ft_xoffset*16,ft_yoffset*16)); 
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT()); 	
    if(zinout<=1.3)
    {      
      ft_xoffset = temp_xoffset;
      ft_yoffset = temp_yoffset;
      if( ft_iteration_cts>150)
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset+3,6,0));      // ftdi string
      ft_xoffset = temp1_xoffset;//acceleration(210,0,zt4,50);	
      if(ft_xoffset<=0)
      {
        ft_xoffset = temp2_xoffset;//deceleration(0,90,zt5,100);
        if(ft_xoffset>=48)
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(48,ft_yoffset,5,0));			// tail
      }  
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,5,1));	// ftdiball*/
    }
    end_frame();
    Ft_App_Flush_Co_Buffer(phost); 
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);  
    if(zt<rate) zt+=2; 
    if(zt1<300) zt1+=2; 
    ft_iteration_cts++;
  }while(ft_iteration_cts<300);
  return 0;
}

ft_uint8_t Logo_Example2()
{
  ft_uint16_t zt=0,zt_s=0,zt1=0,zt_s1=0;
  ft_uint16_t ft_iteration_cts = 0;
  ft_int16_t ft_xoffset=0,ft_yoffset=0,h,adj=0; 
  ft_uint8_t ch=0,handle=0,rangle=30,pausects=0;
  ft_uint8_t zoom_iteration_cts = 160,noofzinout=0,rate=8,tempitcts=0;  
  ft_uint8_t  img_width = 68;
  ft_uint8_t  img_height = 68;
  float z_dst=2,z_src=1;
  ft_iteration_cts = 0;
  Logo_Intial_setup(target_prp,4);
  do
  {
     if(istouch())
      return 1;
     
  //----------------------------------------------------zoominout properties-------------------------------------   
    if(zt1>=zoom_iteration_cts && handle==0)
    {
      if(noofzinout<3)
      {
        noofzinout+=1;
        zt1 = 0;
      }else
      {
        handle=3;
        tempitcts = ft_iteration_cts+20;
        zt1 = 0;   

      }
      if(noofzinout==1)        
      {  
        zoom_iteration_cts = 200;
        z_src = 2;z_dst = 1; rate=10;
      }
      if(noofzinout==2)
      {  
        zoom_iteration_cts = 160;
        z_src = 1;z_dst = 2; rate=8;
      }
      if(noofzinout==3)
      {  
        zoom_iteration_cts = 200;
        z_src = 2;z_dst = 0.1; rate=10;
      }
    }
  //------------------------------------------------------------------------------------------------------------   
    float zinout = linear(z_src,z_dst,zt1,zoom_iteration_cts);  
    ft_xoffset = (FT_DispWidth  - zinout*68)/2;
    ft_yoffset = (FT_DispHeight - zinout*68)/2; 
    
    PROGMEM prog_uint32_t std[] = {
    CMD_DLSTART,
    CLEAR_COLOR_RGB(255,255,255),
    CLEAR(1,1,1),
    BEGIN(BITMAPS),
    };MEMCMD(std); 
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(197,0,15));
    Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
    if(handle==3 && tempitcts<ft_iteration_cts && rangle>0)
    {    
     zinout = linear(0.1,1,zt1,200);	
     ft_xoffset = (FT_DispWidth  - zinout*120)/2;
     ft_yoffset = (FT_DispHeight - zinout*60)/2;
     Ft_Gpu_CoCmd_LoadIdentity(phost);
     Ft_Gpu_CoCmd_Scale(phost,zinout*65536,zinout*65536);
     Ft_Gpu_CoCmd_Translate(phost,60*65536,30*65536);
     Ft_Gpu_CoCmd_Rotate(phost,-rangle*65536/360);
     Ft_Gpu_CoCmd_Translate(phost,-60*65536,-30*65536);
     Ft_Gpu_CoCmd_SetMatrix(phost);      
     Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
     Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset+70-rangle,ft_yoffset+20,2,0));
     zinout = linear(0.1,.5,zt1,200);	
     ft_xoffset = (FT_DispWidth  - zinout*240)/2;
     ft_yoffset = (FT_DispHeight - zinout*150)/2;
     Ft_Gpu_CoCmd_LoadIdentity(phost);
     Ft_Gpu_CoCmd_Scale(phost,zinout*65536,zinout*65536);
     Ft_Gpu_CoCmd_Translate(phost,240*65536,150*65536);
     Ft_Gpu_CoCmd_Rotate(phost,-rangle*65536/360);
     Ft_Gpu_CoCmd_Translate(phost,-240*65536,-150*65536);
     Ft_Gpu_CoCmd_SetMatrix(phost);			
     Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(197,0,15));
     Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset-100,ft_yoffset,3,0));
     if(rangle>0) rangle--; 
     if(zt1<zoom_iteration_cts){ zt1+=rate; } 	
    }	
    if(rangle<=0)
    {
       zinout = linear(0.49,0.8,zt_s,100);	
       ft_xoffset = (FT_DispWidth  - zinout*240)/2;
       ft_yoffset = (FT_DispHeight - zinout*150)/2;
       Ft_Gpu_CoCmd_LoadIdentity(phost);
       Ft_Gpu_CoCmd_Scale(phost,zinout*65536,zinout*65536);
       Ft_Gpu_CoCmd_SetMatrix(phost);			
       Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(197,0,15));
       Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset-96,ft_yoffset,3,0));
       Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());	
       ft_xoffset = 180;
       ft_yoffset = 110;
       Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
       Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset+70,ft_yoffset+20,2,0));
       ft_xoffset = smoothstep(480,250,zt_s,100);
       ft_yoffset = 80;
       if(pausects<50 || pausects>=80)
	{
	  ft_xoffset = smoothstep(480,250,zt_s,100);
	  ft_yoffset = 80;
	}
	else if(pausects>=50 && pausects<80)
	{		
	  ft_xoffset = 240+ft_random(10);
	  ft_yoffset = 70+ft_random(10);; 
	}
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,1,0));
	pausects++;
        if(zt_s<100)zt_s+=2;
    } 
    if(handle==0)
    {
      Ft_Gpu_CoCmd_LoadIdentity(phost);
      Ft_Gpu_CoCmd_Scale(phost,zinout*65536,zinout*65536);
      Ft_Gpu_CoCmd_SetMatrix(phost);
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset,ft_yoffset,handle,0)); 	
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());	
      if(ft_iteration_cts>=100)      // Wait for 100 itreations
      {
        adj = smoothstep(0,220,zt,200);		
        if(zt<200) zt+=4;	
        if(zt1<zoom_iteration_cts){ zt1+=rate; } 	
      }
      ft_xoffset = 206;  //(width-img_width)/2
      ft_yoffset = 102;  //(height-img_height)/2
      for(ft_uint8_t z=0;z<48;z++)
      {
        h = ft_pgm_read_word(hyp+z)+adj;
        ft_int16_t temp_theta = h*pgm_read_float(t_can+z);
  	ft_int16_t xoff = ft_xoffset+temp_theta;	
        temp_theta = h*pgm_read_float(t_san+z);
  	ft_int16_t yoff = ft_yoffset+temp_theta;	
  	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoff*16,yoff*16));
      }		
    }    
    end_frame();
    Ft_App_Flush_Co_Buffer(phost); 
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);  
    ft_iteration_cts++;  
  }while(ft_iteration_cts<500);
  return 0;
}


ft_uint8_t Logo_Example1()
{
  ft_uint16_t zt=0,zt_s=0,zt1=0,zt_s1=0;
  ft_uint16_t ft_iteration_cts = 0;
  ft_int16_t ft_xoffset=0,ft_yoffset=0; 
  ft_uint8_t bargraph=0;
  ft_int32_t ftsize=0,wp=0,adsize,rp,val;
  ft_int16_t bh = 0,xoff=280;
  float v1 = 133,v2 = 310,yoff=133,bw=20;
 
  Logo_Intial_setup(fuse_prp,2);
  Reader barfile,adfile;
  byte file = barfile.openfile("EFUSE.BAR");
  ftsize = barfile.size;
  
  file = adfile.openfile("EFUSE.ULW");
  adsize = adfile.size;
  
  Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_FREQ,44100);
  Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_START,AUDIO_ADDRESS);
  Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_FORMAT,ULAW_SAMPLES);
  Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_LENGTH,8192L);
  Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_LOOP,1);
  Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB,255);
  Load_file2gram(AUDIO_ADDRESS+0UL,2,adfile); 
  wp = 1024;   
  adsize-=1024;
  
  Ft_Gpu_Hal_Wr8(phost,REG_PLAYBACK_PLAY,1);  
  Load_file2gram(RAM_G,1,barfile);
  ftsize-=512L;
  ft_iteration_cts = 0;
  ft_xoffset = 0;
  ft_yoffset = 0;
  zt = zt1 = 0;
  while(adsize > 0)
  { 
    if(istouch())
    {
        Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB,0);
    Ft_Gpu_Hal_Wr8(phost,REG_PLAYBACK_PLAY,0);
      return 1;
    }  
      
     PROGMEM prog_uint32_t std[] = {
     CMD_DLSTART,
     CLEAR_COLOR_RGB(143,10,10),
     CLEAR(1,1,1),
     CLEAR_COLOR_RGB(255,255,255),
   };MEMCMD(std);
    
   Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(ft_xoffset,ft_yoffset)); 
   ft_xoffset = 630-(2*ft_xoffset); 
   ft_yoffset = 272-(2*ft_yoffset);
   Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(ft_xoffset,ft_yoffset));    
   Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
   Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(143,10,10));
   if(ft_iteration_cts>130)
    { 
      ft_yoffset = deceleration(0,60,zt,100);
      if(zt<100)zt+=5; 
    }else ft_yoffset = 0;    
    if(ft_yoffset>=60 && ft_iteration_cts>=220)
    {
      ft_xoffset = deceleration(0,260,zt1,100);
      bh = deceleration(0,158,zt1,100);
      if(zt1<100)zt1+=4; 
    }else ft_xoffset = 0;  
    
    if(ft_xoffset<260)
    {
       PROGMEM prog_uint32_t std1[] = {
        SAVE_CONTEXT(),
        BEGIN(BITMAPS),
        CMD_LOADIDENTITY,
        CMD_SCALE,262140,262140,
        CMD_SETMATRIX,
        VERTEX2II(0,0,0,0),
        RESTORE_CONTEXT(),
      };MEMCMD(std1);
    }
    
    ft_int16_t temp_x = (ft_int16_t)xoff-v2;
    ft_int16_t temp_y = (ft_int16_t)yoff-v1;
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0*16,temp_y*16));   
    temp_y = (ft_int16_t)yoff+v1+6;
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,temp_y*16));         
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_int16_t)(xoff-v2)*16,bh*16));   
    temp_x = temp_x+bw;
    temp_y = 355 - bh;
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(temp_x*16,temp_y*16)); 

    if(ft_xoffset>=260)
    {
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(ft_xoffset+10,ft_yoffset+5,1,0));  
      bargraph = 0;
    }     
    if(bargraph)
    {
      PROGMEM  prog_uint32_t std2[] = {
        BITMAP_HANDLE(2),
        BITMAP_SOURCE(RAM_G),
        BITMAP_LAYOUT(BARGRAPH,256,1),				
        BEGIN(BITMAPS),
      };MEMCMD(std2);
      
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER,BORDER,256,(ft_uint16_t)yoff));	    
      ft_int16_t vx = (ft_int16_t)(temp_x+bw);
      ft_int16_t vy = 0; 
            Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(vx,vy,2,0));  
      Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
      vy = yoff/2;
      Ft_Gpu_CoCmd_LoadIdentity(phost);
      Ft_Gpu_CoCmd_Translate(phost,(vx)*65536L, 65536L*(vy));
      Ft_Gpu_CoCmd_Scale(phost,1*65536, 65536*-1);
      Ft_Gpu_CoCmd_Translate(phost,-(vx)*65536L, 65536L*-(vy));
      Ft_Gpu_CoCmd_SetMatrix(phost);       
      vy = yoff+5;
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(vx,vy,2,0));                
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());		       
      if(vx<256)          // clip second half
      {
        vx = (ft_int16_t)(256+temp_x+bw);
        vy = 0;
        Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(vx,0,2,1));             
        vy = yoff/2;
        Ft_Gpu_CoCmd_LoadIdentity(phost);
        Ft_Gpu_CoCmd_Translate(phost,(vx)*65536L, 65536L*(vy));
        Ft_Gpu_CoCmd_Scale(phost,1*65536, 65536*-1);
        Ft_Gpu_CoCmd_Translate(phost,-(vx)*65536L, 65536L*-(vy));
        Ft_Gpu_CoCmd_SetMatrix(phost); 
        vy = yoff+5;
        Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(vx,vy,2,1));      
        Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
       }  
       Load_file2gram(RAM_G,1,barfile);
       ftsize-=512L;	
    }
   
    end_frame();
    Ft_App_Flush_Co_Buffer(phost); 
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);  

   
    rp = Ft_Gpu_Hal_Rd16(phost,REG_PLAYBACK_READPTR);
    val = 8191L & (rp-wp);				
    if (val > 1024) 
    {
        ft_uint16_t n = min(1024,adsize);  
        Load_file2gram(AUDIO_ADDRESS+wp,2,adfile);  
        wp = (wp +1024) & 8191L;
        adsize-=n;
    }
   
    if(v1>1) v1 = v1-v1/10;     
    else if(v2>1)
    {                   
      v2 = v2-v2/10;   
      bw = bw-bw/100;  
      yoff+=.8;        // move the horizontal gap to bottom
      bargraph = 1;   
    }
    ft_iteration_cts++;
  }
  Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB,0);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAYBACK_PLAY,0);
  return 0;
}
static ft_uint8_t keypressed=0;
ft_uint8_t Read_Keys()
{
  static ft_uint8_t Read_tag=0,temp_tag=0;	
  ft_uint8_t ret_tag=0;
  Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(Read_tag!=NULL)								// Allow if the Key is released
  {
    if(temp_tag!=Read_tag)
    {
      temp_tag = Read_tag;	
      keypressed = Read_tag;										// Load the Read tag to temp variable	
    }  
  }
  else
  {
    if(temp_tag!=0)
    {
      ret_tag = temp_tag;
      temp_tag = 0;
    }  
    keypressed = 0;
  }
  return ret_tag;
}

char *info[] = {  "FT800 Logos Application",
          	  "APP to demonstrate interactive Menu,",
                  "and logo animation,",	 
		  "using Bitmaps,&Track",	 
                };
                       
ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
  Ft_CmdBuffer_Index = 0;
  PROGMEM prog_uint32_t std1[] = {
      CMD_DLSTART,
       CLEAR(1,1,1),
      COLOR_RGB(255,255,255),
  };MEMCMD(std1);
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  PROGMEM  prog_uint32_t std2[] = {
       CMD_CALIBRATE,0,
     DISPLAY(),
     CMD_SWAP,
  };MEMCMD(std2);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ)); 
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -=4;
  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,100000L);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
  do
  {
    Ft_Gpu_CoCmd_Dlstart(phost);   
    Ft_Gpu_CoCmd_Append(phost,100000L,dloffset);
    PROGMEM prog_uint32_t std3[] = {
    BITMAP_TRANSFORM_A(256),
    BITMAP_TRANSFORM_A(256),
    BITMAP_TRANSFORM_B(0),
    BITMAP_TRANSFORM_C(0),
    BITMAP_TRANSFORM_D(0),
    BITMAP_TRANSFORM_E(256),
    BITMAP_TRANSFORM_F(0),
    SAVE_CONTEXT(),
    COLOR_RGB(219,180,150),
    COLOR_A(220),
    BEGIN(EDGE_STRIP_A),
    };MEMCMD(std3);
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
    if(keypressed!='P')
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    else
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));
    PROGMEM prog_uint32_t std4[] = {
    BEGIN(FTPOINTS),
    POINT_SIZE(20*16),
    TAG('P'),
    VERTEX2F(3840,3392),
    COLOR_RGB(180,35,35),
    BEGIN(BITMAPS),
    VERTEX2II(226,197,14,4),
    DISPLAY(),
    CMD_SWAP,
    };MEMCMD(std4);  
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  }while(Read_Keys()!='P');
  Play_Sound(0x52,255);
}

static ft_int32_t trackval = 0;
ft_uint8_t Selection_Menu()
{ 
  ft_uint8_t Menu=0,temp_Menu=0,rotation_flag=0,played=0;
  ft_uint16_t rotation_cts = 0,scroller_vel;
  ft_int16_t prevth=0,currth=0,key_deteted_cts;
  Menu_prp = 1;
  Logo_Intial_setup(menu_prp,3);
  Ft_CmdBuffer_Index = 0;
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,1);
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,2);
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,3);
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,4);
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,5);
  Ft_Gpu_CoCmd_Track(phost,240,136,1,1,6);
  Ft_App_Flush_Co_Buffer(phost);													//L8 format
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  do
  {
     ft_uint32_t sy = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
     ft_uint8_t key_detected = sy&0xff; 
     if(key_detected>0)
     {
       rotation_cts = 0;
       key_deteted_cts++;
       if(key_detected<6)
       temp_Menu = key_detected; else temp_Menu = 0;
       Menu = 0;
     }   
     if(key_detected==0)
     {
       if(key_deteted_cts>0&&key_deteted_cts<15)    
       {          
         if(!rotation_flag)
         Menu = temp_Menu; else
         rotation_flag = 0;
       }    
       key_deteted_cts = 0;
       if(rotation_cts>100)     
       {
         trackval++; 
         rotation_flag = 1;
       }else
       rotation_cts++;
     }     
     if(key_deteted_cts>15)
     {  
       currth = (sy>>16)/182;
       if(prevth!=0)
       trackval += (currth-prevth);
       prevth = currth;
       rotation_cts = 0;
       rotation_flag = 0;
     }else
     {
        prevth=0;    
     }  
     
     PROGMEM prog_uint32_t std[] = 
     {
         CMD_DLSTART,
         CLEAR_COLOR_RGB(12,61,90),      
         CLEAR(1,1,1),  
         BEGIN(BITMAPS),
         SAVE_CONTEXT(),
         BITMAP_TRANSFORM_A(128),
         BITMAP_TRANSFORM_E(128),
         VERTEX2II(0,0,2,0),
         RESTORE_CONTEXT(),
         COLOR_RGB(0,0,0),
         TAG_MASK(1),
         TAG(6),
         VERTEX2II(0,0,1,0), 
         TAG_MASK(0),
         COLOR_RGB(255,255,255),
         POINT_SIZE(40*16),
     };MEMCMD(std);       
     for(ft_uint8_t x=1;x<6;x++)  
     {      
       ft_int16_t theta = (72*x);
       theta +=(-trackval); theta%=360; 
       ft_int16_t xoff = 200+(ft_int16_t)(170*cos(theta*0.0174));
       ft_int16_t yoff = 80-(ft_int16_t)(80*sin(theta*0.0174));
       ft_uint16_t zinout = 350-yoff;
       xoff = xoff+((100-(100*yoff*0.0115)))/2;      
       Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(zinout));
       Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(zinout));
       if(x==key_detected)
       Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(150,150,150));
       else
       Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
       Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); 
       Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoff,yoff,0,x-1));    
       Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS)); 
       Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
       Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));     
       Ft_App_WrCoCmd_Buffer(phost,TAG(x));   // 100% transparent bmp  
       Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xoff+25)*16,(yoff+25)*16));   // 100% transparent bmp 
       Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
       Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
     } 
     end_frame();
     
     Ft_App_Flush_Co_Buffer(phost); 
     Ft_Gpu_Hal_WaitCmdfifo_empty(phost);     
  }while(Menu==0);
  return Menu;  
}

//***********************************Logo*********************************//


ft_void_t Ft_BootupConfig()
{
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

	/* Access address 0 to wake up the FT800 */
	Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);  
	Ft_Gpu_Hal_Sleep(20);

		/* Set the clk to external clock */
#if (!defined(ME800A_HV35R) && !defined(ME810A_HV35R))
		Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);
		Ft_Gpu_Hal_Sleep(100);
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

#if (defined(ME800A_HV35R) || defined(ME810A_HV35R))
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
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);

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

#if 1
#ifdef FT_81X_ENABLE
    /* bootup jt for capacitive touch panel */
    ft_delay(300);
    //Ft_Gpu_Hal_Wr16(phost, REG_CYA_TOUCH, 0x8000);
    //ft_delay(300);
    Ft_Gpu_Hal_Wr8(phost, REG_CPURESET, 2);
    ft_delay(300);
#if defined(FT_811_ENABLE) || defined(FT_813_ENABLE)
    Ft_Gpu_Hal_Wr16(phost, REG_CYA_TOUCH, (Ft_Gpu_Hal_Rd16(phost,REG_CYA_TOUCH) & 0x7fff));//capacitive touch
#endif
#if defined(FT_810_ENABLE) || defined(FT_812_ENABLE)
    Ft_Gpu_Hal_Wr16(phost, REG_CYA_TOUCH, (Ft_Gpu_Hal_Rd16(phost,REG_CYA_TOUCH) | 0x8000));//resistive touch
#endif
    ft_delay(300);
    Ft_Gpu_Hal_Wr8(phost, REG_CPURESET, 0);
    ft_delay(300);
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
    /* Change clock frequency to 25mhz */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);

#if (defined(ENABLE_SPI_QUAD))
    /* Initialize IO2 and IO3 pad/pin for dual and quad settings */
    gpio_function(31, pad_spim_io2);
    gpio_function(32, pad_spim_io3);
    gpio_write(31, 1);
    gpio_write(32, 1);
#endif
	/* Enable FIFO of QSPI */
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
#endif

#ifdef ENABLE_SPI_QUAD
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,4);
#endif
#elif ENABLE_SPI_DUAL	
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,2);
#endif
#else
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,1);
#endif

	
#endif

	phost->ft_cmd_fifo_wp = Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE);
}


/* Main entry point */
#if defined MSVC_PLATFORM  || defined FT900_PLATFORM
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{
  ft_uint8_t chipid;

  Ft_Gpu_HalInit_t halinit;

  halinit.TotalChannelNum = 1;
  Ft_Gpu_Hal_Init(&halinit);
  host.hal_config.channel_no = 0;
	host.hal_config.pdn_pin_no = FT800_PD_N;
	host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;
#ifdef MSVC_PLATFORM_SPI
  host.hal_config.spi_clockrate_khz = 12000; //in KHz
#endif
#ifdef ARDUINO_PLATFORM_SPI
  host.hal_config.spi_clockrate_khz = 4000; //in KHz
#endif
  Ft_Gpu_Hal_Open(&host);

  phost = &host;

  Ft_BootupConfig();


  /*It is optional to clear the screen here*/
  Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
  Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
 
  Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen. 
  
  /* Close all the opened handles */

  pinMode(FT_SDCARD_CS, OUTPUT);
  digitalWrite(FT_SDCARD_CS, HIGH);
  delay(100);
  ft_flags.sd_present =  SD.begin(FT_SDCARD_CS);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
  Ft_Gpu_Hal_WrCmd32(phost,250*1024L);
  Ft_Gpu_Hal_WrCmdBuf(phost,home_star_icon,sizeof(home_star_icon));
  static PROGMEM prog_uint32_t std2[] = {
      CMD_DLSTART,
      CLEAR(1,1,1),
      BITMAP_HANDLE(14),
      BITMAP_SOURCE(250*1024L),
      BITMAP_LAYOUT(L4, 16, 32),
      BITMAP_SIZE(NEAREST, BORDER, BORDER, 32, 32),			
  };MEMCMD(std2);
  end_frame();
  Ft_App_Flush_Co_Buffer(phost); 
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost); 
  Info();
  trackval = 0;
  while(1)
  {
    ft_uint8_t exit = 0;
    ft_uint8_t Menu = Selection_Menu(); 
    if(Menu!=0 && Menu<=5)
    {
      Play_Sound(0x52,255);
      do
      {
        Menu_prp = 0;
        switch(Menu)
        {
          case 1: 
          exit = Logo_Example2();    // target
          break;
          
          case 2: 
          exit = Logo_Example4();    //SYMAN  
          break;
          
          case 3: 
          exit = Logo_Example3();    //HBO
          break;
       
          case 4: 
          exit = Logo_Example1();    // FUSE
          break;
          
          case 5  :    
          exit  = Logo_Example5();  // MUSIC
          break;
        }  
      }while(exit==0);  
    }
  }
  
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
