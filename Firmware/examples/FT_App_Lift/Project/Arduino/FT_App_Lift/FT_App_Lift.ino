/*
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
*/
/*#include "FT_Platform.h"
#include "FT_Hal_SPI.cpp"
#include "FT_CoPro_Cmds.cpp"
#include "Swiss.cpp"*/

/* Sample application to demonstrat FT800 primitives, widgets and customized screen shots */
#include <Wire.h>
#include <string.h>
#include "FT_Platform.h"
#include "sdcard.h"

#include "FT_App_Lift.h"
#include "FT_DataTypes.h"

#define ORIENTATION_PORTRAIT
//#define ORIENTATION_LANDSCAPE

#define STARTUP_ADDRESS	100*1024L
#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)

#define F16(s)        ((ft_int32_t)((s) * 65536))
#define SQ(v) (v*v)
#define AUDIO_SECTORS  4
#define AUDIO_RAM_SIZE  4096L
#define AUDIO_RAM_SPACE (AUDIO_RAM_SIZE-1)
Reader g_reader;


/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
#ifdef DISPLAY_RESOLUTION_WQVGA  
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
#else
ft_int16_t  FT_DispWidth = 320;
ft_int16_t  FT_DispHeight = 240;
ft_int16_t  FT_DispHCycle =  408;
ft_int16_t  FT_DispHOffset = 70;
ft_int16_t  FT_DispHSync0 = 0;
ft_int16_t  FT_DispHSync1 = 10;
ft_int16_t  FT_DispVCycle = 263;
ft_int16_t  FT_DispVOffset = 13;
ft_int16_t  FT_DispVSync0 = 0;
ft_int16_t  FT_DispVSync1 = 2;
ft_uint8_t  FT_DispPCLK = 8;
ft_char8_t  FT_DispSwizzle = 2;
ft_char8_t  FT_DispPCLKPol = 0;
#endif


/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;



ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;
//ft_uint8_t temp_time[7];
//ft_uint32_t music_playing = 0,fileszsave = 0;
//Ft_DlBuffer_Index = 0;

#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif


FT_PROGMEM const FT_Gpu_Fonts_t g_Gpu_Fonts[]  = {
/* VC1 Hardware Fonts index 28*/
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,8,13,12,15,13,5,7,7,9,12,5,9,6,9,12,12,12,12,12,12,12,12,12,12,5,5,11,12,11,10,19,13,13,13,14,12,12,14,15,6,12,14,12,18,15,14,13,15,13,13,13,14,14,18,13,13,13,6,9,6,9,10,7,12,12,11,12,11,8,12,12,5,5,11,5,18,12,12,12,12,7,11,7,12,11,16,11,11,11,7,5,7,14,5,
2,
9,
18,
25,
950172},
};

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

#if 0
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

#endif

//=====================================================================================================================================================//
/* Optimized implementation of sin and cos table - precision is 16  */
FT_PROGMEM ft_prog_uint16_t sintab[] =
{
  0, 402, 804, 1206, 1607, 2009, 2410, 2811, 3211, 3611, 4011, 4409, 4807, 5205, 5601, 5997, 6392,
  6786, 7179, 7571, 7961, 8351, 8739, 9126, 9511, 9895, 10278, 10659, 11038, 11416, 11792, 12166, 12539,
  12909, 13278, 13645, 14009, 14372, 14732, 15090, 15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868,
  18204, 18537, 18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705, 22004, 22301, 22594,
  22883, 23169, 23452, 23731, 24006, 24278, 24546, 24811, 25072, 25329, 25582, 25831, 26077, 26318, 26556, 26789,
  27019, 27244, 27466, 27683, 27896, 28105, 28309, 28510, 28706, 28897, 29085, 29268, 29446, 29621, 29790, 29955,
  30116, 30272, 30424, 30571, 30713, 30851, 30984, 31113, 31236, 31356, 31470, 31580, 31684, 31785, 31880, 31970,
  32056, 32137, 32213, 32284, 32350, 32412, 32468, 32520, 32567, 32609, 32646, 32678, 32705, 32727, 32744, 32757,
  32764, 32767, 32764
};

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

/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
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
/* Example code to display few points at various offsets with various colors */



/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */
FT_PROGMEM const ft_uint8_t FT_DLCODE_BOOTUP[12] =
{
  255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
  7,0,0,38, //GPU instruction CLEAR
  0,0,0,0,  //GPU instruction DISPLAY
};


ft_uint8_t sd_present =0;


//================================Dec to binary and binary to dec =========================================
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}


/* API to load raw data from file into perticular locatio of ft800 */

ft_int32_t Ft_App_LoadRawFromFile(Reader &r,ft_int32_t DstAddr)
{
	ft_int32_t FileLen = 0,Ft800_addr = RAM_G;
        ft_uint8_t bytes_size,sector_size;
	//ft_uint8_t *pbuff = NULL;
        ft_uint8_t pbuff[512];
       
 	Ft800_addr = DstAddr;
        FileLen = r.size;
      
 	while(FileLen > 0)
	{
                /* download the data into the command buffer by 512bytes one shot */
		ft_uint16_t blocklen = FileLen>512?512:FileLen;
                r.readsector(pbuff);
		FileLen -= blocklen;
		/* copy data continuously into FT800 memory */
		Ft_Gpu_Hal_WrMem(phost,Ft800_addr,pbuff,blocklen);
		Ft800_addr += blocklen;
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
	float pi = 3.1415926535;
	float angle = -th*pi/180;
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


/* API for floor number logic */
ft_int32_t FT_LiftApp_FTransition(S_LiftAppCtxt_t *pLACtxt, ft_int32_t dstaddr/*,ft_uint8_t opt_landscape*/)
{

	ft_char8_t StringArray[8];
	ft_int32_t dst_addr_st_file;
	StringArray[0] = '\0';
	
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
                  //for landscape orienation
                  if(pLACtxt->opt_orientation == 1)
                  {
                	if((LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState) && (0 == pLACtxt->AudioPlayFlag))
                	{
                          ft_char8_t *pstring;
                          
                          pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
                          pLACtxt->AudioFileIdx = 0;
                          pLACtxt->AudioCurrAddr = dstaddr;
                          pLACtxt->AudioCurrFileSz = 0;
                          pstring = pLACtxt->AudioFileNames;
                          *pstring = '\0';
                          strcpy(pstring,"bl.wav");
                          pstring += strlen(pstring);
                          *pstring++ = '\0';
                          StringArray[0] = '\0';
                          Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
                          strcat(StringArray, ".wav");
                          strcpy(pstring,StringArray);
                          pstring += strlen(pstring);
                          *pstring++ = '\0';
                          *pstring = '\0';
                          pLACtxt->AudioPlayFlag = 1;
                          //Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                        }
                	  /* load the ring tone */
                 	Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                        
                  }
          
                     //for portrait orientation
                    else if(pLACtxt->opt_orientation == 0)  
                    {
                       
                	if((LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState) && (0 == pLACtxt->AudioPlayFlag))
                	{
                          ft_char8_t *pstring;
                          
                          pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
                          pLACtxt->AudioFileIdx = 0;
                          pLACtxt->AudioCurrAddr = dstaddr;
                          pLACtxt->AudioCurrFileSz = 0;
                          pstring = pLACtxt->AudioFileNames;
                          *pstring = '\0';
                          strcpy(pstring,"bf.wav");
                          pstring += strlen(pstring);
                          *pstring++ = '\0';
                          StringArray[0] = '\0';
                          Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pLACtxt->CurrFloorNum);
                          strcat(StringArray, ".wav");
                          strcpy(pstring,StringArray);
                          pstring += strlen(pstring);
                          *pstring++ = '\0';
                          *pstring = '\0';
                          //Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                          pLACtxt->AudioPlayFlag = 1;
                        }
                	  /* load the ring tone */
                          Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                         
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
                 
		/* delay the below code till rate is completed - modify the rate if application over rights */
		//limitation - make sure the max is +ve
		while((pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum) || (0 == pLACtxt->NextFloorNum))//to make sure the same floor number is not assigned
		{
			pLACtxt->NextFloorNum = pLACtxt->SLATransPrms.MinFloorNum + ft_random(pLACtxt->SLATransPrms.MaxFloorNum - pLACtxt->SLATransPrms.MinFloorNum);
		}

		pLACtxt->ArrowDir = LIFTAPP_DIR_DOWN;
		/* generate a new random number and change the direction as well */
                	//if(LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState)	
                {
                  ft_char8_t *pstring;
                  
                  pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
                  pLACtxt->AudioPlayFlag = 0;
                  pstring = pLACtxt->AudioFileNames;
                  *pstring = '\0';                
        	  /* load the ring tone */
		if(pLACtxt->NextFloorNum > pLACtxt->CurrFloorNum)
		{
			pLACtxt->ArrowDir = LIFTAPP_DIR_UP;
			//Ft_App_LoadRawAndPlay("gu.wav",(ft_int32_t)dstaddr,0,0);
                        strcpy(pstring,"gu.wav");
		}
		else
		{
			//Ft_App_LoadRawAndPlay("gd.wav",(ft_int32_t)dstaddr,0,0);
                        strcpy(pstring,"gd.wav");
		}
                          pLACtxt->AudioFileIdx = 0;
                          pLACtxt->AudioCurrAddr = dstaddr;
                          pLACtxt->AudioCurrFileSz = 0;

                   pstring += strlen(pstring);
                  *pstring++ = '\0';
                  *pstring++ = '\0';
                  *pstring = '\0';
                }
		Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */

		/* set the starting of the arrow resize */
		pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
		//set the curr floor number change 
		pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
		return 0;
	}

	/* moving up or moving down logic */
	/* rate based on count */
        Ft_App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
        
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

/* API to demonstrate calibrate widget/functionality */
ft_void_t SAMAPP_CoPro_Widget_Calibrate()
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

ft_int16_t linear(ft_int16_t p1,ft_int16_t p2,ft_uint16_t t,ft_uint16_t rate)
{
       float st  = (float)t/rate;
       return p1+(st*(p2-p1));
}

/* Startup Screen Information*/ 

char* const info[] PROGMEM = {  "FT800 Lift Application",
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
  //Serial.println(sizeof(S_LiftAppCtxt_t));
  Ft_CmdBuffer_Index = 0;  
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
// Ftdi Logo animation  
  //Serial.println("Before Logo");
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ));
//Copy the Displaylist from DL RAM to GRAM
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -=4; 
#ifdef FT_81X_ENABLE
  dloffset -= 2*4;//remove two more instructions in case of 81x
#endif  
  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,STARTUP_ADDRESS);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
    //Serial.println(dloffset);
//Enter into Info Screen
  do
  {
	Ft_CmdBuffer_Index = 0;  
    Ft_Gpu_CoCmd_Dlstart(phost);   
    Ft_Gpu_CoCmd_Append(phost,STARTUP_ADDRESS,dloffset);
//Reset the BITMAP properties used during Logo animation
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(0));  
//Display the information with transparent Logo using Edge Strip  
	
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(219,180,150));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(220));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(EDGE_STRIP_A));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
 
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,20,28,OPT_CENTERX|OPT_CENTERY,info[0]);
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,60,26,OPT_CENTERX|OPT_CENTERY,info[1]);
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,90,26,OPT_CENTERX|OPT_CENTERY,info[2]);  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,120,26,OPT_CENTERX|OPT_CENTERY,info[3]);  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight-30,26,OPT_CENTERX|OPT_CENTERY,"Click to play");

	
//Check if the Play key and change the color
    if(sk!='P')
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    else
	{
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));		
	}
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

ft_int32_t Ft_App_AudioPlay(S_LiftAppCtxt_t *pCtxt,ft_int32_t DstAddr,ft_int32_t BuffChunkSz)
{
  /* awitch according to the current audio state */	        
  //Serial.println(pCtxt->AudioState,DEC);
  switch(pCtxt->AudioState)
  {
    case LIFTAPPAUDIOSTATE_INIT:
    {
      /* Initialize all the parameters and start downloading */
        pCtxt->AudioFileIdx = 0;
        pCtxt->AudioCurrAddr = DstAddr;
        pCtxt->AudioCurrFileSz = 0;
        pCtxt->AudioState = LIFTAPPAUDIOSTATE_DOWNLOAD; 
        Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,0);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);		
        
      break;
    }
    case LIFTAPPAUDIOSTATE_DOWNLOAD:
    {
      ft_uint8_t pbuff[512];//temp buffer for sd read data
      ft_uint16_t blocklen;
      ft_int16_t i,NumSectors;
      
      /* Initialize all the parameters and start downloading */
      NumSectors = BuffChunkSz/LIFTAPPCHUNKSIZE*1L;//hardcoding wrt sdcard sector size
      /* After downloading all the files into graphics RAM change the state to playback */  
      
      if((pCtxt->AudioFileIdx >= LIFTAPPMAXFILENAMEARRAY) || (0 == strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx])))
      {
        pCtxt->AudioState = LIFTAPPAUDIOSTATE_PLAYBACK;
      }            
      else if(pCtxt->AudioCurrFileSz <= 0)
      {
        /* open a new file and start downloading data into the current audio buffer */
        if(0 == strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]))
        {
          pCtxt->AudioState = LIFTAPPAUDIOSTATE_PLAYBACK;
        }
        else
        {
          
          if(0 == g_reader.openfile(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]))
          {
            /* file open failure */
            pCtxt->AudioFileIdx += strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]);
            pCtxt->AudioFileIdx++;
            break;
          }          
          /* file open success */
          
          g_reader.readsector(pbuff);
          pCtxt->AudioCurrFileSz = (ft_int32_t)((pbuff[45]<<24) | (pbuff[44]<<16) | (pbuff[43]<<8) | (pbuff[42]));
          blocklen = pCtxt->AudioCurrFileSz>512?512:pCtxt->AudioCurrFileSz;
          Ft_Gpu_Hal_WrMem(phost,pCtxt->AudioCurrAddr,&pbuff[46],blocklen - 46);                
          pCtxt->AudioCurrAddr += (blocklen - 46);
          pCtxt->AudioCurrFileSz -= (blocklen - 46);
        }
      }
      else
      {
        /* Download data into audio buffer */
        for(i=0;i<NumSectors;i++)
        {
          g_reader.readsector(pbuff);
          //Serial.println(FileSaveLen,DEC);
          blocklen = pCtxt->AudioCurrFileSz>512?512:pCtxt->AudioCurrFileSz;
          Ft_Gpu_Hal_WrMem(phost,pCtxt->AudioCurrAddr,pbuff,blocklen);
          pCtxt->AudioCurrAddr += (blocklen);
          pCtxt->AudioCurrFileSz -= (blocklen);
         
          if(pCtxt->AudioCurrFileSz <= 0)
          {
            pCtxt->AudioFileIdx += strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]);
            pCtxt->AudioFileIdx++;//increment the index to open a new file
            break;
          }
        }
      }
      break;
    }
    case LIFTAPPAUDIOSTATE_PLAYBACK:
    {
      /* Downloading is finished and start the playback */
		Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,DstAddr);//Audio playback start address 
		//Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,totalbufflen);//Length of raw data buffer in bytes		
		Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,(pCtxt->AudioCurrAddr - DstAddr));
		Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
		Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);				
		Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);	
    	
    Ft_Gpu_Hal_Wr8(phost, REG_GPIOX_DIR, 0x8008);
    Ft_Gpu_Hal_Wr8(phost, REG_GPIOX, 0x8);

        pCtxt->AudioState = LIFTAPPAUDIOSTATE_STOP;

      break;
    }
    case LIFTAPPAUDIOSTATE_STOP:
    {
      ft_int32_t rdptr;
      /* Stop the audio playback in case of one shot */
      
      /* Check wether the playback is finished - audio engine */
      rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) - DstAddr;
      if((pCtxt->AudioCurrAddr <= rdptr) || (0 == rdptr))
      {
        
        /* Reset the respective parameters before entering none state */
        pCtxt->AudioFileIdx = 0;
        pCtxt->AudioState = LIFTAPPAUDIOSTATE_NONE;
        pCtxt->AudioFileNames[0] = '\0';
        pCtxt->AudioFileNames[1] = '\0';
      }      
      break;
    }
    case LIFTAPPAUDIOSTATE_NONE:
    default:
    {
      /* Nothing done in this state */
      break;
    }
    
  }      
  return 0;
}


#if 0
/* API to play the music files */
ft_uint32_t play_music(Reader &r /*ft_char8_t *pFileName*/,ft_uint32_t dstaddr,ft_uint8_t Option,ft_uint32_t Buffersz)
{
	
  ft_uint8_t pbuff[512];
    // open the audio file from the SD card
    //Serial.println(pFileName);    
#ifdef MSVC_PLATFORM        
        FILE *pFile = NULL;
#endif 

        //ft_uint8_t music_playing = 0;
	ft_int32_t filesz = 0,chunksize = 512,totalbufflen = 64*1024,currreadlen = 0;
	//ft_uint8_t *pBuff = NULL;
	ft_uint32_t wrptr = dstaddr,return_val = 0;
	ft_uint32_t rdptr,freebuffspace;
	

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
		return 0;
	}
	if(0 == music_playing)
{
#ifdef MSVC_PLATFORM
  	pFile = fopen(pFileName,"rb+");
	fseek(pFile,0,SEEK_END);
	filesz = ftell(pFile);

      	fseek(pFile,42,SEEK_SET);
	filesz = 0;
	fread(&filesz,1,4,pFile);
	fileszsave = filesz;
#endif

#ifdef ARDUINO_PLATFORM

        filesz = r.size;
        fileszsave = filesz;
        
#endif        
	while(filesz > 0)
	{
		currreadlen = filesz;

		if(currreadlen > 512)
		{
			currreadlen = 512;
		}
#ifdef MSVC_PLATFORM
		fread(pBuff,1,currreadlen,pFile);
#endif

        	r.readsector(pbuff);
       	
		//Ft_Gpu_Hal_WrMemFromFlash(phost, wrptr, (ft_uint8_t *)pBuff,currreadlen);
                Ft_Gpu_Hal_WrMem(phost, wrptr, pbuff,currreadlen);
		wrptr +=  currreadlen;
                //Serial.println(wrptr,DEC);
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

	/* return the current write pointer in any case */
	return wrptr;
}
#endif



ft_void_t font_display(ft_int16_t BMoffsetx,ft_int16_t BMoffsety,ft_uint32_t stringlenghth,ft_char8_t *string_display,ft_uint8_t opt_landscape)
{
        ft_uint16_t k;
        
        if(0 == opt_landscape)
        {
	for(k=0;k<stringlenghth;k++)
	  {
	    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,8,string_display[k]));
            BMoffsety -= pgm_read_byte(&g_Gpu_Fonts[0].FontWidth[string_display[k]]);
            
	  }
        }
       	else if(1 == opt_landscape)
	{
	  for(k=0;k<stringlenghth;k++)
        	{
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,28,string_display[k]));
		BMoffsetx += pgm_read_byte(&g_Gpu_Fonts[0].FontWidth[string_display[k]]);
		}
	
	}
}



/* Lift demo application demonstrates background animation and foreground digits and arrow direction */
ft_void_t FT_LiftApp_Landscape()
{
	/* Download the raw data of custom fonts into memory */
	ft_int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
	ft_int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16;
	ft_int32_t Baddr2,i,SzInc = 16,SzFlag = 0;
	ft_uint8_t fontr = 255,fontg = 255,fontb = 255;
	S_LiftAppRate_t *pLARate;
	S_LiftAppTrasParams_t *pLATParams;
	S_LiftAppBallsLinear_t S_LiftBallsArray[6*10],*pLiftBalls = NULL;//as of now 80 balls are been plotted
	float temptransx,temptransy,running_rate;
	S_LiftAppCtxt_t S_LACtxt;
        


	/* load the bitmap raw data */

	/* Initial setup code to setup all the required bitmap handles globally */
	Ft_DlBuffer_Index = 0;
	pLARate = &S_LACtxt.SLARate;
	pLATParams = &S_LACtxt.SLATransPrms;
        ft_uint32_t time =0;
        time = millis();

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
	S_LACtxt.AudioState = 0;
        S_LACtxt.AudioPlayFlag = 0;
        S_LACtxt.opt_orientation = 1;

	
	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen

	Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS));
	Baddr2 = RAM_G + (20 *1024);
        if(1 == g_reader.openfile("font.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BWd*2, BHt*2));

	Baddr2 = ((Baddr2 + (ft_int32_t)BSt*BHt*11 + 15)&~15);
        if(1 == g_reader.openfile("arr.raw"))
  	    Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowWd*2, BArrowHt*2));//make sure the bitmap is displayed when rotation happens

	Baddr2 = Baddr2 + 80*85;
        if(1 == g_reader.openfile("bs6.raw"))
  	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 60, 60));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));

	Baddr2 += 60*55;
        if(1 == g_reader.openfile("bs5.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));

	Baddr2 += 50*46;
        if(1 == g_reader.openfile("bs4.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 40, 37));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 40, 37));

	Baddr2 += 40*37;
        if(1 == g_reader.openfile("bs3.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));

	Baddr2 += 30*27;
        if(1 == g_reader.openfile("bs2.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));

	Baddr2 += 20*18;
        if(1 == g_reader.openfile("bs1.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0,0,7,0));

	Baddr2 += 10*10;
        if(1 == g_reader.openfile("logo.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 129*2, 50));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 129*2, 50));//make sure whole bitmap is displayed after rotation - as height is greater than width
	Baddr2 += 129*2*50;

	
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
	ft_delay(30);

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

	while(1)
	{		
                //loop_start = millis();
  		/* Logic of user touch - change background or skin wrt use touch */
		/* Logic of transition - change of floor numbers and direction */
		FT_LiftApp_FTransition(&S_LACtxt,Baddr2);		

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		/* Background gradient */
		Ft_Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,0,272,0x132B3B);

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
                                Ft_App_GraphicsMatrix_t S_GPUMatrix;
				Ft_App_TrsMtrxLoadIdentity(&S_GPUMatrix);
				Ft_App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
				Ft_App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
				Ft_App_TrsMtrxFlip(&S_GPUMatrix,FT_APP_GRAPHICS_FLIP_BOTTOM);		
				Ft_App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
		
				//Ft_App_TrsMtrxTranslate(&S_GPUMatrix,-80/2.0,-85/2.0,&temptransx,&temptransy);
                                {
                                  Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
				Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
                                }
			}
			else if(LIFTAPP_DIR_DOWN == S_LACtxt.ArrowDir)
			{
				//perform only scaling as rotation is not required
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256*16/SzInc));
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256*16/SzInc));			
			}
			FT_LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd,(FT_DispHeight/2));		
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
		FT_LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BWd*2,(FT_DispHeight/2));

		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256));


                
		BMoffsety = 240;
                BMoffsetx = 0;
                
                Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS)); 
                ft_int16_t t;
                {
                ft_char8_t text_run_disp[] = "The running text is displayed here.";
                ft_int16_t stringlen_text_run_disp;
		stringlen_text_run_disp = strlen(text_run_disp);
		if(t < 300)
			t++;
		BMoffsetx = linear(480,0,t,300);
                //Ft_Gpu_CoCmd_Text(phost,BMoffsetx,BMoffsety,28,OPT_CENTERX|OPT_CENTERY,"The running test is displayed here.");
		font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp, 1);
                //t++;
                }
                
                BMoffsety = 220;
                BMoffsetx = 0;	
                
                ft_uint32_t disp =0,hr,minutes,sec ;
                ft_uint32_t temp = millis()-time;
                hr = (temp/3600000L)%12;
                minutes = (temp/60000L)%60;
                sec = (temp/1000L)%60;
                 
                       
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,28,(hr/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 10,BMoffsety,28,(hr%10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 22,BMoffsety,28,':'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 30,BMoffsety,28,(minutes/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 40,BMoffsety,28,(minutes%10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 52,BMoffsety,28,':'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx+ 60,BMoffsety,28,(sec/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 70,BMoffsety,28,(sec%10)+'0'));

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



/*  API to construct the display list of the lift application*/
ft_void_t FT_LiftApp_Portrait()
{
	/* Download the raw data of custom fonts into memory */
	ft_int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
	ft_int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16;
	ft_int32_t Baddr2,i,SzInc = 16,SzFlag = 0;
	ft_uint8_t fontr = 255,fontg = 255,fontb = 255;
	S_LiftAppRate_t *pLARate;
	S_LiftAppTrasParams_t *pLATParams;
	S_LiftAppBallsLinear_t S_LiftBallsArray[6*10],*pLiftBalls = NULL;//as of now 80 stars are been plotted
	Ft_App_GraphicsMatrix_t S_GPUMatrix;
	//Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
	float temptransx,temptransy,running_rate;
	S_LiftAppCtxt_t S_LACtxt;

 

	/* load the bitmap raw data */

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
	//S_LACtxt.CurrFloorNum = 87;//current floor number to start with
        S_LACtxt.CurrFloorNum = 80;//current floor number to start with
	//S_LACtxt.NextFloorNum = 80;//destination floor number as of now
        S_LACtxt.NextFloorNum = 77;//destination floor number as of now
	S_LACtxt.CurrArrowResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumResizeRate = pLATParams->ResizeRate;
	S_LACtxt.CurrFloorNumStagnantRate = 0;
	S_LACtxt.CurrFloorNumChangeRate = S_LACtxt.SLATransPrms.FloorNumChangeRate;
        S_LACtxt.AudioState = LIFTAPPAUDIOSTATE_NONE;
        S_LACtxt.AudioFileIdx = 0;
        S_LACtxt.AudioCurrAddr = 0;
        S_LACtxt.AudioCurrFileSz = 0;
        S_LACtxt.AudioFileNames[0] = '\0';
        S_LACtxt.AudioPlayFlag = 0;
        S_LACtxt.opt_orientation = 0;
        
       	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen

	Baddr2 = RAM_G + (20 *1024);
//        Ft_Gpu_Hal_ResetDLBuffer(phost);
        if(1 == g_reader.openfile("font.raw"))
        //Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Font is not displayed");
            Ft_App_LoadRawFromFile(g_reader,Baddr2);
        Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BHt*2, BHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width


	Baddr2 = ((Baddr2 + (ft_int32_t)BSt*BHt*11 + 15)&~15);
        if(1 == g_reader.openfile("arr.raw"))
  	    Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowHt*2, BArrowHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width

	Baddr2 = Baddr2 + 80*85;
        if(1 == g_reader.openfile("bs6.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background stars
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 60, 55));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));

	Baddr2 += 60*55;
        if(1 == g_reader.openfile("bs5.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));

	Baddr2 += 50*46;
        if(1 == g_reader.openfile("bs4.raw"))
	Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8,40,37));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER,40,37));

        Baddr2 += 40*37;
	if(1 == g_reader.openfile("bs3.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
        Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));

	Baddr2 += 30*27;
	if(1 == g_reader.openfile("bs2.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));

	Baddr2 += 20*18;
        if(1 == g_reader.openfile("bs1.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
        Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));

	Baddr2 += 10*10;
        if(1 == g_reader.openfile("logo.raw"))
	  Ft_App_LoadRawFromFile(g_reader,Baddr2);
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(Baddr2));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 129*2, 50));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 512, 512));//make sure whole bitmap is displayed after rotation - as height is greater than width
	Baddr2 += 129*2*50;

	// Read the font address from 0xFFFFC location 
	//FontTableAddress = Ft_Gpu_Hal_Rd32(phost,0xFFFFC);
	
	Ft_CmdBuffer_Index = 0;
	
	/* Download the commands into fifo */	
	//Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	//Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE(8));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L4,9,25));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(950172));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 25, 25));
	Baddr2 += 10*10;
	Baddr2 =  (ft_int32_t)((Baddr2 + 7)&~7);//audio engine's requirement
	
        ft_uint32_t time =0;
        time = millis();

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
       // Serial.begin(9600);
       // while(1);
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
		Ft_Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,480,0,0x132B3B);


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
                        Ft_App_GraphicsMatrix_t S_GPUMatrix;
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
			Ft_App_TrsMtrxLoadIdentity(&S_GPUMatrix);
			Ft_App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
			Ft_App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
			Ft_App_TrsMtrxRotate(&S_GPUMatrix,90);
			if(LIFTAPP_DIR_UP == S_LACtxt.ArrowDir)
			{
				Ft_App_TrsMtrxFlip(&S_GPUMatrix,FT_APP_GRAPHICS_FLIP_RIGHT);		
			}

			Ft_App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
                        {
                        Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
			Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
                        }
			FT_LiftAppComputeBitmap_Single(G_LiftAppFontArrayArrow,0,SzInc*16,BArrowWd,(FT_DispHeight/2));
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
                {
                  Ft_App_PostProcess_Transform_t S_GPUTrasMatrix;
		Ft_App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
                }
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0)); 
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
		FT_LiftAppComputeBitmapRowRotate(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,FT_DispWidth - BHt,(FT_DispHeight/2));
		
                

        	BMoffsetx = ((FT_DispWidth /2) + 210);
		BMoffsety = ((FT_DispHeight /2) + 130);
                Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
                
        	Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(28));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST,BORDER,BORDER,25,25));

		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,18*65536/2,25*65536/2);
		Ft_Gpu_CoCmd_Rotate(phost,-90*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,-25*65536/2,-18*65536/2);
		Ft_Gpu_CoCmd_SetMatrix(phost);

                ft_int16_t t;
                {
                ft_char8_t text_run_disp[] = "The running text is displayed here.";
                ft_int16_t stringlen_text_run_disp;
		stringlen_text_run_disp = strlen(text_run_disp);
		if(t < 200)
			t++;
		BMoffsety = linear(0,272,t,200);
                
		font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp, 0);
                }
                BMoffsetx = ((FT_DispWidth /2) + 190);
		BMoffsety = ((FT_DispHeight /2) + 105);	
                ft_uint32_t disp =0,hr,minutes,sec ;
                ft_uint32_t temp = millis()-time;
                hr = (temp/3600000L)%12;
                minutes = (temp/60000L)%60;
                sec = (temp/1000L)%60;
                                
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,8,(hr/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-10,8,(hr%10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-23,8,':'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-30,8,(minutes/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-40,8,(minutes%10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-53,8,':'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-60,8,(sec/10)+'0'));
                Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-70,8,(sec%10)+'0'));
		/*running_rate = *///linear((BMoffsety + 130),(BMoffsety +250),0,200);
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,129*65536,50*65536);
		Ft_Gpu_CoCmd_Rotate(phost,270*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,-80*65536,-50*65536);
		Ft_Gpu_CoCmd_SetMatrix(phost);

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(11));
		//Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-100*16,120*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(-80*16,150*16));
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


#ifdef MSVC_PLATFORM
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#ifdef ARDUINO_PLATFORM
ft_void_t setup()
#endif
{
  /* Local variables */
  ft_uint8_t chipid;

  Ft_Gpu_HalInit_t halinit;
  //Serial.begin(9600);
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

  /* Do a power cycle for safer side */
  Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);
  Ft_Gpu_Hal_Rd16(phost,RAM_G);
  /* Set the clk to external clock */
  Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);
  Ft_Gpu_Hal_Sleep(100);

  /* Switch PLL output to 48MHz */
  Ft_Gpu_HostCommand(phost,FT_GPU_PLL_48M);
  Ft_Gpu_Hal_Sleep(10);

  /* Do a core reset for safer side */
  Ft_Gpu_HostCommand(phost,FT_GPU_CORE_RESET);
  //Ft_Gpu_CoreReset(phost);

  /* Access address 0 to wake up the FT800 */
  Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);

  {
    //Read Register ID to check if FT800 is ready.
    chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
    while(chipid != 0x7C)
      chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
#ifdef MSVC_PLATFORM
    printf("VC1 register ID after wake up %x\n",chipid);
#endif
  }
//Calibration for WQVA

  /* Configuration of LCD display */
#if 0// SAMAPP_DISPLAY_QVGA
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
  Ft_Gpu_Hal_Wr8(phost, REG_PCLK,FT_DispPCLK);//after this display is visible on the LCD
  Ft_Gpu_Hal_Wr16(phost, REG_HSIZE, FT_DispWidth);
  Ft_Gpu_Hal_Wr16(phost, REG_VSIZE, FT_DispHeight);

  /* Initially fill both ping and pong buffer */
  Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
  Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x0ff);
  /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
  Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,1200);



  /*It is optional to clear the screen here*/
  Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_DL,FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
  Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
  
//  audio volume and enable is done
  Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0x002 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO));

  //Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen.
  
  



  pinMode(FT_SDCARD_CS,OUTPUT);
  digitalWrite(FT_SDCARD_CS,HIGH);
  delay(100);
  sd_present =  SD.begin(FT_SDCARD_CS);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  

  
  Info();
  
  if(sd_present)
  {
/* APP to demonstrate lift functionality */
#ifdef ORIENTATION_PORTRAIT

	FT_LiftApp_Portrait();

#endif
#ifdef ORIENTATION_LANDSCAPE

	FT_LiftApp_Landscape();

#endif
    
  }
  else
  {
    Ft_Gpu_CoCmd_Dlstart(phost);    
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
    Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(FT_DispWidth/12,FT_DispHeight/9));
    Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(FT_DispWidth-2*(FT_DispWidth/12),FT_DispHeight-2*(FT_DispHeight/9)));
    Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(26,98,161));
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Storage Device not Found");
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);	
    delay(5000);       
  }
  
 

#ifdef MSVC_PLATFORM
  return 0;
#endif
}

void loop()
{
}



/* Nothing beyond this */
