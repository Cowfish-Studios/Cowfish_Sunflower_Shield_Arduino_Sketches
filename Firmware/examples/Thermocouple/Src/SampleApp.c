/* Sample application to demonstrate FT800 primitives, widgets and customized screen shots */

#include "FT_Platform.h"
#include "SampleApp.h"

#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)

/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
ft_int16_t FT_DispWidth = 480;
ft_int16_t FT_DispHeight = 272;
ft_int16_t FT_DispHCycle = 548;
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

/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;
#ifdef FT900_PLATFORM
//TUARTBasicConfig     UART0Config;
//TSPIBasicConfig      SPIMConfig;

#endif

ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;

#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif
/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */
const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
    0,0,0,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
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
	        "Welcome to Sample Application ... \r\n"
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
}
#endif


#ifdef SAMAPP_ENABLE_APIS_SET0

/* Optimized implementation of sin and cos table - precision is 16 bit */
FT_PROGMEM ft_prog_uint16_t sintab[] = { 0, 402, 804, 1206, 1607, 2009, 2410,
		2811, 3211, 3611, 4011, 4409, 4807, 5205, 5601, 5997, 6392, 6786, 7179,
		7571, 7961, 8351, 8739, 9126, 9511, 9895, 10278, 10659, 11038, 11416,
		11792, 12166, 12539, 12909, 13278, 13645, 14009, 14372, 14732, 15090,
		15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868, 18204, 18537,
		18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705,
		22004, 22301, 22594, 22883, 23169, 23452, 23731, 24006, 24278, 24546,
		24811, 25072, 25329, 25582, 25831, 26077, 26318, 26556, 26789, 27019,
		27244, 27466, 27683, 27896, 28105, 28309, 28510, 28706, 28897, 29085,
		29268, 29446, 29621, 29790, 29955, 30116, 30272, 30424, 30571, 30713,
		30851, 30984, 31113, 31236, 31356, 31470, 31580, 31684, 31785, 31880,
		31970, 32056, 32137, 32213, 32284, 32350, 32412, 32468, 32520, 32567,
		32609, 32646, 32678, 32705, 32727, 32744, 32757, 32764, 32767, 32764 };

ft_int16_t SAMAPP_qsin(ft_uint16_t a)
{
  ft_uint8_t f;
  ft_int16_t s0,s1;

  if (a & 32768)
    return -SAMAPP_qsin(a & 32767);
  if (a & 16384)
      a = 32768 - a;
  f = a & 127;
  s0 = ft_pgm_read_word(sintab + (a >> 7));
  s1 = ft_pgm_read_word(sintab + (a >> 7) + 1);
  return (s0 + ((ft_int32_t)f * (s1 - s0) >> 7));
}

/* cos funtion */
ft_int16_t SAMAPP_qcos(ft_uint16_t a)
{
  return (SAMAPP_qsin(a + 16384));
}
#endif


/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
ft_void_t SAMAPP_GPU_DLSwap(ft_uint8_t DL_Swap_Type)
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




#ifdef SAMAPP_ENABLE_APIS_SET0
/*****************************************************************************/
/* Example code to display few points at various offsets with various colors */
/*****************************************************************************/
ft_void_t	SAMAPP_GPU_Points()
{
	ft_uint32_t *p_DLRAM = (ft_uint32_t *)RAM_DL;

	/* Construct DL of points */
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 0 , CLEAR_COLOR_RGB(128,128,128));
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 4 , CLEAR(1,1,1));
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 8 , COLOR_RGB(128, 0, 0) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 12, POINT_SIZE(5 * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 16, BEGIN(FTPOINTS) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 20, VERTEX2F((FT_DispWidth/5) * 16, (FT_DispHeight/2) * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 24, COLOR_RGB(0, 128, 0) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 28, POINT_SIZE(15 * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 32, VERTEX2F((FT_DispWidth*2/5) * 16, (FT_DispHeight/2) * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 36, COLOR_RGB(0, 0, 128) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 40, POINT_SIZE(25 * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 44, VERTEX2F((FT_DispWidth*3/5) * 16, (FT_DispHeight/2) * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 48, COLOR_RGB(128, 128, 0) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 52, POINT_SIZE(35 * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 56, VERTEX2F((FT_DispWidth*4/5) * 16, (FT_DispHeight/2) * 16) );
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 60, DISPLAY()); // display the image
	//delayms(100);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();
}

ft_void_t SAMAPP_GPU_Lines()
{
	ft_int16_t LineHeight = 25;

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(128, 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(5 * 16) );
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINES) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth/4) * 16,((FT_DispHeight - LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth/4) * 16,((FT_DispHeight + LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 128, 0) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(10 * 16) );
	LineHeight = 40;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*2/4) * 16,((FT_DispHeight - LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*2/4) * 16,((FT_DispHeight + LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(128, 128, 0) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(20 * 16) );
	LineHeight = 55;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*3/4) * 16, ((FT_DispHeight - LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*3/4) * 16, ((FT_DispHeight + LineHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
     SAMAPP_ENABLE_DELAY();
}
ft_void_t	SAMAPP_GPU_Rectangles()
{
	ft_int16_t RectWidth,RectHeight;


	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 0, 128) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(1 * 16) );//LINE_WIDTH is used for corner curvature
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(RECTS) );
	RectWidth = 5;RectHeight = 25;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth/4) - (RectWidth/2)) * 16,((FT_DispHeight - RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth/4) + (RectWidth/2)) * 16,((FT_DispHeight + RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 128, 0) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(5 * 16) );
	RectWidth = 10;RectHeight = 40;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth*2/4) - (RectWidth/2)) * 16,((FT_DispHeight - RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth*2/4) + (RectWidth/2)) * 16,((FT_DispHeight + RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(128, 128, 0) );
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(10 * 16) );
	RectWidth = 20;RectHeight = 55;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth*3/4) - (RectWidth/2)) * 16,((FT_DispHeight - RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F( ((FT_DispWidth*3/4) + (RectWidth/2)) * 16,((FT_DispHeight + RectHeight)/2) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

/* Display lena face at different locations with various colors, -ve offsets, alpha blend etc */
ft_void_t	SAMAPP_GPU_Bitmap()
{
	SAMAPP_Bitmap_header_t *p_bmhdr;
	ft_int16_t BMoffsetx,BMoffsety;

	p_bmhdr = (SAMAPP_Bitmap_header_t *)&SAMAPP_Bitmap_RawData_Header[0];
	/* Copy raw data into address 0 followed by generation of bitmap */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,&SAMAPP_Bitmap_RawData[p_bmhdr->Arrayoffset], p_bmhdr->Stride*p_bmhdr->Height);


	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(p_bmhdr->Format, p_bmhdr->Stride, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, p_bmhdr->Width, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
	BMoffsetx = ((FT_DispWidth/4) - (p_bmhdr->Width/2));
	BMoffsety = ((FT_DispHeight/2) - (p_bmhdr->Height/2));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(BMoffsetx, BMoffsety, 0, 0));
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255, 64, 64)); // red at (200, 120)
	BMoffsetx = ((FT_DispWidth*2/4) - (p_bmhdr->Width/2));
	BMoffsety = ((FT_DispHeight/2) - (p_bmhdr->Height/2));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(BMoffsetx, BMoffsety, 0, 0));
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(64, 180, 64)); // green at (216, 136)
	BMoffsetx += (p_bmhdr->Width/2);
	BMoffsety += (p_bmhdr->Height/2);
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(BMoffsetx, BMoffsety, 0, 0));
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255, 255, 64)); // transparent yellow at (232, 152)
	Ft_App_WrDlCmd_Buffer(phost,COLOR_A(150));
	BMoffsetx += (p_bmhdr->Width/2);
	BMoffsety += (p_bmhdr->Height/2);
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(BMoffsetx, BMoffsety, 0, 0));
	Ft_App_WrDlCmd_Buffer(phost,COLOR_A(255));
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2F(-10*16, -10*16));//for -ve coordinates use vertex2f instruction
	Ft_App_WrDlCmd_Buffer(phost,END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
/*Bitmap Palette sample*/
ft_void_t	SAMAPP_GPU_Bitmap_Palette_Background()
{
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	SAMAPP_Bitmap_header_t *p_bmhdr;
	ft_int16_t BMoffsetx,BMoffsety;

	ft_int32_t LutFileLen,FileLen,Bufflen;

	ft_uchar8_t *pbuff,*plutbuff;
	ft_int32_t  ft800_memaddr = RAM_G,pal_mem_addr;

	FILE *pFile = fopen("..\\..\\..\\Test\\background_paletted.raw","rb");

	FILE *pLutFile = fopen("..\\..\\..\\Test\\background_paletted_lut.raw","rb");

	fseek(pFile,0,SEEK_END);
	FileLen = ftell(pFile);
	fseek(pFile,0,SEEK_SET);

	pbuff = malloc(FileLen);

	fread(pbuff,1,FileLen,pFile);

	p_bmhdr = (SAMAPP_Bitmap_header_t *)&SAMAPP_Bitmap_RawData_Header[2];

	while (FileLen > 64*1024)
	{
	   Ft_Gpu_Hal_WrMemFromFlash(phost, ft800_memaddr,(ft_uchar8_t *)pbuff,  64*1024);
	   FileLen -= 64*1024;
	   ft800_memaddr += 64*1024;
	   pbuff += 64*1024;
	}
	Ft_Gpu_Hal_WrMemFromFlash(phost, ft800_memaddr,(ft_uchar8_t *)pbuff,  FileLen);
	ft800_memaddr += FileLen;//last chunk if any

    fseek(pLutFile,0,SEEK_END);
	LutFileLen = ftell(pLutFile);
	fseek(pLutFile,0,SEEK_SET);

	plutbuff = malloc(LutFileLen);

	fread(plutbuff,1,LutFileLen,pLutFile);
#ifndef FT_81X_ENABLE
	pal_mem_addr = RAM_PAL;
#else
	pal_mem_addr = ft800_memaddr;
#endif

	Ft_Gpu_Hal_WrMemFromFlash(phost, pal_mem_addr,(ft_uchar8_t *)plutbuff, LutFileLen);

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(p_bmhdr->Format, p_bmhdr->Stride, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, p_bmhdr->Width, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));

	Ft_App_WrDlCmd_Buffer(phost,END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();
#endif
}
ft_void_t	SAMAPP_GPU_Bitmap_Palette()
{
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	SAMAPP_Bitmap_header_t *p_bmhdr;
	ft_int16_t BMoffsetx,BMoffsety;

	ft_int32_t LutFileLen,FileLen,Bufflen,pal_mem_addr;

	ft_void_t *pbuff,*plutbuff;

	FILE *pFile = fopen("..\\..\\..\\Test\\lenaface40_palette.raw","rb");

	FILE *pLutFile = fopen("..\\..\\..\\Test\\lenaface40_palette_lut.raw","rb");

	fseek(pFile,0,SEEK_END);
	FileLen = ftell(pFile);
	fseek(pFile,0,SEEK_SET);

	pbuff = malloc(FileLen);

	fread(pbuff,1,FileLen,pFile);

	p_bmhdr = (SAMAPP_Bitmap_header_t *)&SAMAPP_Bitmap_RawData_Header[1];
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(ft_uchar8_t *)pbuff, p_bmhdr->Stride*p_bmhdr->Height);

    fseek(pLutFile,0,SEEK_END);
	LutFileLen = ftell(pLutFile);
	fseek(pLutFile,0,SEEK_SET);

	plutbuff = malloc(LutFileLen);

	fread(plutbuff,1,LutFileLen,pLutFile);
#ifndef FT_81X_ENABLE
	pal_mem_addr = RAM_PAL;
#else
	pal_mem_addr = RAM_G + p_bmhdr->Stride*p_bmhdr->Height;
#endif

	Ft_Gpu_Hal_WrMemFromFlash(phost, pal_mem_addr,(ft_uchar8_t *)plutbuff, LutFileLen);

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(p_bmhdr->Format, p_bmhdr->Stride, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, p_bmhdr->Width, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
	BMoffsetx = ((FT_DispWidth/4) - (p_bmhdr->Width/2));
	BMoffsety = ((FT_DispHeight/2) - (p_bmhdr->Height/2));
	Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(BMoffsetx, BMoffsety, 0, 0));

	Ft_App_WrDlCmd_Buffer(phost,END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();
#endif
}
#endif

#if defined(FT_81X_ENABLE)
#define SAMAPP_NUM_OF_ROM_FONTS 19
#else
#define SAMAPP_NUM_OF_ROM_FONTS 16
#endif

/* inbuilt font example for proportionate and non proportionate text - hello world */
ft_void_t	SAMAPP_GPU_Fonts()
{
	ft_int32_t i,j,hoffset,voffset,stringlen;
	ft_uint32_t FontTableAddress;
	const ft_uchar8_t Display_string[] = "Hello World";
	FT_Gpu_Fonts_t Display_fontstruct;

	hoffset = ((FT_DispWidth - 100) / 2);
	voffset = FT_DispHeight / 2;

	/* Read the font address from 0xFFFFC location */
	FontTableAddress = Ft_Gpu_Hal_Rd32(phost, ROMFONT_TABLEADDRESS);
#if defined (FT900_PLATFORM) || defined (MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	printf("ROM Font address read from 0x%x is %x \n", ROMFONT_TABLEADDRESS,FontTableAddress);
#endif
	stringlen = sizeof(Display_string) - 1;

	for (i = 0; i < SAMAPP_NUM_OF_ROM_FONTS; i++) {
		/* Read the font table from hardware */
		Ft_Gpu_Hal_RdMem(phost,(FontTableAddress + i*FT_GPU_FONT_TABLE_SIZE),(ft_uint8_t *)&Display_fontstruct,FT_GPU_FONT_TABLE_SIZE);

#if defined (FT900_PLATFORM) || defined (MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		printf(
				"Inbuilt font structure idx %d fmt %d ht %d wd %d strd %d Fontaddr %x \n",
				i + 16, Display_fontstruct.FontBitmapFormat,
				Display_fontstruct.FontHeightInPixels,
				Display_fontstruct.FontLineStride,
				Display_fontstruct.FontWidthInPixels,
				Display_fontstruct.PointerToFontGraphicsData);
#endif

#if defined(FT_81X_ENABLE)
		/* Display hello world by offsetting wrt char size */
		if ((i + 16) > 31) {
			Ft_Gpu_CoCmd_Dlstart(phost);

			//this is a co-processor command and it needs to get pushed to the display list first.
			Ft_Gpu_CoCmd_RomFont(phost, ((i + 16) % 32), (i + 16));

			/* Download the commands into FIFIO */
			Ft_App_Flush_Co_Buffer(phost);
			/* Wait till coprocessor completes the operation */
			Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

			//update the display list pointer for display list commands
			Ft_DlBuffer_Index = Ft_Gpu_Hal_Rd16(phost, REG_CMD_DL);
#if (defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)) && defined(BUFFER_OPTIMIZATION)
			Ft_Gpu_Hal_RdMem(phost, RAM_DL, Ft_DlBuffer, Ft_DlBuffer_Index);
#endif
		}
#endif

		Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 255, 255)); // clear screen

		/* Display string at the center of display */
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
		hoffset = ((FT_DispWidth - 160) / 2);
		voffset = ((FT_DispHeight - Display_fontstruct.FontHeightInPixels) / 2);

		//FT81X devices support larger rom fonts fonts in font handle 32, 33, 34
#if defined(FT_81X_ENABLE)
		/* Display hello world by offsetting wrt char size */
		{
			Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE((i + 16) % 32));
		}
#else
		/* Display hello world by offsetting wrt char size */
		Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE((i+16)));
#endif
		for (j = 0; j < stringlen; j++)
		{
			Ft_App_WrDlCmd_Buffer(phost, CELL(Display_string[j]));
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
			hoffset += Display_fontstruct.FontWidth[Display_string[j]];
		}
		Ft_App_WrDlCmd_Buffer(phost, END());
		Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

		/* Download the DL into DL RAM */
		Ft_App_Flush_DL_Buffer(phost);

		/* Do a swap */
		SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY_VALUE(1000);
	}
}
/* display text8x8 of abcdefgh */
ft_void_t SAMAPP_GPU_Text8x8() {
	/* Write the data into RAM_G */
	const ft_char8_t Text_Array[] = "abcdefgh";
	ft_int32_t String_size,hoffset = 16,voffset = 16;


        String_size = sizeof(Text_Array) - 1;
	Ft_Gpu_Hal_WrMem(phost,RAM_G,(ft_uint8_t *)Text_Array, String_size);

	/*
	      abcdefgh
	      abcdefgh
	*/

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(TEXT8X8, 1*8,1));//L1 format, each input data element is in 1 byte size
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, REPEAT, 8*8, 8*2));//output is 8x8 format - draw 8 characters in horizontal repeated in 2 line

	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	/* Display text 8x8 at hoffset, voffset location */
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));

	/*
           abcdabcdabcdabcd
           efghefghefghefgh
	*/
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(TEXT8X8, 1*4,2));//L1 format and each datatype is 1 byte size
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, BORDER, 8*16, 8*2));//each character is 8x8 in size -  so draw 32 characters in horizontal and 32 characters in vertical
	hoffset = FT_DispWidth/2;
	voffset = FT_DispHeight/2;
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
	Ft_App_WrDlCmd_Buffer(phost, END());

	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

/* display textVGA of random values */
ft_void_t	SAMAPP_GPU_TextVGA()
{
	/* Write the data into RAM_G */
	ft_uint16_t Text_Array[160];
	ft_int32_t String_size,hoffset = 32,voffset = 32,i;

	for(i=0;i<160;i++)
	{
		Text_Array[i] = ft_random(65536);//within range
	}

	String_size = 160*2;
	Ft_Gpu_Hal_WrMem(phost,RAM_G,(ft_uint8_t*)Text_Array, String_size);


	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));

	/* mandatory for textvga as background color is also one of the parameter in textvga format */
	Ft_App_WrDlCmd_Buffer(phost, BLEND_FUNC(ONE,ZERO));

	//draw 8x8
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(TEXTVGA, 2*4,8));//L1 format, but each input data element is of 2 bytes in size
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 8*8, 8*8));//output is 8x8 format - draw 8 characters in horizontal and 8 vertical
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
	Ft_App_WrDlCmd_Buffer(phost, END());

	//draw textvga
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(TEXTVGA, 2*16,8));//L1 format but each datatype is 16bit size
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, REPEAT, 8*32, 8*32));//8 pixels per character and 32 rows/colomns
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	hoffset = FT_DispWidth/2;
	voffset = FT_DispHeight/2;
	/* Display textvga at hoffset, voffset location */
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
	Ft_App_WrDlCmd_Buffer(phost, END());

	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}


ft_void_t	SAMAPP_GPU_Bargraph()
{
#define SAMAPP_BARGRAPH_ARRAY_SIZE (256)
	/* Write the data into RAM_G */
	ft_uint8_t Y_Array[SAMAPP_BARGRAPH_ARRAY_SIZE],NumSineWaves = 4, numchunks = 0;
	ft_int32_t String_size,hoffset = 0,voffset = 0,i, widthaligh,j;

	hoffset = 0;
	voffset = (FT_DispHeight - 256)/2;//centre of the screen

	widthaligh = ALIGN_TWO_POWER_N(FT_DispWidth,SAMAPP_BARGRAPH_ARRAY_SIZE);
	numchunks = widthaligh/SAMAPP_BARGRAPH_ARRAY_SIZE;
	String_size = SAMAPP_BARGRAPH_ARRAY_SIZE;
	for(j=0;j<numchunks;j++)
	{
		for(i=0;i<SAMAPP_BARGRAPH_ARRAY_SIZE;i++)
		{
			Y_Array[i] = ft_random(128) + 64;//within range
		}
		Ft_Gpu_Hal_WrMem(phost,RAM_G+j*SAMAPP_BARGRAPH_ARRAY_SIZE,(ft_uint8_t *)&Y_Array[0], String_size);
	}
	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(255,255,255));
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_LAYOUT(BARGRAPH, 256,1));
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(128, 0,0));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, 256, 256));
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE(0));
	/* Display text 8x8 at hoffset, voffset location */
	for(i = 0;i<numchunks;i++)
	{
		Ft_App_WrDlCmd_Buffer(phost, CELL(i));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
		hoffset += SAMAPP_BARGRAPH_ARRAY_SIZE;
	}

	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();

    /* drawing of sine wave with rising amplitude */
    String_size = SAMAPP_BARGRAPH_ARRAY_SIZE;
    for(j=0;j<numchunks;j++)
    {
		for(i=0;i<SAMAPP_BARGRAPH_ARRAY_SIZE;i++)
		{
			  ft_int32_t tmpval,tmpidx;
			  //tmpidx = (i + 256)&(512 - 1);
			  tmpidx = (i+j*SAMAPP_BARGRAPH_ARRAY_SIZE);
	//	  tmpval = 128 + ((ft_int32_t)(i/3)*SAMAPP_qsin(-65536*i/48)/65536);//within range
			  tmpval = 128 + ((ft_int32_t)(tmpidx/4)*SAMAPP_qsin(65536*tmpidx/48)/65536);//within range

			  Y_Array[i] = tmpval&0xff;
		}
		Ft_Gpu_Hal_WrMem(phost,RAM_G+j*SAMAPP_BARGRAPH_ARRAY_SIZE,Y_Array, String_size);
    }

    SAMAPP_ENABLE_DELAY();

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(BARGRAPH, 256,1));
	Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 256, 256));
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_HANDLE(0));

	/* Display bargraph at hoffset, voffset location */
	voffset = (FT_DispHeight - 256)/2;//centre of the screen
	hoffset = 0;
	for(i=0;i<numchunks;i++)
	{
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0xff, 0, 0));
		Ft_App_WrDlCmd_Buffer(phost, CELL(i));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));

		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 0, 0));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,(voffset + 4)*16));
		hoffset += SAMAPP_BARGRAPH_ARRAY_SIZE;
	}
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();

}

ft_void_t	SAMAPP_GPU_LineStrips()
{

	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(5, 45, 10) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 168, 64) );
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1 ,1 ,1) );
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINE_STRIP) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(16 * 16,16 * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*2/3) * 16,(FT_DispHeight*2/3) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth - 80) * 16,(FT_DispHeight - 20) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();

}

ft_void_t	SAMAPP_GPU_EdgeStrips()
{

	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(5, 45, 10) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 168, 64) );
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1 ,1 ,1) );
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(EDGE_STRIP_R) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(16 * 16,16 * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth*2/3) * 16,(FT_DispHeight*2/3) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((FT_DispWidth - 80) * 16,(FT_DispHeight - 20) * 16) );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();

}
ft_void_t	SAMAPP_GPU_Scissor()
{

	Ft_App_WrDlCmd_Buffer(phost,CLEAR(1,1,1)); // Clear to black
	Ft_App_WrDlCmd_Buffer(phost,SCISSOR_XY(40, 20)); // Scissor rectangle top left at (40, 20)
	Ft_App_WrDlCmd_Buffer(phost,SCISSOR_SIZE(40, 40)); // Scissor rectangle is 40 x 40 pixels
	Ft_App_WrDlCmd_Buffer(phost,CLEAR_COLOR_RGB(255, 255, 0)); // Clear to yellow
	Ft_App_WrDlCmd_Buffer(phost,CLEAR(1, 1, 1));
	Ft_App_WrDlCmd_Buffer(phost,DISPLAY());

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

ft_void_t	SAMAPP_GPU_Stencil()
{
	ft_int16_t PointSize = 50,DispBtwPoints = 60;

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1,1,1)); // Clear to black
	Ft_App_WrDlCmd_Buffer(phost, SCISSOR_XY(40, 20)); // Scissor rectangle top left at (40, 20)
	Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(INCR, INCR) );
	Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(PointSize * 16) );
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(FTPOINTS) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(((FT_DispWidth - DispBtwPoints)/2), (FT_DispHeight/2), 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(((FT_DispWidth + DispBtwPoints)/2), (FT_DispHeight/2), 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(EQUAL, 2, 255) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(128, 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/2), (FT_DispHeight/2), 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY());

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}
/*****************************************************************************/
/* Example code to demonstrated display of point and text                    */
/*****************************************************************************/
ft_void_t	SAMAPP_GPU_FtdiString()
{
	ft_int16_t hoffset,voffset,PointSz;

	voffset = ((FT_DispHeight - 49)/2);//49 is the max height of inbuilt font handle 31
	hoffset = ((FT_DispWidth - 4*36)/2);
	PointSz = 20;
	hoffset += PointSz;
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 0,  CLEAR(1, 1, 1)); // clear screen
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 4,  BEGIN(BITMAPS)); // start drawing bitmaps

	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 8,  VERTEX2II(hoffset, voffset, 31, 'F')); // ascii F in font 31
	hoffset += 24;
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 12, VERTEX2II(hoffset, voffset, 31, 'T')); // ascii T
	hoffset += 26;
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 16, VERTEX2II(hoffset, voffset, 31, 'D')); // ascii D
	hoffset += 29;
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 20, VERTEX2II(hoffset, voffset, 31, 'I')); // ascii I
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 24, END());
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 28, COLOR_RGB(160, 22, 22)); // change color to red
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 32, POINT_SIZE(PointSz * 16)); // set point size
	hoffset = ((FT_DispWidth - 4*36)/2);
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 36, BEGIN(FTPOINTS)); // start drawing points
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 40, VERTEX2II(hoffset, (FT_DispHeight/2), 0, 0)); // red point
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 44, END());
	Ft_Gpu_Hal_Wr32(phost, RAM_DL + 48, DISPLAY()); // display the image

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}
/* Call and return example - simple graph */
ft_void_t	SAMAPP_GPU_StreetMap()
{


	Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(236,232,224)); //light gray
	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1,1,1));
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(170,157,136));//medium gray
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(63));
	Ft_App_WrDlCmd_Buffer(phost, CALL(19));//draw the streets
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(250,250,250));//white
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(48));
	Ft_App_WrDlCmd_Buffer(phost, CALL(19));//draw the streets
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0,0,0));
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(240,91,27,77  ));//draw "Main st." at (240,91)
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(252,91,27,97	));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(260,91,27,105	));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(263,91,27,110	));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(275,91,27,115	));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(282,91,27,116	));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(286,91,27,46	));
	Ft_App_WrDlCmd_Buffer(phost, END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY());
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINES));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(-160,-20  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(320,4160  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(800,-20   ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(1280,4160 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(1920,-20  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(2400,4160 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(2560,-20  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(3040,4160 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(3200,-20  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(3680,4160 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(2880,-20  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(3360,4160 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(-20,0	   ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(5440,-480 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(-20,960   ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(5440,480  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(-20,1920  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(5440,1440 ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(-20,2880  ));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(5440,2400 ));
	Ft_App_WrDlCmd_Buffer(phost, END());
	Ft_App_WrDlCmd_Buffer(phost, RETURN());

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

/* usage of additive blending - draw 3 Gs*/
ft_void_t	SAMAPP_GPU_AdditiveBlendText()
{

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(50, 30, 31, 0x47) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_A( 128 ) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(58, 38, 31, 0x47) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_A( 64 ) );
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II(66, 46, 31, 0x47) );
	Ft_App_WrDlCmd_Buffer(phost, END() );
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

/* Usage of macro */
ft_void_t	SAMAPP_GPU_MacroUsage()
{
	ft_int32_t xoffset,yoffset,xflag = 1,yflag = 1,flagloop = 1;
	SAMAPP_Bitmap_header_t *p_bmhdr;

	xoffset = FT_DispWidth/3;
	yoffset = FT_DispHeight/2;

	/* First write a valid macro instruction into macro0 */
	Ft_Gpu_Hal_Wr32(phost, REG_MACRO_0,VERTEX2F(xoffset*16,yoffset*16));
	/* update lena face as bitmap 0 */

	p_bmhdr = &SAMAPP_Bitmap_RawData_Header[0];
	/* Copy raw data into address 0 followed by generation of bitmap */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,&SAMAPP_Bitmap_RawData[p_bmhdr->Arrayoffset], p_bmhdr->Stride*p_bmhdr->Height);


	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_SOURCE(RAM_G));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_LAYOUT(p_bmhdr->Format, p_bmhdr->Stride, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, p_bmhdr->Width, p_bmhdr->Height));
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(BITMAPS)); // start drawing bitmaps
	Ft_App_WrDlCmd_Buffer(phost, MACRO(0)); // draw the image at (100,120)
	Ft_App_WrDlCmd_Buffer(phost, END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        flagloop = 300;
	while(flagloop-- > 0)
	{
		if(((xoffset + p_bmhdr->Width) >= FT_DispWidth) || (xoffset <= 0))
		{
			xflag ^= 1;
		}
		if(((yoffset + p_bmhdr->Height) >= FT_DispHeight) || (yoffset <= 0))
		{
			yflag ^= 1;
		}
		if(xflag)
		{
			xoffset++;
		}
		else
		{
			xoffset--;
		}
		if(yflag)
		{
			yoffset++;
		}
		else
		{
			yoffset--;
		}
		/*  update just the macro */
		Ft_Gpu_Hal_Wr32(phost, REG_MACRO_0,VERTEX2F(xoffset*16,yoffset*16));
                Ft_Gpu_Hal_Sleep(10);
	}
}

/* Additive blending of points - 1000 points */
ft_void_t	SAMAPP_GPU_AdditiveBlendPoints()
{
	ft_int32_t i,hoffset,hoffsetdiff,voffset,flagloop=1,j,hdiff,vdiff,PointSz;

#define MSVC_PI (3.141592653)
	PointSz = FT_DispWidth/100;
    flagloop = 10;
    hoffsetdiff = FT_DispWidth/160;
	while(flagloop-- > 0)
	{
  		/* Download the DL into DL RAM */
		Ft_App_Flush_DL_Buffer(phost);

		Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(20, 91,20)); // green color for additive blending
		Ft_App_WrDlCmd_Buffer(phost, BLEND_FUNC(SRC_ALPHA,ONE));//input is source alpha and destination is whole color
		Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(PointSz*16));
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(FTPOINTS));


		/* First 100 random values */
		for(i=0;i<100;i++)
		{
			hoffset = ft_random(FT_DispWidth);
			voffset = ft_random(FT_DispHeight);
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
		}

		/* next 480 are sine values of two cycles */
		for(i=0;i<160;i++)
		{
			/* i is x offset, y is sinwave */
			hoffset = i*hoffsetdiff;

			//voffset = 136 + 136*sin((-(360*i/80)*MSVC_PI)/180);
//			voffset = (FT_DispHeight/2) + ((ft_int32_t)(FT_DispHeight/2)*SAMAPP_qsin(-65536*i/(FT_DispWidth/6))/65536);
                        voffset = (FT_DispHeight/2) + ((ft_int32_t)(FT_DispHeight/2)*SAMAPP_qsin(65536*i/(FT_DispWidth/6))/65536);

			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(hoffset*16,voffset*16));
			//randomvertexins[i*5] = VERTEX2F(hoffset*16,voffset*16);
			for(j=0;j<4;j++)
			{
				hdiff = ft_random(PointSz * 6) - (PointSz*3);
				vdiff = ft_random(PointSz * 6) - (PointSz*3);
				Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((hoffset + hdiff)*16,(voffset + vdiff)*16));
			}
		}

		Ft_App_WrDlCmd_Buffer(phost, END());
		Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

		/* Download the DL into DL RAM */
		Ft_App_Flush_DL_Buffer(phost);

		/* Do a swap */
		SAMAPP_GPU_DLSwap(DLSWAP_FRAME);

		Ft_Gpu_Hal_Sleep(10);

	}
}
#endif

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
#if defined(FT_801_ENABLE) || defined(FT_811_ENABLE) || defined(FT_813_ENABLE)
		Ft_Gpu_Hal_Wr8(phost, REG_CTOUCH_EXTENDED, CTOUCH_MODE_COMPATIBILITY);
#endif
		Ft_Gpu_CoCmd_Calibrate(phost, 0);
		/* Download the commands into FIFIO */
		Ft_App_Flush_Co_Buffer(phost);
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		/* Print the configured values */
		Ft_Gpu_Hal_RdMem(phost,	REG_TOUCH_TRANSFORM_A, (ft_uint8_t *) TransMatrix,	4 * 6); //read all the 6 coefficients
#ifdef MSVC_PLATFORM
	printf("Touch screen transform values are A 0x%x,B 0x%x,C 0x%x,D 0x%x,E 0x%x, F 0x%x",TransMatrix[0],TransMatrix[1],TransMatrix[2],TransMatrix[3],TransMatrix[4],TransMatrix[5]);
#endif
	}

}
#ifdef SAMAPP_ENABLE_APIS_SET4
/* API to demonstrate text widget */
ft_void_t SAMAPP_CoPro_Widget_Text()
{

	/*************************************************************************/
	/* Below code demonstrates the usage of text function. Text function     */
	/* draws text with either in built or externally configured text. Text   */
	/* color can be changed by colorrgb and text function supports display of*/
	/* texts on left, right, top, bottom and center respectively             */
	/*************************************************************************/
	{

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw text at 0,0 location */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0x00,0x80));
	Ft_Gpu_CoCmd_Text(phost,0, 0, 29, 0, "FTDI!");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,0, 40, 26, 0, "Text29 at 0,0");//text info
	/* text with centerx */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x00,0x00));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 50, 29, OPT_CENTERX, "FTDI!");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,((FT_DispWidth/2) - 30), 90, 26, 0, "Text29 centerX");//text info
	/* text with centery */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x41,0x01,0x05));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/5), 120, 29, OPT_CENTERY, "FTDI!");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/5), 140, 26, 0, "Text29 centerY");//text info
	/* text with center */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x0b,0x07,0x21));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 180, 29, OPT_CENTER, "FTDI!");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,((FT_DispWidth/2) - 50), 200, 26, 0, "Text29 center");//text info
	/* text with rightx */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x57,0x5e,0x1b));
	Ft_Gpu_CoCmd_Text(phost,FT_DispWidth, 60, 29, OPT_RIGHTX, "FTDI!");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth - 90), 100, 26, 0, "Text29 rightX");//text info
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
#endif

ft_void_t SAMAPP_API_Screen(ft_char8_t *str)
{

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x80,0x00));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), FT_DispHeight/2, 31, OPT_CENTERX, str);

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Ft_Gpu_Hal_Sleep(2000);
}

static ft_void_t SAMAPP_playmutesound()
{
	Ft_Gpu_Hal_Wr16(phost,REG_SOUND,0x0060);
    Ft_Gpu_Hal_Wr8(phost,REG_PLAY,0x01);
}
#ifdef SAMAPP_ENABLE_APIS_SET1
ft_void_t SAMAPP_PowerMode()
{
	/*************************************************
	Senario1:  Transition from Active mode to Standby mode.
	           Transition from Standby mode to Active Mode
	**************************************************/
	//Switch FT800 from Active to Standby mode
	SAMAPP_fadeout();
	SAMAPP_playmutesound();//Play mute sound to avoid pop sound

	/* Power commands should be executred in SPI single channel mode */
#ifdef FT900_PLATFORM

#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#if (defined(ENABLE_SPI_QUAD) || defined(ENABLE_SPI_DUAL))
	/* Set EVE in single channel mode */
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);

	/* Set FT900 to single channel mode */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
	spi_option(SPIM,spi_option_bus_width,1);

#endif

#endif
#endif

    #if defined (MSVC_PLATFORM_SPI) && (MSVC_PLATFORM_SPI_LIBFT4222 == 1)
        /* All host commands of FT81x (except system reset) should be executed
         * in Single channel mode    
         */
        if (phost->spichannel == SPI_IO_QUAD || phost->spichannel == SPI_IO_DUAL)
            Ft_Gpu_Hal_SetSPI(phost, FT_GPU_SPI_SINGLE_CHANNEL, FT_GPU_SPI_ONEDUMMY);
    #endif
	Ft_Gpu_PowerModeSwitch(phost,FT_GPU_STANDBY_M);

	//Wake up from Standby first before accessing FT800 registers.
	Ft_Gpu_PowerModeSwitch(phost,FT_GPU_ACTIVE_M);
        SAMAPP_API_Screen("Power Scenario 1");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);

	/*************************************************
	Senario2:  Transition from Active mode to Sleep mode.
	           Transition from Sleep mode to Active Mode
	**************************************************/
    //Switch FT800 from Active to Sleep mode
    SAMAPP_fadeout();
	SAMAPP_playmutesound();//Play mute sound to avoid pop sound
    Ft_Gpu_PowerModeSwitch(phost,FT_GPU_SLEEP_M);
    Ft_Gpu_Hal_Sleep(50);
	//Wake up from Sleep
	Ft_Gpu_PowerModeSwitch(phost,FT_GPU_ACTIVE_M);
        Ft_Gpu_Hal_Sleep(50);
        SAMAPP_API_Screen("Power Scenario 2");
        SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);

	/*************************************************
	Senario3:  Transition from Active mode to PowerDown mode.
	           Transition from PowerDown mode to Active Mode via Standby mode.
	**************************************************/
    //Switch FT800 from Active to PowerDown mode by sending command
    SAMAPP_fadeout();
	SAMAPP_playmutesound();//Play mute sound to avoid pop sound

	/* Power commands should be executred in SPI single channel mode */
#ifdef FT900_PLATFORM

#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#if (defined(ENABLE_SPI_QUAD) || defined(ENABLE_SPI_DUAL))
	/* Set EVE in single channel mode */
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);

	/* Set FT900 to single channel mode */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
	spi_option(SPIM,spi_option_bus_width,1);

#endif

#endif
#endif

    Ft_Gpu_PowerModeSwitch(phost,FT_GPU_POWERDOWN_M);
	Ft_Gpu_Hal_Sleep(50);
    SAMAPP_BootupConfig();
	//Need download display list again because power down mode lost all registers and memory
	SAMAPP_API_Screen("Power Scenario 3");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);

	/*************************************************
	Senario4:  Transition from Active mode to PowerDown mode by toggling PDN from high to low.
	           Transition from PowerDown mode to Active Mode via Standby mode.
	**************************************************/
    //Switch FT800 from Active to PowerDown mode by toggling PDN
    SAMAPP_fadeout();
	SAMAPP_playmutesound();//Play mute sound to avoid pop sound
	/* Power commands should be executred in SPI single channel mode */
#ifdef FT900_PLATFORM

#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#if (defined(ENABLE_SPI_QUAD) || defined(ENABLE_SPI_DUAL))
	/* Set EVE in single channel mode */
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);

	/* Set FT900 to single channel mode */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
	spi_option(SPIM,spi_option_bus_width,1);

#endif

#endif
#endif

    /* Toggling PD_N pin power cycles FT81X and all register contents are lost and reset 
       to default when the chip is next switched on. This puts FT81X SPI to single mode 
       leaving FT4222 in multi channel if ENABLE_SPI_QUAD/ENABLE_SPI_DUAL enabled.
       Hence explicilty switching the communication to single before power cycle.
    */
    #if defined (MSVC_PLATFORM_SPI) && (MSVC_PLATFORM_SPI_LIBFT4222 == 1) 
        if (phost->spichannel == SPI_IO_QUAD || phost->spichannel == SPI_IO_DUAL)
            Ft_Gpu_Hal_SetSPI(phost, FT_GPU_SPI_SINGLE_CHANNEL, FT_GPU_SPI_ONEDUMMY);
    #endif
    Ft_Gpu_Hal_Powercycle(phost,FT_FALSE);
    SAMAPP_BootupConfig();
	//Need download display list again because power down mode lost all registers and memory
	SAMAPP_API_Screen("Power Scenario 4");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);


	/*************************************************
	Senario5:  Transition from Active mode to PowerDown mode via Standby mode.
	           Transition from PowerDown mode to Active mode via Standby mode.
	**************************************************/
    //Switch FT800 from Active to standby mode
    SAMAPP_fadeout();
	SAMAPP_playmutesound();//Play mute sound to avoid pop sound

#ifdef FT900_PLATFORM
#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#if (defined(ENABLE_SPI_QUAD) || defined(ENABLE_SPI_DUAL))
	/* Set EVE in single channel mode */
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);

	/* Set FT900 to single channel mode */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
	spi_option(SPIM,spi_option_bus_width,1);

#endif

#endif
#endif

    #if defined (MSVC_PLATFORM_SPI) && (MSVC_PLATFORM_SPI_LIBFT4222 == 1)
        /* All host commands of FT81x (except system reset) should be executed
        * in Single channel mode
        */
        if (phost->spichannel == SPI_IO_QUAD || phost->spichannel == SPI_IO_DUAL)
            Ft_Gpu_Hal_SetSPI(phost, FT_GPU_SPI_SINGLE_CHANNEL, FT_GPU_SPI_ONEDUMMY);
    #endif
	Ft_Gpu_PowerModeSwitch(phost,FT_GPU_STANDBY_M);
	Ft_Gpu_Hal_Powercycle(phost,FT_FALSE);

    SAMAPP_BootupConfig();
	//Need download display list again because power down mode lost all registers and memory
	SAMAPP_API_Screen("Power Scenario 5");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);


	/*************************************************
	Senario6:  Transition from Active mode to PowerDown mode via Sleep mode.
	           Transition from PowerDown mode to Active mode via Standby mode.
	**************************************************/
    //Switch FT800 from Active to standby mode
    SAMAPP_fadeout();
    SAMAPP_playmutesound();//Play mute sound to avoid pop sound

#ifdef FT900_PLATFORM
#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#if (defined(ENABLE_SPI_QUAD) || defined(ENABLE_SPI_DUAL))
	/* Set EVE in single channel mode */
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);

	/* Set FT900 to single channel mode */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
	spi_option(SPIM,spi_option_bus_width,1);

#endif

#endif
#endif
    #if defined (MSVC_PLATFORM_SPI) && (MSVC_PLATFORM_SPI_LIBFT4222 == 1)
        /* All host commands of FT81x (except system reset) should be executed
        * in Single channel mode
        */
        if (phost->spichannel == SPI_IO_QUAD || phost->spichannel == SPI_IO_DUAL)
            Ft_Gpu_Hal_SetSPI(phost, FT_GPU_SPI_SINGLE_CHANNEL, FT_GPU_SPI_ONEDUMMY);
    #endif
	Ft_Gpu_PowerModeSwitch(phost,FT_GPU_SLEEP_M);
	Ft_Gpu_Hal_Powercycle(phost,FT_FALSE);//go to powerdown mode

    SAMAPP_BootupConfig();
	//Need download display list again because power down mode lost all registers and memory
	SAMAPP_API_Screen("Power Scenario 6");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);
}
#endif
#ifdef SAMAPP_ENABLE_APIS_SET4
/* API to demonstrate number widget */
ft_void_t SAMAPP_CoPro_Widget_Number()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of number function. Number function */
	/* draws text with only 32bit decimal number, signed or unsigned can also*/
	/* be specified as input parameter. Options like centerx, centery, center*/
	/* and rightx are supported                                              */
	/*************************************************************************/
	{


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw number at 0,0 location */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0x00,0x80));
	Ft_Gpu_CoCmd_Number(phost,0, 0, 29, 0, 1234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,0, 40, 26, 0, "Number29 at 0,0");//text info
	/* number with centerx */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x00,0x00));
	Ft_Gpu_CoCmd_Number(phost,(FT_DispWidth/2), 50, 29, OPT_CENTERX | OPT_SIGNED, -1234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,((FT_DispWidth/2) - 30), 90, 26, 0, "Number29 centerX");//text info
	/* number with centery */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x41,0x01,0x05));
	Ft_Gpu_CoCmd_Number(phost,(FT_DispWidth/5), 120, 29, OPT_CENTERY, 1234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/5), 140, 26, 0, "Number29 centerY");//text info
	/* number with center */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x0b,0x07,0x21));
	Ft_Gpu_CoCmd_Number(phost,(FT_DispWidth/2), 180, 29, OPT_CENTER | OPT_SIGNED, -1234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,((FT_DispWidth/2) - 50), 200, 26, 0, "Number29 center");//text info
	/* number with rightx */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x57,0x5e,0x1b));
	Ft_Gpu_CoCmd_Number(phost,FT_DispWidth, 60, 29, OPT_RIGHTX, 1234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth - 100), 100, 26, 0, "Number29 rightX");//text info

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
/* Main entry point */
/* API to demonstrate button functionality */
ft_void_t SAMAPP_CoPro_Widget_Button()
{

	/*************************************************************************/
	/* Below code demonstrates the usage of button function. Buttons can be  */
	/* constructed using flat or 3d effect. Button color can be changed by   */
	/* fgcolor command and text color is set by COLOR_RGB graphics command   */
	/*************************************************************************/
	{
	ft_int16_t xOffset,yOffset,bWidth,bHeight,bDistx,bDisty;

	bWidth = 60;
	bHeight = 30;
	bDistx = bWidth + ((FT_DispWidth - 4 * bWidth)/5);
	bDisty = bHeight + 5;
	xOffset = 10;
	yOffset = (FT_DispHeight/2 - 2*bDisty);
	/************ Construct a buttons with "ONE/TWO/THREE" text and default background *********************/
	/* Draw buttons 60x30 resolution at 10x40,10x75,10x110 */

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	/* flat effect and default color background */
	Ft_Gpu_CoCmd_FgColor(phost,0x0000ff);
	yOffset = (FT_DispHeight/2 - 2*bDisty);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,OPT_FLAT,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,OPT_FLAT,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,OPT_FLAT,"ABC");
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + 40), 26, 0, "Flat effect");//text info
	/* 3D effect */
	xOffset += bDistx;
	yOffset = (FT_DispHeight/2 - 2*bDisty);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + 40), 26, 0, "3D Effect");//text info
	/* 3d effect with background color */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	xOffset += bDistx;
	yOffset = (FT_DispHeight/2 - 2*bDisty);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_FgColor(phost,0x00ffff);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_FgColor(phost,0xff00ff);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + 40), 26, 0, "3D Color");//text info
	/* 3d effect with gradient color */
	Ft_Gpu_CoCmd_FgColor(phost,0x101010);
	Ft_Gpu_CoCmd_GradColor(phost,0xff0000);
	xOffset += bDistx;
	yOffset = (FT_DispHeight/2 - 2*bDisty);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_GradColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	yOffset += bDisty;
	Ft_Gpu_CoCmd_GradColor(phost,0x0000ff);
	Ft_Gpu_CoCmd_Button(phost,xOffset,yOffset,bWidth,bHeight,28,0,"ABC");
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + 40), 26, 0, "3D Gradient");//text info
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();

	}
}
#endif
#ifdef SAMAPP_ENABLE_APIS_SET1
ft_void_t SAMAPP_CoPro_Logo()
{
	Ft_Gpu_CoCmd_Logo(phost);
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);
	/* Wait till Logo has finished before the next display list construction */
	Ft_Gpu_Hal_WaitLogo_Finish(phost);
	printf("Logo animation has finished playing.\n");

	SAMAPP_ENABLE_DELAY();
}
#endif
#ifdef SAMAPP_ENABLE_APIS_SET4
/* API to demonstrate the use of transfer commands */
ft_void_t SAMAPP_CoPro_AppendCmds()
{
	ft_uint32_t AppendCmds[16];
	ft_int16_t xoffset,yoffset;
	/*************************************************************************/
	/* Below code demonstrates the usage of coprocessor append command - to append any*/
	/* mcu specific graphics commands to coprocessor generated graphics commands      */
	/*************************************************************************/

	/* Bitmap construction by MCU - display lena at 200x60 offset */
	/* Construct the bitmap data to be copied in the temp buffer then by using
	   coprocessor append command copy it into graphics processor DL memory */
	xoffset = ((FT_DispWidth - SAMAPP_Bitmap_RawData_Header[0].Width)/2);
	yoffset = ((FT_DispHeight/3) - SAMAPP_Bitmap_RawData_Header[0].Height/2);

	Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
	AppendCmds[0] = CLEAR_COLOR_RGB(255,0,0);
	AppendCmds[1] = CLEAR(1,1,1);
	AppendCmds[2] = COLOR_RGB(255,255,255);
	AppendCmds[3] = BEGIN(BITMAPS);
	AppendCmds[4] = BITMAP_SOURCE(0);
	AppendCmds[5] = BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,
		SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height);
	AppendCmds[6] = BITMAP_SIZE(BILINEAR,BORDER,BORDER,
		SAMAPP_Bitmap_RawData_Header[0].Width,SAMAPP_Bitmap_RawData_Header[0].Height);
	AppendCmds[7] = VERTEX2F(xoffset * 16,yoffset * 16);
	AppendCmds[8] = END();


        /* Download the bitmap data*/
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(const ft_prog_uchar8_t *)&SAMAPP_Bitmap_RawData[SAMAPP_Bitmap_RawData_Header[0].Arrayoffset],
		SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height);

	/* Download the DL data constructed by the MCU to location 40*40*2 in sram */
	Ft_Gpu_Hal_WrMem(phost,RAM_G + SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height,(ft_uint8_t *)AppendCmds,9*4);

	/* Call the append api for copying the above generated data into graphics processor
	   DL commands are stored at location 40*40*2 offset from the starting of the sram */
	Ft_Gpu_CoCmd_Append(phost,RAM_G + SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height, 9*4);
	/*  Display the text information */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	xoffset -=50;
	yoffset += 40;
	Ft_Gpu_CoCmd_Text(phost,xoffset, yoffset, 26, 0, "Display bitmap by Append");
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);
        SAMAPP_ENABLE_DELAY();
}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET2
/* API to demonstrate the usage of inflate command - compression done via zlib */
ft_void_t SAMAPP_CoPro_Inflate()
{
	const SAMAPP_Bitmap_header_t *pBitmapHdr = NULL;
	ft_uint8_t *pbuff = NULL;
	ft_int16_t xoffset,yoffset;
	/*************************************************************************/
	/* Below code demonstrates the usage of inflate functiona                */
	/* Download the deflated data into command buffer and in turn coprocessor infaltes*/
	/* the deflated data and outputs at 0 location                           */
	/*************************************************************************/
	{
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile = fopen("..\\..\\..\\Test\\lenaface40.bin","rb");//TBD - make platform specific
#endif
	ft_int32_t FileLen = 0;
	pBitmapHdr = &SAMAPP_Bitmap_RawData_Header[0];

	xoffset = ((FT_DispWidth - SAMAPP_Bitmap_RawData_Header[0].Width)/2);
	yoffset = ((FT_DispHeight - SAMAPP_Bitmap_RawData_Header[0].Height)/2);

	/* Clear the memory at location 0 - any previous bitmap data */

	Ft_App_WrCoCmd_Buffer(phost, CMD_MEMSET);
	Ft_App_WrCoCmd_Buffer(phost, 0L);//starting address of memset
	Ft_App_WrCoCmd_Buffer(phost, 255L);//value of memset
	Ft_App_WrCoCmd_Buffer(phost, 1L*pBitmapHdr->Stride*pBitmapHdr->Height);//number of elements to be changed

	/* Set the display list for graphics processor */
	/* Bitmap construction by MCU - display lena at 200x90 offset */
	/* Transfer the data into coprocessor memory directly word by word */
	Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0,0,255));
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,
		SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR,BORDER,BORDER,
		SAMAPP_Bitmap_RawData_Header[0].Width,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(xoffset*16,yoffset*16));
	Ft_App_WrCoCmd_Buffer(phost, END());

	/*  Display the text information */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	xoffset -= 50;
	yoffset += 40;
	Ft_Gpu_CoCmd_Text(phost,xoffset, yoffset, 26, 0, "Display bitmap by inflate");
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());

	Ft_Gpu_CoCmd_Swap(phost);
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);


	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)

	/* inflate the data read from binary file */
	if(NULL == pFile)
	{
		printf("Error in opening file %s \n","lenaface40.bin");
	}
	else
	{
#endif
		/* Inflate the data into location 0 */
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_INFLATE);
		Ft_Gpu_Hal_WrCmd32(phost,  0);//destination address if inflate
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
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
			/* copy data continuously into command memory */
			Ft_Gpu_Hal_WrCmdBuf(phost, pbuff, blocklen);//alignment is already taken care by this api
		}
		/* close the opened binary zlib file */
		fclose(pFile);
		free(pbuff);
	}
#endif
#if defined (ARDUINO_PLATFORM) || defined(FT900_PLATFORM)
      /* Copy the deflated/jpeg encoded data into coprocessor fifo */
      Ft_Gpu_Hal_WrCmdBufFromFlash(phost,Lenaface40,1L*SAMAPP_Lenaface40_SIZE);
#endif

	}
        SAMAPP_ENABLE_DELAY();
}
/* API to demonstrate jpeg decode functionality */
ft_void_t SAMAPP_CoPro_Loadimage()
{
	ft_uint8_t *pbuff;
	const SAMAPP_Bitmap_header_t *pBitmapHdr = NULL;
	ft_int16_t ImgW,ImgH,xoffset,yoffset;
	/*************************************************************************/
	/* Below code demonstrates the usage of loadimage function               */
	/* Download the jpg data into command buffer and in turn coprocessor decodes      */
	/* and dumps into location 0 with rgb565 format                          */
	/*************************************************************************/
	{
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile = fopen("..\\..\\..\\Test\\mandrill256.jpg","rb");
#endif
	ft_int32_t FileLen = 0;
	pBitmapHdr = &SAMAPP_Bitmap_RawData_Header[0];
	ImgW = ImgH = 256;

	xoffset = ((FT_DispWidth - ImgW)/2);
	yoffset = ((FT_DispHeight - ImgH)/2);
	/* Clear the memory at location 0 - any previous bitmap data */

	Ft_App_WrCoCmd_Buffer(phost, CMD_MEMSET);
	Ft_App_WrCoCmd_Buffer(phost, 0L);//starting address of memset
	Ft_App_WrCoCmd_Buffer(phost, 255L);//value of memset
	Ft_App_WrCoCmd_Buffer(phost, 256L*2*256);//number of elements to be changed

	/* Set the display list for graphics processor */

	/* Bitmap construction by MCU - display lena at 112x8 offset */
	/* Transfer the data into coprocessor memory directly word by word */
	Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0,255,255));
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565,ImgW*2,ImgH));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR,BORDER,BORDER,ImgW,ImgH));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(xoffset*16,yoffset*16));
	Ft_App_WrCoCmd_Buffer(phost, END());

	/*  Display the text information */
	xoffset = ((FT_DispWidth)/2);
	yoffset = ((FT_DispHeight)/2);
	Ft_Gpu_CoCmd_Text(phost,xoffset, yoffset, 26, OPT_CENTER, "Display bitmap by jpg decode");
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)

	/* decode the jpeg data */
	if(NULL == pFile)
	{
		printf("Error in opening file %s \n","mandrill256.jpg");
	}
	else
	{
#endif

		/******************* Decode jpg output into location 0 and output color format as RGB565 *********************/
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
		Ft_Gpu_Hal_WrCmd32(phost,  0);//destination address of jpg decode
		Ft_Gpu_Hal_WrCmd32(phost,  0);//output format of the bitmap
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
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
			/* copy data continuously into command memory */
			Ft_Gpu_Hal_WrCmdBuf(phost,pbuff, blocklen);//alignment is already taken care by this api
		}
#endif
#if defined (ARDUINO_PLATFORM) || defined(FT900_PLATFORM)
                /* Copy the deflated/jpeg encoded data into coprocessor fifo */
                Ft_Gpu_Hal_WrCmdBufFromFlash(phost,Mandrill256,SAMAPP_Mandrill256_SIZE);
#endif
                SAMAPP_ENABLE_DELAY();
                /******************** Decode jpg output into location 0 & output as MONOCHROME ******************************/
		/* Clear the memory at location 0 - any previous bitmap data */\
		xoffset = ((FT_DispWidth - ImgW)/2);
		yoffset = ((FT_DispHeight - ImgH)/2);


		Ft_App_WrCoCmd_Buffer(phost, CMD_MEMSET);
		Ft_App_WrCoCmd_Buffer(phost, 0L);//starting address of memset
		Ft_App_WrCoCmd_Buffer(phost, 255L);//value of memset
		Ft_App_WrCoCmd_Buffer(phost, 256L*2*256);//number of elements to be changed

		/* Set the display list for graphics processor */
		/* Bitmap construction by MCU - display lena at 112x8 offset */
		/* Transfer the data into coprocessor memory directly word by word */
		Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0,0,0));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L8,ImgW,ImgH));//monochrome
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR,BORDER,BORDER,ImgW,ImgH));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(xoffset*16,yoffset*16));
		Ft_App_WrCoCmd_Buffer(phost, END());

		/*  Display the text information */
		xoffset = ((FT_DispWidth)/2);
		yoffset = ((FT_DispHeight)/2);
		Ft_Gpu_CoCmd_Text(phost,xoffset, yoffset, 26, OPT_CENTER, "Display bitmap by jpg decode L8");
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);

		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
                SAMAPP_ENABLE_DELAY();

		Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
		Ft_Gpu_Hal_WrCmd32(phost,  0);//destination address of jpg decode
		Ft_Gpu_Hal_WrCmd32(phost,  OPT_MONO);//output format of the bitmap - default is rgb565
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		fseek(pFile,0,SEEK_END);
		FileLen = ftell(pFile);
		fseek(pFile,0,SEEK_SET);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */
			fread(pbuff,1,blocklen,pFile);
			FileLen -= blocklen;
			/* copy data continuously into command memory */
			Ft_Gpu_Hal_WrCmdBuf(phost,pbuff, blocklen);//alignment is already taken care by this api
		}
		free(pbuff);

		/* close the opened jpg file */
		fclose(pFile);
        }
#endif
#if defined (ARDUINO_PLATFORM) || defined (FT900_PLATFORM)
      /* Copy the deflated/jpeg encoded data into coprocessor fifo */
      Ft_Gpu_Hal_WrCmdBufFromFlash(phost,Mandrill256,SAMAPP_Mandrill256_SIZE);
#endif

	}
        SAMAPP_ENABLE_DELAY();

}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET1
/* API to demonstrate clock widget */
ft_void_t SAMAPP_CoPro_Widget_Clock()
{

	/*************************************************************************/
	/* Below code demonstrates the usage of clock function. Clocks can be    */
	/* constructed using flat or 3d effect. Clock background and foreground  */
	/* colors can be set by gbcolor and colorrgb. Clock can be constructed   */
	/* with multiple options such as no background, no needles, no pointer.  */
	/*************************************************************************/
	{
	ft_int16_t xOffset,yOffset,cRadius,xDistBtwClocks;

	xDistBtwClocks = FT_DispWidth/5;
	cRadius = xDistBtwClocks/2 - FT_DispWidth/64;

	/* Download the bitmap data for lena faced clock */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(ft_prog_uchar8_t *)&SAMAPP_Bitmap_RawData[SAMAPP_Bitmap_RawData_Header[0].Arrayoffset],
		SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height);

	/* Draw clock with blue as background and read as needle color */

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	/* flat effect and default color background */
	xOffset = xDistBtwClocks/2;
	yOffset = cRadius + 5;
	Ft_Gpu_CoCmd_BgColor(phost, 0x0000ff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_FLAT,30,100,5,10);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "Flat effect");//text info
	/* no seconds needle */
	Ft_Gpu_CoCmd_BgColor(phost, 0x00ff00);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_FgColor(phost,0xff0000);
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_NOSECS,10,10,5,10);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No Secs");//text info
	/* no background color */
	Ft_Gpu_CoCmd_BgColor(phost, 0x00ffff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0x00));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_NOBACK,10,10,5,10);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No BG");//text info
	/* No ticks */
	Ft_Gpu_CoCmd_BgColor(phost, 0xff00ff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xff,0xff));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_NOTICKS,10,10,5,10);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No Ticks");//text info
	/* No hands */
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xff,0x00));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_NOHANDS,10,10,5,10);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No Hands");//text info
	/* Bigger clock */
	yOffset += (cRadius + 10);
	cRadius = FT_DispHeight - (2*cRadius + 5 + 10);//calculate radius based on remaining height
	cRadius = (cRadius - 5 - 20)/2;
	xOffset = cRadius + 10;
	yOffset += cRadius + 5;
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0x00,0xff));
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,0,10,10,5,10);

	xOffset += 2 * cRadius + 10;
	/* Lena clock with no background and no ticks */
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(10*16));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset - cRadius + 10)*16,(yOffset - cRadius + 10)*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset + cRadius - 10)*16,(yOffset + cRadius - 10)*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset - cRadius + 12)*16,(yOffset - cRadius + 12)*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset + cRadius - 12)*16,(yOffset + cRadius - 12)*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
	/* Lena bitmap - scale proportionately wrt output resolution */
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Scale(phost, 65536*2*cRadius/SAMAPP_Bitmap_RawData_Header[0].Width,65536*2*cRadius/SAMAPP_Bitmap_RawData_Header[0].Height);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,
		SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,
		2*cRadius,2*cRadius));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset - cRadius)*16,(yOffset - cRadius)*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Clock(phost, xOffset,yOffset,cRadius,OPT_NOTICKS | OPT_NOBACK,10,10,5,10);
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}

}
/* API to demonstrate guage widget */
ft_void_t SAMAPP_CoPro_Widget_Guage()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of gauge function. Gauge can be     */
	/* constructed using flat or 3d effect. Gauge background and foreground  */
	/* colors can be set by gbcolor and colorrgb. Gauge can be constructed   */
	/* with multiple options such as no background, no minors/majors and     */
	/* no pointer.                                                           */
	/*************************************************************************/
	{
	ft_int16_t xOffset,yOffset,cRadius,xDistBtwClocks;

	xDistBtwClocks = FT_DispWidth/5;
	cRadius = xDistBtwClocks/2 - FT_DispWidth/64;

	/* Download the bitmap data */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(const ft_prog_uchar8_t *)&SAMAPP_Bitmap_RawData[SAMAPP_Bitmap_RawData_Header[0].Arrayoffset],
		SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height);

	/* Draw gauge with blue as background and read as needle color */

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	/* flat effect and default color background */
	xOffset = xDistBtwClocks/2;
	yOffset = cRadius + 5;
	Ft_Gpu_CoCmd_BgColor(phost, 0x0000ff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_FLAT,5,4,45,100);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "Flat effect");//text info
	/* 3d effect */
	Ft_Gpu_CoCmd_BgColor(phost, 0x00ff00);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_FgColor(phost,0xff0000);
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,0,5,1,60,100);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "3d effect");//text info
	/* no background color */
	Ft_Gpu_CoCmd_BgColor(phost, 0x00ffff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0x00));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOBACK,1,6,90,100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No BG");//text info
	/* No ticks */
	Ft_Gpu_CoCmd_BgColor(phost, 0xff00ff);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xff,0xff));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOTICKS,5,4,20,100);
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No Ticks");//text info
	/* No hands */
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xff,0x00));
	xOffset += xDistBtwClocks;
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOHANDS,5,4,55,100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,xOffset, (yOffset + cRadius + 6), 26, OPT_CENTER, "No Hands");//text info
	/* Bigger gauge */
	yOffset += cRadius + 10;
	cRadius = FT_DispHeight - (2*cRadius + 5 + 10);//calculate radius based on remaining height
	cRadius = (cRadius - 5 - 20)/2;
	xOffset = cRadius + 10;
	yOffset += cRadius + 5;
	Ft_Gpu_CoCmd_BgColor(phost, 0x808000);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOPOINTER,5,4,80,100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOTICKS | OPT_NOBACK,5,4,30,100);

	xOffset += 2*cRadius + 10;
	/* Lena guage with no background and no ticks */
	Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(cRadius*16));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xOffset*16,yOffset*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((cRadius - 2)*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xOffset*16,yOffset*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
	/* Lena bitmap - scale proportionately wrt output resolution */
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Scale(phost, 65536*2*cRadius/SAMAPP_Bitmap_RawData_Header[0].Width,65536*2*cRadius/SAMAPP_Bitmap_RawData_Header[0].Height);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,
		SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,
		2*cRadius,2*cRadius));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((xOffset - cRadius)*16,(yOffset - cRadius)*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost );

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Gauge(phost, xOffset,yOffset,cRadius,OPT_NOTICKS | OPT_NOBACK,5,4,30,100);
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}

}
/* API to demonstrate gradient widget */
ft_void_t SAMAPP_CoPro_Widget_Gradient()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of gradient function. Gradient func */
	/* can be used to construct three effects - horizontal, vertical and     */
	/* diagonal effects.                                                      */
	/*************************************************************************/
	{
	ft_int16_t wScissor,hScissor,xOffset,yOffset;

	wScissor = ((FT_DispWidth - 4*10)/3);
	hScissor = ((FT_DispHeight - 3*20)/2);
	xOffset = 10;
	yOffset = 20;
	/* Draw gradient  */

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(wScissor,hScissor));
	/* Horizontal gradient effect */
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0x808080,(xOffset + wScissor),yOffset,0xffff00);
	/* Vertical gradient effect */
	xOffset += wScissor + 10;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0xff0000,xOffset,(yOffset + hScissor),0x00ff00);
	/* diagonal gradient effect */
	xOffset += wScissor + 10;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0x800000,(xOffset + wScissor),(yOffset + hScissor),0xffffff);
	/* Diagonal gradient with text info */
	xOffset = 10;
	yOffset += hScissor + 20;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(wScissor,30));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,(yOffset + hScissor/2 - 15)));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,(yOffset + hScissor/2 - 15),0x606060,(xOffset + wScissor),(yOffset + hScissor/2 + 15),0x404080);
	Ft_Gpu_CoCmd_Text(phost,(xOffset + wScissor/2), (yOffset + hScissor/2), 28, OPT_CENTER, "Heading 1");//text info

	/* Draw horizontal, vertical and diagonal with alpha */
	xOffset += wScissor + 10;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(wScissor,hScissor));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0x808080,(xOffset + wScissor),yOffset,0xffff00);
	wScissor -= 30; hScissor -= 30;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(wScissor,hScissor));
	xOffset += 15; yOffset += 15;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0x800000,xOffset,(yOffset + hScissor),0xffffff);
	wScissor -= 30; hScissor -= 30;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(wScissor,hScissor));
	xOffset += 15; yOffset += 15;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(xOffset,yOffset));//clip the display
	Ft_Gpu_CoCmd_Gradient(phost, xOffset,yOffset,0x606060,(xOffset + wScissor),(yOffset + hScissor),0x404080);

	/* Display the text wrt gradient */
	wScissor = ((FT_DispWidth - 4*10)/3);
	hScissor = ((FT_DispHeight - 3*20)/2);
	xOffset = 10 + wScissor/2;
	yOffset = 20 + hScissor + 5;
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));//set to default values
#ifdef FT_81X_ENABLE
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(2048,2048));
#else
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(512,512));
#endif
	Ft_Gpu_CoCmd_Text(phost,xOffset,yOffset, 26, OPT_CENTER, "Horizontal grad");//text info
	xOffset += wScissor + 10;
	Ft_Gpu_CoCmd_Text(phost,xOffset,yOffset, 26, OPT_CENTER, "Vertical grad");//text info
	xOffset += wScissor + 10;
	Ft_Gpu_CoCmd_Text(phost,xOffset,yOffset, 26, OPT_CENTER, "Diagonal grad");//text info

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}

ft_void_t SAMAPP_CoPro_Widget_Keys_Interactive()
{
       /*************************************************************************/
       /* Below code demonstrates the usage of keys function. keys function     */
       /* draws buttons with characters given as input parameters. Keys support */
       /* Flat and 3D effects, draw at (x,y) coordinates or center of the display*/
       /* , inbuilt or custom fonts can be used for key display                 */
       /*************************************************************************/
       {
              ft_int32_t loopflag = 600;
              ft_int16_t TextFont = 29,ButtonW = 30,ButtonH = 30,yBtnDst = 5,yOffset,xOffset;
#define SAMAPP_COPRO_WIDGET_KEYS_INTERACTIVE_TEXTSIZE (512)
              ft_char8_t DispText[SAMAPP_COPRO_WIDGET_KEYS_INTERACTIVE_TEXTSIZE],CurrChar = '|';
              ft_uint8_t CurrTag = 0,PrevTag = 0,Pendown = 1;
              ft_int32_t CurrTextIdx = 0;
#ifdef SAMAPP_DISPLAY_QVGA
              TextFont = 27;
              ButtonW = 22;
              ButtonH = 22;
              yBtnDst = 3;
#endif

#ifdef 	SAMAPP_DISPLAY_WVGA
		TextFont = 30;
		ButtonW = 38;
		ButtonH = 38;
		yBtnDst = 15;
#endif

       while(loopflag --)
       {


       /* Check the user input and then add the characters into array */
       CurrTag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
      // Pendown = ((Ft_Gpu_Hal_Rd32(phost,REG_TOUCH_DIRECT_XY)>>31) & 0x01);

       CurrChar = CurrTag;
       if(0 == CurrTag)
       {
              CurrChar = '|';
       }

       /* check whether pwndown has happened */
	   if(!CurrTag && PrevTag)
	   {
		   /* check whether pwndown has happened */
		   if(( 1 == Pendown) && (0 != PrevTag))
		   {
				  CurrTextIdx++;
				  /* clear all the charaters after 100 are pressed */
				  if(CurrTextIdx > 24)
				  {
						 CurrTextIdx = 0;
				  }
		   }
	   }

       Ft_Gpu_CoCmd_Dlstart(phost);
       Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
       Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
       Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
       /* Draw text entered by user */
       /* make sure the array is a string */
       DispText[CurrTextIdx] = CurrChar;
       DispText[CurrTextIdx + 1] = '\0';

       Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
       Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 40, TextFont, OPT_CENTER, DispText);//text info
       Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));


       //yOffset = 80 + 10;
       yOffset = ButtonW*2 + 10;
       /* Construct a simple keyboard - note that the tags associated with the keys are the character values given in the arguments */
       Ft_Gpu_CoCmd_FgColor(phost,0x404080);
       Ft_Gpu_CoCmd_GradColor(phost,0x00ff00);
       Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, (OPT_CENTER | CurrTag), "qwertyuiop");
       Ft_Gpu_CoCmd_GradColor(phost,0x00ffff);
       yOffset += ButtonH + yBtnDst;
       Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, (OPT_CENTER | CurrTag), "asdfghjkl");
       Ft_Gpu_CoCmd_GradColor(phost,0xffff00);
       yOffset += ButtonH + yBtnDst;
       Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, (OPT_CENTER | CurrTag), "zxcvbnm");//hilight button z
       yOffset += ButtonH + yBtnDst;
       Ft_App_WrCoCmd_Buffer(phost,TAG(' '));
       if(' ' == CurrTag)
       {
              Ft_Gpu_CoCmd_Button(phost,yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, OPT_CENTER | OPT_FLAT, " ");//mandatory to give '\0' at the end to make sure coprocessor understands the string end
       }
       else
       {
              Ft_Gpu_CoCmd_Button(phost,yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, OPT_CENTER, " ");//mandatory to give '\0' at the end to make sure coprocessor understands the string end
       }
       yOffset = ButtonW*2 + 10;
       Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, (0 | CurrTag), "789");
       yOffset += ButtonH + yBtnDst;
       Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, (0 | CurrTag), "456");
       yOffset += ButtonH + yBtnDst;
       Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, (0 | CurrTag), "123");
       yOffset += ButtonH + yBtnDst;
       Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
       Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, (0 | CurrTag), "0.");//hilight button 0
       Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
       Ft_Gpu_CoCmd_Swap(phost);

       /* Download the commands into fifo */
       Ft_App_Flush_Co_Buffer(phost);

       /* Wait till coprocessor completes the operation */
       Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
       Ft_Gpu_Hal_Sleep(10);
       PrevTag = CurrTag;
       }
       }
}

/* API to demonstrate keys widget */
ft_void_t SAMAPP_CoPro_Widget_Keys()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of keys function. keys function     */
	/* draws buttons with characters given as input parameters. Keys support */
	/* Flat and 3D effects, draw at (x,y) coordinates or center of the display*/
	/* , inbuilt or custom fonts can be used for key display                 */
	/*************************************************************************/
	{
		ft_int16_t TextFont = 29,ButtonW = 30,ButtonH = 30,yBtnDst = 5,yOffset,xOffset;
#ifdef SAMAPP_DISPLAY_QVGA
		TextFont = 27;
		ButtonW = 22;
		ButtonH = 22;
		yBtnDst = 3;
#endif

#ifdef 	SAMAPP_DISPLAY_WVGA
		TextFont = 30;
		ButtonW = 38;
		ButtonH = 38;
		yBtnDst = 15;
#endif
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
	/* Draw keys with flat effect */
	yOffset = ButtonW + 10 + yBtnDst;
	Ft_Gpu_CoCmd_FgColor(phost,0x404080);
	Ft_Gpu_CoCmd_Keys(phost, 10, 10, 4*ButtonW, 30, TextFont, OPT_FLAT, "ABCD");
	Ft_Gpu_CoCmd_Text(phost,10, yOffset, 26, 0, "Flat effect");//text info
	/* Draw keys with 3d effect */
	Ft_Gpu_CoCmd_FgColor(phost,0x800000);
	xOffset = 4*ButtonW + 20;
	Ft_Gpu_CoCmd_Keys(phost, xOffset, 10, 4*ButtonW, 30, TextFont, 0, "ABCD");
	Ft_Gpu_CoCmd_Text(phost,xOffset, yOffset, 26, 0, "3D effect");//text info
	/* Draw keys with center option */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff000);
	xOffset += 4*ButtonW + 20;
	Ft_Gpu_CoCmd_Keys(phost, xOffset, 10, (FT_DispWidth - 230), 30, TextFont, OPT_CENTER, "ABCD");
	Ft_Gpu_CoCmd_Text(phost,xOffset, yOffset, 26, 0, "Option Center");//text info

	yOffset = ButtonW*2 + 10;
	/* Construct a simple keyboard - note that the tags associated with the keys are the character values given in the arguments */
	Ft_Gpu_CoCmd_FgColor(phost,0x404080);
	Ft_Gpu_CoCmd_GradColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, OPT_CENTER, "qwertyuiop");
	Ft_Gpu_CoCmd_GradColor(phost,0x00ffff);
	yOffset += ButtonH + yBtnDst;
	Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, OPT_CENTER, "asdfghjkl");
	Ft_Gpu_CoCmd_GradColor(phost,0xffff00);
	yOffset += ButtonH + yBtnDst;
	Ft_Gpu_CoCmd_Keys(phost, yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, (OPT_CENTER | 'z'), "zxcvbnm");//hilight button z
	yOffset += ButtonH + yBtnDst;
	Ft_Gpu_CoCmd_Button(phost,yBtnDst, yOffset, 10*ButtonW, ButtonH, TextFont, OPT_CENTER, " ");//mandatory to give '\0' at the end to make sure coprocessor understands the string end
	yOffset = 80 + 10;
	Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, 0, "789");
	yOffset += ButtonH + yBtnDst;
	Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, 0, "456");
	yOffset += ButtonH + yBtnDst;
	Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, 0, "123");
	yOffset += ButtonH + yBtnDst;
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Keys(phost, 11*ButtonW, yOffset, 3*ButtonW, ButtonH, TextFont, (0 | '0'), "0.");//hilight button 0
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
/* API to demonstrate progress bar widget */
ft_void_t SAMAPP_CoPro_Widget_Progressbar()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of progress function. Progress func */
	/* draws process bar with fgcolor for the % completion and bgcolor for   */
	/* % remaining. Progress bar supports flat and 3d effets                 */
	/*************************************************************************/
	{
		ft_int16_t xOffset,yOffset,yDist = FT_DispWidth/12,ySz = FT_DispWidth/24;

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw progress bar with flat effect */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_BgColor(phost, 0x404080);
	Ft_Gpu_CoCmd_Progress(phost, 20, 10, 120, 20, OPT_FLAT, 50, 100);//note that h/2 will be added on both sides of the progress bar
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,20, 40, 26, 0, "Flat effect");//text info
	/* Draw progress bar with 3d effect */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xff,0x00));
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Progress(phost, 180, 10, 120, 20, 0, 75, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,180, 40, 26, 0, "3D effect");//text info
	/* Draw progress bar with 3d effect and string on top */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0x00,0x00));
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	Ft_Gpu_CoCmd_Progress(phost, 30, 60, 120, 30, 0, 19660, 65535);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,78, 68, 26, 0, "30 %");//text info

	xOffset = 20;yOffset = 120;
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0xa0,0x00));
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Progress(phost, xOffset, yOffset, 150, ySz, 0, 10, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Progress(phost, xOffset, yOffset, 150, ySz, 0, 40, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Progress(phost, xOffset, yOffset, 150, ySz, 0, 70, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Progress(phost, xOffset, yOffset, 150, ySz, 0, 90, 100);

	Ft_Gpu_CoCmd_Text(phost,xOffset + 180, 80, 26, 0, "40 % TopBottom");//text info
	Ft_Gpu_CoCmd_Progress(phost, xOffset + 180, 100, ySz, 150, 0, 40, 100);

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    SAMAPP_ENABLE_DELAY();
	}
}
/* API to demonstrate scroll widget */
ft_void_t SAMAPP_CoPro_Widget_Scroll()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of scroll function. Scroll function */
	/* draws scroll bar with fgcolor for inner color and current location and*/
	/* can be given by val parameter */
	/*************************************************************************/
	{
		ft_int16_t xOffset,yOffset,xDist = FT_DispWidth/12,yDist = FT_DispWidth/12,wSz;


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw scroll bar with flat effect */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x404080);
	Ft_Gpu_CoCmd_Scrollbar(phost, 20, 10, 120, 8, OPT_FLAT, 20, 30, 100);//note that h/2 size will be added on both sides of the progress bar
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,20, 40, 26, 0, "Flat effect");//text info
	/* Draw scroll bar with 3d effect */
	Ft_Gpu_CoCmd_FgColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Scrollbar(phost, 180, 10, 120, 8, 0, 20, 30, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,180, 40, 26, 0, "3D effect");//text info

	xOffset = 20;
	yOffset = 120;
	wSz = ((FT_DispWidth/2) - 40);
	/* Draw horizontal scroll bars */
	Ft_Gpu_CoCmd_FgColor(phost,0x00a000);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, wSz, 8, 0, 10, 30, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, wSz, 8, 0, 30, 30, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, wSz, 8, 0, 50, 30, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, wSz, 8, 0, 70, 30, 100);

	xOffset = (FT_DispWidth/2) + 40;
	yOffset = 80;
	wSz = (FT_DispHeight - 100);
	/* draw vertical scroll bars */
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, 8, wSz, 0, 10, 30, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, 8, wSz, 0, 30, 30, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, 8, wSz, 0, 50, 30, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Scrollbar(phost, xOffset, yOffset, 8, wSz, 0, 70, 30, 100);

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
/* API to demonstrate slider widget */
ft_void_t SAMAPP_CoPro_Widget_Slider()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of slider function. Slider function */
	/* draws slider bar with fgcolor for inner color and bgcolor for the knob*/
	/* , contains input parameter for position of the knob                   */
	/*************************************************************************/
	{
		ft_int16_t xOffset,yOffset,xDist = FT_DispWidth/12,yDist = FT_DispWidth/12,wSz;

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw scroll bar with flat effect */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	Ft_Gpu_CoCmd_Slider(phost, 20, 10, 120, 10, OPT_FLAT, 30, 100);//note that h/2 size will be added on both sides of the progress bar
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,20, 40, 26, 0, "Flat effect");//text info
	/* Draw scroll bar with 3d effect */
	Ft_Gpu_CoCmd_FgColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Slider(phost, 180, 10, 120, 10, 0, 50, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,180, 40, 26, 0, "3D effect");//text info

	xOffset = 20;
	yOffset = 120;
	wSz = ((FT_DispWidth/2) - 40);
	/* Draw horizontal slider bars */
	Ft_Gpu_CoCmd_FgColor(phost,0x00a000);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(41,1,5));
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, wSz, 10, 0, 10, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(11,7,65));
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, wSz, 10, 0, 30, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(87,94,9));
	yOffset += yDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, wSz, 10, 0, 50, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(51,50,52));
	yOffset += yDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, wSz, 10, 0, 70, 100);

	xOffset = (FT_DispWidth/2) + 40;
	yOffset = 80;
	wSz = (FT_DispHeight - 100);
	/* draw vertical slider bars */
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, 10, wSz, 0, 10, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, 10, wSz, 0, 30, 100);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, 10, wSz, 0, 50, 100);
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	xOffset += xDist;
	Ft_Gpu_CoCmd_Slider(phost, xOffset, yOffset, 10, wSz, 0, 70, 100);

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
/*API to demonstrate dial widget */
ft_void_t SAMAPP_CoPro_Widget_Dial()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of dial function. Dial function     */
	/* draws rounded bar with fgcolor for knob color and colorrgb for pointer*/
	/* , contains input parameter for angle of the pointer                   */
	/*************************************************************************/
	{

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw dial with flat effect */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	Ft_Gpu_CoCmd_Dial(phost, 50, 50, 40, OPT_FLAT, 0.2*65535);//20%
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,15, 90, 26, 0, "Flat effect");//text info
	/* Draw dial with 3d effect */
	Ft_Gpu_CoCmd_FgColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Dial(phost, 140, 50, 40, 0, 0.45*65535);//45%
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,105, 90, 26, 0, "3D effect");//text info

	/* Draw increasing dials horizontally */
	Ft_Gpu_CoCmd_FgColor(phost,0x800000);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(41,1,5));
	Ft_Gpu_CoCmd_Dial(phost, 30, 160, 20, 0, 0.30*65535);
	Ft_Gpu_CoCmd_Text(phost,20, 180, 26, 0, "30 %");//text info
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(11,7,65));
	Ft_Gpu_CoCmd_FgColor(phost,0x000080);
	Ft_Gpu_CoCmd_Dial(phost, 100, 160, 40, 0, 0.45*65535);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,90, 200, 26, 0, "45 %");//text info
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(87,94,9));
	Ft_Gpu_CoCmd_Dial(phost, 210, 160, 60, 0, 0.60*65535);
	Ft_Gpu_CoCmd_Text(phost,200, 220, 26, 0, "60 %");//text info
	Ft_Gpu_CoCmd_FgColor(phost,0x808080);

#ifndef SAMAPP_DISPLAY_QVGA
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(51,50,52));
	Ft_Gpu_CoCmd_Dial(phost, 360, 160, 80, 0, 0.75*65535);
	Ft_Gpu_CoCmd_Text(phost,350, 240, 26, 0, "75 %");//text info
#endif
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}
}
/* API to demonstrate toggle widget */
ft_void_t SAMAPP_CoPro_Widget_Toggle()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of toggle function. Toggle function */
	/* draws line with inside knob to choose between left and right. Toggle  */
	/* inside color can be changed by bgcolor and knob color by fgcolor. Left*/
	/* right texts can be written and the color of the text can be changed by*/
	/* colorrgb graphics function                                            */
	/*************************************************************************/
	{
		ft_int16_t xOffset,yOffset,yDist = 40;

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* Draw toggle with flat effect */
	Ft_Gpu_CoCmd_FgColor(phost,0xffff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));

	Ft_Gpu_CoCmd_Toggle(phost, 40, 10, 30, 27, OPT_FLAT, 0.5*65535,"no""\xff""yes");//circle radius will be extended on both the horizontal sides
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,40, 40, 26, 0, "Flat effect");//text info
	/* Draw toggle bar with 3d effect */
	Ft_Gpu_CoCmd_FgColor(phost,0x00ff00);
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_Toggle(phost, 140, 10, 30, 27, 0, 1*65535,"stop""\xff""run");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_Gpu_CoCmd_Text(phost,140, 40, 26, 0, "3D effect");//text info

	xOffset = 40;
	yOffset = 80;
	/* Draw horizontal toggle bars */
	Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
	Ft_Gpu_CoCmd_FgColor(phost,0x410105);
	Ft_Gpu_CoCmd_Toggle(phost, xOffset, yOffset, 30, 27, 0, 0*65535,"-ve""\xff""+ve");
	Ft_Gpu_CoCmd_FgColor(phost,0x0b0721);
	Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
	yOffset += yDist;
	Ft_Gpu_CoCmd_Toggle(phost, xOffset, yOffset, 30, 27, 0, 0.25*65535,"zero""\xff""one");
	Ft_Gpu_CoCmd_BgColor(phost, 0xffff00);
	Ft_Gpu_CoCmd_FgColor(phost,0x575e1b);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	yOffset += yDist;
	Ft_Gpu_CoCmd_Toggle(phost, xOffset, yOffset, 30, 27, 0, 0.5*65535,"exit""\xff""init");
	Ft_Gpu_CoCmd_BgColor(phost, 0x808080);
	Ft_Gpu_CoCmd_FgColor(phost,0x333234);
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	yOffset += yDist;
	Ft_Gpu_CoCmd_Toggle(phost, xOffset, yOffset, 30, 27, 0, 0.75*65535,"off""\xff""on");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY();
	}

}
/* API to demonstrate spinner widget */
ft_void_t SAMAPP_CoPro_Widget_Spinner()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of spinner function. Spinner func   */
	/* will wait untill stop command is sent from the mcu. Spinner has option*/
	/* for displaying points in circle fashion or in a line fashion.         */
	/*************************************************************************/
	{


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Spinner circle");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 27, OPT_CENTER, "Please Wait ...");
	Ft_Gpu_CoCmd_Spinner(phost, (FT_DispWidth/2),(FT_DispHeight/2),0,1);//style 0 and scale 0

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(1000);

	/**************************** spinner with style 1 and scale 1 *****************************************************/


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Spinner line");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 27, OPT_CENTER, "Please Wait ...");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x00,0x00,0x80));
	Ft_Gpu_CoCmd_Spinner(phost, (FT_DispWidth/2),(FT_DispHeight/2),1,1);//style 1 and scale 1

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);


	Ft_Gpu_Hal_Sleep(1000);



	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Spinner clockhand");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 27, OPT_CENTER, "Please Wait ...");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x00,0x00));
	Ft_Gpu_CoCmd_Spinner(phost, (FT_DispWidth/2),((FT_DispHeight/2) + 20),2,1);//style 2 scale 1

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);
	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(1000);


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Spinner two dots");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 27, OPT_CENTER, "Please Wait ...");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x00,0x00));
	Ft_Gpu_CoCmd_Spinner(phost, (FT_DispWidth/2),((FT_DispHeight/2) + 20),3,1);//style 3 scale 0

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);
	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(1000);

	/* Send the stop command */
	Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);
	/* Update the command buffer pointers - both read and write pointers */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	}

	Ft_Gpu_Hal_Sleep(1000);

}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET4
/* API to demonstrate screen saver widget - playing of bitmap via macro0 */
ft_void_t SAMAPP_CoPro_Screensaver()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of screensaver function. Screen     */
	/* saver modifies macro0 with the vertex2f command.                      */
	/* MCU can display any static display list at the background with the    */
	/* changing bitmap                                                       */
	/*************************************************************************/
	{
	/* Download the bitmap data */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(ft_prog_uchar8_t *)&SAMAPP_Bitmap_RawData[SAMAPP_Bitmap_RawData_Header[0].Arrayoffset],
		SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height);

	/* Send command screen saver */

	Ft_App_WrCoCmd_Buffer(phost, CMD_SCREENSAVER);//screen saver command will continuously update the macro0 with vertex2f command
	/* Copy the display list */
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0,0,0x80));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	/* lena bitmap */
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Scale(phost, 3*65536,3*65536);//scale the bitmap 3 times on both sides
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,
		SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,
		SAMAPP_Bitmap_RawData_Header[0].Width*3,SAMAPP_Bitmap_RawData_Header[0].Height*3));
	Ft_App_WrCoCmd_Buffer(phost,MACRO(0));
	Ft_App_WrCoCmd_Buffer(phost,END());
	/* Display the text */
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), (FT_DispHeight/2), 27, OPT_CENTER, "Screen Saver ...");
	Ft_Gpu_CoCmd_MemSet(phost, (RAM_G + 3200), 0xff, (160L*2*120));
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(3*1000);

	/* Send the stop command */
	Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);
	/* Update the command buffer pointers - both read and write pointers */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	}
}
/* Sample app to demonstrate snapshot widget/functionality */
ft_void_t SAMAPP_CoPro_Snapshot()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of snapshot function. Snapshot func */
	/* captures the present screen and dumps into bitmap with color formats  */
	/* argb4.  FT81x supports a simplier snapshot2 command to capture the    */
	/* screen, both methods can be used in FT81X.                            */
	/*************************************************************************/
	{
		ft_uint16_t bitmapWidth = 160, bitmapHeight = 120;
		ft_uint16_t WriteByte = 0, RGB565RamOffset = 0, ARGB4RamOffset = RGB565RamOffset + bitmapWidth * 2 * bitmapHeight, ARGB8RamOffset = ARGB4RamOffset + bitmapWidth * 2 * bitmapHeight;
		SAMAPP_fadeout();
#ifndef FT_81X_ENABLE

#if !defined(ME800A_HV35R) && !defined(ME810A_HV35R)
		/* fadeout before switching off the pclock */

		/* Switch off the lcd */
		{
			ft_uint8_t n = 0;
			Ft_Gpu_Hal_Wr8(phost, REG_GPIO, 0x7f);			
			Ft_Gpu_Hal_Sleep(100);
		}
#endif
		/* Disable the pclock */
		Ft_Gpu_Hal_Wr8(phost, REG_PCLK,WriteByte);
		/* Configure the resolution to 160x120 dimention */
		WriteByte = bitmapWidth;
		Ft_Gpu_Hal_Wr16(phost, REG_HSIZE,WriteByte);
		WriteByte = bitmapHeight;
		Ft_Gpu_Hal_Wr16(phost, REG_VSIZE,WriteByte);
#endif
		/* Construct screen shot for snapshot */

		Ft_Gpu_CoCmd_Dlstart(phost);
		//Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0, 0, 0));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(28, 20, 99));
		/* captured snapshot */
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_A(128));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(20 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(0 * 16, 0 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(25 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(20 * 16, 10 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(30 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(40 * 16, 20 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(35 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(60 * 16, 30 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(40 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(80 * 16, 40 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(45 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(100 * 16, 50 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(50 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(120 * 16, 60 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(55 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(140 * 16, 70 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(60 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(160 * 16, 80 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(65 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(0 * 16, 120 * 16));
		Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(70 * 16));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(160 * 16, 0 * 16));
		Ft_App_WrCoCmd_Buffer(phost, END()); //display the bitmap at the center of the display
		Ft_App_WrCoCmd_Buffer(phost, COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(32, 32, 32));
		Ft_Gpu_CoCmd_Text(phost, 80, 60, 26, OPT_CENTER, "Points");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);


	Ft_Gpu_Hal_Sleep(100);//timeout for snapshot to be performed by coprocessor

#ifndef FT_81X_ENABLE
		/* Take snap shot of the current screen */
		Ft_Gpu_Hal_WrCmd32(phost, CMD_SNAPSHOT);
		Ft_Gpu_Hal_WrCmd32(phost, 3200); //store the rgb content at location 3200

		//timeout for snapshot to be performed by coprocessor

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		Ft_Gpu_Hal_Sleep(100); //timeout for snapshot to be performed by coprocessor

		/* reconfigure the resolution wrt configuration */
		WriteByte = FT_DispWidth;
		Ft_Gpu_Hal_Wr16(phost, REG_HSIZE,WriteByte);
		WriteByte = FT_DispHeight;
		Ft_Gpu_Hal_Wr16(phost, REG_VSIZE,WriteByte);
#else

		Ft_Gpu_CoCmd_Snapshot2(phost, ARGB4, ARGB4RamOffset, 0, 0, bitmapWidth, bitmapHeight);
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		Ft_Gpu_Hal_Sleep(100); //timeout for snapshot to be performed by coprocessor

		Ft_Gpu_CoCmd_Snapshot2(phost, RGB565, RGB565RamOffset, 0, 0, bitmapWidth, bitmapHeight);
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);



		Ft_Gpu_Hal_Sleep(100); //timeout for snapshot to be performed by coprocessor
		/*

		 Ft_Gpu_CoCmd_Snapshot2(phost, 0x20, ARGB8RamOffset, 0, 0, bitmapWidth, bitmapHeight);
		 Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		 Ft_Gpu_Hal_Sleep(100); //timeout for snapshot to be performed by coprocessor
		 */
#endif

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0xff, 0xff, 0xff));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(0xff, 0xff, 0xff));
		/* captured snapshot */

#ifndef FT_81X_ENABLE
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(3200));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4,bitmapWidth*2,bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,bitmapWidth,bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(((FT_DispWidth - 160)/2)*16,((FT_DispHeight - 120)/2)*16));
		Ft_App_WrCoCmd_Buffer(phost,END()); //display the bitmap at the center of the display
		/* Display the text info */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));
		Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 40, 27, OPT_CENTER, "Snap shot");
#else
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(RGB565RamOffset));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, bitmapWidth * 2, bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, bitmapWidth, bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(((FT_DispWidth) / 6) * 16, ((FT_DispHeight) / 2) * 16));
		Ft_App_WrCoCmd_Buffer(phost, END()); //display the bitmap at the center of the display
		/* Display the text info */
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(32, 32, 32));
		Ft_Gpu_CoCmd_Text(phost, ((FT_DispWidth) / 6), ((FT_DispHeight) / 2) - 20, 27, 0, "RGB565");

		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(ARGB4RamOffset));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(ARGB4, bitmapWidth * 2, bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, bitmapWidth, bitmapHeight));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(((FT_DispWidth) / 2) * 16, ((FT_DispHeight) / 2) * 16));
		Ft_App_WrCoCmd_Buffer(phost, END()); //display the bitmap at the center of the display
		/* Display the text info */
		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(32, 32, 32));
		Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), ((FT_DispHeight) / 2) - 20, 27, 0, "ARGB4");

		/*
		 Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(ARGB*RamOffset));
		 Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4,160*2,bitmapHeight));
		 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,160,bitmapHeight));
		 Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(((FT_DispWidth - 160)/2)*16,((FT_DispHeight)/2)*16));
		 Ft_App_WrCoCmd_Buffer(phost,END()); //display the bitmap at the center of the display
		 Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));
		 Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 40, 27, 0, "ARGB8");
		 */

		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(32, 32, 32));
		Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 40, 30, OPT_CENTER, "Snap Shot");
#endif
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
#ifndef FT_81X_ENABLE
		/* reenable the pclock */
		WriteByte = FT_DispPCLK;
		Ft_Gpu_Hal_Wr8(phost, REG_PCLK,WriteByte);
		Ft_Gpu_Hal_Sleep(60);
#if !defined(ME800A_HV35R) && !defined(ME810A_HV35R)
		/* Power on the LCD */
		{
			ft_uint8_t n = 0x80;
			Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);
		}
#endif
#endif
	Ft_Gpu_Hal_Sleep(200);//give some time for the lcd to switchon - hack for one perticular panel

		/* set the display pwm back to 128 */
		{
			SAMAPP_fadein();
		}
		SAMAPP_ENABLE_DELAY();

	}
}
/* API to demonstrate sketch widget */
ft_void_t SAMAPP_CoPro_Sketch()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of sketch function. Sketch function */
	/* draws line for pen movement. Skecth supports L1 and L8 output formats */
	/* Sketch draws till stop command is executed.                           */
	/*************************************************************************/
	{
	ft_int16_t BorderSz = 40;
    ft_uint32_t MemZeroSz;
	/* Send command sketch */


    MemZeroSz = 1L*(FT_DispWidth - 2*BorderSz)*(FT_DispHeight - 2*BorderSz);
	Ft_Gpu_CoCmd_MemZero(phost, RAM_G,MemZeroSz);
#ifdef FT_801_ENABLE
	Ft_Gpu_CoCmd_CSketch(phost, BorderSz,BorderSz,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz),0,L1,1500L);//sketch in L1 format
#else
	Ft_Gpu_CoCmd_Sketch(phost, BorderSz,BorderSz,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz),RAM_G,L1);//sketch in L1 format
#endif
	/* Display the sketch */
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0x80,0,0x00));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE((FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(BorderSz,BorderSz));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

	/* default the scissor size */
#ifdef FT_81X_ENABLE
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(2048,2048));
#else
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(512,512));
#endif

	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	/* L1 bitmap display */
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
#ifdef FT_81X_ENABLE

	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L1,(FT_DispWidth - 2*BorderSz)/8,(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H((((FT_DispWidth - 2*BorderSz)/8)>>10),((FT_DispHeight - 2*BorderSz)>>9)));

	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H(((FT_DispWidth - 2*BorderSz)>>9),((FT_DispHeight - 2*BorderSz)>>9)));

#else
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L1,(FT_DispWidth - 2*BorderSz)/8,(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
#endif
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(BorderSz*16,BorderSz*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	/* Display the text */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Sketch L1");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(3*1000);//sleep for 10 seconds

	/* Send the stop command */
	Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);
	/* Update the command buffer pointers - both read and write pointers */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	/* Sketch L8 format */

	/* Send command sketch */

	Ft_Gpu_CoCmd_MemZero(phost, RAM_G,MemZeroSz);
#ifdef FT_801_ENABLE
	Ft_Gpu_CoCmd_CSketch(phost, BorderSz,BorderSz,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz),0,L8,1500L);//sketch in L8 format
#else
	Ft_Gpu_CoCmd_Sketch(phost, BorderSz,BorderSz,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz),0,L8);//sketch in L8 format
#endif
	/* Display the sketch */
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0x00,0,0x80));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE((FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(BorderSz,BorderSz));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

#ifdef FT_81X_ENABLE
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(2048,2048));
#else
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(512,512));
#endif
	Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	/* L8 bitmap display */
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
#ifdef FT_81X_ENABLE

	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H((((FT_DispWidth - 2*BorderSz))>>10),((FT_DispHeight - 2*BorderSz)>>9)));

	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H(((FT_DispWidth - 2*BorderSz)>>9),((FT_DispHeight - 2*BorderSz)>>9)));

#else
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,(FT_DispWidth - 2*BorderSz),(FT_DispHeight - 2*BorderSz)));
#endif
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(BorderSz*16,BorderSz*16));
	Ft_App_WrCoCmd_Buffer(phost,END());
	/* Display the text */
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "Sketch L8");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(3*1000);//sleep for 3 seconds

	/* Send the stop command */
	Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);
	/* Update the command buffer pointers - both read and write pointers */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	}
}

/* API to demonstrate scale, rotate and translate functionality */
ft_void_t SAMAPP_CoPro_Matrix()
{
	/*************************************************************************/
	/* Below code demonstrates the usage of bitmap matrix processing apis.   */
	/* Mainly matrix apis consists if scale, rotate and translate.           */
	/* Units of translation and scale are interms of 1/65536, rotation is in */
	/* degrees and in terms of 1/65536. +ve theta is anticlock wise, and -ve  */
	/* theta is clock wise rotation                                          */
	/*************************************************************************/

	/* Lena image with 40x40 rgb565 is used as an example */
	{
	ft_int32_t imagewidth,imagestride,imageheight,imagexoffset,imageyoffset;

	/* Download the bitmap data */
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,(ft_prog_uchar8_t *)&SAMAPP_Bitmap_RawData[SAMAPP_Bitmap_RawData_Header[0].Arrayoffset],
		SAMAPP_Bitmap_RawData_Header[0].Stride*SAMAPP_Bitmap_RawData_Header[0].Height);


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));
	Ft_Gpu_CoCmd_Text(phost,10, 5, 16, 0, "BM with rotation");
	Ft_Gpu_CoCmd_Text(phost,10, 20 + 40 + 5, 16, 0, "BM with scaling");
	Ft_Gpu_CoCmd_Text(phost,10, 20 + 40 + 20 + 80 + 5, 16, 0, "BM with flip");

	imagewidth = SAMAPP_Bitmap_RawData_Header[0].Width;
	imageheight = SAMAPP_Bitmap_RawData_Header[0].Height;
	imagestride = SAMAPP_Bitmap_RawData_Header[0].Stride;
	imagexoffset = 10*16;
	imageyoffset = 20*16;

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format,imagestride,imageheight));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth,imageheight));
	/******************************************* Perform display of plain bitmap ************************************/
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 45 degrees anti clock wise and the rotation is performed on top left coordinate */
	imagexoffset += (imagewidth + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Rotate(phost, (-45*65536/360));//rotate by 45 degrees anticlock wise
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 30 degrees clock wise and the rotation is performed on top left coordinate */
	imagexoffset += (imagewidth*1.42 + 10)*16;//add the width*1.41 as diagonal is new width and extra 10 pixels
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Rotate(phost, 30*65536/360);//rotate by 33 degrees clock wise
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 45 degrees anti clock wise and the rotation is performed wrt centre of the bitmap */
	imagexoffset += (imagewidth*1.42 + 10)*16;//add the width*1.41 as diagonal is new width and extra 10 pixels
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Rotate(phost, -45*65536/360);//rotate by 45 degrees anticlock wise
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 45 degrees clock wise and the rotation is performed so that whole bitmap is viewable */
	imagexoffset += (imagewidth*1.42 + 10)*16;//add the width*1.41 as diagonal is new width and extra 10 pixels
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Rotate(phost, 45*65536/360);//rotate by 45 degrees clock wise
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/10,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth*2,imageheight*2));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 90 degrees anti clock wise and the rotation is performed so that whole bitmap is viewable */
	imagexoffset += (imagewidth*2 + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Rotate(phost, -90*65536/360);//rotate by 90 degrees anticlock wise
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth,imageheight));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 180 degrees clock wise and the rotation is performed so that whole bitmap is viewable */
	imagexoffset += (imagewidth + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Rotate(phost, -180*65536/360);//rotate by 180 degrees anticlock wise
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));
	/******************************************* Perform display of bitmap with scale ************************************/
	/* Perform display of plain bitmap with scale factor of 2x2 in x & y direction */
	imagexoffset = (10)*16;
	imageyoffset += (imageheight + 20)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Scale(phost, 2*65536,2*65536);//scale by 2x2
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth*2,imageheight*2));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with scale factor of .5x.25 in x & y direction, rotate by 45 degrees clock wise wrt top left */
	imagexoffset += (imagewidth*2 + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center

	Ft_Gpu_CoCmd_Rotate(phost, 45*65536/360);//rotate by 45 degrees clock wise
	Ft_Gpu_CoCmd_Scale(phost, 65536/2,65536/4);//scale by .5x.25
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with scale factor of .5x2 in x & y direction, rotate by 75 degrees anticlock wise wrt center of the image */
	imagexoffset += (imagewidth + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Rotate(phost, -75*65536/360);//rotate by 75 degrees anticlock wise
	Ft_Gpu_CoCmd_Scale(phost, 65536/2,2*65536);//scale by .5x2
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/8);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth*5/2,imageheight*5/2));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));
	/******************************************* Perform display of bitmap flip ************************************/
	/* perform display of plain bitmap with 1x1 and flip right */
	imagexoffset = (10)*16;
	imageyoffset += (imageheight*2 + 20)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Scale(phost, -1*65536,1*65536);//flip right
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/2);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth,imageheight));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 2x2 scaling, flip bottom */
	imagexoffset += (imagewidth + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center
	Ft_Gpu_CoCmd_Scale(phost, 2*65536,-2*65536);//flip bottom and scale by 2 on both sides
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/4,-65536*imageheight/1.42);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,imagewidth*4,imageheight*4));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	/* Perform display of plain bitmap with 2x1 scaling, rotation and flip right and make sure whole image is viewable */
	imagexoffset += (imagewidth*2 + 10)*16;
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Translate(phost, 65536*imagewidth/2,65536*imageheight/2);//make the rotation coordinates at the center

	Ft_Gpu_CoCmd_Rotate(phost, -45*65536/360);//rotate by 45 degrees anticlock wise
	Ft_Gpu_CoCmd_Scale(phost, -2*65536,1*65536);//flip right and scale by 2 on x axis
	Ft_Gpu_CoCmd_Translate(phost, -65536*imagewidth/2,-65536*imageheight/8);
	Ft_Gpu_CoCmd_SetMatrix(phost );
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR,BORDER,BORDER,(imagewidth*5),(imageheight*5)));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(imagexoffset,imageyoffset));

	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
        SAMAPP_ENABLE_DELAY_VALUE(2000);

	}
}

#endif
#ifdef SAMAPP_ENABLE_APIS_SET3
/* API to demonstrate custom font display */
ft_void_t SAMAPP_CoPro_Setfont()
{
	ft_uint8_t *pbuff;
	ft_uint8_t FontIdxTable[148];
	/*************************************************************************/
	/* Below code demonstrates the usage of setfont. Setfont function draws  */
	/* custom configured fonts on screen. Download the font table and raw    */
	/* font data in L1/L4/L8 format and disply text                          */
	/*************************************************************************/
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
{
	ft_uint32_t fontaddr = (128+5*4);
	FILE *pFile = fopen("..\\..\\..\\Test\\font16.jpg","rb");
	ft_int32_t FileLen = 0;

	memset(FontIdxTable,16,128);
	fontaddr = 0x00000003;//l8 format
	memcpy(&FontIdxTable[128],&fontaddr,4);
	fontaddr = 16;//stride
	memcpy(&FontIdxTable[128+4],&fontaddr,4);
	fontaddr = 16;//max width
	memcpy(&FontIdxTable[128+8],&fontaddr,4);
	fontaddr = 16;//max height
	memcpy(&FontIdxTable[128+12],&fontaddr,4);
	fontaddr = (1024);//data address - starts at location 1024
	memcpy(&FontIdxTable[128+16],&fontaddr,4);

	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G,	FontIdxTable,(128+5*4));

	/* download the jpeg image and decode */
	/* Characters from 32 to 128 are present and each character is 16*16 dimention */
	if(NULL == pFile)
	{
		printf("Error in opening file %s \n","font16.jpg");
	}
	else
	{
		/******************* Decode jpg output into location 0 and output color format as RGB565 *********************/
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
		Ft_Gpu_Hal_WrCmd32(phost,  (9216));//destination address of jpg decode
		Ft_Gpu_Hal_WrCmd32(phost,  OPT_MONO);//output format of the bitmap
		fseek(pFile,0,SEEK_END);
		FileLen = ftell(pFile);
		fseek(pFile,0,SEEK_SET);
		pbuff = malloc(8192);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

				/* copy the data into pbuff and then transfter it to command buffer */
				fread(pbuff,1,blocklen,pFile);
				FileLen -= blocklen;
				/* copy data continuously into command memory */
				Ft_Gpu_Hal_WrCmdBuf(phost,pbuff, blocklen); //alignment is already taken care by this api
			}
			fclose(pFile);
		}


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));//set the background to white
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));//black color text

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "SetFont - format L8");
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(1024));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,16,16));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,16,16));

	Ft_Gpu_CoCmd_SetFont(phost, 7,0);

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 7, OPT_CENTER, "The quick brown fox jumps");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 120, 7, OPT_CENTER, "over the lazy dog.");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 160,7, OPT_CENTER, "1234567890");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(1000);//timeout for snapshot to be performed by coprocessor

	fclose(pFile);

	}
#endif
	/* Display custom font by reading from the binary file - header of 148 bytes is at the starting followed by font data of 96 characters */
	/*Roboto-BoldCondensed.ttf*/
	{
	ft_uint32_t fontaddr = (128+5*4);//header size
	ft_int32_t FileLen = 0,i;
	ft_uint16_t blocklen;
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile = fopen("..\\..\\..\\Test\\Roboto-BoldCondensed_12.bin","rb");

	//first 148 bytes in the file is the header and later is the raw data for ascii 32 to 128 index charaters
	if(NULL == pFile)
	{

		printf("Error in opening file %s \n","Roboto-BoldCondensed_12.bin");
	}
	else
	{
#endif
		fontaddr = RAM_G;
		blocklen = 128+5*4;//header size

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		fseek(pFile,0,SEEK_END);
		FileLen = ftell(pFile);
		fseek(pFile,0,SEEK_SET);

				//pbuff = malloc(8192);
				/* First download the header into location 0 */
				fread(pbuff,1,blocklen,pFile);
#endif

#ifdef ARDUINO_PLATFORM
                pbuff = FontIdxTable;
                /* Copy data from starting of the array into buffer */
                //hal_memcpy((ft_char8_t*)pbuff,(ft_char8_t*)Roboto_BoldCondensed_12,1L*blocklen);
                memcpy_P((ft_char8_t*)pbuff,(ft_char8_t*)Roboto_BoldCondensed_12,1L*blocklen);
#endif
#ifdef FT900_PLATFORM
                pbuff = FontIdxTable;
                /* Copy data from starting of the array into buffer */
                //hal_memcpy((ft_char8_t*)pbuff,(ft_char8_t*)Roboto_BoldCondensed_12,1L*blocklen);
                //memcpy((ft_char8_t*)pbuff,Roboto_BoldCondensed_12,1L*blocklen);
				{
				ft_int32_t i;
					for(i=0;i<1L*blocklen;i++)
					{
						pbuff[i] = ft_pgm_read_byte_near(&Roboto_BoldCondensed_12[i]);
					}
				}
#endif
		{
			ft_uint32_t *ptemp = (ft_uint32_t *)&pbuff[128+4*4],i;
			*ptemp = 1024;//download the font data at location 1024+32*8*25
			//memset(pbuff,16,32);
                        for(i=0;i<32;i++)
                        {
                          pbuff[i] = 16;
                        }
		}
		/* Modify the font data location */
		Ft_Gpu_Hal_WrMem(phost,fontaddr,(ft_uint8_t *)pbuff,blocklen);

		/* Next download the data at location 32*8*25 - skip the first 32 characters */
		/* each character is 8x25 bytes */
		fontaddr += (1024+32*8*25);//make sure space is left at the starting of the buffer for first 32 characters - TBD manager this buffer so that this buffer can be utilized by other module
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		FileLen -= blocklen;
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 8kb one shot */
			blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */
			fread(pbuff,1,blocklen,pFile);
			/* copy data continuously into command memory */
			Ft_Gpu_Hal_WrMemFromFlash(phost, fontaddr,(ft_uint8_t *)pbuff,blocklen);
			FileLen -= blocklen;
			fontaddr += blocklen;
		}
		free(pbuff);
		fclose(pFile);
          }
#endif
#ifdef ARDUINO_PLATFORM
        Ft_Gpu_Hal_WrMemFromFlash(phost, fontaddr,&Roboto_BoldCondensed_12[blocklen],1L*SAMAPP_Roboto_BoldCondensed_12_SIZE);
#endif
#ifdef FT900_PLATFORM
        Ft_Gpu_Hal_WrMemFromFlash(phost, fontaddr,(const ft_prog_uchar8_t *)&Roboto_BoldCondensed_12[blocklen],1L*SAMAPP_Roboto_BoldCondensed_12_SIZE);
#endif

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));//set the background to white
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));//black color text

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "SetFont - format L4");
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));//give index table 6
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(1024));//make the address to 0
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4,8,25));//stride is 8 and height is 25
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,16,25));//width is 16 and height is 25

	Ft_Gpu_CoCmd_SetFont(phost, 6,0);
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 6, OPT_CENTER, "The quick brown fox jumps");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 120, 6, OPT_CENTER, "over the lazy dog.");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 160, 6, OPT_CENTER, "1234567890");

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(1000);//timeout for snapshot to be performed by coprocessor
	}
}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET4
/* Sample app api to demonstrate track widget funtionality */
ft_void_t SAMAPP_CoPro_Track()
{
	ft_uint8_t *pbuff;
	ft_uint32_t NumBytesGen = 0;
	ft_uint16_t CurrWriteOffset = 0;
	SAMAPP_Bitmap_header_t *pBitmapHdr = NULL;

	/*************************************************************************/
	/* Below code demonstrates the usage of track function. Track function   */
	/* tracks the pen touch on any specific object. Track function supports  */
	/* rotary and horizontal/vertical tracks. Rotary is given by rotation    */
	/* angle and horizontal/vertucal track is offset position.               */
	/*************************************************************************/
	{
	ft_int32_t LoopFlag = 0;
	ft_uint32_t TrackRegisterVal = 0;
	ft_uint16_t angleval = 0,slideval = 0,scrollval = 0;

	/* Set the tracker for 3 bojects */

	Ft_Gpu_CoCmd_Track(phost, FT_DispWidth/2, FT_DispHeight/2, 1,1, 10);
	Ft_Gpu_CoCmd_Track(phost, 40, (FT_DispHeight - 40), (FT_DispWidth - 80),8, 11);
	Ft_Gpu_CoCmd_Track(phost, (FT_DispWidth - 40), 40, 8,(FT_DispHeight - 80), 12);
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);


	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	LoopFlag = 600;
	/* update the background color continuously for the color change in any of the trackers */
	while(LoopFlag--)
	{
		ft_uint8_t tagval = 0;
		TrackRegisterVal = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER);
		tagval = TrackRegisterVal & 0xff;
		if(0 != tagval)
		{
			if(10 == tagval)
			{
				angleval = TrackRegisterVal>>16;
			}
			else if(11 == tagval)
			{
				slideval = TrackRegisterVal>>16;
			}
			else if(12 == tagval)
			{
				scrollval = TrackRegisterVal>>16;
				if((scrollval + 65535/10) > (9*65535/10))
				{
					scrollval = (8*65535/10);
				}
				else if(scrollval < (1*65535/10))
				{
					scrollval = 0;
				}
				else
				{
					scrollval -= (1*65535/10);
				}
			}
		}
		/* Display a rotary dial, horizontal slider and vertical scroll */

		Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);

		{
                        ft_int32_t tmpval0,tmpval1,tmpval2;
			ft_uint8_t angval,sldval,scrlval;

                        tmpval0 = (ft_int32_t)angleval*255/65536;
                        tmpval1 = (ft_int32_t)slideval*255/65536;
                        tmpval2 = (ft_int32_t)scrollval*255/65536;

                        angval = tmpval0&0xff;
                        sldval = tmpval1&0xff;
                        scrlval = tmpval2&0xff;
			Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(angval,sldval,scrlval));
		}
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));

		/* Draw dial with 3d effect */
		Ft_Gpu_CoCmd_FgColor(phost,0x00ff00);
		Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
		Ft_App_WrCoCmd_Buffer(phost,TAG(10));
		Ft_Gpu_CoCmd_Dial(phost, (FT_DispWidth/2), (FT_DispHeight/2), (FT_DispWidth/8), 0, angleval);

		/* Draw slider with 3d effect */
		Ft_Gpu_CoCmd_FgColor(phost,0x00a000);
		Ft_Gpu_CoCmd_BgColor(phost, 0x800000);
		Ft_App_WrCoCmd_Buffer(phost,TAG(11));
		Ft_Gpu_CoCmd_Slider(phost, 40, (FT_DispHeight - 40), (FT_DispWidth - 80),8, 0, slideval, 65535);

		/* Draw scroll with 3d effect */
		Ft_Gpu_CoCmd_FgColor(phost,0x00a000);
		Ft_Gpu_CoCmd_BgColor(phost, 0x000080);
		Ft_App_WrCoCmd_Buffer(phost,TAG(12));
		Ft_Gpu_CoCmd_Scrollbar(phost, (FT_DispWidth - 40), 40, 8, (FT_DispHeight - 80), 0, scrollval, (65535*0.2), 65535);

		Ft_Gpu_CoCmd_FgColor(phost,TAG_MASK(0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), ((FT_DispHeight/2) + (FT_DispWidth/8) + 8), 26, OPT_CENTER, "Rotary track");
		Ft_Gpu_CoCmd_Text(phost,((FT_DispWidth/2)), ((FT_DispHeight - 40) + 8 + 8), 26, OPT_CENTER, "Horizontal track");
		Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth - 40), 20, 26, OPT_CENTER, "Vertical track");

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);

		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		Ft_Gpu_Hal_Sleep(10);
	}

	/* Set the tracker for 3 bojects */

	Ft_Gpu_CoCmd_Track(phost, 240, 136, 0,0, 10);
	Ft_Gpu_CoCmd_Track(phost, 40, 232, 0,0, 11);
	Ft_Gpu_CoCmd_Track(phost, 400, 40, 0,0, 12);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	}
}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET0
ft_void_t SAMAPP_Gpu_Polygon()
{

	Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(INCR,INCR) );
	Ft_App_WrDlCmd_Buffer(phost, COLOR_MASK(0,0,0,0) );//mask all the colors
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/2),(FT_DispHeight/4),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth*4/5),(FT_DispHeight*4/5),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/4),(FT_DispHeight/2),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/2),(FT_DispHeight/4),0,0));
	Ft_App_WrDlCmd_Buffer(phost, END());
	Ft_App_WrDlCmd_Buffer(phost, COLOR_MASK(1,1,1,1) );//enable all the colors
	Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(EQUAL,1,255));
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(FT_DispWidth*16,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(FT_DispWidth*16,FT_DispHeight*16));
	Ft_App_WrDlCmd_Buffer(phost, END());

	/* Draw lines at the borders to make sure anti aliazing is also done */
	Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(ALWAYS,0,255));
	Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(1*16));
	Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 0, 0) );
	Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINES));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/2),(FT_DispHeight/4),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth*4/5),(FT_DispHeight*4/5),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth*4/5),(FT_DispHeight*4/5),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/4),(FT_DispHeight/2),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/4),(FT_DispHeight/2),0,0));
	Ft_App_WrDlCmd_Buffer(phost, VERTEX2II((FT_DispWidth/2),(FT_DispHeight/4),0,0));
	Ft_App_WrDlCmd_Buffer(phost, END());
	Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}

ft_void_t SAMAPP_Gpu_Cube()
{
    ft_uint32_t points[6*5],x,y,i,z;
    ft_int16_t xoffset,yoffset,CubeEdgeSz;

// color vertices
    ft_uint16_t colors[6][3] = { 255,0,0,
                                255,0,150,
                                0,255,0,
                                110,120,110,
                                0,0,255,
                                128,128,0
	                            };

// Cube dimention is of 100*100*100
	CubeEdgeSz = 100;
	xoffset = (FT_DispWidth/2 - CubeEdgeSz);yoffset=((FT_DispHeight - CubeEdgeSz)/2);


// xy plane(front)
       points[0] = VERTEX2F(xoffset*16,yoffset*16);
       points[1] = VERTEX2F((xoffset+CubeEdgeSz)*16,yoffset*16);
       points[2] = VERTEX2F((xoffset+CubeEdgeSz)*16,(yoffset+CubeEdgeSz)*16);
       points[3] = VERTEX2F(xoffset*16,(yoffset+CubeEdgeSz)*16);
       points[4] = points[0];

//yz plane (left)
       x = (xoffset+(CubeEdgeSz/2));            //     xoff+w/2
       y = (yoffset-(CubeEdgeSz/2));            //     yoff-h/2

       points[5] = points[0];
       points[6] = VERTEX2F(x*16,y*16);
       points[7] = VERTEX2F(x*16,(y+CubeEdgeSz)*16);
       points[8] = points[3];
       points[9] = points[5];

//xz plane(top)
       points[10] = points[0];
       points[11] = points[1];
       points[12] = VERTEX2F((x+CubeEdgeSz)*16,(y)*16);
       points[13] = points[6];
       points[14] = points[10];

//xz plane(bottom)
       points[15] = points[3];
       points[16] = points[2];
       points[17] = VERTEX2F((x+CubeEdgeSz)*16,(y+CubeEdgeSz)*16);
       points[18] = points[7];
       points[19] = points[15];

//yz plane (right)
       points[20] = points[2];
       points[21] = points[17];
       points[22] = points[12];
       points[23] = points[1];
       points[24] = points[20];

//yz plane (back)
       points[25] = points[6];
       points[26] = points[7];
       points[27] = points[17];
       points[28] = points[12];
       points[29] = points[25];


       Ft_App_WrDlCmd_Buffer(phost,CLEAR(1,1,1));
       Ft_App_WrDlCmd_Buffer(phost,LINE_WIDTH(16));
       Ft_App_WrDlCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255) );
       Ft_App_WrDlCmd_Buffer(phost,CLEAR(1,1,1) );
       Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));

// Draw a cube
       Ft_App_WrDlCmd_Buffer(phost,STENCIL_OP(INCR,INCR));
       Ft_App_WrDlCmd_Buffer(phost,COLOR_A(192));
       for(z=0;z<6;z++)
       {

			Ft_App_WrDlCmd_Buffer(phost,CLEAR(0,1,1) );//clear stencil buffer
			Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(0,0,0,0));//mask all the colors and draw one surface
			Ft_App_WrDlCmd_Buffer(phost,STENCIL_FUNC(ALWAYS,0,255));//stencil function to increment all the values
            Ft_App_WrDlCmd_Buffer(phost,BEGIN(EDGE_STRIP_L));
            for(i = 0;i<5;i++)
            {
				Ft_App_WrDlCmd_Buffer(phost,points[z*5 + i]);
            }
            Ft_App_WrDlCmd_Buffer(phost,END());
			/* set the color and draw a strip */
			Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
			Ft_App_WrDlCmd_Buffer(phost,STENCIL_FUNC(EQUAL,1,255));
			//Ft_App_WrDlCmd_Buffer(phost,STENCIL_FUNC(EQUAL,(z+1),255));
            Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(colors[z][0],colors[z][1],colors[z][2]));
			Ft_App_WrDlCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(xoffset,0,0,0));
			Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(xoffset + CubeEdgeSz*2,yoffset + CubeEdgeSz*2,0,0));
			Ft_App_WrDlCmd_Buffer(phost,END());
       }
    Ft_App_WrDlCmd_Buffer(phost,DISPLAY());

	/* Download the DL into DL RAM */
	Ft_App_Flush_DL_Buffer(phost);

	/* Do a swap */
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
        SAMAPP_ENABLE_DELAY();
}
#endif

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
/* API to demonstrate music playback in streaming way*/
ft_void_t SAMAPP_Aud_Music_Player_Streaming()
{
	FILE *pFile = NULL;
	ft_uint32_t filesz = 0,chunksize = 16*1024,totalbufflen = 64*1024,currreadlen = 0;
	ft_uint8_t *pBuff = NULL, music_playing = 0;
	ft_uint32_t wrptr = RAM_G;
	ft_uint32_t rdptr,freebuffspace;
	//pFile = fopen("..\\..\\..\\Test\\SerenadeChopin.raw","rb+");
	pFile = fopen("..\\..\\..\\Test\\Devil_Ride_30_11025hz.raw","rb+");

	fseek(pFile,0,SEEK_END);
	filesz = ftell(pFile);

	fseek(pFile,0,SEEK_SET);

	pBuff = malloc(totalbufflen);

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));//set the background to white
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));//black color text

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), FT_DispHeight/2, 27, OPT_CENTER, "Now you will hear the music");
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_GPU_DLSwap(DLSWAP_FRAME);

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
		wrptr = wrptr % (RAM_G + totalbufflen);

		filesz -= currreadlen;

		if (music_playing == 0)
		{
			Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,RAM_G);//Audio playback start address
			Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,totalbufflen);//Length of raw data buffer in bytes
			Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,LINEAR_SAMPLES);//Current sampling frequency
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,1);
			Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);
			Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);
		    music_playing = 1;
		}

		{
			//Check the freespace if the file has more
			do
	     	{
				ft_uint32_t fullness = 0;
				rdptr = Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR);
				fullness = (wrptr - rdptr) % totalbufflen;
				freebuffspace = totalbufflen - fullness;
			}while (freebuffspace < chunksize);

			//if the file is sent over and there is one more chunk size free space.
			if (filesz ==0)
			{
				//Clear the chunksize to make mute sound.
				Ft_Gpu_CoCmd_MemSet(phost,wrptr,0,chunksize);
				Ft_App_Flush_Co_Buffer(phost);
				while(Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ) != Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE));
			}
		}
	}

	//if read pointer is already passed over write pointer
	if (Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr) {
		//wait till the read pointer will be wrapped over
		while(Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) > wrptr);
	}

	//wait till read pointer pass through write pointer
	while (Ft_Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) < wrptr);


	//The file is done , mute the sound first.
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
	Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,0);//Length of raw data buffer in bytes
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,0);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);

	fclose(pFile);
	free(pBuff);
}
#endif
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
/* API to demonstrate music playback */
ft_void_t SAMAPP_Aud_Music_Player()
{
	FILE *pFile = NULL;
	ft_int32_t filesz,freebuffspace = 100*1024,chunksize = 10*1024,totalbufflen = 100*1024,currreadlen;
	ft_uint8_t *pBuff = NULL,loopflag = 1;
	pFile = fopen("..\\..\\..\\Test\\Devil_Ride_30_11025hz.raw","rb+");
	fseek(pFile,0,SEEK_END);
	filesz = ftell(pFile);

	fseek(pFile,0,SEEK_SET);

	/* Ideally allocate memory wrt sampling frequency and amount of buffering */
	pBuff = malloc(chunksize);

	while(filesz > 0)
	{
		currreadlen = filesz;

		if(currreadlen > chunksize)
		{
			currreadlen = chunksize;
		}
		fread(pBuff,1,currreadlen,pFile);
		Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + (totalbufflen - freebuffspace),(ft_uint8_t *)pBuff,chunksize);
		filesz -= chunksize;
		freebuffspace -= chunksize;
		if(freebuffspace <= 0)
		{
			break;
		}
	}

	/* check if single shot or not */
	if(filesz <= 0)
	{
		loopflag = 0;

	}
	Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,RAM_G);//Audio playback start address in sram
	Ft_Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,(totalbufflen - freebuffspace));//Length of raw data buffer in bytes
	Ft_Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Current read address
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,LINEAR_SAMPLES);//Current sampling frequency
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);//Audio playback format
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_PB,255);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);

	fclose(pFile);
	free(pBuff);
}
#endif
float lerp(float t, float a, float b)
{
	return (float)((1 - t) * a + t * b);
}
float smoothlerp(float t, float a, float b)
{

    float lt = 3 * t * t - 2 * t * t * t;

    return lerp(lt, a, b);
}

#ifdef SAMAPP_ENABLE_APIS_SET0
/* First draw points followed by lines to create 3d ball kind of effect */
ft_void_t SAMAPP_GPU_Ball_Stencil()
{
	ft_int16_t xball = (FT_DispWidth/2),yball = 120,rball = (FT_DispWidth/8);
	ft_int16_t numpoints = 6,numlines = 8,i,asize,aradius,gridsize = 20;
	ft_int32_t asmooth,loopflag = 1,dispr = (FT_DispWidth - 10),displ = 10,dispa = 10,dispb = (FT_DispHeight - 10),xflag = 1,yflag = 1;

	dispr -= ((dispr - displ)%gridsize);
	dispb -= ((dispb - dispa)%gridsize);
	/* write the play sound */
	Ft_Gpu_Hal_Wr16(phost, REG_SOUND,0x50);
	Ft_Gpu_Hal_Wr8(phost,REG_VOL_SOUND,100);
        loopflag = 100;
	while(loopflag-- >0 )
	{
		if(((xball + rball + 2) >= dispr) || ((xball - rball - 2) <= displ))
		{
			xflag ^= 1;
			Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);
		}
		if(((yball + rball + 8) >= dispb) || ((yball - rball - 8) <= dispa))
		{
			yflag ^= 1;
			Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);
		}
		if(xflag)
		{
			xball += 2;
		}
		else
		{
			xball -= 2;
		}
		if(yflag)
		{
			yball += 8 ;
		}
		else
		{
			yball -= 8;
		}


		Ft_App_WrDlCmd_Buffer(phost, CLEAR_COLOR_RGB(128, 128, 0) );
		Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(INCR,INCR) );
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 0, 0) );
		/* draw grid */
		Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(16));
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINES));
		for(i=0;i<=((dispr - displ)/gridsize);i++)
		{
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((displ + i*gridsize)*16,dispa*16));
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((displ + i*gridsize)*16,dispb*16));
		}
		for(i=0;i<=((dispb - dispa)/gridsize);i++)
		{
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(displ*16,(dispa + i*gridsize)*16));
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(dispr*16,(dispa + i*gridsize)*16));
		}
		Ft_App_WrDlCmd_Buffer(phost, END());
		Ft_App_WrDlCmd_Buffer(phost, COLOR_MASK(0,0,0,0) );//mask all the colors
		Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(rball*16) );
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(xball*16,yball*16));
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(INCR,ZERO) );
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(GEQUAL,1,255));
		/* one side points */

		for(i=1;i<=numpoints;i++)
		{
			asize = i*rball*2/(numpoints + 1);
			asmooth = (ft_int16_t)smoothlerp((float)((float)(asize)/(2*(float)rball)),0,2*(float)rball);

			if(asmooth > rball)
			{
				//change the offset to -ve
				ft_int32_t tempsmooth;
				tempsmooth = asmooth - rball;
				aradius = (rball*rball + tempsmooth*tempsmooth)/(2*tempsmooth);
				Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(aradius*16) );
				Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball - aradius + tempsmooth)*16,yball*16));
			}
			else
			{
				ft_int32_t tempsmooth;
				tempsmooth = rball - asmooth;
				aradius = (rball*rball + tempsmooth*tempsmooth)/(2*tempsmooth);
				Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(aradius*16) );
				Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball+ aradius - tempsmooth)*16,yball*16));
			}
		}



		Ft_App_WrDlCmd_Buffer(phost, END());
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(LINES));
		/* draw lines - line should be at least radius diameter */
		for(i=1;i<=numlines;i++)
		{
			asize = (i*rball*2/numlines);
			asmooth = (ft_int16_t)smoothlerp((float)((float)(asize)/(2*(float)rball)),0,2*(float)rball);
			Ft_App_WrDlCmd_Buffer(phost, LINE_WIDTH(asmooth * 16));
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball - rball)*16,(yball - rball )*16));
			Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball + rball)*16,(yball - rball )*16));
		}
		Ft_App_WrDlCmd_Buffer(phost, END());

		Ft_App_WrDlCmd_Buffer(phost, COLOR_MASK(1,1,1,1) );//enable all the colors
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(ALWAYS,1,255));
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 255, 255) );
		Ft_App_WrDlCmd_Buffer(phost, POINT_SIZE(rball*16) );
		Ft_App_WrDlCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball - 1)*16,(yball - 1)*16));
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(0, 0, 0) );//shadow
		Ft_App_WrDlCmd_Buffer(phost, COLOR_A(160) );
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F((xball+16)*16,(yball+8)*16));
		Ft_App_WrDlCmd_Buffer(phost, COLOR_A(255) );
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 255, 255) );
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(xball*16,yball*16));
		Ft_App_WrDlCmd_Buffer(phost, COLOR_RGB(255, 0, 0) );
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_FUNC(GEQUAL,1,1));
		Ft_App_WrDlCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
		Ft_App_WrDlCmd_Buffer(phost, VERTEX2F(xball*16,yball*16));

		Ft_App_WrDlCmd_Buffer(phost, END());

		Ft_App_WrDlCmd_Buffer(phost,DISPLAY());

		/* Download the DL into DL RAM */
		Ft_App_Flush_DL_Buffer(phost);

		/* Do a swap */
		SAMAPP_GPU_DLSwap(DLSWAP_FRAME);

		Ft_Gpu_Hal_Sleep(30);

	}

}
#endif

#ifdef SAMAPP_ENABLE_APIS_SET1
/* API to explain the usage of touch engine */
#if defined(FT_801_ENABLE) || defined(FT_811_ENABLE)

ft_void_t SAMAPP_Touch()
{
	ft_int32_t LoopFlag = 0,wbutton,hbutton,tagval,tagoption;
	ft_char8_t StringArray[100],StringArray1[100];
	ft_uint32_t ReadWord;
	ft_int16_t xvalue,yvalue,pendown;


	/*************************************************************************/
	/* Below code demonstrates the usage of touch function. Display info     */
	/* touch raw, touch screen, touch tag, raw adc and resistance values     */
	/*************************************************************************/
	LoopFlag = 300;
	wbutton = FT_DispWidth/8;
	hbutton = FT_DispHeight/8;
	Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
	while(LoopFlag--)
	{

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		StringArray[0] = '\0';
		strcat(StringArray,"Touch Screen XY0 (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_CTOUCH_TOUCH0_XY);
		/*yvalue = (ft_uint16_t)(ReadWord & 0xffff);
		xvalue = (ft_uint16_t)((ReadWord>>16) & 0xffff);*/
  		yvalue = (ReadWord & 0xffff);
		xvalue = (ReadWord>>16);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 50, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch Screen XY1 (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_CTOUCH_TOUCH1_XY);
  		yvalue = (ReadWord & 0xffff);
		xvalue = (ReadWord>>16);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 70, 26, OPT_CENTER, StringArray);


		StringArray[0] = '\0';
		strcat(StringArray,"Touch Screen XY2 (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_CTOUCH_TOUCH2_XY);
  		yvalue = (ReadWord & 0xffff);
		xvalue = (ReadWord>>16);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 90, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch Screen XY3 (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_CTOUCH_TOUCH3_XY);
  		yvalue = (ReadWord & 0xffff);
		xvalue = (ReadWord>>16);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 110, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
			StringArray1[0] = '\0';
		strcat(StringArray,"Touch Screen XY4 (");
		xvalue = Ft_Gpu_Hal_Rd16(phost, REG_CTOUCH_TOUCH4_X);
		yvalue = Ft_Gpu_Hal_Rd16(phost, REG_CTOUCH_TOUCH4_Y);


		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray1,(ft_int32_t)yvalue);
		strcat(StringArray1,")");
		strcat(StringArray,StringArray1);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 130, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch TAG (");
		ReadWord = Ft_Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,ReadWord);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 170, 26, OPT_CENTER, StringArray);
		tagval = ReadWord;


		Ft_Gpu_CoCmd_FgColor(phost,0x008000);
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));

		Ft_App_WrCoCmd_Buffer(phost,TAG(13));
		tagoption = 0;
		if(13 == tagval)
		{
			tagoption = OPT_FLAT;
		}
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth/2)- (wbutton/2) ,(FT_DispHeight*3/4) - (hbutton/2),wbutton,hbutton,26,tagoption,"Tag13");

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);

		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		Ft_Gpu_Hal_Sleep(30);

	}

		Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_COMPATIBILITY);
		Ft_Gpu_Hal_Sleep(30);
}
#else

ft_void_t SAMAPP_Touch()
{
ft_int32_t LoopFlag = 0,wbutton,hbutton,tagval,tagoption;
ft_char8_t StringArray[100];
ft_uint32_t ReadWord;
ft_uint16_t xvalue,yvalue,pendown;

	/*************************************************************************/
	/* Below code demonstrates the usage of touch function. Display info     */
	/* touch raw, touch screen, touch tag, raw adc and resistance values     */
	/*************************************************************************/
	LoopFlag = 300;
	wbutton = FT_DispWidth/8;
	hbutton = FT_DispHeight/8;
	while(LoopFlag--)
	{

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
	/* Draw informative text at width/2,20 location */
	StringArray[0] = '\0';
	strcat(StringArray,"Touch Raw XY (");
	ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RAW_XY);
	yvalue = (ft_uint16_t)(ReadWord & 0xffff);
	xvalue = (ft_uint16_t)((ReadWord>>16) & 0xffff);
	Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_uint32_t)xvalue);
	strcat(StringArray,",");
	Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_uint32_t)yvalue);
	strcat(StringArray,")");
	Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 10, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch RZ (");
		ReadWord = Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RZ);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,ReadWord);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 25, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch Screen XY (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_SCREEN_XY);
		yvalue = (ft_int16_t)(ReadWord & 0xffff);
		xvalue = (ft_int16_t)((ReadWord>>16) & 0xffff);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 40, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch TAG (");
		ReadWord = Ft_Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,ReadWord);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 55, 26, OPT_CENTER, StringArray);
		tagval = ReadWord;
		StringArray[0] = '\0';
		strcat(StringArray,"Touch Direct XY (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_DIRECT_XY);
		yvalue = (ft_int16_t)(ReadWord & 0x03ff);
		xvalue = (ft_int16_t)((ReadWord>>16) & 0x03ff);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		pendown = (ft_int16_t)((ReadWord>>31) & 0x01);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)pendown);
		strcat(StringArray,")");
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 70, 26, OPT_CENTER, StringArray);

		StringArray[0] = '\0';
		strcat(StringArray,"Touch Direct Z1Z2 (");
		ReadWord = Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_DIRECT_Z1Z2);
		yvalue = (ft_int16_t)(ReadWord & 0x03ff);
		xvalue = (ft_int16_t)((ReadWord>>16) & 0x03ff);
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)xvalue);
		strcat(StringArray,",");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)yvalue);
		strcat(StringArray,")");

		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2, 85, 26, OPT_CENTER, StringArray);

		Ft_Gpu_CoCmd_FgColor(phost,0x008000);
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		tagoption = 0;
		if(12 == tagval)
		{
			tagoption = OPT_FLAT;
		}

		Ft_App_WrCoCmd_Buffer(phost,TAG(12));
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth/4) - (wbutton/2),(FT_DispHeight*2/4) - (hbutton/2),wbutton,hbutton,26,tagoption,"Tag12");
		Ft_App_WrCoCmd_Buffer(phost,TAG(13));
		tagoption = 0;
		if(13 == tagval)
		{
			tagoption = OPT_FLAT;
		}
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth*3/4) - (wbutton/2),(FT_DispHeight*3/4) - (hbutton/2),wbutton,hbutton,26,tagoption,"Tag13");

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);

		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		Ft_Gpu_Hal_Sleep(30);

	}
}
#endif
#endif

#ifdef SAMAPP_ENABLE_APIS_SET4
FT_PROGMEM ft_prog_uchar8_t SAMAPP_Snd_Array[5*58] = "Slce\0Sqrq\0Sinw\0Saww\0Triw\0Beep\0Alrm\0Warb\0Crsl\0Pp01\0Pp02\0Pp03\0Pp04\0Pp05\0Pp06\0Pp07\0Pp08\0Pp09\0Pp10\0Pp11\0Pp12\0Pp13\0Pp14\0Pp15\0Pp16\0DMF#\0DMF*\0DMF0\0DMF1\0DMF2\0DMF3\0DMF4\0DMF5\0DMF6\0DMF7\0DMF8\0DMF9\0Harp\0Xyph\0Tuba\0Glok\0Orgn\0Trmp\0Pian\0Chim\0MBox\0Bell\0Clck\0Swth\0Cowb\0Noth\0Hiht\0Kick\0Pop\0Clak\0Chak\0Mute\0uMut\0";

FT_PROGMEM ft_prog_uchar8_t SAMAPP_Snd_TagArray[58] = {
	0x63,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x23,0x2a,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,
	0x60,0x61
};


/* API to demonstrate the usage of sound engine of FT800 */
ft_void_t SAMAPP_Sound()
{
	ft_int32_t LoopFlag = 0,wbutton,hbutton,tagval,tagvalsnd = -1,numbtnrow,numbtncol,i,j,prevtag = -1;
	ft_uint32_t freqtrack = 0,currfreq = 0,prevcurrfreq;
	FT_PROGMEM ft_prog_uchar8_t *pString;
	FT_PROGMEM ft_prog_uchar8_t *pTagArray;
	ft_char8_t StringArray[8] = {0};
	/*************************************************************************/
	/* Below code demonstrates the usage of sound function. All the supported*/
	/* sounds and respective pitches are put as part of keys/buttons, by     */
	/* choosing particular key/button the sound is played                    */
	/*************************************************************************/
	LoopFlag = 100;
	numbtnrow = 7 /*16*/;//number of rows to be created - note that mute and unmute are not played in this application
	numbtncol = 8 /*13*/;//number of colomns to be created
	wbutton = (FT_DispWidth - 40)/numbtncol;
	hbutton = FT_DispHeight/numbtnrow;

	/* set the volume to maximum */
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,0xFF);
	/* set the tracker to track the slider for frequency */

	Ft_Gpu_CoCmd_Track(phost, FT_DispWidth - 15, 20, 8, (FT_DispHeight - 40), 100);
	Ft_App_Flush_Co_Buffer(phost);
	while(LoopFlag--)
	{
		tagval = Ft_Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
		freqtrack = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER);

		if(100 == (freqtrack & 0xff))
		{
			currfreq = (ft_uint32_t)(freqtrack >> 16);
			currfreq = (ft_uint32_t)(88*currfreq)/65536;
			if(currfreq > 108)
				currfreq = 108;
		}
		if((tagval > 0))
		{
			if( tagval <= 99)
			{
				tagvalsnd = tagval;
			}
			if(0x63 == tagvalsnd)
			{
				tagvalsnd = 0;
			}
			if((prevtag != tagval) || (prevcurrfreq != currfreq))
			{
				/* Play sound wrt pitch */
				Ft_Gpu_Hal_Wr16(phost, REG_SOUND,(((currfreq + 21) << 8) | tagvalsnd));
				Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);
			}
			if(0 == tagvalsnd)
				tagvalsnd = 99;
		}
		/* start a new display list for construction of screen */

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(64,64,64));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		/* line width for the rectangle */
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));

		/* custom keys for sound input */
		pString = SAMAPP_Snd_Array;
		pTagArray = SAMAPP_Snd_TagArray;
		/* First draw all the rectangles followed by the font */
		/* yellow color for background color */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x80,0x00));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
		for(i=0;i<numbtnrow;i++)
		{
			for(j=0;j<numbtncol;j++)
			{
				Ft_App_WrCoCmd_Buffer(phost,TAG(ft_pgm_read_byte(pTagArray)));
				Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((j*wbutton + 2)*16,(hbutton*i + 2)*16));
				Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(((j*wbutton) + wbutton - 2)*16,((hbutton*i) + hbutton - 2)*16));
				pTagArray++;
			}
		}
		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		/* draw the highlight rectangle and text info */
		pTagArray = SAMAPP_Snd_TagArray;
		pString = SAMAPP_Snd_Array;
		for(i=0;i<numbtnrow;i++)
		{
			for(j=0;j<numbtncol;j++)
			{
				Ft_App_WrCoCmd_Buffer(phost,TAG(ft_pgm_read_byte(pTagArray)));
				if(tagvalsnd == ft_pgm_read_byte(pTagArray))
				{
					/* red color for highlight effect */
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0x80,0x00,0x00));
					Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
					Ft_App_WrCoCmd_Buffer(phost,TAG(ft_pgm_read_byte(pTagArray)));
					Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((j*wbutton + 2)*16,(hbutton*i + 2)*16));
					Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(((j*wbutton) + wbutton - 2)*16,((hbutton*i) + hbutton - 2)*16));
					Ft_App_WrCoCmd_Buffer(phost,END());
					/* reset the color to make sure font doesnt get impacted */
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
				}
				/* to make sure that highlight rectangle as well as font to take the same tag values */

#ifdef ARDUINO_PLATFORM
                                strcpy_P(StringArray,(const prog_char*)pString);
#endif
#ifdef FT900_PLATFORM
								{
								    /* Copy data from flash to RAM */
									ft_int32_t idx = 0,slen;
									ft_char8_t *pstring,tempchar;
                                	//strcpy(StringArray,pString);
									//slen = strlen(StringArray);
									pstring = StringArray;
									do
									{
									    tempchar = ft_pgm_read_byte_near(&pString[idx]);
										*pstring++ = tempchar;
										idx++;
									}while(tempchar);
								}
#endif
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
                                strcpy(StringArray,pString);
#endif
                                Ft_Gpu_CoCmd_Text(phost,(wbutton/2) + j*wbutton,(hbutton/2) + hbutton*i,26,OPT_CENTER,StringArray);

                                pString += (strlen(StringArray) + 1);
				pTagArray++;
			}
		}

		/* Draw vertical slider bar for frequency control */
		StringArray[0] = '\0';
		strcat(StringArray,"Pt ");
		Ft_Gpu_Hal_Dec2Ascii(StringArray,(ft_int32_t)(currfreq + 21));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth - 20,10,26,OPT_CENTER,StringArray);
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(100));
		Ft_Gpu_CoCmd_Slider(phost, FT_DispWidth - 15, 20, 8, (FT_DispHeight - 40), 0, currfreq, 88);

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		prevtag = tagval;

		prevcurrfreq = currfreq;
		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		Ft_Gpu_Hal_Sleep(10);
	}

	Ft_Gpu_Hal_Wr16(phost, REG_SOUND,0);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);
}
#endif



ft_void_t SAMAPP_BootupConfig()
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
    ft_delay(120);

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



#ifdef SAMAPP_ENABLE_APIS_SET5
ft_void_t SAMAPP_ChineseFont()
{

	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + 1000, SAMApp_ChineseFont_MetricBlock, SAMAPP_ChineseFont_Metric_SIZE);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + 1000 + SAMAPP_ChineseFont_Metric_SIZE,SAMApp_ChineseFont_FontBmpData,SAMAPP_ChineseFont_BitmapData_SIZE);


	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0xff,0xff,0xff));//set the background to white
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));//black color text

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 20, 27, OPT_CENTER, "FangSong Font L8 Traditional Chinese");
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));//black color text
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(196));

	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,28,34));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,28,34));

	Ft_Gpu_CoCmd_SetFont(phost, 7, RAM_G + 1000);

	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80, 7, OPT_CENTER, "\x01\x02\x03\x04\x05");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80 + 34, 7, OPT_CENTER, "\x06\x07\x08\x09\x0A");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80 + 34 + 34, 7, OPT_CENTER, "\x0B\x0C\x0D\x0E\x0F");
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth/2), 80 + 34 + 34 + 34, 7, OPT_CENTER, "\x10\x11\x12\x13\x14");

	Ft_Gpu_CoCmd_Button(phost, (FT_DispWidth/2), 80 + 34 + 34 + 34 + 34, 80,34,7,0, "\x15\x16");


	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	Ft_Gpu_Hal_Sleep(3000);//timeout for snapshot to be performed by coprocessor
}

#endif


#ifdef SAMAPP_ENABLE_APIS_SET6
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
/* Beginning MainWindow section */



FT_PROGMEM t_imageprp Main_Icons[1] =
{
     {"..\\..\\..\\Test\\tile3.bin",ImH,RGB565,NEAREST,ImW,ImH,ImW*2,0L},
};

  void load_inflate_image(ft_uint32_t address, const char *filename)
{
	FILE *afile;
	ft_uint32_t ftsize=0;
	ft_uint8_t pbuff[8192];
	ft_uint16_t blocklen;

	Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
	Ft_Gpu_Hal_WrCmd32(phost,address*1024L);
	afile = fopen(filename,"rb");		// read Binary (rb)
	fseek(afile,0,SEEK_END);
	ftsize = ftell(afile);
	fseek(afile,0,SEEK_SET);
	while(ftsize>0)
	{
		blocklen = ftsize>8192?8192:ftsize;
		fread(pbuff,1,blocklen,afile);							/* copy the data into pbuff and then transfter it to command buffer */
		ftsize -= blocklen;
		Ft_Gpu_Hal_WrCmdBuf(phost,pbuff,blocklen);				/* copy data continuously into command memory */
	}
	fclose(afile);												/* close the opened jpg file */
}


void Logo_Intial_setup(struct logoim sptr[],ft_uint8_t num)
{
	ft_uint8_t z;
   for(z=0;z<num;z++)
   {
     load_inflate_image(sptr[z].gram_address,sptr[z].name);
   }

   Ft_Gpu_CoCmd_Dlstart(phost);        // start
   Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
   for(z=0;z<num;z++)
   {
			 Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(z));
			 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(sptr[z].gram_address*1024L));
			 Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(sptr[z].image_format,sptr[z].linestride,sptr[z].image_height));
			 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(sptr[z].filter,BORDER, BORDER,480,272));

   }
   Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
   Ft_Gpu_CoCmd_Swap(phost);
   Ft_App_Flush_Co_Buffer(phost);
   Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}

void Load_file2gram(ft_uint32_t add,ft_uint8_t sectors, FILE *afile)
{
  ft_uint8_t pbuff[512],temp[512],tval;
  ft_uint16_t z = 0;
  for(z=0;z<sectors;z++)
  {
	fread(pbuff,1,512,afile);
	Ft_Gpu_Hal_WrMem(phost,add,pbuff,512L);
	add+=512;
  }
}



ft_int32_t RowNo[5],ColNo[5];
ft_int32_t Volume;
ft_void_t CheckTouch_tile(Squares *Sq, ft_int32_t TouchXY,ft_uint8_t TouchNo)
{
	ft_uint8_t i=0;

	if((TouchXY >> 16) != -32768)
	{
		Sq->x = TouchXY>>16;
		Sq->y =  (TouchXY & 0xffff);
		Volume  = (TouchNo+1)*51;
		for(i =0;i<7;i++)
		{
			/* find row number*/
			if((Sq->y > i*(ImH+2)) && (Sq->y < (i+1)*(ImH+2)))
				RowNo[TouchNo] = i;
			if(((Sq->x) > (i*(ImW+2)))  && ((Sq->x)< ((i+1)*(ImW+2))))
				ColNo[TouchNo] =i;
		}
	}
	else
	{
		RowNo[TouchNo] = -1000;
		ColNo[TouchNo] =-1000;
	}
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));

	 Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(((ImW+2)*ColNo[TouchNo])*16,((ImH+2)*RowNo[TouchNo])*16));

}

ft_void_t MainWindow()
{
		ft_uint8_t i=0,j,TouchNo=0;
		ft_uint32_t val[6];

		FILE *afile;
		ft_int32_t ftsize=0,AddrOffset;


		ft_int32_t rp=0,audioval,wp=0;

		Squares SqCall,*Sq;
		Sq = &SqCall;

		Logo_Intial_setup(Main_Icons,1);
		Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
		Ft_Gpu_Hal_Sleep(30);
		AddrOffset = 102400L;

		/*Audio*/

		afile = fopen("..\\..\\..\\Test\\Devil_Ride_30_44100_ulaw.wav","rb");		// read Binary (rb)

		fseek(afile,0,SEEK_END);
		ftsize = ftell(afile);
		fseek(afile,0,SEEK_SET);

		Load_file2gram(AddrOffset+0UL,64*2,afile);

		wp = 1024;
		ftsize-=1024;


		Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_FREQ,44100);
		Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_START,AddrOffset);
		Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_FORMAT,ULAW_SAMPLES);
		Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_LENGTH,APPBUFFERSIZE);
		Ft_Gpu_Hal_Wr32(phost,REG_PLAYBACK_LOOP,1);
		Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB, Volume);
		Ft_Gpu_Hal_Wr8(phost,REG_PLAYBACK_PLAY,1);


		//while(ftsize>0)
		for(j=0;j<150;j++){
			Ft_App_WrCoCmd_Buffer(phost,CMD_DLSTART);
			Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0,0,0));
			Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

			val[0] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);
			val[1] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);
			val[2] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);
			val[3] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);
			val[4] = (Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_X)<<16) | (Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_Y));

			for(i=0; i<NO_OF_TOUCH; i++)
			{
					 CheckTouch_tile(&SqCall,(ft_int32_t)val[i],i);
			}
			if((val[0] ==  2147516416) && (val[1] == 2147516416) && (val[2] == 2147516416) && (val[3] == 2147516416) && (val[4] == 2147516416) )
				Volume =0;
			  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
			  Ft_Gpu_CoCmd_Swap(phost);
			Ft_App_Flush_Co_Buffer(phost);

			rp = Ft_Gpu_Hal_Rd16(phost,REG_PLAYBACK_READPTR);
			audioval = APPBUFFERSIZEMINUSONE & (rp-wp);
			if (audioval > 1024)
			{
				  ft_uint16_t n = min(1024,ftsize);
				  Load_file2gram(AddrOffset+wp,2,afile);
				  wp = (wp +1024) & APPBUFFERSIZEMINUSONE;
				  ftsize-=n;
				  Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB, Volume);

			}
		}
		Ft_Gpu_Hal_Wr8(phost,REG_VOL_PB,0);
		Ft_Gpu_Hal_Wr8(phost,REG_PLAYBACK_PLAY,0);
fclose(afile);
}


/* End MainWindow section */
#endif
#endif

#ifdef SAMAPP_ENABLE_APIS_SET7
/* Beginning BouncingSquares section */



ft_void_t BouncingSquaresCall(ft_int16_t BRx,ft_int16_t BRy,ft_int16_t MovingRy,ft_uint8_t SqNumber)
{
	 ft_int16_t MovingRx,leap=0;

	 ft_int16_t R1,G1,B_1,R2,G2,B2;
	  MovingRx = BRx;

	  if(BRy<=60)BRy=60;
	  if(BRy >= 260)BRy = 260;

	  //different colours are set for the different rectangles
	  if(SqNumber==0){

		  R1 = 63; G1 =72;  B_1=204;
		  R2 = 0;  G2= 255; B2 = 255;
	  }

	   if(SqNumber==1)
	  {
		  R1 = 255; G1 =255;  B_1=0;
		  R2 = 246;  G2= 89; B2 = 12;
	  }

	   if(SqNumber==2)
	  {
		  R1 = 255; G1 =0;  B_1=0;
		  R2 = 237;  G2= 28; B2 = 36;
	  }

	   if(SqNumber==3)
	  {
		  R1 = 131; G1 = 171;  B_1=9;
		  R2 = 8;  G2= 145; B2 = 76;
	  }

	   if(SqNumber == 4)
	   {
		   R1 = 141; G1 = 4;  B_1=143;
		  R2 = 176;  G2= 3; B2 = 89;
	   }

	// Draw the rectanles here
	  Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS) );
	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(R1, G1, B_1) );
	  Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(10 * 16) );


	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(BRx * 16, (BRy) * 16) );
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((BRx+45) * 16, (260) * 16) );


	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(R2, G2, B2) );
	  Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(5 * 16) );

	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(MovingRx * 16, (MovingRy) * 16) );
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((MovingRx+45) * 16, (MovingRy+5) * 16) );
}


ft_int16_t MovingRect(ft_int16_t BRy,ft_int16_t MovingRy,ft_uint8_t EndPtReach)
{
		if(MovingRy <=0)
		{
			EndPtReach =0;
			MovingRy =1;
		}

		 if(EndPtReach == 1&& MovingRy >0)
			  MovingRy-=1;//the smaller rectangles are moved behind
		 else	  if((EndPtReach == 0) )
			  MovingRy+=2;//the smaller rectangles are moved forward slightly faster
		 return MovingRy;
}


ft_void_t RectangleCalc(BouncingSquares_Context *context,ft_uint8_t Arrayno)
{
	ft_uint8_t Arr;
	ft_int16_t MovingRy1,leap=0;

	if(context->RectNo[Arrayno]  ==1)
	{
			Arr = Arrayno;
			//the limits for the smaller rectangles forward and backward movement is set here
			if(context->My[Arr]  == 0 && (context->My[Arr] +25) < context->BRy[Arr])
				context->E[Arr]=0;//inc
			else if(context->My[Arr]+25 >= context->BRy[Arr])
				context->E[Arr]=1;//dec

			// the smaller rectangles are moved accordingly according to the flags set above ion this function call
			MovingRy1 = MovingRect(context->BRy[Arr],context->My[Arr],context->E[Arr]);

			if(context->BRy[Arr] == 0)MovingRy1 = 4;
				context->My[Arr] = MovingRy1;

			if(context->My[Arr] > (context->BRy[Arr]-15))
			{
				leap = context->My[Arr] - context->BRy[Arr];
				context->My[Arr] = context->My[Arr] - (leap+25);
			}
	}
}

ft_void_t CheckTouch(BouncingSquares_Context *context,ft_int16_t Tx1,ft_int32_t val1)
{
	ft_int16_t MovingRy1,i=0;
	ft_uint8_t Arrayno=-1;

		// Check which rectangle is being touched according to the coordinates
		if(Tx1 >= 60 && Tx1 <= 105)
			Arrayno =0;
		if(Tx1 >= 140 && Tx1 <= 185)
			Arrayno = 1;
		if(Tx1 >= 220 && Tx1 <= 265)
			Arrayno = 2;
		if(Tx1 >= 300 && Tx1 <= 345)
			Arrayno = 3;
		if(Tx1 >= 380 && Tx1 <= 425)
			Arrayno=4;

		//Set the flag for the rectangle being touched
			if(Arrayno < 255){
			context->RectNo[Arrayno] =1;

			//store the vertices of the rectangle selected according to the flag
			if((val1>>16) != -32768)
			{
				context->BRx[Arrayno] = (val1>>16)&0xffff;
				context->BRy[Arrayno] = (val1 & 0xffff);
			}

			//limit the Bigger rectangle's height
			if(context->BRy[Arrayno]<=60) context->BRy[Arrayno]=60;
		}

		//According to the bigger rectangle values move the smaller rectangles
		for( i=0;i<NO_OF_RECTS;i++)
		{
			RectangleCalc(context, i);
		}
}



ft_void_t BouncingSquares()
{
	ft_uint8_t i=0;
	ft_int16_t RectX[5],leap=0;
	ft_int32_t val[5];
	ft_uint32_t k;

	BouncingSquares_Context context;

	//Calculate the X vertices where the five rectangles have to be placed

	for(i =1;i<5;i++)
	{
		RectX[0] = 60;
		RectX[i] = RectX[i-1] + 80;
	}

	for(i=0;i<5;i++){
		context.BRy[i] = 0;
		context.My[i] = 0;
		context.RectNo[i]=0;
		context.E[i]=0;
	}

	Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
	Ft_Gpu_Hal_Sleep(30);
	  for(k=0;k<150;k++){

			 /* first touch*/
			 val[0] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);

			 /*second touch*/
			 val[1] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);

			 /*third touch*/
			 val[2] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);

			/*fourth  touch*/
			val[3] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);

			/*fifth  touch*/
			val[4] = ((ft_uint32_t)Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_X)<<16L) | (Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_Y) &0xffffL);



	  Ft_Gpu_CoCmd_Dlstart(phost);
			  Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0,0,0));
			  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));


			//Check which rectangle is being touched using the coordinates and move the respective smaller rectangle
			for(i =0;i<NO_OF_RECTS;i++)
			{
				CheckTouch(&context, (val[i]>>16)&0xffffL,val[i]);
				BouncingSquaresCall(RectX[i],context.BRy[i],context.My[i],i);
			}

			Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
			Ft_Gpu_CoCmd_Swap(phost);
			Ft_App_Flush_Co_Buffer(phost);
			Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
			context.Count++;

	  }
}

/* End BouncingSquares section */


/* Beginning BouncingCircle section */


ft_uint16_t ConcentricCircles(float C1,ft_uint16_t R,ft_uint16_t G,ft_uint16_t B)
{
		ft_int16_t C1X=0,Total=0;

		C1X = C1;

		Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_A(0));

		/* Using ColorMask to disable color buffer updates, and
		set the BlendFunc to a value that writes 	incoming alpha
		directly into the alpha buffer, by specifying a source blend factor
		of ONE */

		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(ONE, ONE_MINUS_SRC_ALPHA));

		/*Draw the Outer circle */

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((ft_uint16_t)(C1) * 16) );		//outer circle
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(240,136,0,0));

		/* Draw the inner circle in a blend mode that clears any drawn
		pixels to zero, so the source blend factor is ZERO */
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(ZERO, ONE_MINUS_SRC_ALPHA));

		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((ft_uint16_t)(C1-2) * 16) );//inner circle
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(240,136,0,0));

		/*Enable the color Mask and the source blend factor is set to DST ALPHA, so the
	transparency values come from the alpha buffer*/

		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA, ONE));

		/* draw the outer circle again with the color mask enabled andd the blend factor
		is set to SRC_ALPHA */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(R ,G ,B));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((ft_uint16_t)(C1) * 16) );
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(240,136,0,0));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA));

		Ft_App_WrCoCmd_Buffer(phost,END());
}

ft_void_t TouchPoints(ft_int16_t C1X,ft_int16_t C1Y,ft_uint8_t i)
{
	/* Draw the five white circles for the Touch areas with their rescpective numbers*/
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((14) * 16) );
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255 ,255 ,255));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(C1X,C1Y,0,0));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(155 ,155 ,0));
	Ft_Gpu_CoCmd_Number(phost,C1X,C1Y,29,OPT_CENTERX|OPT_CENTERY,i);
}

ft_void_t PlotXY()
{

	ft_uint8_t i=0;
	ft_uint16_t PlotHt=0,PlotWth=0,X=0,Y=0;

	PlotHt = FT_DispHeight/10;
	PlotWth = FT_DispWidth/10;

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(36 ,54 ,125));
	//Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0 ,0 ,0));
	/* Horizontal Lines */
	for(i=1;i<11;i++)
	{
		Y = i* PlotHt;
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1 * 16) );
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,Y*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,Y*16));
	}
	/* Vertical Lines */
	for(i=1;i<11;i++)
	{
		X = i* PlotWth;
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1 * 16) );
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(X*16,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(X*16,FT_DispHeight*16));
	}
	Ft_App_WrCoCmd_Buffer(phost,END());
}

ft_uint8_t CheckCircleTouchCood(BouncingCircles_Context *context, ft_int32_t val,ft_uint8_t TouchNum,ft_uint8_t i)
{
	double CX=0;
	double CY=0;

	ft_uint8_t j=0,AllClear=0;

	if((val >> 16) != -32768)
	{
		 CX = (val>>16);
		 CY = (val & 0xffff);

		for(j=0; j<NO_OF_CIRCLE; j++)
		{
				if( context->TN[TouchNum].F[j] == 0)
				{
					if(AllClear!= 10)
					AllClear =j;
				}
				else
					AllClear =10;
		}

		if(AllClear!=10)
			AllClear = 1;

		if(AllClear == 1)
		{
			if( context->TN[TouchNum].F[i] !=1)
			{
					/* check which circle being touched falls according to its coordinates and set its flag*/
					if((CX > (context->C1X[i]-15))&&(CX < (context->C1X[i]+15) ) )
					{
							if((CY > (context->C1Y[i] -30))&&(CY < context->C1Y[i]+30))
							{

								context->C1X[i] = CX;
								context->C1Y[i] = CY;
								context->TN[TouchNum].F[i] =1;
							  }
					  }
			}
			AllClear =0;
		}

		if(context->TN[TouchNum].F[i] == 1)
		{
			context->C1X[i] = CX;
			context->C1Y[i] = CY;
		}
	}

	else
	{
		context->TN[TouchNum].F[i]=0;

	}
}

ft_uint16_t CirclePlot(BouncingCircles_Context *context, ft_uint16_t X,ft_uint16_t Y,ft_uint8_t Val)
{
		double Xsq1[NO_OF_CIRCLE],Ysq1[NO_OF_CIRCLE];
		Xsq1[Val]= (X - (FT_DispWidth/2))*(X- (FT_DispWidth/2));
		Ysq1[Val] = (Y - (FT_DispHeight/2))*(Y - (FT_DispHeight/2));
		context->Tsq1[Val] = (Xsq1[Val] + Ysq1[Val]);
		context->Tsq1[Val] = sqrt(context->Tsq1[Val]);
		return context->Tsq1[Val];
}

ft_void_t StoreTouch(BouncingCircles_Context *context, ft_int32_t Touchval,ft_uint8_t TouchNo)
{
	 if(Touchval >> 16 != -32768)
	 {
			context->TouchX[TouchNo] = Touchval>>16;
			context->TouchY[TouchNo] = Touchval&0xffff;
	 }

}

ft_void_t BouncingCircles()
{

	ft_uint8_t CircleNo[NO_OF_CIRCLE];
	ft_uint8_t i=0,j=0;
	ft_uint16_t k;
	ft_int32_t Touchval[NO_OF_CIRCLE];
	BouncingCircles_Context context;

	Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
	Ft_Gpu_Hal_Sleep(30);
	/* calculate the intital radius of the circles before the touch happens*/
	context.Tsq1[0] = 50; context.C1X[0] = 190; context.C1Y[0] = 136;
	for(i = 1;i<NO_OF_CIRCLE;i++)
	{
		 context.Tsq1[i] = context.Tsq1[i-1] +30;
		 context.C1X[i] = context.C1X[i-1] - 30;
		 context.C1Y[i] = 136;
	}

	 for(k=0;k<150;k++)
	  {
		  Ft_Gpu_CoCmd_Dlstart(phost);
		 // Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
	      Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		  /* values of the five touches are stored here */

		  Touchval[0] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);
		  Touchval[1] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);
		  Touchval[2] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);
		  Touchval[3] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);
		  Touchval[4] = ((ft_int32_t)Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_X)<<16) | (Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_Y));

		  for(i=0;i<NO_OF_CIRCLE;i++)
		 {
				StoreTouch(&context,Touchval[i],i);
		  }
		  /* The plot is drawn here */
		 PlotXY();

		 /* check which circle has been touched based on the coordinates and store the[0] number of the circle touched*/

		 for(i=0;i<NO_OF_CIRCLE;i++)
		 {
			 CheckCircleTouchCood(&context,Touchval[0],0,i);
			 CheckCircleTouchCood(&context,Touchval[1],1,i);
			 CheckCircleTouchCood(&context,Touchval[2],2,i);
			 CheckCircleTouchCood(&context,Touchval[3],3,i);
			 CheckCircleTouchCood(&context,Touchval[4],4,i);
		  }
		/* calculate the radius of each circle according to the touch of each individual circle */

		for(i =0;i<NO_OF_CIRCLE;i++)
		{
			context.Tsq1[i] = CirclePlot(&context,context.C1X[i],context.C1Y[i],i);
		}
		/* with the calculated radius draw the circles as well as the Touch points */

		for(i =0;i<(NO_OF_CIRCLE);i++)
		{
			ConcentricCircles(context.Tsq1[i],255,0,0);
			TouchPoints(context.C1X[i],context.C1Y[i],i+1);
		}

		  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		  Ft_Gpu_CoCmd_Swap(phost);
		  Ft_App_Flush_Co_Buffer(phost);
		  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	  }

}

/* End BouncingCircle section */


/* Beginning BouncingPoints section */



ft_void_t BlobColor(BlobsInst *pBInst,ft_int32_t TouchXY)
{

	ft_uint8_t r,g,b;
	ft_uint8_t i=0,j=0;
	// if there is touch store the values
	if(((TouchXY >> 16) != -32768))
	{
			pBInst->blobs[pBInst->CurrIdx].x = (TouchXY>>16)&0xffff;
			pBInst->blobs[pBInst->CurrIdx].y =  (TouchXY & 0xffff);
	}
	else
	{
			pBInst->blobs[pBInst->CurrIdx].x = OFFSCREEN;
			pBInst->blobs[pBInst->CurrIdx].y = OFFSCREEN;
	}

	//calculate the current index
	pBInst->CurrIdx = (pBInst->CurrIdx + 1) & (NBLOBS - 1);

	Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(60,166,117));
	for ( i = 0; i < NBLOBS; i++)
	{
			// Blobs fade away and swell as they age
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(i<<1));

			Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((68)+(i<<3)));

			// Random color for each blob, keyed from (blob_i + i)
			j = (pBInst->CurrIdx + i) & (NBLOBS - 1);
			/* r = j * 17;
			 g = j * 23;
			 b = j * 147;*/
			//Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(r,g,b));

			// Draw it!
			if(pBInst->blobs[j].x != OFFSCREEN)
				Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((pBInst->blobs[j].x)*16, (pBInst->blobs[j].y)*16));
	}
}



ft_void_t BouncingPoints()
{
	ft_int32_t val[5];
	ft_uint8_t i=0,j=0;
	BlobsInst gBlobsInst[FT_APP_BLOBS_NUMTOUCH],*pBInst;
	ft_uint16_t k;
    Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
	Ft_Gpu_Hal_Sleep(30);
	pBInst = &gBlobsInst[0];

	//set all coordinates to OFFSCREEN position
	for(j=0;j<FT_APP_BLOBS_NUMTOUCH;j++)
	{
		for( i = 0; i < NBLOBS; i++)
		{
			pBInst->blobs[i].x = OFFSCREEN;
			pBInst->blobs[i].y = OFFSCREEN;
		}
		pBInst->CurrIdx=0;
		pBInst++;
	}

	  for(k=0;k<150;k++)
	  {

		  val[0] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);
		  val[1] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);
		  val[2] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);
		  val[3] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);
		  val[4] = (((ft_int32_t)Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_X)<<16) | (Ft_Gpu_Hal_Rd16(phost,REG_CTOUCH_TOUCH4_Y)&0xffff));

		  Ft_App_WrCoCmd_Buffer(phost,CMD_DLSTART);
#if 1
		  Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(43,73,59));
		  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		  //Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		  Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE));
		  Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
#else
		  Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
		  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
#endif

		  // draw blobs according to the number of touches
		  for(j=0;j<FT_APP_BLOBS_NUMTOUCH;j++)
		  {
			  BlobColor(&gBlobsInst[j],(ft_int32_t)val[j]);
		  }

		  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		  Ft_Gpu_CoCmd_Swap(phost);
		  Ft_App_Flush_Co_Buffer(phost);
		  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	  }
}

/* End BouncingPoints section */

/* Beginning MovingPoints section*/

ft_void_t ColorSelection(ft_int16_t k,ft_int16_t i)
{
		if(k ==0)
		{
				 if(i&1)
					 Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(128,0,255));//purple
				else
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,43,149));//pink
		}
		if(k ==1)
		{
				 if(i&1)
					 Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));//red
				else
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,255,0));//green
		}
		if(k == 2)
		{
				 if(i&1)
					 Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,128,64));//orange
				else
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,255,255));//blue
		}
		if(k == 3)
		{
				 if(i&1)
					 Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(128,0,0));//orange
				else
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,128));//blue
		}

}

ft_int16_t linear(float p1,float p2,ft_uint16_t t,ft_uint16_t rate)
{
	float st  = (float)t/rate;
	return (ft_int16_t)(p1+(st*(p2-p1)));
}

ft_void_t PointsCalc(MovingPoints_Context *context, ft_uint8_t TouchNo, ft_int16_t *X, ft_int16_t *Y, ft_uint8_t *t)
{

		ft_int16_t alpha=255,i,k=0,j=0, pointset=0;
		ft_int16_t tempDeltaX, tempDeltaY;

			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
			Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(20*16));
			Ft_App_WrCoCmd_Buffer(phost, COLOR_A(120));

			/* For total number of points calculate the offsets of movement */
			for(k=0;k<NO_OF_POINTS*4L;k++)
			{
				{
					pointset = k/NO_OF_POINTS;
					if((t[k] > NO_OF_POINTS))
					{
					context->t[k]=0;
					context->X[k] = (context->val[pointset]>>16)&0xffff;
					context->Y[k] = (context->val[pointset] & 0xffff);
					}

					ColorSelection(pointset,k);

					if(context->X[k] != -32768)
					{
						tempDeltaX = linear(context->X[k],context->SmallX[pointset], context->t[k],NO_OF_POINTS);
						tempDeltaY  = linear(context->Y[k],context->SmallY, context->t[k],NO_OF_POINTS);
						Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(tempDeltaX*16L,tempDeltaY*16L));
					}
					t[k]++;
				}
			}

}



ft_void_t MovingPoints()
{
	
		ft_uint16_t i=0,j,k,tn=0;
		MovingPoints_Context context;
		context.Flag=1;
		Ft_Gpu_Hal_Wr8(phost,REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
		Ft_Gpu_Hal_Sleep(30);
		/* Initialize all coordinates */
		for(j=0;j<4;j++){
				for(i=0;i<NO_OF_POINTS;i++){
					 context.t[i+j*NO_OF_POINTS] = i;
                     context.X[i+j*NO_OF_POINTS] = -32768;
				}
		}

		context.SmallX[0] = 180; context.SmallY = 20;
		for(i = 0; i<5;i++)
		{
			context.SmallX[i+1] = context.SmallX[i]+50;
		}

#if defined(FT900_PLATFORM)
for (k = 0; k < 800; k++)
#elif defined(ARDUINO_PLATFORM)
for(i=0;i<700;i++)
#else
		for(k=0;k<150;k++)
#endif
		{
	Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));

		context.val[0] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH0_XY);
		context.val[1] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH1_XY);
		context.val[2] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH2_XY);
		context.val[3] = Ft_Gpu_Hal_Rd32(phost,REG_CTOUCH_TOUCH3_XY);


		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));

		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));
		PointsCalc(&context, tn, &context.X[0],&context.Y[0],&context.t[0]);
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	  }
}

/* End MovingPoints section */

#endif


#if defined(FT_81X_ENABLE)
ft_int32_t SAMAPP_LoadRawFromFile(ft_char8_t *fileName, ft_int32_t ramOffset) {
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
ft_int32_t FileLen = 0, FileSz = 0;
ft_uint8_t *pbuff = NULL;
FILE *file = fopen(fileName,"rb");
if(NULL == file)
{
	printf("Error while opening file.\n");
	return 0;
}
else
{
	fseek(file,0,SEEK_END);
	FileSz = FileLen = ftell(file);
	fseek(file,0,SEEK_SET);
	pbuff = (ft_uint8_t *)malloc(8192);
	while(FileLen > 0)
	{
		/* download the data into the command buffer by 2kb one shot */
		ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

		/* copy the data into pbuff and then transfter it to command buffer */
		fread(pbuff,1,blocklen,file);
		FileLen -= blocklen;
		/* copy data continuously into command memory */
		Ft_Gpu_Hal_WrMem(phost,ramOffset,(ft_uint8_t *)pbuff,blocklen);
		ramOffset+=blocklen;
	}
	fclose(file);
	free(pbuff);
}
return FileSz;
#elif defined(FT900_PLATFORM)
#elif defined(ARDUINO_PLATFORM)

#endif
}

ft_void_t NumberBases() {
ft_uint16_t startingY = 150;
Ft_Gpu_CoCmd_Dlstart(phost);
Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 50, 31, OPT_CENTER, "Built-in bases conversion");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 100, 31, OPT_CENTER, "bases from 2 to 32:");

#if DISPLAY_RESOLUTION_WVGA
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 150, 30, 0, "Binary(2):");
	Ft_Gpu_CoCmd_SetBase(phost, 2);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 150, 30, 0, 1000);

Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 200, 30, 0, "Octal(8):");
Ft_Gpu_CoCmd_SetBase(phost, 8);
Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 200, 30, 0, 1000);

Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 250, 30, 0, "Decimal(10):");
Ft_Gpu_CoCmd_SetBase(phost, 10);
Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 250, 30, 0, 1000);

	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 300, 30, 0, "Hex(16):");
	Ft_Gpu_CoCmd_SetBase(phost, 16);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 300, 30, 0, 1000);
#else
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 150, 30, 0, "Binary(2):");
	Ft_Gpu_CoCmd_SetBase(phost, 2);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 150, 30, 0, 1000);

	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 180, 30, 0, "Octal(8):");
	Ft_Gpu_CoCmd_SetBase(phost, 8);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 180, 30, 0, 1000);

	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 230, 30, 0, "Decimal(10):");
	Ft_Gpu_CoCmd_SetBase(phost, 10);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 230, 30, 0, 1000);

	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2) - 130, 280, 30, 0, "Hex(16):");
	Ft_Gpu_CoCmd_SetBase(phost, 16);
	Ft_Gpu_CoCmd_Number(phost, FT_DispWidth / 2 + 80, 280, 30, 0, 1000);
#endif

	//Revert the base back to decimal because numeric base is part of the global context.
	Ft_Gpu_CoCmd_SetBase(phost, 10);


Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
Ft_Gpu_CoCmd_Swap(phost);
Ft_App_Flush_Co_Buffer(phost);
Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
SAMAPP_ENABLE_DELAY_VALUE(3000);
}

#if defined(FT900_PLATFORM) || defined(MSVC_PLATFORM)  || defined(MSVC_FT800EMU)
#define SCRATCH_BUFF_SZ		(1024*4)
#else
#define SCRATCH_BUFF_SZ		(1024*4)
#endif

#if (defined (SAMAPP_ENABLE_APIS_SET8)) && (!defined(FT900_PLATFORM)) && !(defined(ARDUINO_PLATFORM))
//video playback via frame by frame
ft_void_t VideoPlayBackFrameByFrame()
{
	//ft_uint16_t aviw = 854,avih = 480;//TBD put a parser and get the values from it
	ft_uint16_t aviw = 854,avih = 480, videoFrameStatusAddr;//TBD put a parser and get the values from it
	Ft_Fifo_t stFifo;
	ft_uint32_t mediafifo,mediafifolen,i;
	ft_uint8_t g_scratch[SCRATCH_BUFF_SZ];
	ft_uint32_t filesz,currchunk,bytesread,cmdrd,cmdwr;
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
FILE *pFile;
#endif

	/* construct the DL and display */
    Ft_Gpu_CoCmd_Dlstart(phost);
    Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1) );
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

    /* start video playback, load the data into media fifo */
    mediafifo = aviw*avih*2L+32L+4; //the starting address of the media fifo, the begining space is for the decoded video frame,
    videoFrameStatusAddr = RAM_G;  //the 4 byte address for the videoframe status
    mediafifolen = RAM_G_SIZE - mediafifo;
    Ft_Gpu_CoCmd_MediaFifo(phost,mediafifo,mediafifolen);//address of the media fifo buffer

    printf("Mediafifo: Start address and length %d %d\n", mediafifo,mediafifolen);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
#if defined(FT900_PLATFORM)
#elif defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
pFile = fopen("..\\..\\..\\Test\\chickens-4.avi","rb");
if (pFile != NULL)
{
	printf("\nFopen success!\n");
}
else{
	printf("\nFailed to open file.\n");
	return;
}
#endif

#if defined(FT900_PLATFORM)

#elif defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
			fseek(pFile,0,SEEK_END);
			filesz = ftell(pFile);
			fseek(pFile,0,SEEK_SET);
#endif

    	printf("file size %x %d %d %d %d\n",filesz,mediafifo,mediafifolen,Ft_Gpu_Hal_Rd32(phost,REG_CMD_READ),Ft_Gpu_Hal_Rd32(phost,REG_CMD_WRITE));
    	Ft_Fifo_Init(&stFifo,mediafifo,mediafifolen,REG_MEDIAFIFO_READ,REG_MEDIAFIFO_WRITE);

        Ft_Gpu_CoCmd_MediaFifo(phost,mediafifo,mediafifolen);//address of the media fifo buffer - init of fifo done by coprocessor

        Ft_Gpu_CoCmd_Dlstart(phost);

        Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
        Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1) );

        Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(4));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,aviw*2L,avih));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H(((aviw*2L)>>10),((avih)>>9)));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,aviw,avih));
        Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H((aviw>>9),(avih>>9)));
        Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,0));

        Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
        Ft_Gpu_CoCmd_Swap(phost);
        Ft_App_Flush_Co_Buffer(phost);
        Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

        printf("media fifo init done \n");

    	/* fill the complete fifo buffer before entering into steady state */
#if defined(MSVC_PLATFORM)

#elif defined(FT900_PLATFORM)

#endif
    	//printf("Start fifofill %d %x %x %x %x\n",filesz,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
    	{
    		currchunk = mediafifolen - SCRATCH_BUFF_SZ;
			currchunk = ALIGN_TWO_POWER_N(currchunk,SCRATCH_BUFF_SZ);
			//printf("Fill fifo %d\n", currchunk);
			while(currchunk > 0)
			{
				ft_uint32_t tempchunk = currchunk;
				if(tempchunk > SCRATCH_BUFF_SZ)
				{
					tempchunk = SCRATCH_BUFF_SZ;
				}
				else
				{
					tempchunk = currchunk;
				}
#if defined(MSVC_PLATFORM)|| defined(MSVC_FT800EMU)
				bytesread = fread(g_scratch, 1, tempchunk, pFile);
#elif defined(FT900_PLATFORM)

#endif


#if defined(MSVC_PLATFORM)|| defined(MSVC_FT800EMU)
				Ft_Gpu_Hal_WrMem(phost,stFifo.fifo_buff + stFifo.fifo_wp,g_scratch,tempchunk);
#elif defined(FT900_PLATFORM)

#endif


				stFifo.fifo_wp += tempchunk;
				currchunk -= tempchunk;
			}
    	}

    	/* update the data written */
    	filesz -= stFifo.fifo_wp;

    	/* update the read and write pointers of mediafifo */
    	Ft_Gpu_Hal_Wr32(phost,stFifo.HW_Write_Reg,stFifo.fifo_wp);
    	printf("Media fifo rd wr %d %d %d %d \n",Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg),Ft_Gpu_Hal_Rd32(phost,REG_CMD_READ),Ft_Gpu_Hal_Rd32(phost,REG_CMD_WRITE));


    	Ft_Gpu_CoCmd_VideoStart(phost);  //initialize AVI video decoder
    	Ft_Gpu_CoCmd_VideoFrame(phost,4,videoFrameStatusAddr);  //load 1 frame
    	Ft_App_Flush_Co_Buffer(phost);
    	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    	//ft_delay(50);

    	cmdrd = Ft_Gpu_Hal_Rd32(phost,REG_CMD_READ);
    	cmdwr = Ft_Gpu_Hal_Rd32(phost,REG_CMD_WRITE);

    	//printf("Start playback of new video %d %d %d %d %d %d \n", fResult,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg),stFifo.fifo_wp);

    	//while((cmdrd != cmdwr) || (filesz > 0))//loop till end of the file
    	while(((Ft_Fifo_GetFreeSpace(phost,&stFifo) != (stFifo.fifo_len - 4)) || (filesz > 0)) )//loop till end of the file
    	{
    		ft_int32_t availfreesz = Ft_Fifo_GetFreeSpace(phost,&stFifo),chunkfilled = 0;

    		if(Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ) == Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE))
    		{
				Ft_Gpu_CoCmd_VideoFrame(phost,4,videoFrameStatusAddr);
				Ft_App_Flush_Co_Buffer(phost);
    		}
    		while(filesz > 0)
    		{
				if(availfreesz > SCRATCH_BUFF_SZ)
				{

					if(filesz > SCRATCH_BUFF_SZ)
					{
						currchunk = SCRATCH_BUFF_SZ;
					}
					else
					{
						currchunk = filesz;
					}
#if defined(MSVC_PLATFORM)|| defined(MSVC_FT800EMU)
				bytesread = fread(g_scratch, 1, currchunk, pFile);
#elif defined(FT900_PLATFORM)
#endif
					//printf("fifoloop %d %d %d %x %x %x %x\n",filesz,currchunk,bytesread,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
					//Ft_Fifo_WriteWait(phost,&stFifo,g_scratch,currchunk);//download the whole chunk into ring buffer - blocking call
					chunkfilled = Ft_Fifo_Write(phost,&stFifo,g_scratch,bytesread);//download the data and update
					if(chunkfilled != bytesread)
					{
						printf("filled sz and curr sz are wrong %d %d %d\n ",chunkfilled,currchunk,bytesread);
					}
					filesz -= chunkfilled;
					availfreesz -= chunkfilled;
					availfreesz = Ft_Fifo_GetFreeSpace(phost,&stFifo);
				}
				else
				{
					break;
				}

    		}

    		if(Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg) == Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg))
    		{
    			printf("end of videoframe %d %d %d %x %x %x %x\n",filesz,currchunk,bytesread,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
    			break;
    		}


        	cmdrd = Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ);
        	cmdwr = Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE);

    	}

    	printf("playback finished %d %x %x %x %x\n",filesz,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
		fclose(pFile);
}

#endif
//new bitmap L2 format, palette, and full screen option
ft_void_t FT81xBitmapFormatAndOptions() {
	ft_int32_t FileLen, Bufflen, bitmapWidth = 128, bitmapHeight = 128, dlPTR;
	ft_uchar8_t* pbuff;

	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		FILE *pFile;
	#endif

	Ft_Gpu_CoCmd_Dlstart(phost);
	//Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255) );
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1) );
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Ft_Gpu_Hal_WrCmd32(phost, CMD_LOADIMAGE);
	Ft_Gpu_Hal_WrCmd32(phost, RAM_G);//destination address of jpg decode
	Ft_Gpu_Hal_WrCmd32(phost, 0);//output format of the bitmap, supported options are OPT_MONO-bitmap to be monochrome, OPT_FULLSCREEN-attempt to scale to full screen size, and OPT_MEDIAFIFO-use media fifo for image data. If the bitmap format to be used is other than RGB565 and mono.  NODL option was chosen becuase the png has transparancy and we want to draw it by ARGB4 format
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		pFile = fopen("..\\..\\..\\Test\\lenaface40.png","rb");
		if (pFile == NULL)
			return;
		pbuff = (ft_uchar8_t *)malloc(8192);
		fseek(pFile,0,SEEK_END);
		FileLen = ftell(pFile);
		fseek(pFile,0,SEEK_SET);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */
			fread(pbuff,1,blocklen,pFile);
			FileLen -= blocklen;
			/* copy data continuously into command memory */
			Ft_Gpu_Hal_WrCmdBuf(phost, pbuff, blocklen);//alignment is already taken care by this api
		}
		/* close the opened binary zlib file */
		fclose(pFile);
		free(pbuff);
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((FT_DispWidth / 2 - bitmapWidth / 2) * 16, (FT_DispHeight / 2 - bitmapHeight / 2) * 16));
		Ft_App_WrCoCmd_Buffer(phost, END());
	#elif defined(FT900_PLATFORM)
		Ft_Gpu_Hal_WrCmdBufFromFlash(phost, SAMAPP_LENAFACE40_PNG_HEX_DUMP, SAMAPP_LENAFACE40_PNG_HEX_DUMP_SIZE);
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format, SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, SAMAPP_Bitmap_RawData_Header[0].Width,SAMAPP_Bitmap_RawData_Header[0].Height));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((FT_DispWidth/2 - SAMAPP_Bitmap_RawData_Header[0].Width/2) * 16, (FT_DispHeight/2 - SAMAPP_Bitmap_RawData_Header[0].Height/2 - SAMAPP_Bitmap_RawData_Header[0].Height) * 16));
		Ft_App_WrCoCmd_Buffer(phost, END());
	#endif
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 50, 30, OPT_CENTER, "PNG images can now be loaded directly.");
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);
	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_ENABLE_DELAY_VALUE(2000);
}

         
//this function demonstrates the usage of the paletted bitmap converted by the FTDI palette converter 
ft_void_t SAMAPP_81X_Paletted_Bitmap() {
	ft_int32_t paletteTbSz = 0, paletteIdxSz = 0;
	ft_uint16_t bitmapHeight = 128, bitmapWidth = 128, bitmapStride = bitmapWidth;

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));
 #if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	 paletteTbSz = SAMAPP_LoadRawFromFile("..\\..\\..\\Test\\Tomato_lut.raw", RAM_G);
	 paletteIdxSz = SAMAPP_LoadRawFromFile("..\\..\\..\\Test\\Tomato_index.raw", 1024);
	 Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	 Ft_App_WrCoCmd_Buffer(phost, PALETTE_SOURCE(RAM_G));
	 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(1024));
	 Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(PALETTED4444,bitmapWidth,bitmapHeight)); 
	 Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,bitmapWidth,bitmapHeight));
	 Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth/2 - bitmapWidth/2) * 16, (FT_DispHeight/2 - bitmapHeight/2 - bitmapHeight) * 16)); 

#elif defined(FT900_PLATFORM) || defined(ARDUINO_PLATFORM)
	Ft_Gpu_Hal_WrCmdBufFromFlash(phost,Lenaface40,1L*SAMAPP_Lenaface40_SIZE);
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(SAMAPP_Bitmap_RawData_Header[0].Format, SAMAPP_Bitmap_RawData_Header[0].Stride,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, SAMAPP_Bitmap_RawData_Header[0].Width,SAMAPP_Bitmap_RawData_Header[0].Height));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((FT_DispWidth/2 - SAMAPP_Bitmap_RawData_Header[0].Width/2) * 16, (FT_DispHeight/2 - SAMAPP_Bitmap_RawData_Header[0].Height/2 - SAMAPP_Bitmap_RawData_Header[0].Height) * 16));
	Ft_App_WrCoCmd_Buffer(phost, END());
#endif
 
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_ENABLE_DELAY_VALUE(2000);
}


//Multi touch on a single tracked object can be individually tracked to save the MCU calculations on rotary and linear tracking. Maximum of 5 trackers.
ft_void_t SAMAPP_CoPro_MultiTracker() {
	ft_uint32_t trackers[5], delayLoop = 300, trackerVal;
	ft_uint8_t tagval, RDialTag = 100, GDialTag = 101, BDialTag = 102, ADialTag =
			103, DialR = FT_DispWidth/10, i, j, rectRed=0, rectGreen=0, rectBlue=0, rectAlpha = 255;
	ft_uint16_t RDialX = DialR + 20, RDialY = FT_DispHeight / 4, GDialX = DialR
			+ 20, GDialY = RDialY * 3, BDialX = FT_DispWidth - 20 - DialR, BDialY =
			FT_DispHeight / 4, ADialX = FT_DispWidth - 20 - DialR, ADialY = BDialY
			* 3, rectWidth = FT_DispWidth/2.6, rectHeight = FT_DispHeight/2, rectX = FT_DispWidth / 2
			- rectWidth / 2, rectY = FT_DispHeight / 2 - rectHeight / 2,
			RDialTrackVal = 0, GDialTrackVal = 0, BDialTrackVal = 0, ADialTrackVal =
					65535;
	ft_uint32_t dumpDL = 1, dlindex=RAM_DL, totaldl, dloffset;

	Ft_Gpu_CoCmd_Track(phost, RDialX, RDialY, 1, 1, RDialTag);
	Ft_Gpu_CoCmd_Track(phost, GDialX, GDialY, 1, 1, GDialTag);
	Ft_Gpu_CoCmd_Track(phost, BDialX, BDialY, 1, 1, BDialTag);
	Ft_Gpu_CoCmd_Track(phost, ADialX, ADialY, 1, 1, ADialTag);

	Ft_Gpu_Hal_Wr8(phost, REG_CTOUCH_EXTENDED, CTOUCH_MODE_EXTENDED);
	while (delayLoop != 0) {
		//tagval = Ft_Gpu_Hal_Rd8(phost, REG_TOUCH_TAG);
		trackers[0] = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER);
		trackers[1] = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER_1);
		trackers[2] = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER_2);
		trackers[3] = Ft_Gpu_Hal_Rd32(phost, REG_TRACKER_3);

	for (i = 0; i < 4; i++) {
		tagval = (trackers[i] & 0xff);
		trackerVal = (trackers[i] >> 16) & 0xffff;
		if (tagval == RDialTag) {
			rectRed = (ft_int32_t) trackerVal * 255 / 65536;
			RDialTrackVal = trackerVal;
		} else if (tagval == GDialTag) {
			rectGreen = (ft_int32_t) trackerVal * 255 / 65536;
			GDialTrackVal = trackerVal;
		} else if (tagval == BDialTag) {
			rectBlue = (ft_int32_t) trackerVal * 255 / 65536;
			BDialTrackVal = trackerVal;
		} else if (tagval == ADialTag) {
			rectAlpha = (ft_int32_t) trackerVal * 255 / 65536;
			ADialTrackVal = trackerVal;
		}
	}

	Ft_Gpu_CoCmd_Dlstart(phost); // clear screen
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_A(255));
	Ft_App_WrCoCmd_Buffer(phost, TAG_MASK(1));
	Ft_App_WrCoCmd_Buffer(phost, TAG(RDialTag));
	Ft_Gpu_CoCmd_Dial(phost, RDialX, RDialY, DialR, 0, RDialTrackVal);
	Ft_App_WrCoCmd_Buffer(phost, TAG(GDialTag));
	Ft_Gpu_CoCmd_Dial(phost, GDialX, GDialY, DialR, 0, GDialTrackVal);
	Ft_App_WrCoCmd_Buffer(phost, TAG(BDialTag));
	Ft_Gpu_CoCmd_Dial(phost, BDialX, BDialY, DialR, 0, BDialTrackVal);
	Ft_App_WrCoCmd_Buffer(phost, TAG(ADialTag));
	Ft_Gpu_CoCmd_Dial(phost, ADialX, ADialY, DialR, 0, ADialTrackVal);
	Ft_App_WrCoCmd_Buffer(phost, TAG_MASK(0));

	Ft_Gpu_CoCmd_Text(phost, RDialX, RDialY, 28, OPT_CENTER, "Red"); //text info
	Ft_Gpu_CoCmd_Text(phost, GDialX, GDialY, 28, OPT_CENTER, "Green"); //text info
	Ft_Gpu_CoCmd_Text(phost, BDialX, BDialY, 28, OPT_CENTER, "Blue"); //text info
	Ft_Gpu_CoCmd_Text(phost, ADialX, ADialY, 28, OPT_CENTER, "Alpha"); //text info

	Ft_App_WrCoCmd_Buffer(phost, BEGIN(RECTS));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(rectRed, rectGreen, rectBlue));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_A(rectAlpha));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(rectX * 16, rectY * 16));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((rectX + rectWidth) * 16, (rectY + rectHeight) * 16));

		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		delayLoop--;
	}

}


ft_void_t VideoPlaybackViaCMDBuff() {
	//ft_uint16_t aviw = 854,avih = 480;//TBD put a parser and get the values from it
	//ft_uint16_t aviw = 960,avih = 540;//TBD put a parser and get the values from it
	ft_uint16_t aviw = 960,avih = 540;//TBD put a parser and get the values from it
	Ft_Fifo_t stFifo;
	ft_uint32_t mediafifo,mediafifolen,i;
	ft_uint8_t g_scratch[SCRATCH_BUFF_SZ];
	ft_uint32_t filesz,currchunk,bytesread,cmdrd,cmdwr;
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile;
	#endif

	/* construct the DL and display */
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1) );
	//Ft_Gpu_CoCmd_Gradient(phost,0,0,0x00ffff,0,480,0xff0000);
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	//Ft_App_Flush_Co_Buffer(phost);
	//Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	pFile = fopen("..\\..\\..\\Test\\csf_qhd.avi","rb");
	if (pFile != NULL)
	{
		printf("\nFopen success!\n");
	}
	else{
		printf("\nFailed to open file.\n");
		return;
	}
	#elif defined(FT900_PLATFORM)
	#endif


	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
				fseek(pFile,0,SEEK_END);
				filesz = ftell(pFile);
				fseek(pFile,0,SEEK_SET);
	#elif defined(FT900_PLATFORM)
	#endif

		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		Ft_Gpu_Hal_WrCmd32(phost, CMD_PLAYVIDEO);
		Ft_Gpu_Hal_WrCmd32(phost, OPT_FULLSCREEN | OPT_NOTEAR);  //original

		{
			ft_int32_t availfreesz = 0,freadbufffill = 0,chunkfilled = 0;
			ft_uint8_t *pbuff = g_scratch;
			printf("Video playback starts.\n");
			while(filesz > 0)
			{


				availfreesz = Ft_Gpu_Hal_Rd32(phost,REG_CMDB_SPACE);
				//printf("freesize %d \n",availfreesz);
				if(availfreesz > 0)
				{

					if(0 == freadbufffill)
					{
						if(filesz > SCRATCH_BUFF_SZ)
						{
							freadbufffill = SCRATCH_BUFF_SZ;
						}
						else
						{
							freadbufffill = filesz;
						}
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
						bytesread = fread(g_scratch, 1, freadbufffill, pFile);
	#elif defined(FT900_PLATFORM)
	#endif
						//printf("fread %d %d %d %x %x %x %x\n",filesz,currchunk,bytesread,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
						pbuff = g_scratch;
						filesz -= bytesread;
					}

					if(availfreesz > freadbufffill)
					{
						availfreesz = freadbufffill;
					}

					if(availfreesz > 0)
					{

						//printf("fifoloop %d %d %d %x %x %x %x\n",filesz,currchunk,bytesread,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
						//chunkfilled = Ft_Fifo_Write(phost,&stFifo,g_scratch,bytesread);//download the data and update


	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
						Ft_Gpu_Hal_WrMem(phost, REG_CMDB_WRITE, pbuff,availfreesz);
	#elif defined(FT900_PLATFORM)
	#endif

					}

				pbuff += availfreesz;
				freadbufffill -= availfreesz;
			}
		}
	}

		printf("playback finished %d %x %x %x %x\n",filesz,cmdrd,cmdwr,Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Read_Reg),Ft_Gpu_Hal_Rd32(phost,stFifo.HW_Write_Reg));
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		fclose(pFile);
#endif
}

#if (defined (SAMAPP_ENABLE_APIS_SET8)) && (!defined(FT900_PLATFORM)) && !(defined(ARDUINO_PLATFORM))
ft_void_t VideoPlayback() {  //media playback with audio
	//ft_uint16_t aviw = 854, avih = 480; //TBD put a parser and get the values from it
	ft_uint16_t aviw = 256, avih = 144;
	ft_uint16_t count = 0;
	ft_uint16_t curWriteMark;
	ft_uint8_t g_scratch[SCRATCH_BUFF_SZ];
	Ft_Fifo_t stFifo;
	ft_uint32_t mediafifo, mediafifolen, i;
	ft_uint32_t filesz, currchunk, bytesread, cmdrd, cmdwr;
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile;
	#endif

	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	pFile = fopen("..\\..\\..\\Test\\bbb_lo.avi","rb");
	if (pFile == NULL)
	{ 
    	#elif defined(FT900_PLATFORM)
    	if(0){
    	#endif
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), FT_DispHeight / 2, 29,	OPT_CENTER, "Unable to open file.");
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		printf("Error while openning file.\n");
		return;
	} else {
		/* construct the DL and display */
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		/* start video playback, load the data into media fifo */
		mediafifo = aviw * avih * 2L + 32 * 1024L; //starting address of the media fifo
		mediafifolen = RAM_G_SIZE - mediafifo; //how much memory will be allocated for the video playback fifo
		Ft_Gpu_CoCmd_MediaFifo(phost, mediafifo, mediafifolen); //address of the media fifo buffer
		printf("Mediafifo: Start address and length %d %d\n", mediafifo, mediafifolen);

		Ft_Fifo_Init(&stFifo, mediafifo, mediafifolen, REG_MEDIAFIFO_READ, REG_MEDIAFIFO_WRITE); //initialize application media fifo structure
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
				fseek(pFile,0,SEEK_END);
				filesz = ftell(pFile);
				fseek(pFile,0,SEEK_SET);
	#elif defined(FT900_PLATFORM)
	#endif


		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		Ft_Gpu_Hal_WrCmd32(phost, CMD_PLAYVIDEO);
		Ft_Gpu_Hal_WrCmd32(phost, OPT_MEDIAFIFO | OPT_NOTEAR | OPT_SOUND);

		/* fill the complete fifo buffer before entering into steady state */
	#if defined(FT900_PLATFORM)

	#endif
		{
			currchunk = mediafifolen - SCRATCH_BUFF_SZ;
			currchunk = ALIGN_TWO_POWER_N(currchunk, SCRATCH_BUFF_SZ);
			//printf("Fill fifo %d\n", currchunk);
			while (currchunk > 0) {
				ft_uint32_t tempchunk = currchunk;
				if (tempchunk > SCRATCH_BUFF_SZ) {
					tempchunk = SCRATCH_BUFF_SZ;
				} else {
					tempchunk = currchunk;
				}
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
				fread(g_scratch, 1, tempchunk, pFile);
	#elif defined(FT900_PLATFORM)

	#endif

				Ft_Fifo_WriteWait(phost, &stFifo, g_scratch, tempchunk);
				currchunk -= tempchunk;
			}
		}
		cmdrd = Ft_Gpu_Hal_Rd16(phost, REG_CMD_READ);
		cmdwr = Ft_Gpu_Hal_Rd16(phost, REG_CMD_WRITE);

		filesz -= stFifo.fifo_wp;
		while ((cmdrd != cmdwr) || (filesz>0))  //loop till end of the file
		{
			if (filesz > 0) {
				if (filesz > SCRATCH_BUFF_SZ) {
					currchunk = SCRATCH_BUFF_SZ;
				} else {
					currchunk = filesz;
				}
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
				bytesread = fread(g_scratch, 1, currchunk, pFile);
	#elif defined(FT900_PLATFORM)

	#endif
				Ft_Fifo_WriteWait(phost, &stFifo, g_scratch, bytesread); //download the whole chunk into ring buffer - blocking call

				filesz -= currchunk;
			}

		cmdrd = Ft_Gpu_Hal_Rd16(phost, REG_CMD_READ);
		cmdwr = Ft_Gpu_Hal_Rd16(phost, REG_CMD_WRITE);

		}
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	fclose(pFile);
#endif
	}
}
#endif
/*
 Load DXT1 compressed image. The FTDI DXT1 conversion utility outputs 4 seperate files: c0,c1,b0,b1.
 The 4 files should be combined to create the final image.  The bitmap size can be reduced up to 4 folds of the original size.

 */
ft_void_t SAMAPP_GPU_DXT1() {
	//load each .raw file into graphics RAM from directory ‘test’
	//RAM_G is starting address in graphics RAM, for example 00 0000h
	ft_uint16_t imgWidth = 128, imgHeight = 128, c0_c1_width = 32, c0_c1_height = 32, c0_c1_stride = c0_c1_width * 2, b0_b1_width =	imgWidth, b0_b1_height = imgHeight, b0_b1_stride = b0_b1_width/ 8, szPerFile = 2048;

	#if defined(ARDUINO_PLATFORM) || defined(FT900_PLATFORM)
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G, SAMAPP_Tomato_DXT1_C0_Data_Raw,	szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile,	SAMAPP_Tomato_DXT1_C1_Data_Raw, szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile * 2,	SAMAPP_Tomato_DXT1_B0_Data_Raw, szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile * 3,	SAMAPP_Tomato_DXT1_B1_Data_Raw, szPerFile);
	#endif

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G, SAMAPP_Tomato_DXT1_C0_Data_Raw, szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile, SAMAPP_Tomato_DXT1_C1_Data_Raw, szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile * 2, SAMAPP_Tomato_DXT1_B0_Data_Raw, szPerFile);
	Ft_Gpu_Hal_WrMemFromFlash(phost, RAM_G + szPerFile * 3, SAMAPP_Tomato_DXT1_B1_Data_Raw, szPerFile);
#endif

	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost);

	Ft_App_WrCoCmd_Buffer(phost, SAVE_CONTEXT());
	//B0&B1 handle
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(1));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(RAM_G + szPerFile*2));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L1, b0_b1_stride, b0_b1_height)); //L1 format stride is 1 bit per pixel
	Ft_App_WrCoCmd_Buffer(phost,
			BITMAP_SIZE(NEAREST, BORDER, BORDER, imgWidth, imgHeight)); //draw in full size
	//C0&C1 handle
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(2));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(RAM_G));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, c0_c1_stride, c0_c1_height)); //RGB565 format stride is 2 bytes per pixel
	Ft_App_WrCoCmd_Buffer(phost,
			BITMAP_SIZE(NEAREST, BORDER, BORDER, imgWidth, imgHeight)); //draw in full size
	// start drawing bitmaps
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost, BLEND_FUNC(ONE, ZERO));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_A(0x55));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth / 2 - b0_b1_width / 2, FT_DispHeight / 2 - b0_b1_height / 2, 1, 0));
	Ft_App_WrCoCmd_Buffer(phost, BLEND_FUNC(ONE, ONE));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_A(0xAA));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth / 2 - b0_b1_width / 2, FT_DispHeight / 2 - b0_b1_height / 2, 1, 1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(1, 1, 1, 0));
	Ft_Gpu_CoCmd_Scale(phost, 4 * 65536, 4 * 65536);
	Ft_Gpu_CoCmd_SetMatrix(phost);
	Ft_App_WrCoCmd_Buffer(phost, BLEND_FUNC(DST_ALPHA, ZERO));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth / 2 - b0_b1_width / 2, FT_DispHeight / 2 - b0_b1_height / 2, 2, 1));
	Ft_App_WrCoCmd_Buffer(phost, BLEND_FUNC(ONE_MINUS_DST_ALPHA, ONE));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth / 2 - b0_b1_width / 2, FT_DispHeight / 2 - b0_b1_height / 2, 2, 0));
	Ft_App_WrCoCmd_Buffer(phost, END());
	Ft_App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());

	//reset the transformation matrix because its not part of the context, RESTORE_CONTEXT() command will not revert the command.
	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost);

	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 0, 0));
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 50, 31, OPT_CENTER, "DXT1: 8KB.");
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 80, 31, OPT_CENTER,
			"Original: 32KB.");

	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	//swap the current display list with the new display list
	Ft_Gpu_CoCmd_Swap(phost);
	//write to the FT800 FIFO command buffer - bitmap will appear after this command
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Ft_Gpu_Hal_Sleep(3000);
}

  ft_void_t     SAMAPP_GPU_Bitmap_Paletted8()
{
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
       SAMAPP_Bitmap_header_t *p_bmhdr;
       ft_int16_t BMoffsetx,BMoffsety;

       ft_int32_t LutFileLen,FileLen,Bufflen;

       ft_uchar8_t *pbuff,*plutbuff;
       ft_int32_t  ft800_memaddr = RAM_G,pal_mem_addr;

       FILE *pFile = fopen("..\\..\\..\\Test\\background_paletted.raw","rb");

       FILE *pLutFile = fopen("..\\..\\..\\Test\\background_paletted_lut.raw","rb");

       fseek(pFile,0,SEEK_END);
       FileLen = ftell(pFile);
       fseek(pFile,0,SEEK_SET);

       pbuff = malloc(FileLen);

       fread(pbuff,1,FileLen,pFile);

       p_bmhdr = (SAMAPP_Bitmap_header_t *)&SAMAPP_Bitmap_RawData_Header[2];

       while (FileLen > 64*1024)
       {
          Ft_Gpu_Hal_WrMemFromFlash(phost, ft800_memaddr,(ft_uchar8_t *)pbuff,  64*1024);
          FileLen -= 64*1024;
          ft800_memaddr += 64*1024;
          pbuff += 64*1024;
       }
       Ft_Gpu_Hal_WrMemFromFlash(phost, ft800_memaddr,(ft_uchar8_t *)pbuff,  FileLen);
       ft800_memaddr += FileLen;//last chunk if any

    fseek(pLutFile,0,SEEK_END);
       LutFileLen = ftell(pLutFile);
       fseek(pLutFile,0,SEEK_SET);

       plutbuff = malloc(LutFileLen);

       fread(plutbuff,1,LutFileLen,pLutFile);
#ifndef FT_81X_ENABLE
       pal_mem_addr = RAM_PAL;
#else
       pal_mem_addr = ft800_memaddr;
#endif

       Ft_Gpu_Hal_WrMemFromFlash(phost, pal_mem_addr,(ft_uchar8_t *)plutbuff, LutFileLen);
       //Ft_App_WrDlCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
       Ft_App_WrDlCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
       Ft_App_WrDlCmd_Buffer(phost, BEGIN(EDGE_STRIP_B)); // clear screen
       Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
       Ft_App_WrDlCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16, 0));

       //Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));
       Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
#ifdef FT_81X_ENABLE
       Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(PALETTED8, p_bmhdr->Stride, p_bmhdr->Height));
#else
       Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(p_bmhdr->Format, p_bmhdr->Stride, p_bmhdr->Height));
#endif
       Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, p_bmhdr->Width, p_bmhdr->Height));
       Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps

#ifdef FT_81X_ENABLE
       Ft_App_WrDlCmd_Buffer(phost,BLEND_FUNC(ONE, ZERO));
    Ft_App_WrDlCmd_Buffer(phost,BEGIN(BITMAPS));

       Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
       Ft_App_WrDlCmd_Buffer(phost,PALETTE_SOURCE(ft800_memaddr + 3));
    Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
    
    Ft_App_WrDlCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA, ONE_MINUS_DST_ALPHA));
    Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(1,0,0,0));
    Ft_App_WrDlCmd_Buffer(phost,PALETTE_SOURCE(ft800_memaddr + 2));
    Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
    
    Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(0,1,0,0));
    Ft_App_WrDlCmd_Buffer(phost,PALETTE_SOURCE(ft800_memaddr + 1));
    Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
    
    Ft_App_WrDlCmd_Buffer(phost,COLOR_MASK(0,0,1,0));
    Ft_App_WrDlCmd_Buffer(phost,PALETTE_SOURCE(ft800_memaddr + 0));
    Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
    
#else
       Ft_App_WrDlCmd_Buffer(phost,PALETTE_SOURCE(ft800_memaddr));
       Ft_App_WrDlCmd_Buffer(phost,VERTEX2II(0, 0, 0, 0));
#endif

       Ft_App_WrDlCmd_Buffer(phost,END());
       Ft_App_WrDlCmd_Buffer(phost, DISPLAY() );

       /* Download the DL into DL RAM */
       Ft_App_Flush_DL_Buffer(phost);

       /* Do a swap */
       SAMAPP_GPU_DLSwap(DLSWAP_FRAME);
    SAMAPP_ENABLE_DELAY();
#endif
}


//simpler method to load RAM font.  Use the font conversion utility to convert the desired subset of the ASCII characters, load font data, and use cmd_setfont2 command.
ft_void_t SAMAPP_CoPro_Setfont2() {
ft_uint16_t fontFileAddress = 100; //the converted font file will have a starting address of RAM_G or 0th byte mark
Ft_Gpu_CoCmd_Dlstart(phost);
Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
SAMAPP_LoadRawFromFile("..\\..\\..\\Test\\Tuffy.ttf_24_L2.raw", fontFileAddress); //load font data consits of font metric block and font data from the FTDI font conversion utility.  The file has been packaged with the font metric block at the beginning of the font data.
Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(1)); //associate to font handle 1
Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(fontFileAddress + 148)); //The starting address for the font file 148 bytes into the font file because the first 148 bytes are font metric informational block.

Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L2, 5, 29)); //font stride and height metrics can be found in the .rawh file
Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 29)); //font width and height metrics can be found in the .rawh file

Ft_Gpu_CoCmd_SetFont2(phost, 1, fontFileAddress, 97); //set ram font to font handle 1, font metric block starts at fontFileAddress(RAM_G), and set the starting character to the 32th character in the set.

Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)-30, (FT_DispHeight / 2) - 80, 1, OPT_CENTER, "f");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)-15, (FT_DispHeight / 2) - 80, 1, OPT_CENTER, "t");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)+15, (FT_DispHeight / 2) - 80, 1, OPT_CENTER, "d");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)+30, (FT_DispHeight / 2) - 80, 1, OPT_CENTER, "i");
#elif defined(ARDUINO_PLATFORM) || defined(FT900_PLATFORM)
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)-30, (FT_DispHeight / 2) - 80, 29, OPT_CENTER, "f");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)-15, (FT_DispHeight / 2) - 80, 29, OPT_CENTER, "t");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)+15, (FT_DispHeight / 2) - 80, 29, OPT_CENTER, "d");
Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2)+30, (FT_DispHeight / 2) - 80, 29, OPT_CENTER, "i");
#endif

Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
Ft_Gpu_CoCmd_Swap(phost);
Ft_App_Flush_Co_Buffer(phost);
Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
SAMAPP_ENABLE_DELAY_VALUE(5000);
}


//Change screen orientation from landscape to portrait mode.  Setscratch command is also introduced, it sets the temporary bitmap handle for buttons, keys, and graidents.
ft_void_t ScreenOrientation() {

Ft_Gpu_CoCmd_Dlstart(phost);
Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));

Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), 50, 31, OPT_CENTER, "Landscape Mode");  //The default is landscape mode
Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
Ft_Gpu_CoCmd_Swap(phost);
Ft_App_Flush_Co_Buffer(phost);
Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
SAMAPP_ENABLE_DELAY_VALUE(3000);

Ft_Gpu_CoCmd_Dlstart(phost);
Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
Ft_Gpu_CoCmd_SetRotate(phost, 2);
Ft_Gpu_CoCmd_Text(phost, (FT_DispHeight / 2), 50, 31, OPT_CENTER, "Portrait Mode"); //In portrait mode (90 degree counter clockwise rotation), the x and y components are flipped.
Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
Ft_Gpu_CoCmd_Swap(phost);
Ft_App_Flush_Co_Buffer(phost);
Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
SAMAPP_ENABLE_DELAY_VALUE(3000);

//reset the rotation angle
Ft_Gpu_CoCmd_SetRotate(phost, 0);

}

/*
 FT80x can only display bitmaps no larger than 512x512, FT81x support bitmap resolutions up to 2048x2048.
 If the bitmap dimensions are bigger than 512 in either direction
 */
ft_void_t HigherResolutionBitmap() {
	ft_uint16_t imgWidth = 40, imgHeight = 40;
	ft_uint16_t stride = imgWidth * 2;  //RGB565 format is 2 bytes per pixel

	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));

	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Ft_Gpu_Hal_WrCmd32(phost, CMD_LOADIMAGE);
	Ft_Gpu_Hal_WrCmd32(phost, RAM_G);  //destination address if inflate
	Ft_Gpu_Hal_WrCmd32(phost, 0);

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	Ft_Gpu_Hal_WrCmdBuf(phost, SAMAPP_LENAFACE40_PNG_HEX_DUMP, SAMAPP_LENAFACE40_PNG_HEX_DUMP_SIZE);
#else
	Ft_Gpu_Hal_WrCmdBufFromFlash(phost, SAMAPP_LENAFACE40_PNG_HEX_DUMP, SAMAPP_LENAFACE40_PNG_HEX_DUMP_SIZE);
#endif
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, (imgWidth * 2), imgHeight));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT_H((imgWidth * 2) >> 10, imgHeight >> 9)); //the two most significant bits of stride and height are specified in this API.  NOTE: the stride is a 10bit value while height and width are 9bits
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, imgWidth, imgHeight));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE_H(imgWidth>>9, imgHeight>>9));  //the upper 2 bits of the height and width
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(((FT_DispWidth / 2) - (imgWidth / 2))*16, ((FT_DispHeight / 2) - (imgHeight / 2))*16));
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_ENABLE_DELAY_VALUE(3000);

}


ft_void_t SAMAPP_GPU_Vertex_Translate() {
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost, CALL(10));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_TRANSLATE_X(150 * 16));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_TRANSLATE_Y(150 * 16));
	Ft_App_WrCoCmd_Buffer(phost, CALL(10));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_TRANSLATE_X(-150 * 16));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_TRANSLATE_Y(-150 * 16));
	Ft_App_WrCoCmd_Buffer(phost, CALL(10));
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());

	Ft_Gpu_CoCmd_Gauge(phost, 391, 285, 36, 0, 4, 8, 40, 100);
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINES));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0)); //change VERTEX2F drawing precision to pixel precision, maximum range is -16384 to 16383.
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(272, 162));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(432, 162));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(1)); //change VERTEX2F drawing precision to 1/2th of a pixel precision, maximum range is -8192 to 8191
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(432*2, 323*2));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(432*2, 162*2));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(2)); //change VERTEX2F drawing precision to 1/4th of a pixel precision, maximum range is  -4096 to 4095
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(432*4, 323*4));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(272*4, 323*4));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(3)); //change VERTEX2F drawing precision to 1/8th of a pixel precision, maximum range is -2048 to 2047
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(272*8, 162*8));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(272*8, 323*8));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(4));  //Revert back to the default VERTEX2F drawing precision, 1/16th of a pixel precision.
	Ft_App_WrCoCmd_Buffer(phost, END());
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
	Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(160));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(323*16, 279*16));
	Ft_App_WrCoCmd_Buffer(phost, END());
	Ft_Gpu_CoCmd_Dial(phost, 391, 206, 36, 0, 6144);
	Ft_Gpu_CoCmd_Clock(phost, 313, 205, 36, 0, 13, 51, 17, 0);
	Ft_App_WrCoCmd_Buffer(phost, RETURN());

	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_CoCmd_Swap(phost);

	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_ENABLE_DELAY_VALUE(3000);
}
#endif


#ifdef SAMAPP_ENABLE_APIS_SET9
#if defined(FT_81X_ENABLE)
 //change the clock frequency for FT81X series chips
ft_void_t SAMAPP_ChangeFreq() {
	SAMAPP_fadeout();
	SAMAPP_playmutesound(); //Play mute sound to avoid pop sound
	Ft_Gpu_PowerModeSwitch(phost, FT_GPU_SLEEP_M); //put device in sleep mode
	Ft_Gpu_Hal_Sleep(50);

	//change the clock frequency to 60mhz during sleep mode to avoid any behaviors during the clock transition period.
	Ft_Gpu_81X_SelectSysCLK(phost, FT_GPU_SYSCLK_48M); //change system frequency to 48mhz

	Ft_Gpu_PowerModeSwitch(phost, FT_GPU_ACTIVE_M); //put device in active mode
	Ft_Gpu_Hal_Sleep(50);
	SAMAPP_API_Screen("System clock is now at 48mhz.");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);

	SAMAPP_fadeout();
	SAMAPP_playmutesound(); //Play mute sound to avoid pop sound
	Ft_Gpu_PowerModeSwitch(phost, FT_GPU_SLEEP_M);
	Ft_Gpu_Hal_Sleep(50);

	//change the clock frequency to 60mhz during sleep mode to avoid any behaviors during the clock transition period.
	Ft_Gpu_81X_SelectSysCLK(phost, FT_GPU_SYSCLK_60M); //change system frequency to 60mhz

	Ft_Gpu_PowerModeSwitch(phost, FT_GPU_ACTIVE_M);
	Ft_Gpu_Hal_Sleep(50);
	SAMAPP_API_Screen("System clock is now at 60mhz.");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);
}

 //when some ROMs and ADC are not being used they can be powered off to save power
ft_void_t SAMAPP_81X_PowerOffComponents() {
	//components are powered off by explicitly pass in the elements in the FT_GPU_81X_ROM_AND_ADC_T table to the Ft_GPU_81X_PowerOffComponents() function.  Elements that are not part of the parameter will be turned on.
	//Application should retain the ROMs and ADCs setting on its own as the state of the ROMs and ADCs are not readable from the chip.
	Ft_Gpu_CoCmd_Dlstart(phost);
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), (FT_DispHeight / 2) - 50, 31, OPT_CENTER, "Some ROMs and ADCs can be");
	Ft_Gpu_CoCmd_Text(phost, (FT_DispWidth / 2), (FT_DispHeight / 2) + 50, 31, OPT_CENTER, "powered off individually.");
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	SAMAPP_ENABLE_DELAY_VALUE(3000);

	Ft_GPU_81X_PowerOffComponents(phost, FT_GPU_SAMPLE_ROM | FT_GPU_JABOOT_ROM); //disable audio

	SAMAPP_ENABLE_DELAY_VALUE(1000);

	//Power on all ROMs and ADC
	Ft_GPU_81X_PowerOffComponents(phost, FT_GPU_POWER_ON_ROM_AND_ADC);

	SAMAPP_ENABLE_DELAY_VALUE(1000);
}

 //The current of the GPIOs and IOs can be increased upto 20 milliamps or decrease down to 5 milliamps, by default the GPIOs and IOs specified in the FT_GPU_81X_GPIO_GROUP_T table are outputing 5 milliamps.
ft_void_t SAMAPP_81X_ChangePadDriveStrength() {
	SAMAPP_fadeout();
	SAMAPP_playmutesound();  //Play mute sound to avoid pop sound
	Ft_GPU_81X_PadDriveStrength(phost, FT_GPU_10MA, FT_GPU_GPIO0);  //change GPIO0

	//Ft_GPU_81X_PadDriveStrength(phost, FT_GPU_20MA, FT_GPU_DE);
	SAMAPP_API_Screen("GPIO0 is now driving 10 milliamps.");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);
}

  //demonstration to hold the device during the system reset.
ft_void_t SAMAPP_HoldResetState() {
	SAMAPP_fadeout();
	SAMAPP_playmutesound();		//Play mute sound to avoid pop sound
	SAMAPP_API_Screen("Reset state hold.");
	Ft_Gpu_CoreReset(phost);		//perform a device reset.
	Ft_Gpu_81X_ResetActive(phost);		//hold the device in reset state
	Ft_Gpu_Hal_Sleep(3000);

	//during the device reset holding period, the application can perform operations that require the device to be non-operational or pre-operational status.

	Ft_Gpu_81X_ResetRemoval(phost);	//exit reset state, the device will power on and enter into its default state.
	SAMAPP_BootupConfig();		//reconfigure the display
	SAMAPP_API_Screen("Reset state released.");
	SAMAPP_fadein();
	Ft_Gpu_Hal_Sleep(3000);
}

#endif
#endif



#if defined(MSVC_PLATFORM) || defined(FT900_PLATFORM)
/* Main entry point */
ft_int32_t main(ft_int32_t argc, ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{
	ft_uint32_t TransMatrix[6];

Ft_Gpu_HalInit_t halinit;

#ifdef FT900_PLATFORM
FT900_Config();
#endif

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

SAMAPP_BootupConfig();

#ifdef MSVC_PLATFORM_SPI
printf("reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
printf("reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
printf("reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
printf("reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));
#endif

/*It is optional to clear the screen here*/
Ft_Gpu_Hal_WrMem(phost, RAM_DL, (ft_uint8_t *) FT_DLCODE_BOOTUP, sizeof(FT_DLCODE_BOOTUP));
Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP, DLSWAP_FRAME);

Ft_Gpu_Hal_Sleep(1000); //Show the booting up screen.


	/* Sample code for GPU primitives */
#ifdef SAMAPP_ENABLE_APIS_SET0
	SAMAPP_API_Screen("Set0   START");
	SAMAPP_GPU_Points();
	SAMAPP_GPU_Lines();
	SAMAPP_GPU_Rectangles();
	SAMAPP_GPU_Bitmap();
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	SAMAPP_GPU_Bitmap_Palette();
	SAMAPP_GPU_Bitmap_Palette_Background();
#endif
	SAMAPP_GPU_Fonts();
	SAMAPP_GPU_Text8x8();
	SAMAPP_GPU_TextVGA();
	SAMAPP_GPU_Bargraph();
	SAMAPP_GPU_LineStrips();
	SAMAPP_GPU_EdgeStrips();
	SAMAPP_GPU_Scissor();
	SAMAPP_Gpu_Polygon();
	SAMAPP_Gpu_Cube();
	SAMAPP_GPU_Ball_Stencil();
	SAMAPP_GPU_FtdiString();
	SAMAPP_GPU_StreetMap();
	SAMAPP_GPU_AdditiveBlendText();
	SAMAPP_GPU_MacroUsage();
	SAMAPP_GPU_AdditiveBlendPoints();
	SAMAPP_API_Screen("Set0   End!");
#endif /* #ifdef SAMAPP_ENABLE_APIS_SET0 */

#ifdef SAMAPP_ENABLE_APIS_SET1
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set1   START");
	SAMAPP_CoPro_Logo();
	SAMAPP_CoPro_Widget_Calibrate();
	SAMAPP_CoPro_Widget_Clock();
	SAMAPP_CoPro_Widget_Guage();
	SAMAPP_CoPro_Widget_Gradient();
	SAMAPP_CoPro_Widget_Keys();
	SAMAPP_CoPro_Widget_Keys_Interactive();
	SAMAPP_CoPro_Widget_Progressbar();
	SAMAPP_CoPro_Widget_Scroll();
	SAMAPP_CoPro_Widget_Slider();
	SAMAPP_CoPro_Widget_Dial();
	SAMAPP_CoPro_Widget_Toggle();
	SAMAPP_CoPro_Widget_Spinner();
	SAMAPP_PowerMode();
	SAMAPP_CoPro_Widget_Calibrate();
	SAMAPP_Touch();

	SAMAPP_API_Screen("Set1   END!");
#endif /* #ifdef SAMAPP_ENABLE_APIS_SET1 */	

#ifdef SAMAPP_ENABLE_APIS_SET2
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set2   START");
	SAMAPP_CoPro_Inflate();
	SAMAPP_CoPro_Loadimage();
	SAMAPP_API_Screen("Set2   END!");
#endif /* #ifdef SAMAPP_ENABLE_APIS_SET2 */

#ifdef SAMAPP_ENABLE_APIS_SET3
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set3   START");
	SAMAPP_CoPro_Setfont();
	SAMAPP_API_Screen("Set3   END!");
#endif /* #ifdef SAMAPP_ENABLE_APIS_SET3 */

#ifdef SAMAPP_ENABLE_APIS_SET4     
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set4   START");
	/* Sample code for coprocessor widgets */
	SAMAPP_CoPro_Widget_Text();
	SAMAPP_CoPro_Widget_Number();
	SAMAPP_CoPro_Widget_Button();
	SAMAPP_CoPro_AppendCmds();
	SAMAPP_CoPro_Widget_Calibrate();

	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND, 255);
	SAMAPP_Sound();
	SAMAPP_CoPro_Screensaver();
	SAMAPP_CoPro_Snapshot();
	SAMAPP_CoPro_Sketch();

	SAMAPP_CoPro_Matrix();
	SAMAPP_CoPro_Track();

	SAMAPP_API_Screen("Set4   END!");

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	//Enable Audio out by setting GPIO
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x083 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO));
	/* Audio playback api*/
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,255);
	SAMAPP_Aud_Music_Player_Streaming();
	//SAMAPP_Aud_Music_Player();
#endif

#endif

#ifdef SAMAPP_ENABLE_APIS_SET5
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU) || defined(FT900_PLATFORM)
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set5   START");
	SAMAPP_ChineseFont();
	SAMAPP_API_Screen("Set5   END!");
#endif
#endif

#ifdef SAMAPP_ENABLE_APIS_SET6
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
#if defined(FT_801_ENABLE) || defined(FT_811_ENABLE)
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set6   Start!");
	SAMAPP_CoPro_Widget_Calibrate();
	SAMAPP_API_Screen("Main Windows");
	MainWindow();
	SAMAPP_API_Screen("Set6   End!");
#endif
#endif
#endif

#ifdef SAMAPP_ENABLE_APIS_SET7
#if defined(FT_801_ENABLE) || defined(FT_811_ENABLE)
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set7   Start!");
	SAMAPP_CoPro_Widget_Calibrate();
	SAMAPP_API_Screen("Bouncing Squares");
	BouncingSquares();

	SAMAPP_API_Screen("Bouncing Circles");
	BouncingCircles();

	SAMAPP_API_Screen("Bouncing Points");
	BouncingPoints();

	SAMAPP_API_Screen("Moving Points");
	MovingPoints();

	SAMAPP_API_Screen("Set7   End!");

#endif //FT_801_ENABLE
#endif

#ifdef SAMAPP_ENABLE_APIS_SET8
#if defined(FT_81X_ENABLE)
	
	
	SAMAPP_API_Screen("Set8   Start!");
	SAMAPP_CoPro_Widget_Calibrate();

	Ft_Gpu_CoCmd_ColdStart(phost);

	SAMAPP_API_Screen("Multi-track on an object.");
	SAMAPP_CoPro_MultiTracker();

	SAMAPP_API_Screen("Number Bases");
	NumberBases();

	SAMAPP_API_Screen("New bitmap formats, options, and sizes.");
	FT81xBitmapFormatAndOptions();  //png loading

	SAMAPP_API_Screen("Screen Orientation");
	ScreenOrientation();

	SAMAPP_API_Screen("Bitmap up to 2048pix x 2048pix.");
	HigherResolutionBitmap();
	
	SAMAPP_API_Screen("Paletted bitmap.");
	SAMAPP_81X_Paletted_Bitmap();

	SAMAPP_API_Screen("DXT1: reduce bitmap size upto 4 folds.");
	SAMAPP_GPU_DXT1();

	SAMAPP_API_Screen("Paletted ARGB888 format.");
	SAMAPP_GPU_Bitmap_Paletted8();
	

#if !defined(FT900_PLATFORM) && (!defined(ARDUINO_PLATFORM))
	{
		SAMAPP_API_Screen("AVI video playback");
		VideoPlayback();

		SAMAPP_API_Screen("AVI video playback via CMD buffer");
		VideoPlaybackViaCMDBuff();

		SAMAPP_API_Screen("AVI video playback via frame by frame");
		VideoPlayBackFrameByFrame();
	}
#endif
	SAMAPP_API_Screen("Set8   End!");

#endif
#endif
	/*
	 HAL Library
	 Demo app porting (6)

	 */
#if (defined (SAMAPP_ENABLE_APIS_SET9)) && (!defined(FT900_PLATFORM))
#ifdef SAMAPP_ENABLE_APIS_SET9
#if defined(FT_81X_ENABLE) && !defined(MSVC_FT800EMU)
	//SAMAPP_API_Reset_Context();
	SAMAPP_API_Screen("Set9   Start!");

	SAMAPP_API_Screen("Configurable clock frequency.");
	SAMAPP_ChangeFreq();

	SAMAPP_81X_PowerOffComponents();

	SAMAPP_API_Screen("Configurable GPIO/IO drive strength..");
	SAMAPP_81X_ChangePadDriveStrength();

	SAMAPP_API_Screen("Set9   End!");

#endif
#endif
#endif

/* Close all the opened handles */
Ft_Gpu_Hal_Close(phost);
Ft_Gpu_Hal_DeInit();
#if defined(MSVC_PLATFORM) || defined(FT900_PLATFORM)
return 0;
#endif
}

void loop() {
}

/* Nothing beyond this */

