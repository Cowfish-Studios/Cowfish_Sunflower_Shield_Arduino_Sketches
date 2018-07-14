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


/* Sample application to demonstrat FT800 primitives, widgets and customized screen shots */
#include "FT_Platform.h"

#if defined(ARDUINO_PLATFORM)
#include <Wire.h>
#include <string.h>
#include "sdcard.h"
#endif

#include "FT_App_Refrigerator.h"

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
#include "time.h"
#include "math.h"
#endif

#ifdef FT900_PLATFORM
#include "ff.h"
#endif
#define STARTUP_ADDRESS	100*1024L
#define FREEZER_POINT 1;
#define  FRIDGE_POINT 2;
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
/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;

#ifdef FT900_PLATFORM
	FATFS FatFs;
#endif

const ft_uint8_t FT_DLCODE_BOOTUP[12] =
{
    255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};


/* Startup Screen Information*/ 

char *info[] = { "FT800 Lift Application",
                          "API to demonstrate lift application,",
                          "with FTDI logo",
                          "& date and time display"
                       }; 

#ifdef FT900_PLATFORM
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
    } 	 local_time;
	local_time st,lt;



	 DWORD get_fattime (void)
	 {
	 	/* Returns current time packed into a DWORD variable */
	 	return 0;
	 }

#endif


ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;
ft_uint32_t music_playing = 0,fileszsave = 0;

#if defined(ARDUINO_PLATFORM)
Reader g_reader;
ft_uint8_t sd_present ;
#endif

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

/* Optimized implementation of sin and cos table - precision is 16 bit */
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


static void polarxy(ft_int32_t r, float th, ft_int16_t *x, ft_int16_t *y,ft_uint16_t ox,ft_uint16_t oy)
{
  th = (th * 32768L / 180);
  *x = (16 * ox + (((long)r * qsin(th)) >> 11) + 16);
  *y = (16 * oy - (((long)r * qcos(th)) >> 11));
}

ft_uint16_t da(ft_int32_t i)
{
  return (i - 45) * 32768L / 360;
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

/*
ft_void_t FT_Play_Sound(ft_uint8_t sound,ft_uint8_t vol,ft_uint8_t midi)
{
  ft_uint16_t val = (midi << 8) | sound;
  Ft_Gpu_Hal_Wr8(phost,REG_SOUND,val);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAY,1);
}
*/





ft_void_t Ft_BootupConfig()
{

	/* Do a power cycle for safer side */
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

		/* Access address 0 to wake up the FT800 */
		Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);
		Ft_Gpu_Hal_Sleep(20);

		/* Set the clk to external clock */
#if (!defined(ME800A_HV35R) && !defined(ME810A_HV35R))
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
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *pFile = fopen(pFileName,"rb");//TBD - make platform specific
#endif

#ifdef FT900_PLATFORM

	if(f_open(&FilSrc, pFileName, FA_READ) != FR_OK)
	printf("Error in opening file");
	else
	{
#endif



#if  defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU) || defined(FT900_Platform)

	/* inflate the data read from binary file */
	if(NULL == pFile)
	{
		printf("Error in opening file %s \n",pFileName);
	}
	else
	{
#endif
		Ft800_addr = DstAddr;

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
			/* copy data continuously into FT800 memory */
			Ft_Gpu_Hal_WrMem(phost,Ft800_addr,pbuff,blocklen);
			Ft800_addr += blocklen;
		}
		/* close the opened binary zlib file */
		fclose(pFile);
		free(pbuff);
	}
#endif


#ifdef FT900_PLATFORM

		FileLen = f_size(&FilSrc);
		pbuff = (ft_uint8_t *)malloc(8192);
		while(FileLen > 0)
		{
			/* download the data into the command buffer by 2kb one shot */
			ft_uint16_t blocklen = FileLen>8192?8192:FileLen;

			/* copy the data into pbuff and then transfter it to command buffer */
			f_read(&FilSrc,pbuff,blocklen,&numBytesRead);
			FileLen -= blocklen;
			/* copy data continuously into FT800 memory */
			Ft_Gpu_Hal_WrMem(phost,Ft800_addr,pbuff,blocklen);
			Ft800_addr += blocklen;
		}
		/* close the opened binary zlib file */
		f_close(&FilSrc);
		free(pbuff);
	}
#endif
	return 0;
}


/********API to return the assigned TAG value when penup,for the primitives/widgets******/

static ft_uint8_t keypressed=0;
ft_uint8_t Read_Keys()
{
  static ft_uint8_t Read_tag=0,temp_tag=0,ret_tag=0;	
  Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(Read_tag!=NULL)		
  {
    if(temp_tag!=Read_tag && Read_tag!=255)
    {
      temp_tag = Read_tag;	
      keypressed = Read_tag;
      //FT_Play_Sound(0x51,100,108);
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
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
// Ftdi Logo animation  
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ)); 
//Copy the Displaylist from DL RAM to GRAM
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -= 4;

#ifdef FT_81X_ENABLE
  dloffset -= 2*4;//remove two more instructions in case of 81x
#endif


  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,STARTUP_ADDRESS);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
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

	
//	Check if the Play key and change the color
    if(keypressed!='P')
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
  //FT_Play_Sound(0x50,255,108);
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



struct
{
	ft_uint8_t unlock :1;
	ft_uint8_t freeze :1;
	ft_uint8_t home :1;
	ft_uint8_t brightness:1;
	ft_uint8_t fridge:1;
	ft_uint8_t cubed:1;
	ft_uint8_t settings:1;
	ft_uint8_t change_background:1;
	ft_uint8_t font_size:1;
	ft_uint8_t sketch:1;
	ft_uint8_t food_tracker:1;
	ft_uint8_t clear:1;
	
}bitfield;

static byte istouch()
{
  return !(Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}

ft_int16_t font[] = {26, 27, 28, 29};
ft_int16_t freezer_temp[] = {-2,-4,-6,-8,-10,-12,-14,34,36,38,40,42,44,46};
ft_uint8_t incremental_flag = 1;

ft_void_t change_background()
{
	ft_int32_t Read_tag,tag = 0, pen_up = 0,xoffset, yoffset, yoffset1,xoffset1,yoffset3;
	ft_uint8_t z = 0; 
	char *bgd_raw_files[] = {"31.raw","32.raw","33.raw","34.raw","35.raw","36.raw"};//,"37.raw","38.raw","39.raw"};

	//xoffset = xoffset1 = 10;
	//yoffset = 30;
	//yoffset1 = 110;
	//yoffset3 = 190;
	//Load_Thumbnails();
	Ft_App_LoadRawFromFile("BG.raw",158*1024);
	Ft_Gpu_CoCmd_Dlstart(phost); 
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(158*1024));	
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,100*2,50));	
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 100, 50));
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	while(1)
	{
		Read_tag = Read_Keys();//Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
		//pen_up = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_DIRECT_Z1Z2);
		printf("keypressed %d\n",keypressed);
		if(8 == Read_tag)
		{
			break;
		}
		else if(Read_tag >=30 && Read_tag < 40)
		{
			Ft_App_LoadRawFromFile(bgd_raw_files[Read_tag - 30],RAM_G);		
		}
		
		Ft_Gpu_CoCmd_Dlstart(phost); 
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		
		
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		
		
		Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(0));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
		//Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 480, 272));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 800, 480));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 100, 50));
		Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_A(256));
		Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_E(256));
		Ft_App_WrCoCmd_Buffer(phost,TAG(8));
		//Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 10, 3, 3));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(750, 10, 3, 3));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		xoffset = 10;
		yoffset = 30;
		for(z = 0;z < 6; z++)
		{
			
			if(z == 4 ){ yoffset += 80; xoffset = 10;}
			
			Ft_App_WrCoCmd_Buffer(phost,TAG(30+z));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset , yoffset, 5, z));
			
			xoffset += 110;
		}

		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));			
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);				
	}
	//while(Read_Keys()==0);	
	bitfield.change_background = 0;
}


ft_void_t Sketch(ft_int16_t val_font)
{
  ft_uint32_t  tracker,color=0;
  ft_uint16_t  val=32768;
  ft_uint8_t tag =0;
  ft_int32_t xoffset1 = 10, Baddr;
  ft_int16_t NumSnowRange = 6, NumSnowEach = 10,RandomVal = 16,xoffset,yoffset, j;
  S_RefrigeratorAppSnowLinear_t S_RefrigeratorSnowArray[8*10],*pRefrigeratorSnow = NULL;

  Ft_Gpu_CoCmd_Dlstart(phost);
  Ft_Gpu_CoCmd_Sketch(phost,0,50,FT_DispWidth-60,FT_DispHeight-70,110*1024L,L8);
  
  Ft_Gpu_CoCmd_MemZero(phost,110*1024L,140*1024L);  
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(110*1024L));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,(FT_DispWidth-60),FT_DispHeight-20));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,(FT_DispWidth-60),(FT_DispHeight-20)));

  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);				
  while(1)
  {
    tag = Read_Keys();//Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
	//color = val*255;

    if(keypressed==2)	bitfield.clear = 1;
	
	if(tag == 8)
	{

		bitfield.sketch = 0;
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);//to stop the sketch command
		return;

	}

	if(bitfield.clear)
	{
		Ft_Gpu_CoCmd_Dlstart(phost);  
  		Ft_Gpu_CoCmd_MemZero(phost,110*1024L,140*1024L); // Clear the gram frm 1024 		
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		bitfield.clear = 0;
	}
    
    Ft_Gpu_CoCmd_Dlstart(phost);                  // Start the display list
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));	
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
	Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
	Ft_Gpu_CoCmd_Text(phost,200,10,font[val_font],OPT_CENTERX,"Add a description on the Notes");
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
    Ft_App_WrCoCmd_Buffer(phost,TAG(1));          // assign the tag value 
    //Ft_Gpu_CoCmd_FgColor(phost,0x060252);
	//Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255)); 
	


	Ft_Gpu_CoCmd_FgColor(phost,0x60252);
    Ft_App_WrCoCmd_Buffer(phost,TAG(2));          // assign the tag value 
    Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth-55),(FT_DispHeight-45),45,25,font[val_font],keypressed,"CLR");
    
	Ft_App_WrCoCmd_Buffer(phost,TAG(8));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));//Bitmap of the home icon
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 10, 3, 3));
	
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
        
    Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,50*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_int16_t)(FT_DispWidth-60)*16,(ft_int16_t)(FT_DispHeight-20)*16));
    
   // Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB((color>>16)&0xff,(color>>8)&0xff,(color)&0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(8, 8, 62));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0,50,0,0));

	animation();

    Ft_App_WrCoCmd_Buffer(phost,END());
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);	
  }
}

ft_uint8_t months[12][15] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
ft_uint8_t item_selection = 40;
ft_void_t food_tracker(ft_int16_t val_font)
{
	ft_int32_t x_fridge_icon, y_fridge_icon, x_fridge_icon_1,z,Read_tag,j,y,Baddr,m, n;
	ft_int32_t r_x1_rect, r_x2_rect, r_y1_rect, r_y2_rect;
	char *raw_files[] =  {"app1.raw","car0.raw","ki3.raw","tom1.raw","bery1.raw","chic1.raw","snap1.raw","prk1.raw","bf1.raw","sas1.raw"};
	
	Ft_Gpu_CoCmd_Dlstart(phost); 
	Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));//handle 7 is used for fridge items
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(78*1024));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 80*2, 80));
	//Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565, 80*2, 80));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 80, 80));

	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Baddr = 78*1024;



	for(y = 0; y < 10; y++)
	{
		Ft_App_LoadRawFromFile(raw_files[y],Baddr);
		Baddr += 80*80*2;
	}

	while(1)
	{
		Read_tag = Read_Keys();
		if(keypressed!=0 && keypressed>=40)
		item_selection=keypressed;
		if(Read_tag == 8){
				bitfield.food_tracker = 0;
				return;
			}
		Ft_Gpu_CoCmd_Dlstart(phost);  
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_Gpu_CoCmd_Text(phost,180,0,font[val_font],OPT_CENTERX,"Food Manager");
		Ft_Gpu_CoCmd_Text(phost,90,30,font[val_font],OPT_CENTERX,"Left Door");
		Ft_Gpu_CoCmd_Text(phost,310,30,font[val_font],OPT_CENTERX,"Right Door");
		animation();
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(5*16));
		
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(20*16,60*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(180*16,220*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(200*16,60*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(450*16,220*16));
		r_x1_rect = 30; 
		r_y1_rect = 70;
		for(n = 0; n < 4 ; n++)
		{
			if(n==2){ r_y1_rect += 80; r_x1_rect = 30;}
			if(	item_selection==40+n)
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));			
			else
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(80));	
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,TAG(40+n));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16*r_x1_rect,16*(r_y1_rect)));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16*(r_x1_rect+60),16*(r_y1_rect+60)));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
			Ft_App_WrCoCmd_Buffer(phost,TAG(40+n));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(r_x1_rect-10, r_y1_rect-10, 7, n));
			r_x1_rect += 80;
		}
		r_x1_rect = 210;
        r_y1_rect = 70;
		for(n = 0; n < 6 ; n++)
        {
			if(n==3){ r_y1_rect += 80; r_x1_rect = 210;}
			if(	item_selection==44+n)
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));		
			else
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(80));		
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,TAG(44+n));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16*r_x1_rect,16*(r_y1_rect)));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(16*(r_x1_rect+60),16*(r_y1_rect+60)));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));	
			Ft_App_WrCoCmd_Buffer(phost,TAG(44+n));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(r_x1_rect-10, r_y1_rect-10, 7, 4+n));
			r_x1_rect += 80;
          }

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

		Ft_App_WrCoCmd_Buffer(phost,TAG(8));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 0, 3, 3));

		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

}


}



ft_void_t cubed_setting(ft_uint16_t *cubed,ft_uint32_t val_font)
{
	ft_int32_t Read_tag,Baddr;
	static ft_int16_t cubed_funct= 0,img1_flag = 1,img2_flag = 0;

	cubed_funct = *cubed;
	Baddr = 94*1024;
	Ft_Gpu_CoCmd_Dlstart(phost); 
	Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
	Ft_App_LoadRawFromFile("cube1.raw",Baddr);
 	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(8));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 200*2, 200));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 200, 200));
	Baddr += 200*200*2;
	Ft_App_LoadRawFromFile("crush1.raw",Baddr);
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(9));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 200*2, 200));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 200, 200));
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);


	while(1)
	{
		Read_tag = Read_Keys();
		

		if(Read_tag == 8){
			bitfield.cubed = 0;
			return;
		}
		
		

		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

		Ft_Gpu_CoCmd_Text(phost,120,30,font[val_font],OPT_CENTER,"Cubed");
		Ft_Gpu_CoCmd_Text(phost,350,30,font[val_font],OPT_CENTER,"Crushed");
		animation();
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
		if(keypressed == 25 || img1_flag == 1)
		{
			
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(50));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(10*16,50*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(210*16,250*16));
			cubed_funct = 0;
			img1_flag = 1;
			img2_flag = 0;
		}
		
		if(keypressed == 26 || img2_flag == 1)
		{
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(50));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(250*16,50*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(450*16,250*16));
			cubed_funct = 1;
			img1_flag = 0;
			img2_flag = 1;
		}
		 
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(25));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(100));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(10, 50, 8, 0));
		Ft_App_WrCoCmd_Buffer(phost,TAG(26));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(250, 50, 9, 0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,TAG(8));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
		*cubed = cubed_funct;
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
	Ft_App_WrCoCmd_Buffer(phost,END());
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		
}
	
}


ft_int16_t NumSnowRange = 6, NumSnowEach = 10,RandomVal = 16,xoffset,yoffset, j;
S_RefrigeratorAppSnowLinear_t S_RefrigeratorSnowArray[8*10],*pRefrigeratorSnow = NULL;

ft_void_t animation()
{
	
		
		/* Draw background snow bitmaps */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		pRefrigeratorSnow = S_RefrigeratorSnowArray;
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(64));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
		for(j=0;j<(NumSnowRange*NumSnowEach);j++)
		{			
			
							
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));
			
			if( ( (pRefrigeratorSnow->xOffset > ((FT_DispWidth + 60)*16)) || (pRefrigeratorSnow->yOffset > ((FT_DispHeight + 60) *16)) ) ||
				( (pRefrigeratorSnow->xOffset < (-60*16)) || (pRefrigeratorSnow->yOffset < (-60*16)) ) )
			{
				pRefrigeratorSnow->xOffset = ft_random(FT_DispWidth*16);//FT_DispWidth*16 + ft_random(80*16);
				pRefrigeratorSnow->yOffset = FT_DispHeight*16 + ft_random(80*16);//ft_random(FT_DispHeight*16);
				pRefrigeratorSnow->dx = ft_random(RandomVal*8) - RandomVal*4;//-1*ft_random(RandomVal*8);
				pRefrigeratorSnow->dy = -1*ft_random(RandomVal*8);//ft_random(RandomVal*8) - RandomVal*4;

			}
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(pRefrigeratorSnow->xOffset, pRefrigeratorSnow->yOffset));
			pRefrigeratorSnow->xOffset += pRefrigeratorSnow->dx;
			pRefrigeratorSnow->yOffset += pRefrigeratorSnow->dy;
			pRefrigeratorSnow++;
		}	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));

}

ft_int16_t origin_xy[][4] = {{ 65,0,0,-90},
								 { 0,0,90,-65},
								 { 0,0,65,-90},
								 { 0,65,90,0},
								 { -65,90,0,0},
								 { -90,0,0,65},
								 { 0,0,-65,90},
								 };




ft_void_t construct_structure(ft_int32_t xoffset, ft_int16_t flag)//, ft_uint32_t tag)
{
		ft_int32_t z;
		ft_int16_t x0,y0,x1,y1,x2,x3,y2,y3,next_flag,thcurrval=0;
		ft_uint16_t angle = 0;//, angle1 = 0, angle2 = 0;


		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(60*16));
		Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(INCR,INCR));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset*16,150*16));
		
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINES));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(2 * 16));
		for(z = 45; z < 361; z+=45)
		{
			polarxy(100,z,&x0,&y0,xoffset,150);
			polarxy(60,z,&x1,&y1,xoffset,150);
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x0,y0));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1,y1));
		}

		Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(102,180,232));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(100*16));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(EQUAL,0,255));//stencil function to increment all the values

		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset*16,150*16));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(65*16));
		Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(INCR,INCR));
		
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
		

		if(flag >= 0 && flag < 7 )//&& angle2 >= 0)
		{
			next_flag = flag+1;
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
			angle = flag*45;
			polarxy(150,angle,&x0,&y0,xoffset,150);
			polarxy(150,angle+180,&x1,&y1,xoffset,150);
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x0 + origin_xy[flag][0] *16,y0 + origin_xy[flag][2]*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1 + origin_xy[flag][0] *16,y1 + origin_xy[flag][2] *16));
			
			polarxy(150,angle+45,&x0,&y0,xoffset,150);
			polarxy(150,angle+45+180,&x1,&y1,xoffset,150);
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x0 + origin_xy[flag][1] *16,y0 + origin_xy[flag][3]*16 ));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1 + origin_xy[flag][1] *16,y1 + origin_xy[flag][3] *16));

			Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
			
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
			//Ft_App_WrCoCmd_Buffer(phost, TAG(tag));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(19,43,59));
			Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(100*16));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
			Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(EQUAL,0,255));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset*16,150*16));

		}

		Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
		
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(19,43,59));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(100*16));
		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(EQUAL,0,255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset*16,150*16));			
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
}


ft_void_t change_temperature(ft_uint32_t *freeze_temp ,ft_uint32_t *fridge_temp, ft_int16_t val_font)
{
			
	ft_uint32_t track_val_min,track_val_max, tracker = 0;
	ft_int32_t Read_tag,trackval = 0, y, z = 45 , adj_val=0, prev_th = 0,  BaseTrackValSign = 0,major_axis_number = 0, i;
	ft_uint16_t track_val = 0;
	ft_int16_t x0,y0,x1,y1,x2,x3,y2,y3,temp,thcurrval=0;
	ft_int16_t thcurr = 0;
	static ft_uint16_t angle = 0, angle1 = 0, angle2 = 0,flag = 0,next_flag,flag_1=0, angle_val = 0;
	ft_uint8_t freezer_tempr = 0, fridge_tempr = 0;
	ft_int16_t r;



	ft_int16_t NumSnowRange = 6, NumSnowEach = 10,RandomVal = 16,xoffset,yoffset, j;
	S_RefrigeratorAppSnowLinear_t S_RefrigeratorSnowArray[8*10],*pRefrigeratorSnow = NULL;

	thcurr = *freeze_temp;
	thcurrval  = *fridge_temp;

	Ft_Gpu_CoCmd_Track(phost,120,150,1,1,20);
	Ft_Gpu_CoCmd_Track(phost,350,150,1,1,30);

	while(1)
	{
		Read_tag = Read_Keys();
		tracker = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
		track_val  = tracker >> 16;

		if(keypressed == 8)
		{
			bitfield.freeze = 0;
			bitfield.fridge = 0;
			return;
		}
		if(istouch())
		{	
			if(keypressed==30 || keypressed==20)
			{
				temp = track_val/182;
				if(temp < 316)
				{	
					if(keypressed==30)
					angle2 = track_val/182;
					else if(keypressed==20)
					angle1 = track_val/182;
				
					flag =   angle1/45;
					flag_1 =   angle2/45;
				}
					
			}
		}

		Ft_Gpu_CoCmd_Dlstart(phost);  	
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
		
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_Gpu_CoCmd_Text(phost,120,30,font[val_font],OPT_CENTER,"Freezer Temperature");
		Ft_Gpu_CoCmd_Text(phost,350,30,font[val_font],OPT_CENTER,"Fridge Temperature");

		construct_structure(120,flag);//,20);
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(0,1,0));
		construct_structure(350,flag_1);//,30);

		for(r = 0;r<2;r++)
		{
			
			if(r == 0) 	x2 = 120;
			else		x2 = 350;
		
			angle_val = 135+23; 
			for(z=0;z<7;z++)
			{
				angle_val +=45;
				angle_val %= 360;
                polarxy(80,angle_val,&x0,&y0,x2,150);
				Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,28,OPT_SIGNED|OPT_CENTER,freezer_temp[(r*7)+z]);

			}
	}

		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(1,1,1));
		*freeze_temp = flag*-2-2;
		polarxy(0,0,&x0,&y0,120,150);
		Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,30,OPT_SIGNED|OPT_CENTER,*freeze_temp);
		Ft_Gpu_CoCmd_Text(phost,150,145,17,OPT_CENTER,"x");
		Ft_Gpu_CoCmd_Text(phost,160,150,30,OPT_CENTER,"F");
		*fridge_temp = flag_1*2 + 34;
		polarxy(0,0,&x0,&y0,350,150);
		Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,30,OPT_SIGNED|OPT_CENTER,*fridge_temp);
		Ft_Gpu_CoCmd_Text(phost,370,145,17,OPT_CENTER,"x");
		Ft_Gpu_CoCmd_Text(phost,380,150,30,OPT_CENTER,"F");
				
		
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(140 , 205, 3,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(370 , 205, 3,1));
		

		animation();


		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(100*16));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(20));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(120*16,150*16));
		Ft_App_WrCoCmd_Buffer(phost,TAG(30));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(350*16,150*16));

		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(60*16));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(31));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(120*16,150*16));
		Ft_App_WrCoCmd_Buffer(phost,TAG(32));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(350*16,150*16));

		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,TAG(8));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	}

}

ft_uint16_t sleep_cts=0;
ft_uint32_t val;
ft_void_t change_brightness(ft_uint32_t val,ft_uint32_t val_font)
{
	ft_int32_t Read_tag,x;
	static ft_uint32_t tracker = 0,track_val = 420;
	ft_uint16_t pwm_val=0,rval = 0,pwmi_val = 0;

	val = 128;
	Ft_Gpu_CoCmd_Track(phost,20,200,420,40,12);
	
	while(1)
	{
		Read_tag = Read_Keys();

		tracker = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);

		if((tracker&0xff)==12)
		{
			
			pwmi_val = tracker>>16;
			
			if(pwmi_val<65535)
			track_val=pwmi_val/154;
		}
	
		if(keypressed == 8){
			bitfield.brightness = 0;
			return;
		}
		


		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		animation();

		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_Gpu_CoCmd_Text(phost,200,10,font[val_font],OPT_CENTERX,"Track the bar to adjust the brightness");
		Ft_App_WrCoCmd_Buffer(phost,TAG(8));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
		
		Ft_App_WrCoCmd_Buffer(phost,TAG(12));
		Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(20, 200)); // Scissor rectangle bottom left at (20, 200)
		Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(420, 40)); // Scissor rectangle is 420 x 40 pixels
		Ft_Gpu_CoCmd_Gradient(phost, 20,0,0x0000ff,440,0,0xff0000);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255, 255, 255)); 

		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));


	
		if(track_val)
		val = 10+(track_val/3);
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,val);
		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));

		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((20 + track_val)*16,190*16));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(440*16,250*16));	

			
		//}

		//Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		}
	//return val;
}
ft_void_t scrensaver()
{
	ft_int32_t xoffset,yoffset,flagloop,xflag = 1,yflag = 1;
	ft_uint8_t Read_tag = 0;

	xoffset = FT_DispWidth/3;
	yoffset = FT_DispHeight/2;
	
	while(1)
	{

		if(istouch())
		{
			sleep_cts = 0;
			return;
		}
		else
		{
			
		Ft_App_WrCoCmd_Buffer(phost, CMD_SCREENSAVER);//screen saver command will continuously update the macro0 with vertex2f command	
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));

		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Scale(phost, 2*65536,2*65536);//scale the bitmap 3 times on both sides
		Ft_Gpu_CoCmd_SetMatrix(phost);
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(4));
		Ft_App_WrCoCmd_Buffer(phost,MACRO(0));
		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
              
	}
	}


	
}

ft_void_t FT_LiftApp_Refrigerator()
{
	ft_int32_t xoffset1,xoffest2,BMoffsetx1,BMoffsety1, BMoffsetx2,BMoffsety2, y, j;
	ft_uint32_t val = 0, tracker = 0,tracking_tag=0,track_val_min,track_val_max, unit_val, val_font = 0;
	ft_char8_t img_n;
	ft_uint16_t track_val = 0, cubed = 0;
	ft_int16_t rval=0,pwm_val=0, font_number,m,x;
	ft_int16_t freeze_temp = 0,fridge_temp = 0;
	ft_int32_t Baddr;
	ft_uint32_t trackval = 0;
	ft_uint8_t k,ice_cell;
	static ft_uint8_t blob_i;
	ft_uchar8_t r,g, b;
#ifndef FT900_PLATFORM
	SYSTEMTIME lt;
#else

#endif
	ft_uint16_t yoff;
	ft_uint8_t Clocks_DL[548],snapflag = 0;
	
	 const char ice[2][8] = {"Cubed","Crushed"};
	ft_uint32_t thcurr = 0,thcurrval=0, pwmval_return = 0;
	//ft_int16_t thcurr = 0,thcurrval=0;
	ft_uint8_t Read_tag = 0;
	ft_int32_t i = 0,pwm = 0;
	
	ft_uint8_t fonts_size_t = 1;

	ft_uint32_t temp = ft_millis();
	
	xoffset1 = 10;
	xoffest2 = 50;
	thcurr = -2;
	thcurrval = 34;
	cubed = 0;

	Ft_Gpu_CoCmd_Dlstart(phost);  
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
	Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());

	Ft_App_LoadRawFromFile("gn.raw",RAM_G);//240 * 136	green and blue
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565, 240*2, 136));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 512, 512));
	
	Baddr = RAM_G + 240*2*136;
	Ft_App_LoadRawFromFile("5icons_5.raw",Baddr);
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 18, 37));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 35, 35));

	Baddr += 221*(36/2); 
	Ft_App_LoadRawFromFile("f1icons.raw", Baddr);
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(3));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 15, 32));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 32));
		
	Baddr += 192*(32/2);
	Ft_App_LoadRawFromFile("logo1.raw",Baddr);
 	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(4));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 65*2, 25));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 512, 512));
	Baddr += 65*25*2;

	Ft_App_LoadRawFromFile("snow_1.raw",Baddr);
 	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 25, 50));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 50, 50));
	Baddr += 50*(50/2);

	Ft_App_WrCoCmd_Buffer(phost,END());
		
	Ft_Gpu_CoCmd_Track(phost,170,180,120,8,17);
	//Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,128);//Ft_App_fadein();

	val = 128;
	pwm_val = 10;
	rval = 0;


	/* compute the random values at the starting*/
	pRefrigeratorSnow = S_RefrigeratorSnowArray;
	for(j=0;j<(NumSnowRange*NumSnowEach);j++)
	{
		//always start from the right and move towards left
		pRefrigeratorSnow->xOffset = ft_random(FT_DispHeight*16);//ft_random(FT_DispWidth*16);
		pRefrigeratorSnow->yOffset = ft_random(FT_DispWidth*16);//ft_random(FT_DispHeight*16);
		pRefrigeratorSnow->dx =  ft_random(RandomVal*8) - RandomVal*8;//-1*ft_random(RandomVal*8);//always -ve
		pRefrigeratorSnow->dy = -1*ft_random(RandomVal*8);//ft_random(RandomVal*8) - RandomVal*8;
		pRefrigeratorSnow++;
	} 

	//scrensaver();


  	while(1)
	{
#ifndef FT900_PLATFORM
  		GetLocalTime(&lt);
#endif
		Read_tag = Read_Keys();

		if(!istouch() && bitfield.unlock)
		{
			sleep_cts++;
			if(sleep_cts>100)
			scrensaver();
		}
		
			pwm_val = (val - rval) / 16;
             rval += pwm_val;  
		 if(rval <= 10)
			rval = 10;
		 if(keypressed == 17 || keypressed == 12)
		 tracker = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
	   
		if(Read_tag == 1)
		{
			bitfield.unlock = (bitfield.unlock == 0)?1 : 0;
			if(bitfield.unlock == 0)
			Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,rval);
		}	
		if(Read_tag == 2)
			bitfield.settings = 1;
		if((Read_tag == 8) &(bitfield.settings == 1))
			bitfield.settings = 0;
		if((Read_tag == 8) &(bitfield.font_size == 1))
			bitfield.font_size = 0;
		if(Read_tag == 9)
			bitfield.change_background = 1;
		if(Read_tag == 10)
			bitfield.font_size ^= 1;
		if(Read_tag == 3)
			bitfield.brightness ^= 1;
		if(Read_tag == 4)
			bitfield.sketch = 1;
		if(Read_tag == 11)
			bitfield.food_tracker = 1;
		if(Read_tag == 5)
			bitfield.freeze = 1;
		if(Read_tag == 6)
			bitfield.fridge = 1;
		if(Read_tag == 7)
			bitfield.cubed = 1;
		if(Read_tag == 15)	
			fonts_size_t = 0;
		if(Read_tag == 16)
			fonts_size_t = 1;	
		
		if(keypressed == 17)
		{
			trackval = tracker >> 16;
			val_font = (trackval/16384);
		}

		


		Ft_Gpu_CoCmd_Dlstart(phost);  
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		Ft_App_WrCoCmd_Buffer(phost,TAG(0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Scale(phost, 65536,65536);//scale the bitmap 3 times on both sides
		Ft_Gpu_CoCmd_SetMatrix(phost);
		//Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(390, 0, 4, 0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(280, 0, 4, 0));

		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		animation();

		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

		Ft_Gpu_CoCmd_Number(phost,(270 - (val_font * 38)),135,font[val_font],OPT_CENTERX|OPT_CENTERY|OPT_SIGNED,thcurr);
		Ft_Gpu_CoCmd_Text(phost,(282  - (val_font * 37) ),135,17,OPT_CENTERX|OPT_CENTERY,"x");
		Ft_Gpu_CoCmd_Text(phost,(292 - (val_font * 38)),135,font[val_font],OPT_CENTERX|OPT_CENTERY,"F");


		Ft_Gpu_CoCmd_Number(phost,(350 - (val_font * 37)),135,font[val_font],OPT_CENTERX|OPT_CENTERY,thcurrval);
		//Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((362 - (val_font * 36)), 135, 17, 'x'));
		Ft_Gpu_CoCmd_Text(phost,(362 - (val_font * 36)),135,17,OPT_CENTERX|OPT_CENTERY,"x");
		Ft_Gpu_CoCmd_Text(phost,(372 - (val_font * 36)),135,font[val_font],OPT_CENTERX|OPT_CENTERY,"F");

		ice_cell  = (cubed == 0)?4 : 5;
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((420 - (val_font * 30)), 120, 3, ice_cell));

		Ft_App_WrCoCmd_Buffer(phost,TAG(1));
		 Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		if(bitfield.unlock)
		{

			
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 5, 200, 2, 0));
			Ft_App_WrCoCmd_Buffer(phost,TAG(0));
			Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,25);
			Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		}
		else
		{
			
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 5, 200, 2, 1));
		}

		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
		Ft_App_WrCoCmd_Buffer(phost,TAG(2));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 70, 200, 2, 2));
		Ft_App_WrCoCmd_Buffer(phost,TAG(3));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 142, 200, 2, 3));
		Ft_App_WrCoCmd_Buffer(phost,TAG(4));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 210, 200, 2, 4));
		Ft_App_WrCoCmd_Buffer(phost,TAG(11));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset1 + 280, 200, 2, 5));

		Ft_App_WrCoCmd_Buffer(phost,TAG(5));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(250 - (val_font * 50) , 95, 3,0));
		Ft_Gpu_CoCmd_Text(phost,(300 - (val_font * 45)) ,110,font[val_font],OPT_CENTERX|OPT_CENTERY,"Freeze");


		Ft_App_WrCoCmd_Buffer(phost,TAG(6));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(320 - (val_font * 40), 95, 3, 1));
		Ft_Gpu_CoCmd_Text(phost,(370 - (val_font * 37) ),110,font[val_font],OPT_CENTERX|OPT_CENTERY,"Fridge");


		Ft_App_WrCoCmd_Buffer(phost,TAG(7));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((400 - (val_font * 30)), 95, 3, 2));
       	Ft_Gpu_CoCmd_Text(phost,(450 - (val_font * 25)) ,110,font[val_font],OPT_CENTERX|OPT_CENTERY,ice[cubed]);


		switch(val_font)
		{
			case 0:
			{
				Ft_Gpu_CoCmd_Text(phost,53,34,font[val_font],OPT_CENTERX|OPT_CENTERY,months+(lt.wMonth-1));
				Ft_Gpu_CoCmd_Number(phost,98, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wDay);
				Ft_Gpu_CoCmd_Text(phost,108,34,font[val_font],OPT_CENTERX|OPT_CENTERY,",");
				Ft_Gpu_CoCmd_Number(phost,128,34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wYear);
				Ft_Gpu_CoCmd_Number(phost,40,54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wHour);
				Ft_Gpu_CoCmd_Text(phost,50,54,font[val_font],OPT_CENTERX|OPT_CENTERY,":");
				Ft_Gpu_CoCmd_Number(phost,60, 54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wMinute);
				break;
			}

			case 1:{
				Ft_Gpu_CoCmd_Text(phost,65,34,font[val_font],OPT_CENTERX|OPT_CENTERY,months+(lt.wMonth-1));
				Ft_Gpu_CoCmd_Number(phost,120, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wDay);
				Ft_Gpu_CoCmd_Text(phost,130,34,font[val_font],OPT_CENTERX|OPT_CENTERY,",");
				Ft_Gpu_CoCmd_Number(phost,155, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wYear);
				Ft_Gpu_CoCmd_Number(phost,48, 54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wHour);
				Ft_Gpu_CoCmd_Text(phost,58,54,font[val_font],OPT_CENTERX|OPT_CENTERY,":");
				Ft_Gpu_CoCmd_Number(phost,70, 54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wMinute);
				break;
			}
			case 2:{
				Ft_Gpu_CoCmd_Text(phost,65,34,font[val_font],OPT_CENTERX|OPT_CENTERY,months+(lt.wMonth-1));
				Ft_Gpu_CoCmd_Number(phost,130, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wDay);
				Ft_Gpu_CoCmd_Text(phost,145,34,font[val_font],OPT_CENTERX|OPT_CENTERY,",");
				Ft_Gpu_CoCmd_Number(phost,175, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wYear);
				Ft_Gpu_CoCmd_Number(phost,60, 54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wHour);
				Ft_Gpu_CoCmd_Text(phost,75,54,font[val_font],OPT_CENTERX|OPT_CENTERY,":");
				Ft_Gpu_CoCmd_Number(phost,87, 54,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wMinute);
				break;
			}
			case 3:{
				Ft_Gpu_CoCmd_Text(phost,65,34,font[val_font],OPT_CENTERX|OPT_CENTERY,months+(lt.wMonth-1));
				Ft_Gpu_CoCmd_Number(phost,140, 34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wDay);
				Ft_Gpu_CoCmd_Text(phost,160,34,font[val_font],OPT_CENTERX|OPT_CENTERY,",");
				Ft_Gpu_CoCmd_Number(phost,190,34,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wYear);
				Ft_Gpu_CoCmd_Number(phost,48, 64,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wHour);
				Ft_Gpu_CoCmd_Text(phost,63,64,font[val_font],OPT_CENTERX|OPT_CENTERY,":");
				Ft_Gpu_CoCmd_Number(phost,83, 64,font[val_font],OPT_CENTERX|OPT_CENTERY,lt.wMinute);
				break;
			}

		}


		if(bitfield.settings)
		{
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(34, 50, 224));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(150*16,30*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(370*16,60*16));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
			Ft_Gpu_CoCmd_Text(phost,240,45,font[val_font],OPT_CENTERX|OPT_CENTERY,"SETTINGS");

			Ft_App_WrCoCmd_Buffer(phost,TAG(9));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(162, 229, 242));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(150*16,60*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(370*16,130*16));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0, 0, 0));
			Ft_Gpu_CoCmd_Text(phost,260,85,font[val_font],OPT_CENTERX|OPT_CENTERY,"Change Background");

			Ft_App_WrCoCmd_Buffer(phost,TAG(10));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(34, 50, 224));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(150*16,110*16));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(370*16,160*16));
			//Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(162, 229, 242));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0, 0, 0));
			Ft_Gpu_CoCmd_Text(phost,250,130,font[val_font],OPT_CENTERX|OPT_CENTERY,"Change Font Size");

			Ft_App_WrCoCmd_Buffer(phost,TAG(8));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));//Bitmap of the home icon
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(320, 30, 3, 3));

		}


		if(bitfield.font_size)
		{
			//bitfield.settings = 0;
			Ft_App_WrCoCmd_Buffer(phost,TAG(17));
			Ft_Gpu_CoCmd_Slider(phost,170, 180, 120, 8, 0,trackval, 65535);
			Ft_Gpu_CoCmd_Text(phost,200,170,font[val_font],OPT_CENTERX|OPT_CENTERY,"FONT SIZE");
			Ft_Gpu_CoCmd_Number(phost,170,200,font[val_font],OPT_CENTERX|OPT_CENTERY,val_font);
		}
		
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));


		Ft_App_WrCoCmd_Buffer(phost,END());
		Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);


		if(bitfield.cubed)
			cubed_setting(&cubed,val_font);
		if(bitfield.brightness)
		{
			change_brightness(val,val_font);
		}

		if(bitfield.change_background)
			change_background();		
		if(bitfield.sketch)
			Sketch(val_font);
		if(bitfield.freeze | bitfield.fridge)// | bitfield.cubed)
			change_temperature(&thcurr,&thcurrval,val_font);
		if(bitfield.food_tracker)
			food_tracker(val_font);
			
	}
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
		        "Welcome to Refrigerator Example ... \r\n"
		        "\r\n"
		        "--------------------------------------------------------------------- \r\n"
		        );

		    //init_printf(UART0,myputc);

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
	ft_char8_t *path = "";
	FT900_Config();

		sys_enable(sys_device_sd_card);
	//	sdhost_sys_init();
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
#elif defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	ft_char8_t *path = "..\\..\\..\\Test";
#endif
	Ft_Gpu_HalInit_t halinit;
	
	halinit.TotalChannelNum = 1;

	Ft_Gpu_Hal_Init(&halinit);
	host.hal_config.channel_no = 0;
		host.hal_config.pdn_pin_no = FT800_PD_N;
		host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;
#ifdef MSVC_PLATFORM_SPI
	//host.hal_config.spi_clockrate_khz = 15000; //in KHz
	host.hal_config.spi_clockrate_khz = 12000; //in KHz
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


#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	if(_chdir(path))
		printf("Error: unable to change working directory.\n");
#endif
	Info();
	//FT_LiftApp_Portrait();
	//FT_LiftApp_landscape();

	FT_LiftApp_Refrigerator();
	scrensaver();
	//menu();
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













