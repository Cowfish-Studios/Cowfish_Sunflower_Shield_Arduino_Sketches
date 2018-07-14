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
#include <SPI.h>
#include <Wire.h>
#include <string.h>
#include "FT_Platform.h"
#include "sdcard.h"
#include "FT_App_Refrigerator.h"


#define STARTUP_ADDRESS	100*1024L
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


const ft_uint8_t FT_DLCODE_BOOTUP[12] =
{
    255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};
ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;
ft_uint32_t music_playing = 0,fileszsave = 0;
Reader g_reader;
ft_uint8_t sd_present ;
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


/********API to return the assigned TAG value when penup,for the primitives/widgets******/

ft_uint8_t keypressed=0;
ft_uint8_t temp_tag_keys=0;
ft_uint8_t tracker_tag = 1;
ft_uint8_t Read_Keys()
{
  
  ft_uint8_t  ret_tag_keys=0,Read_tag_keys=0;
  Read_tag_keys = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag_keys = 0;
  if(Read_tag_keys!=0)                  
  {
    if(temp_tag_keys!=Read_tag_keys && Read_tag_keys!=255)
    {
      temp_tag_keys = Read_tag_keys;     
      keypressed = Read_tag_keys;
      if(!tracker_tag)
      Play_Sound(0x51,100,108);
    }  
  }
  else
  {
    if(temp_tag_keys!=0 && !istouch())
    {
        ret_tag_keys = temp_tag_keys;
                temp_tag_keys = 0;
    }  
    keypressed = 0;
  }
  return ret_tag_keys;
}

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
	}

}

ft_void_t Play_Sound(ft_uint8_t sound,ft_uint8_t vol,ft_uint8_t midi)
{
  ft_uint16_t val = (midi << 8) | sound;
  Ft_Gpu_Hal_Wr8(phost,REG_SOUND,val);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAY,1);

  Ft_Gpu_Hal_Wr8(phost, REG_GPIOX_DIR, 0x8008);
  Ft_Gpu_Hal_Wr8(phost, REG_GPIOX, 0x8);
}

typedef struct Bitfield
{
	ft_uint8_t unlock;
	ft_uint8_t freeze;
	ft_uint8_t brightness;
	ft_uint8_t fridge;
	ft_uint8_t cubed;
	ft_uint8_t settings;
	ft_uint8_t change_background;
	ft_uint8_t font_size;
	ft_uint8_t sketch;
	ft_uint8_t food_tracker;	
}Bitfield;

static byte istouch()
{
  return !(Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}



FT_PROGMEM ft_prog_char8_t font[4] = {26, 27, 28, 29};
ft_int16_t freezer_temp[] = {-2,-4,-6,-8,-10,-12,-14,34,36,38,40,42,44,46};
FT_PROGMEM ft_prog_char8_t bgd_raw_files[9][7] = {"31.raw","32.raw","33.raw","34.raw","35.raw","36.raw"};//,"37.raw","38.raw","39.raw"};
Bitfield bitfield;

ft_void_t change_background()
{
  char filename[10];
  ft_int32_t xoffset = 10;
  ft_uint8_t Read_tag = 0;
  
  g_reader.openfile("BG.raw");
  Ft_App_LoadRawFromFile(g_reader,158*1024L);

  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(158*1024L));	
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,100*2,50));	
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER,  100, 50));	
  Ft_App_WrCoCmd_Buffer(phost,END());
  Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  do
  {
    Read_tag = Read_Keys();
    if(Read_tag >=30 && Read_tag < 40)
    {
      strcpy_P(filename,&bgd_raw_files[Read_tag - 30][0]);
      g_reader.openfile(filename);
      Ft_App_LoadRawFromFile(g_reader,RAM_G);			
    }
    		
    Ft_Gpu_CoCmd_Dlstart(phost); 
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_A(128));
    Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_E(128));
    Ft_App_WrCoCmd_Buffer(phost,TAG(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 480, 272));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 100, 50));
    Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer( phost,BITMAP_TRANSFORM_E(256));
    Ft_App_WrCoCmd_Buffer(phost,TAG(8));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 10, 3, 3));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
    
	  
    xoffset = 10; 
    int yoff = 30;
    for(int x = 0;x < 6; x++)
    {
      if(x == 4 )
      {
        yoff += 80; xoffset = 10;
       }
    Ft_App_WrCoCmd_Buffer(phost,TAG(30 + x));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset , yoff, 5, x));
    xoffset += 110;
    }
                
    Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);				
    }while(Read_tag!=8);
	bitfield.change_background = 0;
}


ft_void_t Sketch(ft_int16_t val_font)
{

  ft_uint8_t tag =0;
  ft_int32_t xoffset1 = 10;
  
  Ft_Gpu_CoCmd_Dlstart(phost);
  Ft_Gpu_CoCmd_Sketch(phost,0,50,FT_DispWidth-60,FT_DispHeight-70,110*1024L,L8); 
  Ft_Gpu_CoCmd_MemZero(phost,110*1024L,140*1024L);  
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(110*1024L));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,(FT_DispWidth-60),FT_DispHeight-50));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,(FT_DispWidth-60),(FT_DispHeight-50)));
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);				
  
  while(1)
  {
    tag = Read_Keys();
    if(tag==2)
    {
      Ft_Gpu_CoCmd_Dlstart(phost);  
      Ft_Gpu_CoCmd_MemZero(phost,110*1024L,140*1024L); // Clear the gram frm 1024 		
      Ft_App_Flush_Co_Buffer(phost);
      Ft_Gpu_Hal_WaitCmdfifo_empty(phost);	
    }
    if(tag == 8)
    {
      bitfield.sketch = 0;
      Ft_Gpu_Hal_WrCmd32(phost,  CMD_STOP);//to stop the sketch command
      return;
    }
    
    Ft_Gpu_CoCmd_Dlstart(phost);                  // Start the display list
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));	
    
     	
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));	
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    Ft_Gpu_CoCmd_Text(phost,200,10,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX,"Add a description on the Notes");
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
    Ft_Gpu_CoCmd_FgColor(phost,0x060252);
    Ft_App_WrCoCmd_Buffer(phost,TAG(2));          // assign the tag value 
    Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth-55),(FT_DispHeight-45),45,25,ft_pgm_read_byte(&font[val_font]),0,"CLR");
   
    Ft_App_WrCoCmd_Buffer(phost,TAG(8));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 10, 3, 3));
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
    Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,50*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((ft_int16_t)(FT_DispWidth-60)*16,(ft_int16_t)(FT_DispHeight-20)*16));
    
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

FT_PROGMEM ft_prog_char8_t raw_files[10][10] = {"app1.raw","car0.raw","ki3.raw","tom1.raw","bery1.raw","chic1.raw","snap1.raw","prk1.raw","bf1.raw","sas1.raw"};
ft_uint8_t item_selection = 40;

ft_void_t food_tracker(ft_int16_t val_font)
{
  ft_int32_t z,Read_tag,j,y,Baddr,m,n;
  ft_int32_t r_x1_rect, r_x2_rect, r_y1_rect, r_y2_rect;
  char filename[10] = "\0";
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
  Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));//handle 7 is used for fridge items
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(78*1024L));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 80*2, 80));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 80, 80));

  Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  Baddr = 78*1024L;
  for(y = 0; y < 10; y++)
  {
    strcpy_P(filename,&raw_files[y][0]);
    g_reader.openfile(filename);
    Ft_App_LoadRawFromFile(g_reader,Baddr);
    Baddr += 80*80*2;
  }
  while(1)
  {
    Read_tag = Read_Keys();
    if(keypressed!=0 && keypressed>=40)
	item_selection=keypressed;
    if(Read_tag == 8)
    {
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
    Ft_Gpu_CoCmd_Text(phost,240,0,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX,"Food Manager");
    Ft_Gpu_CoCmd_Text(phost,90,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX,"Left Door");
    Ft_Gpu_CoCmd_Text(phost,310,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX,"Right Door");
    animation();
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(5*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(80));
  
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(20*16,60*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(180*16,220*16));
		
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(200*16,60*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(450*16,220*16));

    r_x1_rect = 30; 
    r_y1_rect = 70;
    for(n = 0; n < 4 ; n++)
    {
      if(n==2)
      {
       r_y1_rect += 80;
       r_x1_rect = 30;
      }
      if(item_selection==40+n)
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
	if(n==3)
        {
         r_y1_rect += 80;
         r_x1_rect = 210;
        }
	if(item_selection==44+n)
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

	  Ft_App_WrCoCmd_Buffer(phost,TAG(8));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 0, 3, 3));

          Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	  Ft_Gpu_CoCmd_Swap(phost);
	  Ft_App_Flush_Co_Buffer(phost);
	  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
      }
}


ft_void_t cubed_setting(ft_uint16_t *cubed,ft_int16_t val_font)
{
   ft_int32_t Read_tag,Baddr;
   static ft_int16_t cubed_funct= 0,img1_flag = 1,img2_flag = 0;
   Baddr = 94*1024L;
   cubed_funct = *cubed;

    Ft_Gpu_CoCmd_Dlstart(phost); 
    g_reader.openfile("cube1.raw");
    Ft_App_LoadRawFromFile(g_reader,Baddr);
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(8));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 200*2, 200));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 200, 200));
    Baddr += 80*1024L;
    g_reader.openfile("crush1.raw");
    Ft_App_LoadRawFromFile(g_reader,Baddr);
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(9));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 200*2, 200));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 200, 200));
    Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    do
    {
      Read_tag = Read_Keys();
      Ft_Gpu_CoCmd_Dlstart(phost);
      Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
      Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

      Ft_Gpu_CoCmd_Text(phost,120,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTER,"Cubed");
      Ft_Gpu_CoCmd_Text(phost,350,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTER,"Crushed");
		
      animation();
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(50));  
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
      
      if(keypressed == 25 || img1_flag == 1)
      {
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(10*16,50*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(210*16,250*16));
	cubed_funct = 0;
	img1_flag = 1;
	img2_flag = 0;
      }
      if(keypressed == 26 || img2_flag == 1)
      {
        Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(250*16,50*16));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(450*16,250*16));
	cubed_funct = 1;
	img1_flag = 0;
	img2_flag = 1;
    }  
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,TAG(25));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(100));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(10, 50, 8, 0));
    Ft_App_WrCoCmd_Buffer(phost,TAG(26));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(250, 50, 9, 0));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,TAG(8));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
    *cubed = cubed_funct;

    Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    }while(Read_tag!=8);	
}


FT_PROGMEM ft_prog_uint16_t NumSnowRange = 4, NumSnowEach = 10,RandomVal = 16;
ft_int16_t xoffset,yoffset, j;
S_RefrigeratorAppSnowLinear_t S_RefrigeratorSnowArray[4*10],*pRefrigeratorSnow = NULL;


ft_void_t animation()
{
  /* Draw background snow bitmaps */
  pRefrigeratorSnow = S_RefrigeratorSnowArray;
  Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
  
  Ft_App_WrCoCmd_Buffer(phost,COLOR_A(64));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));
  for(j=0;j<(NumSnowRange*NumSnowEach);j++)
  {			
    if( ( (pRefrigeratorSnow->xOffset > ((FT_DispWidth + 60)*16)) || (pRefrigeratorSnow->yOffset > ((FT_DispHeight + 60) *16)) ) ||
				( (pRefrigeratorSnow->xOffset < (-60*16)) || (pRefrigeratorSnow->yOffset < (-60*16)) ) )
    {
	pRefrigeratorSnow->xOffset = ft_random(FT_DispWidth*16);
	pRefrigeratorSnow->yOffset = FT_DispHeight*16 + ft_random(80*16);
	pRefrigeratorSnow->dx = ft_random(RandomVal*8) - RandomVal*4;
	pRefrigeratorSnow->dy = -1*ft_random(RandomVal*8);
    }
    
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(pRefrigeratorSnow->xOffset, pRefrigeratorSnow->yOffset));
    pRefrigeratorSnow->xOffset += pRefrigeratorSnow->dx;
    pRefrigeratorSnow->yOffset += pRefrigeratorSnow->dy;
    pRefrigeratorSnow++;
    }	

}

ft_int16_t origin_xy[][4] = {{ 65,0,0,-90},
                             { 0,0,90,-65},
			     { 0,0,65,-90},
			     { 0,65,90,0},
			     { -65,90,0,0},
                             { -90,0,0,65},
                             { 0,0,-65,90},
			      };


ft_void_t construct_structure(ft_int32_t xoffset, ft_int16_t flag)
{
  ft_int32_t z;
  ft_int16_t x0,y0,x1,y1,x2,x3,y2,y3,next_flag,thcurrval=0;
  ft_uint16_t angle = 0, angle1 = 0, angle2 = 0;
  
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
  Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS) );
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(102,180,232));
  Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(100*16));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
  Ft_App_WrCoCmd_Buffer(phost,STENCIL_FUNC(EQUAL,0,255));//stencil function to increment all the values
  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset*16,150*16));
  Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(65*16));
  Ft_App_WrCoCmd_Buffer(phost,STENCIL_OP(INCR,INCR));
  
  Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
  if(flag >= 0 && flag < 7 )
  {
    next_flag = flag+1;
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0));
    angle2 = flag*45;
    polarxy(150,angle2,&x0,&y0,xoffset,150);
    polarxy(150,angle2+180,&x1,&y1,xoffset,150);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x0 + origin_xy[flag][0] *16,y0 + origin_xy[flag][2]*16 ));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1 + origin_xy[flag][0] *16,y1 + origin_xy[flag][2] *16));
			
    polarxy(150,angle2+45,&x0,&y0,xoffset,150);
    polarxy(150,angle2+45+180,&x1,&y1,xoffset,150);
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x0 + origin_xy[flag][1] *16,y0 + origin_xy[flag][3]*16 ));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1 + origin_xy[flag][1] *16,y1 + origin_xy[flag][3] *16));
    
    Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
    Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
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
  ft_uint32_t tracker = 0;
  ft_int32_t Read_tag,trackval = 0, y, z = 45 ,i;
  ft_uint16_t track_val = 0;
  ft_int16_t x0,y0,x2,r,temp,thcurrval=0,thcurr = 0;
  static ft_uint16_t angle = 0, angle1 = 0, angle2 = 0,flag = 0,flag_1=0;
  ft_uint8_t freezer_tempr = 0, fridge_tempr = 0;

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

      Ft_Gpu_CoCmd_Dlstart(phost);  	
      Ft_App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
      Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
      Ft_Gpu_CoCmd_Text(phost,120,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTER,"Freezer Temperature");
      Ft_Gpu_CoCmd_Text(phost,350,30,ft_pgm_read_byte(&font[val_font]),OPT_CENTER,"Fridge Temperature");
    
      construct_structure(120,flag);
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,CLEAR(0,1,0));
      construct_structure(350,flag_1);

      for(r = 0;r<2;r++)
      {
	if(r == 0)
          x2 = 120;
	else
          x2 = 350;
	  angle = 135+23; 
          for(z=0;z<7;z++)
	  {
	    angle += 45;
	    angle %= 360;
            polarxy(80,angle,&x0,&y0,x2,150);
	    Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,28,OPT_SIGNED|OPT_CENTER,freezer_temp[(r*7)+z]);      
          }
	}

	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(1,1,1));
	thcurr = flag*-2-2;
	*freeze_temp = thcurr;
        polarxy(0,0,&x0,&y0,120,150);
	Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,30,OPT_SIGNED|OPT_CENTER,thcurr);
	Ft_Gpu_CoCmd_Text(phost,150,145,17,OPT_CENTER,"x");
	Ft_Gpu_CoCmd_Text(phost,160,150,30,OPT_CENTER,"F");
	thcurrval = flag_1*2 + 34;
	*fridge_temp = thcurrval;
        polarxy(0,0,&x0,&y0,350,150);
	Ft_Gpu_CoCmd_Number(phost,x0/16,y0/16,30,OPT_CENTER,thcurrval);
	Ft_Gpu_CoCmd_Text(phost,370,145,17,OPT_CENTER,"x");
	Ft_Gpu_CoCmd_Text(phost,380,150,30,OPT_CENTER,"F");
				
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(140 , 205, 3,0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(370 , 205, 3,1));
		
        animation();
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
	Ft_App_WrCoCmd_Buffer(phost,TAG(0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(120*16,150*16));
	Ft_App_WrCoCmd_Buffer(phost,TAG(0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(350*16,150*16));
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

	Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,TAG(8));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
	
	
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	}

  }
  
ft_uint16_t sleep_cts=0;
ft_uint32_t val;

ft_void_t change_brightness(ft_uint32_t val,ft_int16_t val_font)
{
  ft_int32_t Read_tag,x;
  static ft_uint32_t tracker = 0,track_val = 420;
  ft_uint16_t pwmi_val = 0;
  
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
      if(keypressed == 8)
      {
	bitfield.brightness = 0;
	return;
      }
		
      Ft_Gpu_CoCmd_Dlstart(phost);
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
      animation();
      Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
      Ft_Gpu_CoCmd_Text(phost,210,10,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX,"Track the bar to adjust the brightness ");
      Ft_App_WrCoCmd_Buffer(phost,TAG(8));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(450, 5, 3, 3));
                
      Ft_App_WrCoCmd_Buffer(phost,TAG(12));
      Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(20, 200)); // Scissor rectangle bottom left at (20, 200)
      Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(420, 40)); // Scissor rectangle is 420 x 40 pixels
      Ft_Gpu_CoCmd_Gradient(phost, 20,0,0x0000ff,440,0,0xff0000);
      if(track_val){
	val = 10+(track_val/3);
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,val);
      }
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((20 + track_val)*16,190*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(440*16,250*16));	
    Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    }
}
	

ft_void_t scrensaver()
{
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
	
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));

        Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_Scale(phost, 2*65536,2*65536);//scale the bitmap 3 times on both sides
	Ft_Gpu_CoCmd_SetMatrix(phost);
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(4));
	Ft_App_WrCoCmd_Buffer(phost,MACRO(0));
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
      }
   }
	
}


ft_void_t FT_LiftApp_Refrigerator()
{
  const char ice[2][8] = {"Cubed","Crushed"};
  ft_int32_t y, j,Baddr,i = 0,pwm = 0;
  ft_uint32_t val = 0, tracker = 0,val_font = 0, trackval = 0,thcurr = 0,thcurrval=0;
  ft_uint16_t cubed = 0;
  ft_int16_t rval=0,pwm_val=0,m,x,freeze_temp = 0,fridge_temp = 0;
  ft_uint8_t k,Read_tag = 0,ice_cell;
  ft_int32_t time =0;
  time = millis();
  thcurr = -2;
  thcurrval = 34;
  val = 128;
  pwm_val = 10;
  rval = 0;
	
  
  Ft_Gpu_CoCmd_Dlstart(phost);  
  
  g_reader.openfile("gn.raw");
  Ft_App_LoadRawFromFile(g_reader,RAM_G);
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(RAM_G));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565, 240*2, 136));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 480, 272));
	
  Baddr = RAM_G + 240*2*136L;
  g_reader.openfile("5icons_5.raw");
  Ft_App_LoadRawFromFile(g_reader,Baddr);
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 18, 37));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 35, 35));

  Baddr += 221L*(36/2);
  g_reader.openfile("f1icons.raw");
  Ft_App_LoadRawFromFile(g_reader,Baddr);
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(3));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 15, 32));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 32));
  
  Baddr += 192*(32/2);
  g_reader.openfile("logo1.raw");
  Ft_App_LoadRawFromFile(g_reader,Baddr);
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(4));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4, 65*2, 25));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 512, 512));
  Baddr += 65*25*2L;

  g_reader.openfile("snow_1.raw");
  Ft_App_LoadRawFromFile(g_reader,Baddr);
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(Baddr));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 25, 50));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 50));
  	
  Ft_Gpu_CoCmd_Track(phost,170,180,120,8,17);
  //Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,1,0));
             
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,128);//Ft_App_fadein();

  /* compute the random values at the starting*/
  pRefrigeratorSnow = S_RefrigeratorSnowArray;
  for(j=0;j<(NumSnowRange*NumSnowEach);j++)
  {
      pRefrigeratorSnow->xOffset = ft_random(FT_DispHeight*16);//ft_random(FT_DispWidth*16);
      pRefrigeratorSnow->yOffset = ft_random(FT_DispWidth*16);//ft_random(FT_DispHeight*16);
      pRefrigeratorSnow->dx =  ft_random(RandomVal*8) - RandomVal*8;//-1*ft_random(RandomVal*8);//always -ve
      pRefrigeratorSnow->dy = -1*ft_random(RandomVal*8);//ft_random(RandomVal*8) - RandomVal*8;
      pRefrigeratorSnow++;
  } 

    while(1)
    {  
      Read_tag = Read_Keys();
      if(tracker_tag)
         Read_tag = 0;
      if(!istouch())
      {
          tracker_tag = 0;     
          if(bitfield.unlock)
           { 
              sleep_cts++;
               if(sleep_cts>100)
                   scrensaver();
          }
        }

	pwm_val = (val - rval)>> 4;
        rval += pwm_val;  
        rval = (rval <= 10)?10:rval;
	if(Read_tag == 1)
	{
	    bitfield.unlock = (bitfield.unlock == 0)?1 : 0;
            bitfield.settings = 0;
            bitfield.font_size = 0;
	    if(bitfield.unlock == 0)
	        Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,rval);
        }	
		
      if(keypressed == 17)
      {
        tracker_tag = 1;    
	tracker = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
        trackval = tracker >> 16;
	val_font = (trackval/16384);
      }

      Ft_Gpu_CoCmd_Dlstart(phost);  
      Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
      Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
      Ft_App_WrCoCmd_Buffer(phost,TAG(0));
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 1, 0));
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(390, 0, 4, 0));
      animation();
      Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
    
      ft_uint32_t disp =0,minutes,hr ;
      ft_uint32_t temp = millis()-time;
      hr = (temp/3600000L)%12;
      minutes = (temp/60000L)%60;
      Ft_Gpu_CoCmd_Text(phost,5 ,50,ft_pgm_read_byte(&font[val_font]),0,"February");
      Ft_Gpu_CoCmd_Number(phost,65 + (val_font *15), 50,ft_pgm_read_byte(&font[val_font]),0,25);
      Ft_Gpu_CoCmd_Text(phost,85 + (val_font *18),50,ft_pgm_read_byte(&font[val_font]),0,",");
      Ft_Gpu_CoCmd_Number(phost,90+ (val_font * 20) , 50,ft_pgm_read_byte(&font[val_font]),0,2014);  
      Ft_Gpu_CoCmd_Number(phost,10,75,ft_pgm_read_byte(&font[val_font]),2,hr);
      Ft_Gpu_CoCmd_Text(phost,35,75,ft_pgm_read_byte(&font[val_font]),0,":"); 
      Ft_Gpu_CoCmd_Number(phost,40,75,ft_pgm_read_byte(&font[val_font]),2,minutes);
               
      Ft_Gpu_CoCmd_Text(phost,(300 - (val_font * 45)) ,110,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"Freeze");
      Ft_Gpu_CoCmd_Number(phost,(270 - (val_font * 45)),135,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY|OPT_SIGNED,thcurr);
      Ft_Gpu_CoCmd_Text(phost,(282  - (val_font * 43) ),135,17,OPT_CENTERX|OPT_CENTERY,"x");
      Ft_Gpu_CoCmd_Text(phost,(292 - (val_font * 43)),135,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"F");


      Ft_Gpu_CoCmd_Text(phost,(370 - (val_font * 37) ),110,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"Fridge");
      Ft_Gpu_CoCmd_Number(phost,(350 - (val_font * 37)),135,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,thcurrval);
      Ft_Gpu_CoCmd_Text(phost,(362 - (val_font * 36)),135,17,OPT_CENTERX|OPT_CENTERY,"x");
      Ft_Gpu_CoCmd_Text(phost,(372 - (val_font * 36)),135,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"F");
                
      Ft_Gpu_CoCmd_Text(phost,(440 - (val_font * 25)) ,110,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,ice[cubed]);
      ice_cell  = (cubed == 0)?4 : 5;
      Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((420 - (val_font * 30)), 120, 3, ice_cell)); 

      Ft_App_WrCoCmd_Buffer(phost,TAG(1));   
      if(bitfield.unlock)
      {
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(15, 200, 2, 0));
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,25);
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
      }
      else
	{
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(15, 200, 2, 1));
	 }
                
         int xoff = 80;
         for(int z=2;z<5;z++)
         {
           Ft_App_WrCoCmd_Buffer(phost,TAG(z));
           Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoff, 200, 2, z)); 
           xoff+=75;                
         }
                
	Ft_App_WrCoCmd_Buffer(phost,TAG(11));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(290, 200, 2, 5));
        
        xoff = 250;
        int adj = 50;
        for(int z=5;z<8;z++)
        {
	  Ft_App_WrCoCmd_Buffer(phost,TAG(z));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(xoff - (val_font * adj) , 95, 3,z-5));
          xoff+=70;
          adj-=10;
        }

	if(bitfield.settings)
	{
	  Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
          Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(34, 50, 224));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(170*16,30*16));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(390*16,60*16));			
	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(162, 229, 242));
          Ft_App_WrCoCmd_Buffer(phost,TAG(9));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(170*16,60*16));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(390*16,130*16));
	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(34, 50, 224));
	  Ft_App_WrCoCmd_Buffer(phost,TAG(10));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(170*16,110*16));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(390*16,160*16));
          Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
			

	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0, 0, 0));
	  Ft_Gpu_CoCmd_Text(phost,278,85,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"Change Background");
	  Ft_Gpu_CoCmd_Text(phost,265,130,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"Change Font Size");
	  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
	  Ft_Gpu_CoCmd_Text(phost,240,45,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"SETTINGS");	
	  Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
	  Ft_App_WrCoCmd_Buffer(phost,TAG(8));
	  Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(340, 30, 3, 3));
	}


	if(bitfield.font_size)
	{
	  Ft_App_WrCoCmd_Buffer(phost,TAG(17));
	  Ft_Gpu_CoCmd_Slider(phost,170, 180, 120, 8, 0,trackval, 65535);
	  Ft_Gpu_CoCmd_Text(phost,200,170,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,"FONT SIZE");
	  Ft_Gpu_CoCmd_Number(phost,170,200,ft_pgm_read_byte(&font[val_font]),OPT_CENTERX|OPT_CENTERY,val_font);
	}

	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

        if(Read_tag == 2)
          bitfield.settings = 1;
                
	if((Read_tag == 8) && (bitfield.settings == 1))
        {
          bitfield.settings = 0;
          bitfield.font_size = 0;
         }  

        if(Read_tag == 10)
	  bitfield.font_size ^= 1;
	if(Read_tag == 3)
          bitfield.brightness ^= 1;
        if(Read_tag == 7)
        {
            bitfield.settings = 0;
            bitfield.font_size = 0;
          cubed_setting(&cubed,val_font);
        }
        if(bitfield.brightness)
        {
            bitfield.settings = 0;
            bitfield.font_size = 0;
          change_brightness(val,val_font);
        }
	if(Read_tag == 9)
        {
           bitfield.settings = 0;
            bitfield.font_size = 0;
	  change_background();		
        }
        if(Read_tag == 4)
        {
            bitfield.settings = 0;
            bitfield.font_size = 0;
	   Sketch(val_font);
        }
        if(Read_tag == 11)
        {
            bitfield.settings = 0;
            bitfield.font_size = 0;
          food_tracker(val_font);
        }
        if(Read_tag == 5 || Read_tag == 6)
        {
            bitfield.settings = 0;
            bitfield.font_size = 0;
	  change_temperature(&thcurr,&thcurrval,val_font);
        }
			
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
	  ft_uint8_t chipid;	
// ft_char8_t *path = "..\\..\\..\\Test";
  
	Ft_Gpu_HalInit_t halinit;
	//Serial.begin(9600);
	halinit.TotalChannelNum = 1;

  //             _chdir(path); 
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



  pinMode(FT_SDCARD_CS,OUTPUT);
  digitalWrite(FT_SDCARD_CS,HIGH);
  delay(100);
  sd_present =  SD.begin(FT_SDCARD_CS);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

	//Info();
SAMAPP_CoPro_Widget_Calibrate();
FT_LiftApp_Refrigerator();


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
