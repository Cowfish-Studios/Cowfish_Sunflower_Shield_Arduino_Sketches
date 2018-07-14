/*****************************************************************************
* Copyright (c) Future Technology Devices International 2015
* propriety of Future Technology devices International.
*
* Software License Agreement
*
* This code is provided as an example only and is not guaranteed by FTDI. 
* FTDI accept no responsibility for any issues resulting from its use. 
* The developer of the final application incorporating any parts of this 
* sample project is responsible for ensuring its safe and correct operation 
* and for any consequences resulting from its use.
* 
* Also requires the SparkFun MAX31855K Thermocouple Breakout library for Arduino.
* https://github.com/sparkfun/MAX31855K_Thermocouple_Breakout
*****************************************************************************/

#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include "FT_Platform.h"
#include "SampleApp.h"
#include <SparkFunMAX31855k.h> // Using the max31855k driver

#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)

#define F16(s)        ((ft_int32_t)((s) * 65536))
#define SQ(v) (v*v)

// Define SPI Arduino pin numbers
const uint8_t CHIP_SELECT_PIN = 3; 

// This example doesn't use IO to power the IC, but has serial debug enabled
SparkFunMAX31855k probe(CHIP_SELECT_PIN, NONE, NONE, true);

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
ft_char8_t FT_DispDither = 0;

/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;

#define SAMAPP_ENABLE_APIS_SET0
#ifdef SAMAPP_ENABLE_APIS_SET0

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
#endif


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
  FT_DispPCLK = 6;
  FT_DispSwizzle = 2;
  FT_DispPCLKPol = 1;
	FT_DispCSpread = 0;
	FT_DispDither = 0;

#endif
#ifdef DISPLAY_RESOLUTION_WVGA
	/* Values specific to QVGA LCD display */
	FT_DispWidth = 800;
	FT_DispHeight = 480;
	FT_DispHCycle =  1056;
	FT_DispHOffset = 46;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 10;
	FT_DispVCycle = 525;
	FT_DispVOffset = 23;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 10;
	FT_DispPCLK = 2;
	FT_DispSwizzle = 0;
	FT_DispPCLKPol = 1;
	FT_DispCSpread = 0;
	FT_DispDither = 0;
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
	FT_DispPCLK = 5;
	FT_DispSwizzle = 2;
	FT_DispPCLKPol = 1;
	FT_DispCSpread = 1;
	FT_DispDither = 0;

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


//    /*It is optional to clear the screen here*/
//    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
//    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);


    Ft_Gpu_Hal_Wr8(phost, REG_PCLK,FT_DispPCLK);//after this display is visible on the LCD

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

phost->ft_cmd_fifo_wp = Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE);

}

/* Main entry point */
ft_void_t setup()
{
  Serial.begin(9600);
  Serial.println("\nBeginning...");
  delay(100);  // Let IC stabilize or first readings will be garbage

  float temperature;
  
  ft_uint8_t chipid;

	Ft_Gpu_HalInit_t halinit;
	
	halinit.TotalChannelNum = 1;

              
	Ft_Gpu_Hal_Init(&halinit);
	host.hal_config.channel_no = 0;
	host.hal_config.pdn_pin_no = FT800_PD_N;
	host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;

#ifdef ARDUINO_PLATFORM_SPI
  host.hal_config.spi_clockrate_khz = 4000; //in KHz
#endif
  Ft_Gpu_Hal_Open(&host);

  phost = &host;

    Ft_BootupConfig();

    while(1){

    Ft_Gpu_CoCmd_Dlstart(phost);

    /*Setting first screen*/
    Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0x30,0xd5,0xc8)); //Blue color
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));//clear screen

    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));

    Ft_Gpu_CoCmd_Text(phost, 160, 25, 29, OPT_CENTER, "MAX31855K Thermocouple");

    // Read the temperature in Fahrenheit
    temperature = probe.readTempF();
    Ft_Gpu_CoCmd_Text(phost, 110, 50, 29, OPT_RIGHTX, "Temp[F]=");
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 0, 0 ));
    Ft_Gpu_CoCmd_Number(phost, 160, 50, 29, OPT_RIGHTX, temperature);

    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));

    // Read the temperature in Celsius 
    temperature = probe.readTempC();
    Ft_Gpu_CoCmd_Text(phost, 110, 100, 29, OPT_RIGHTX, "Temp[C]=");
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 0, 0 ));
    Ft_Gpu_CoCmd_Number(phost, 160, 100, 29, OPT_RIGHTX, temperature);

    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));

    // Read the temperature in Kelvin  
    temperature = probe.readTempK();
    Ft_Gpu_CoCmd_Text(phost, 110, 150, 29, OPT_RIGHTX, "Temp[K]=");
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 0, 0 ));
    Ft_Gpu_CoCmd_Number(phost, 160, 150, 29, OPT_RIGHTX, temperature);

    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));

    // Read the temperature in Rankine  
    temperature = probe.readTempR();
    Ft_Gpu_CoCmd_Text(phost, 110, 200, 29, OPT_RIGHTX, "Temp[R]=");
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 0, 0 ));
    Ft_Gpu_CoCmd_Number(phost, 160, 200, 29, OPT_RIGHTX, temperature);
   
    delay(350);

    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);

    /* Download the commands into fifo */
    Ft_App_Flush_Co_Buffer(phost);

    /* Wait till coprocessor completes the operation */
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
    
  }
}

void loop()
{
}



/* Nothing beyond this */





/* Nothing beyond this */
