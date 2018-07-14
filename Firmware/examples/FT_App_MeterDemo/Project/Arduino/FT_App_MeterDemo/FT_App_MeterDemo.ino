
/*
#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
*/
/* Sample application to demonstrat FT800 primitives, widgets and customized screen shots */

#include "FT_Platform.h"
#include "FT_App_MeterDemo.h"
#include "sdcard.h"



#define STARTUP_ADDRESS	100*1024L
#define ICON_ADDRESS	250*1024L
#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)
#define BLK_LEN	  1024


//#define Absolute Dial
#define Relative Dial

#define CLOCK_HANDLE	(0x01)

#define DIAL		1

#define F16(s)           ((ft_int32_t)((s) * 65536))

ft_int32_t BaseTrackVal = 0,BaseTrackValInit = 0,BaseTrackValSign = 0,MemLoc=0;
ft_int16_t MoveDeg =0;
/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
#if defined(DISPLAY_RESOLUTION_WQVGA)	
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
	/* Values specific to QVGA LCD display */
ft_int16_t	FT_DispWidth = 320;
ft_int16_t	FT_DispHeight = 240;
ft_int16_t	FT_DispHCycle =  408;
ft_int16_t	FT_DispHOffset = 70;
ft_int16_t	FT_DispHSync0 = 0;
ft_int16_t	FT_DispHSync1 = 10;
ft_int16_t	FT_DispVCycle = 263;
ft_int16_t	FT_DispVOffset = 13;
ft_int16_t	FT_DispVSync0 = 0;
ft_int16_t	FT_DispVSync1 = 2;
ft_uint8_t	FT_DispPCLK = 8;
ft_char8_t	FT_DispSwizzle = 2;
ft_char8_t	FT_DispPCLKPol = 0;
#endif

const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
    0,255,0,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR
    0,0,0,0,  //GPU instruction DISPLAY
};


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

#define MAX_IMAGES  2
//FT_PROGMEM char imagename[MAX_IMAGES][20] = {"AbsDial.raw","strips.raw"};		
static ft_int32_t fsize = 0;
static ft_uint8_t sd_present =0;
Reader binfile;

#define CALIBRATION_FONTSIZE 28




char* const info[] PROGMEM = {  "FT800 Meter Dial Application",
                          "APP to demonstrate interactive Meter dial,",
                          "using Points, Track",
                          "& Stencil"
                       }; 


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
//#endif


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

#ifdef Relative Dial
static struct {
  ft_int32_t dragprev;
  ft_int32_t vel;      // velocity
  ft_int32_t base;    // screen x coordinate, in 1/16ths pixel
//  ft_int32_t limit;
}scroller ;

static ft_void_t scroller_init(ft_uint32_t limit)
{
  scroller.dragprev = 0;
  scroller.vel = 0;      // velocity
  scroller.base = 0;     // screen x coordinate, in 1/16ths pixel
 // scroller.limit = limit;
}

static ft_void_t scroller_run()
{
	  ft_int32_t change;
	  ft_uint32_t sx;
	 

	  Ft_Gpu_CoCmd_Track(phost,(240)+40,136, DIAL, DIAL, 1);
	//  signed short sx = Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
   
	  sx =   Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
	  printf("%d\n,",sx);


	  /*if ((sx != -32768) & (scroller.dragprev != -32768)) */
	  if ((sx != 0) && (scroller.dragprev !=0)) 
	  {
		scroller.vel = (scroller.dragprev - sx) << 4;
	  } 
	  else
	  {
		change = max(1, abs(scroller.vel) >> 5);
		if (scroller.vel < 0)
		  scroller.vel += change;
		if (scroller.vel > 0)
		  scroller.vel -= change;
	  }
	  scroller.dragprev = sx;
	 // printf("%d\n,sx", sx);
	  scroller.base += scroller.vel;
	  scroller.base = max(0, (scroller.base));
 
}

#endif

static void polarxy(ft_uint32_t r, float th, ft_int16_t *x, ft_int16_t *y,ft_uint16_t ox,ft_uint16_t oy)
{
	th = (th+180) * 65536 / 360;
	*x = (16 * ox + (((long)r * SAMAPP_qsin(th)) >> 11));
	*y = (16 * oy - (((long)r * SAMAPP_qcos(th)) >> 11));
}






                             
static int jpeg_index;
static dirent jpeg_dirent;
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

ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
// Touch Screen Calibration
  Ft_CmdBuffer_Index = 0;  
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,CALIBRATION_FONTSIZE,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
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
//Enter into Info Screen
  do
  {
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

    Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(1,1,1,1) );//enable all the colors
    Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(EQUAL,1,255));
    Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth,0,0,0));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth,FT_DispHeight,0,0));
	
    Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
    Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(ALWAYS,0,255));
    Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
	
    Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-2+15),(FT_DispHeight-67+8),0,0));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-57+11),0,0));
    Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(((FT_DispWidth/2)-14+10),(FT_DispHeight-77+5),0,0));
	
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  }while(Read_Keys()!='P');
  /* To cover up some distortion while demo is being loaded */
  Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
  Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
/* wait until Play key is not pressed*/ 
}

/* Apis used to upload the image to GRAM from SD card*/
ft_void_t Load_Jpeg(ft_uint8_t bmphandle)
{
 
    ft_uint8_t imbuff[512];
    ft_uint16_t blocklen; 
    ft_int32_t  fsize = binfile.size,dstaddr = RAM_G,dstaddr1=RAM_G+45000;   
 
    while (fsize>0)
    {  
      uint16_t n = min(512, fsize);
      n = (n + 3) & ~3;   // force 32-bit alignment
      binfile.readsector(imbuff); 
      if(bmphandle==0)
      Ft_Gpu_Hal_WrMem(phost,dstaddr,imbuff,n);
      else
      Ft_Gpu_Hal_WrMem(phost,dstaddr1,imbuff,n);

      fsize-=512;
      dstaddr += n;
      dstaddr1+=n;

    }    
}


ft_void_t Loadimage2ram(ft_uint8_t bmphandle)
{
  static byte image_index = 0,max_files=2;
  byte file;
  
        SPI.setClockDivider(SPI_CLOCK_DIV2);
        SPI.setBitOrder(MSBFIRST);
        SPI.setDataMode(SPI_MODE0);
  if(sd_present)
  {
      
    if(bmphandle==0)  
    {
    #ifdef Absolute Dial
    file = binfile.openfile("AbsDial.raw"); 
    #endif
    #ifdef Relative Dial
    file = binfile.openfile("RelDial.raw"); 
    #endif
    }
    else
    file = binfile.openfile("strips.raw"); 
    
    if(file==NULL)
    {
     
    }else
    {  
      Load_Jpeg(bmphandle);
     
    }  
  }
  else
  {
    file = NULL;
    max_files = 0;
  }  
  
  image_index++;
  if(image_index > (max_files-1)) image_index = 0;
  return;
}

#ifdef Absolute Dial
ft_void_t Abs_Meter_Dial()
{
	ft_uint8_t i=0,Volume=0,Read_tag,prevtag=0;
	ft_int16_t ScreenWidthMid,ScreenHeightMid,th,theta,X1,Y1,X2,Y2,Touch,NoTouch,Sx1=0,Sy1=0;
	ft_int16_t OuterCircleRadius,PrevVal=0,CircleX,CircleY,ButY,ButWid,NoY,Tx1,Tx2,Tx3,Ty1,Ty2,Ty3 ;

	ft_int32_t PrevTh,CurrTh,adjusting=0,BaseTrackValSign = 0,PrevBaseVal=0,Delta =0;

	ft_uint32_t RotaryTag=0,G1,G2;




	ScreenWidthMid = FT_DispWidth/2;
	ScreenHeightMid = FT_DispHeight/2;
	
	NoTouch = -32768;
	
	
	Tx1= Tx2=Tx3 =Ty1=Ty2=Ty3=0;

	#ifdef DISPLAY_RESOLUTION_WQVGA
        Sx1 = Sy1 = 160;
	OuterCircleRadius = ScreenWidthMid - 144;
	CircleX = ScreenWidthMid;
	CircleY = ScreenHeightMid;
	ButY = FT_DispHeight -262;
	ButWid = 30;
	NoY = (FT_DispHeight-262 + 30/2);
	Tx1 = ScreenWidthMid-86;
	Tx2 = ScreenWidthMid-70 + 146/2;
	Ty2 = ScreenHeightMid-67 + 146/2;
	Tx3 = ScreenWidthMid+80;
	#endif
	#ifdef DISPLAY_RESOLUTION_QVGA
        Sx1 = Sy1 = 130;
	ButY = 1;
	ButWid = 23;
	NoY = 12;
	OuterCircleRadius = ScreenWidthMid - 64;
	CircleX = ScreenWidthMid-70 + 146/2;
	CircleY = ScreenHeightMid-67 + 146/2;
	Tx1 = ScreenWidthMid-63;
	Tx2 = ScreenWidthMid-70 + 146/2;
	Ty2 = ScreenHeightMid-67 + 146/2;
	Tx3 = ScreenWidthMid+72;

	#endif


        Ft_Gpu_Hal_WrCmd32(phost, CMD_MEMSET);
	Ft_Gpu_Hal_WrCmd32(phost, 0L);//starting address of memset
	Ft_Gpu_Hal_WrCmd32(phost, 55L);//value of memset
	Ft_Gpu_Hal_WrCmd32(phost,256*1024);

        Ft_Gpu_CoCmd_Dlstart(phost);
        Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(123,0,0));	
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

        for(ft_uint8_t r = 0;r<2;r++)
        {      
             Loadimage2ram(r);	    
        }
    
       for(ft_uint8_t r = 0;r<2;r++)
        {
           Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(r));
           if(r==0)
           {    
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, 146*2, 146));        
          }
          else
          {    
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(RAM_G+45000));
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L4,146/2,146));
          }
          Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR,BORDER,BORDER,146,146));  
	}
 
	/* set the vol to max */
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,0);
	/* Play carousel*/
	Ft_Gpu_Hal_Wr16(phost, REG_SOUND, 8);
	

	
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));	

        Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));       
        Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(0,0,0, 0));	
        
        Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(150,0,1, 0));	
	Ft_App_WrCoCmd_Buffer(phost, DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
 

       
	/*asiign track to the black dot*/
	Ft_Gpu_CoCmd_Track(phost,CircleX,CircleY, DIAL, DIAL, 1);
	G1 = 0x00000;
	G2 = 0x4D4949;
        Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);
	do{
                
		Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
		Touch = Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_SCREEN_XY+2);
		RotaryTag = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		
//		Ft_Gpu_CoCmd_Gradient(phost,0, 0, G1,0, FT_DispHeight, G2);
                Ft_Gpu_CoCmd_Gradient(phost,0, 0, G1,Sx1, Sy1, G2);
		
		/* Assign Stencil Value to get the triangle shape */
		Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(0,0,0,1) );//only alpha is enabled,so that no color is used for strip
		Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(1*16));
		Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(INCR,DECR) );
		Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(EQUAL,0,255) );
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_B));		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(136 ,136 ,136)); 


		Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(Tx1,FT_DispHeight,0,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(Tx2,Ty2,0,0));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(Tx3,FT_DispHeight,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,INCR) );
		Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(NOTEQUAL,255,255) );
		Ft_App_WrCoCmd_Buffer(phost, COLOR_MASK(1,1,1,1) );//enable all colors

	
		/* Outer Black circle*/
		/* Based on stencil update Points color,leaving the triangular area*/		
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius) * 16) );
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0 ,0 ,0)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
				
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(((OuterCircleRadius-3) * 16)) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (15,77,116)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
				
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(((OuterCircleRadius-5) * 16)) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (54,115,162)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(((OuterCircleRadius-7) * 16)) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (76,195,225)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(((OuterCircleRadius-9) * 16)) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (89,211,232)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(((OuterCircleRadius-11) * 16)) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (123,248,250)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius-17) * 16) );		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (89,211,232)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));

		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius-18) * 16) );
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (76,195,225)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius-19) * 16) );
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (54,115,162)); 
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius-20) * 16) );	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB (15,77,116));  
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
	
		/*Managing the track*/	
		if((Touch != NoTouch))
		{
			if((RotaryTag & 0xff)== 1)
			{
				CurrTh = (ft_int16_t)((RotaryTag >> 16) & 0xffff);
				 /* BaseTrackVal is the rotary track value of the particular white circle that is moving */
				if((adjusting!=0) )
				{					
					BaseTrackValSign += (ft_int16_t)(CurrTh - PrevTh);
				} 
				PrevTh = CurrTh;
				adjusting = (RotaryTag & 0xff);				
				//PrevValDeg = NextTh;
			
			}
			
		}

		/* alpha bend for strips */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,0,0));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((OuterCircleRadius-2) * 16) );		
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));

		/* Parameters for Strips */
		Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(2*16));	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(30 ,30 ,30)); 
		Delta = BaseTrackValInit - BaseTrackValSign;

		

		if((prevtag == 0) && (Read_tag == 1)   &&(Touch != -32768) )	
		{			
			PrevBaseVal = BaseTrackValInit;
		}
		if((prevtag == 1) && (Read_tag == 1)   &&(Touch!= -32768) )	
		{
			PrevBaseVal = PrevBaseVal - Delta;			
			MoveDeg = (ft_uint16_t)(PrevBaseVal / 182);				
		}
		
		if(PrevBaseVal > 60294) PrevBaseVal = 60294;
		if(PrevBaseVal <0)PrevBaseVal = 0;
		if(MoveDeg > 330) MoveDeg = 330;
		if(MoveDeg < 30) MoveDeg =31;
		
		if(MoveDeg <= 90)
		{
			/* First Left edge strip */
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_L));				
			polarxy((OuterCircleRadius-2),MoveDeg,&X1,&Y1,CircleX,CircleY);			
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(CircleX*16,CircleY*16));		
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X1,Y1));
		}
	
		if((MoveDeg <= 90) ||
				(MoveDeg > 90) && 
					(MoveDeg <= 180))
		{
			/* Second Above edge strip */
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_A));		
			if(MoveDeg<=90)
			{
				polarxy((OuterCircleRadius-2),90,&X1,&Y1,CircleX,CircleY);			
			}
			else if (MoveDeg>90)
			{
				polarxy((OuterCircleRadius-2),MoveDeg,&X1,&Y1,CircleX,CircleY);			
			}
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(CircleX*16,CircleY*16));		
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X1,Y1));
		
		}
		if((MoveDeg <= 90) || 
			((MoveDeg > 90) && (MoveDeg <= 180))||
					((MoveDeg > 180) && (MoveDeg <= 270)))
		{
			/* Third Above edge strip */
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_R));		
			if(MoveDeg<=180)
			{
				polarxy((OuterCircleRadius-2),180,&X1,&Y1,CircleX,CircleY);			
			}
			else if (MoveDeg>180)
			{
				polarxy((OuterCircleRadius-2),MoveDeg,&X1,&Y1,CircleX,CircleY);			
			}	
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X1,Y1));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(CircleX*16,CircleY*16));					
		}
		
		if((MoveDeg <= 90) ||
				((MoveDeg > 90)&& (MoveDeg <= 180))||
					((MoveDeg > 180)&& (MoveDeg <= 270))||
						((MoveDeg > 270)&& (MoveDeg <= 330)))
		{
			/* Fourth Above edge strip */
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(EDGE_STRIP_B));		
			if(MoveDeg<=270)
			{
				polarxy((OuterCircleRadius-1),270,&X1,&Y1,CircleX,CircleY);		
			}
			else if (MoveDeg>270)
			{
				polarxy((OuterCircleRadius-1),MoveDeg,&X1,&Y1,CircleX,CircleY);								
			}
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((CircleX)*16,CircleY*16));		
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X1,Y1));
		}		
		
			
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));

		Ft_App_WrCoCmd_Buffer(phost, STENCIL_OP(KEEP,KEEP));
		Ft_App_WrCoCmd_Buffer(phost, STENCIL_FUNC(ALWAYS,0,255));

		/*alpha blend for rgb bitmap*/
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0xff));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,0,0));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((146/2) * 16) );	
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX,CircleY,0,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));

		/* Meter Dial bitmap placement */
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX - 146/2,CircleY - 146/2,0,0));
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
		Ft_App_WrCoCmd_Buffer(phost,END());

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(20));
		Ft_App_WrCoCmd_Buffer(phost,TAG(1));
		
		/* Alpha Meter Dial bitmap placement & rotation */
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,F16(73),F16(73));
		Ft_Gpu_CoCmd_Rotate(phost,MoveDeg*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,F16(-(73)),F16(-(73)));		  
		Ft_Gpu_CoCmd_SetMatrix(phost);	
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX - 146/2,CircleY - 146/2,1,0));		
		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

		/* restore default alpha value */
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));		

		Ft_Gpu_CoCmd_FgColor(phost, 0x59D3E8);
		Ft_Gpu_CoCmd_GradColor(phost, 0x7BF8FA);

		Ft_Gpu_CoCmd_Button(phost, (ScreenWidthMid - 40), ButY, 80, ButWid, 28, 0, "");// placement of button

		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));//color of number(text) color
	
	
		
		if((MoveDeg >= 30)&& (MoveDeg <= 330))
		{			
			Volume = PrevBaseVal>>8;						
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
			Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(30 ,30 ,30)); 
			polarxy((OuterCircleRadius-3),MoveDeg,&X1,&Y1,CircleX,CircleY);	
			polarxy(74,MoveDeg,&X2,&Y2,CircleX,CircleY);	
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X1,Y1));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X2,Y2));						
		}

		Ft_Gpu_CoCmd_Number(phost,(ScreenWidthMid),NoY,28,OPT_CENTER,Volume);
		Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,Volume);
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());

		Ft_Gpu_CoCmd_Swap(phost);

		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);

		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		prevtag = Read_tag;		
		BaseTrackValInit = BaseTrackValSign;
		Ft_Gpu_Hal_Sleep(30);
		}while(1);		


}
#endif

#ifdef Relative Dial
ft_void_t Relative_Meter_Dial()
{
	ft_uint8_t i=0,j=0,k=0,Read_tag,prevtag=0,currtag;
	ft_int16_t ScreenWidthMid,ScreenHeightMid,NextTh=0,Touch,drag=0;
	ft_int16_t OuterCircleRadius,PrevValDeg=0,CircleX,CircleY,ButY,ButWid,NoY,Delta,G1,G2,Sx1,Sy1,X2=0,Y2=0;

	ft_int32_t PrevTh=0,CurrTh,adjusting=0,BaseTrackValSign = 0,Vx1=0,Vy1=0,Vx2=0,Vy2=0,Volume=0,PrevBaseTrackVal=0;

	ft_uint32_t RotaryTag=0;	

	ft_int32_t change;
	ft_int32_t sx,storesx,storepresx,Velocity=0;	
	

	ScreenWidthMid = FT_DispWidth/2;
	ScreenHeightMid = FT_DispHeight/2;
	
	PrevValDeg =0;
	Sx1 = Sy1 = 180;
	


	#ifdef DISPLAY_RESOLUTION_WQVGA
	Sx1 = Sy1 = 240;
	OuterCircleRadius = ScreenWidthMid - 144;
	CircleX = ScreenWidthMid;
	CircleY = ScreenHeightMid;
	ButY = FT_DispHeight -262;
	ButWid = 30;
	NoY = (FT_DispHeight-262 + 30/2);
	#endif
	#ifdef DISPLAY_RESOLUTION_QVGA
	Sx1 = Sy1 = 130;
	ButY = 10;
	ButWid = 23;
	NoY = 12;
	OuterCircleRadius = ScreenWidthMid - 64;
	CircleX = ScreenWidthMid-70 + 146/2;
	CircleY = ScreenHeightMid-67 + 146/2;

	#endif

	

	Ft_Gpu_Hal_WrCmd32(phost, CMD_MEMSET);
	Ft_Gpu_Hal_WrCmd32(phost, 0L);//starting address of memset
	Ft_Gpu_Hal_WrCmd32(phost, 55L);//value of memset
	Ft_Gpu_Hal_WrCmd32(phost,256*1024);

	

	/* set the vol to max */
	Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,0);
	/* Play carousel*/
	Ft_Gpu_Hal_Wr16(phost, REG_SOUND, 8);

  Ft_Gpu_Hal_Wr8(phost, REG_GPIOX_DIR, 0x8008);
  Ft_Gpu_Hal_Wr8(phost, REG_GPIOX, 0x8);
	
	/* Construction of starting screen shot, assign all the bitmap handles here */
	Ft_App_WrCoCmd_Buffer(phost,CMD_DLSTART);
	Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
	
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

        for(ft_uint8_t r = 0;r<2;r++)
        {      
             Loadimage2ram(r);	    
        }
    
        for(ft_uint8_t r = 0;r<2;r++)
        {
           Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(r));
           if(r==0)
           {    
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0));
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, 146*2, 146));        
          }
          else
          {    
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(RAM_G+45000));
             Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L4,146/2,146));
          }
          Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR,BORDER,BORDER,146,146));  
	}


	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));	

        Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));       
//        Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(0,0,0, 0));	
//        
//        Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(150,0,1, 0));	
        
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	Ft_App_Flush_Co_Buffer(phost);

	/* Wait till coprocessor completes the operation */
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);


	/*asiign track to the bitmap*/
	Ft_Gpu_CoCmd_Track(phost,(CircleX)+40,CircleY, DIAL, DIAL, 1);
	Ft_Gpu_Hal_Wr8(phost, REG_PLAY,1);


	do{
		Ft_App_WrCoCmd_Buffer(phost,CLEAR_TAG(255));
		Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
		Touch = Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_SCREEN_XY+2);
		RotaryTag = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);
		Ft_Gpu_CoCmd_Dlstart(phost);
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0,0,0));

		#ifdef DISPLAY_RESOLUTION_WQVGA
			Ft_Gpu_CoCmd_Gradient(phost,0, FT_DispHeight, 0x1E1E1E,ScreenWidthMid-60/*180*/, ScreenHeightMid-21/*115*/, 0x999999);			
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
			Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));		
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth-2)*16,(FT_DispHeight)*16 ));
			Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(ScreenWidthMid*16,0 *16));
		#endif
		#ifdef DISPLAY_RESOLUTION_QVGA
			Ft_Gpu_CoCmd_Gradient(phost,0, FT_DispHeight, 0x1E1E1E,140/*120*/, 140/*101*/, 0x999999);
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		        Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));		
		        Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth-2)*16,(FT_DispHeight)*16 ));
		        Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(146*16,0 *16));
		#endif
   
		sx = 0;	 
		currtag = (RotaryTag&0xff);
               
		if(1 == currtag)
		{
		  sx = RotaryTag >> 16;
		}
	   
                
		if((prevtag == 1) && (1 == currtag))
		{
			scroller.vel = (-scroller.dragprev + sx);
	
			if((scroller.vel > 32000))
			{
				scroller.vel-= 65536;
			}

			else if (scroller.vel < -32768)
			{
				scroller.vel+= 65536;
			}
			scroller.vel <<= 4;
		
			storepresx = scroller.dragprev;
			storesx = sx;			
	  } 
	  else
	  {
		change = max(1, abs(scroller.vel) >> 5);
		if (scroller.vel < 0)
		  scroller.vel += change;

		if (scroller.vel > 0)
		  scroller.vel -= change;
		
	  }
	  prevtag = currtag;
	  scroller.dragprev = sx;
	  scroller.base += scroller.vel;
#ifdef FT_801_ENABLE

          if(scroller.vel == 0)scroller.vel =Velocity;
#endif	  
	
	  drag = scroller.base>>4;

		
		/*Managing the track*/	
		{
			if((1 == currtag))
			{
				CurrTh = sx;				

				if((adjusting!=0) )
				{
					
					PrevBaseTrackVal = (CurrTh - PrevTh);
					
					if((PrevBaseTrackVal > 32000))
					{
						PrevBaseTrackVal-= 65536;
					}
					else if (PrevBaseTrackVal < -32768)
					{
						PrevBaseTrackVal+= 65536;
					}
					BaseTrackValSign += PrevBaseTrackVal;
					BaseTrackVal += PrevBaseTrackVal;
					
				}
				
				adjusting = (RotaryTag & 0xff);
				PrevTh = CurrTh;
				NextTh = (ft_uint16_t)(BaseTrackValSign / 182);			
				PrevValDeg = NextTh;
				
			}

			else if((0 != scroller.vel))
			{

				BaseTrackValSign += (scroller.vel>>4); //original copy
                                BaseTrackVal +=  (scroller.vel>>4);
				NextTh = (ft_uint16_t)(BaseTrackValSign / 182);			
				PrevValDeg = NextTh;
				adjusting = 0;
			}
			else
			{
				adjusting = 0;
			}
			
		}

		/*alpha blend for rgb bitmap*/
		
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0xff));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,0,0));
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE((146/2) * 16) );	
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(CircleX +40,CircleY,0,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));

		/* Meter Dial bitmap placement */
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		

		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((CircleX - 146/2)+40,CircleY - 146/2,0,0));

		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
			

		Delta = BaseTrackValSign - BaseTrackValInit;

	
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(20));
				
		/* Alpha Meter Dial bitmap placement & rotation */
		Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		
		Ft_Gpu_CoCmd_LoadIdentity(phost);
		Ft_Gpu_CoCmd_Translate(phost,F16(73),F16(73));
		Ft_Gpu_CoCmd_Rotate(phost,PrevValDeg*65536/360);
		Ft_Gpu_CoCmd_Translate(phost,F16(-(73)),F16(-(73)));		  
		Ft_Gpu_CoCmd_SetMatrix(phost);	
		Ft_App_WrCoCmd_Buffer(phost,TAG(1));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((CircleX - 146/2)+40,CircleY - 146/2,1,0));			
		Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
	
		/* Draw points around the bitmap*/
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));	
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));		
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(5*16));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(227 ,249 ,20)); 
		polarxy(55,PrevValDeg,&X2,&Y2,CircleX+40,CircleY);

		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(X2,Y2));		
		//printf("%d\n,Touch, %d\n,prevtag", prevtag,Touch);
		
		for(k=0;k<=11;k++)
		{
			Vy1 = 200;			
			if(k==0)/* First button Vertices*/
			{
				Vx1 = 10;Vy1 = 200;
			}
			else if((k== 1)||(k==2)||(k==3))/* Second,third,fourth button Vertices*/
			{
				Vx1 = 10;Vy1 = Vy1-(k)*60;
			}
			else if(k==4)/* fifth button Vertices*/
			{
				Vx1 = 40;Vy1 = 200;
			}
			else if((k== 5)|(k==6)|(k==7))/* sixth,seventh,eighth button Vertices*/
			{
				Vx1 =40;Vy1 = Vy1-(k-4)*60 ;
			}
			else if(k==8)/* ninth button Vertices*/
			{
				Vx1 = 70;Vy1 = 200;
			}
			else if((k== 9)|(k==10)|(k==11))/* tenth,eleven,twelth button Vertices*/
			{
				Vx1 =70;Vy1 = Vy1-(k-8)*60 ;
			}
			

			if(BaseTrackVal < 11*65535)
			{
				/* Rotary tag when within range of particular index, highlight only the indexed button in yellow*/
				if(((BaseTrackVal >= k*65535)&&(BaseTrackVal < (k+1)*65535)))//|| ((BaseTrackValSign<0)&&(k==0)))
				{
					Ft_Gpu_CoCmd_FgColor(phost,0xE3F914);//yellow
					Ft_Gpu_CoCmd_Button(phost,Vx1,Vy1,20,30,16,0,"");	
				}
				else/* Color the non indexed buttons in grey color*/
				{
					Ft_Gpu_CoCmd_FgColor(phost,0x787878);//grey
					Ft_Gpu_CoCmd_Button(phost,Vx1,Vy1,20,30,16,0,"");
				}
			}
			
			/*Limit track value when its greater than 12th button(11th index) and also highlight twelth button*/
			else if((BaseTrackVal >= 11*65535))
			{
				BaseTrackVal = 11*65535;
				
				if(k<11)
				{
					Ft_Gpu_CoCmd_FgColor(phost,0x787878);//grey
				}
				else	
				{
					Ft_Gpu_CoCmd_FgColor(phost,0xE3F914);//yellow					
				}
				Ft_Gpu_CoCmd_Button(phost,Vx1,Vy1,20,30,16,0,"");
			}

			/*Limit track value when its negative and also highlight first button*/
			if((BaseTrackVal<0)&&(k==0))
			{
				if((Delta>0))
				BaseTrackVal =0;
				Ft_Gpu_CoCmd_FgColor(phost,0xE3F914);//yellow	
				Ft_Gpu_CoCmd_Button(phost,Vx1,Vy1,20,30,16,0,"");
			}
	
		}
		
		/* play volume*/		
		if(BaseTrackVal < 0)Volume =0;
		else
		Volume = BaseTrackVal>>12;
				
		Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,Volume);

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		/* Download the commands into fifo */
		Ft_App_Flush_Co_Buffer(phost);
		/* Wait till coprocessor completes the operation */
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		BaseTrackValInit = BaseTrackValSign;
		prevtag = Read_tag;
                if(scroller.vel!=0)Velocity = scroller.vel;
		Ft_Gpu_Hal_Sleep(30);
	}while(1);
			


}
#endif

ft_void_t SAMAPP_BootupConfig()
{
	/* Do a power cycle for safer side */
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

	/* Set the clk to external clock */
	Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);  
	Ft_Gpu_Hal_Sleep(100);
	  

	/* Switch PLL output to 48MHz */
	Ft_Gpu_HostCommand(phost,FT_GPU_PLL_48M);  
	Ft_Gpu_Hal_Sleep(10);

	/* Do a core reset for safer side */
	Ft_Gpu_HostCommand(phost,FT_GPU_CORE_RESET);     

	/* Access address 0 to wake up the FT800 */
	Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);  


	{
		ft_uint8_t chipid;
		//Read Register ID to check if FT800 is ready. 
		chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
		while(chipid != 0x7C)
			chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
#ifdef MSVC_PLATFORM
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

    /*Set DISP_EN to 1*/
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0x83 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO_DIR));
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x083 | Ft_Gpu_Hal_Rd8(phost,REG_GPIO));
    
    /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
    Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,1200);



}

/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */

#ifdef MSVC_PLATFORM
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#ifdef ARDUINO_PLATFORM
ft_void_t setup()
#endif
{
  Serial.begin(9600);
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

        SAMAPP_BootupConfig();

	printf("reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
	printf("reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
	printf("reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
        /*It is optional to clear the screen here*/	
        Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
        Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
        
        Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen. );
	printf("reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));


        pinMode(FT_SDCARD_CS, OUTPUT);
        digitalWrite(FT_SDCARD_CS, HIGH);
        delay(100);
        sd_present =  SD.begin(FT_SDCARD_CS);
      
      
        SPI.setClockDivider(SPI_CLOCK_DIV2);
        SPI.setBitOrder(MSBFIRST);
        SPI.setDataMode(SPI_MODE0);

	
	Info();
	if(sd_present)
        {
	  #ifdef Absolute Dial
      	      Abs_Meter_Dial();
      	  #endif
      	  #ifdef Relative Dial
      	      Relative_Meter_Dial();
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
