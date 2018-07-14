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

Author : FTDI 

Revision History: 
Version 3.1 - Jun/23/2014 - Removed REG_ROTATE
Version 3.0 - Support for Emulator.
Version 2.0 - Support for FT801 platform.
Version 1.1.2 - Final version based on the requirements.
*/



#include "FT_Platform.h"
#ifdef FT900_PLATFORM
#include "ff.h"
#endif
#define SAMAPP_DELAY_BTW_APIS (1000)
#define SAMAPP_ENABLE_DELAY() Ft_Gpu_Hal_Sleep(SAMAPP_DELAY_BTW_APIS)
#define SAMAPP_ENABLE_DELAY_VALUE(x) Ft_Gpu_Hal_Sleep(x)
#define F16(s)        ((ft_int32_t)((s) * 65536))
#define NORMAL_PRESSURE 1200
#define MAX_ADDITIONAL_VEL 64
#define BASE_VELOCITY 32
#define COLUMN_GAP 8
#define POINTS_PER_COLUMN 5
#define BUTTON_GAP 5
#define BUTTON_CURVATURE 5
#define NUM_OF_ICONS 14
#define ICON_HEIGHT bitmapInfo[0].Height
#define ICON_WIDTH bitmapInfo[0].Width
#define ICON_STRIDE bitmapInfo[0].Stride
#define BOUNCE_AMOUNT 12
#define LOADIMAGE 1
#define INFLATE 2
#define LOAD 3
#define VISIBLE_ICONS 3
#define COMBINATION_AMOUNT 5
#define PAYOUT_TABLE_SIZE 15
#define TOTAL_BET_LINES 12
#define BET_TEXT "Bet:"
#define COLUMN_TEXT "Column: "
#define LINE_TEXT "Line: "
#define BALANCE_TEXT "Balance: "
#define RESET_TEXT "reset"
#define PAYOUT_TEXT "Payout"
#define INITIAL_BALANCE 1000
#define ANY_FRUIT_TEXT "Fruits"

#ifdef TEST
#define SPINNING_SOUND 0x00        
#define COIN_COLLISION_SOUND 0x00 
#define BUTTON_PRESS_SOUND 0x00  
#define COIN_REWARD_SOUND 0x00  
#else
#define SPINNING_SOUND 0x50        //click sound effect
#define COIN_COLLISION_SOUND 0x43 //glockenspiel sound effect
#define BUTTON_PRESS_SOUND 0x51  //switch sound effect
#define COIN_REWARD_SOUND 0x52  //cowbell sound effect
#endif 

/*
#ifdef FT900_PLATFORM
#define FT_FALSE 0
#define TRUE 1
#endif
*/


#if defined(DISPLAY_RESOLUTION_WVGA)
#define SPIN_COLUMNS 5
#define STATUS_BAR_HEIGHT 95
#elif defined(DISPLAY_RESOLUTION_WQVGA)
#define SPIN_COLUMNS 5
#define STATUS_BAR_HEIGHT 50 
#elif defined(DISPLAY_RESOLUTION_QVGA)
#define SPIN_COLUMNS 3
#define STATUS_BAR_HEIGHT 50 

#endif


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


typedef struct bitmap_header_t
{
	ft_uint16_t Width;
	ft_uint16_t Height;
	ft_uint16_t Stride;
	ft_int32_t Offset;
}bitmap_header;

typedef struct spinning_column_t
{
	ft_uint8_t curIndex;
	ft_uint8_t iconArray[20];
	ft_schar8_t velocity;
	ft_uint8_t drawnIndex;
	ft_schar8_t bounceOffset;
	ft_uint8_t bounceAmount;
}spinning_column;


typedef struct coins_t
{
	ft_int16_t xPos;
	ft_int16_t yPos;
	ft_schar8_t xVel;
	ft_schar8_t yVel;
	ft_uint8_t index;
	ft_bool_t fall;
}spin_coins;

typedef struct bet_lines_t
{
	ft_int16_t x0;
	ft_int16_t y0;
	ft_int16_t x1;
	ft_int16_t y1;
	ft_int16_t x2;
	ft_int16_t y2;
	ft_int16_t x3;
	ft_int16_t y3;
	ft_uint8_t r;
	ft_uint8_t g;
	ft_uint8_t b;
}bet_lines;

typedef struct bet_lines_index_t
{
	ft_char8_t line[5];
}bet_lines_index;


spin_coins spinCoins[8];


typedef struct ui_attributes_t
{
	ft_uint8_t spinButtonWidth;
	ft_uint8_t spinColumns;
	ft_uint16_t spinColumnXOffset;
	ft_uint16_t spinColumnYOffset;
	ft_uint16_t visibleIconsHeight;
	ft_uint16_t columnPixelHeight;
}ui_attributes;

typedef struct jackpot_attributes_t
{
	ft_uint8_t spinColumns;
	ft_bool_t spinning;
	ft_bool_t released;
	ft_bool_t rewardedPoints;
	ft_bool_t showPayoutTable;
	ft_bool_t reset;
	ft_bool_t betChanged;
	ft_bool_t displayRewardLine;
	ft_int16_t balance;
	ft_uint8_t totalBetAmount; 
	ft_uint8_t stoppedColumns;
	ft_uint8_t spinColumnSelected;
	ft_uint8_t payoutTableSize;
	ft_uint8_t winningIndex[SPIN_COLUMNS];
	ft_uint8_t touchTag;
	ft_uint8_t pendown;
	ft_int16_t payoutTableShift;
	ft_uint8_t lineBet[12][5];
	ft_uint8_t selectedBetLine;
	ft_uint8_t selectedBetMultiplier;
	ft_uint8_t winningLineIcons;
}jackpot_attributes;



ft_uint8_t Read_Keys();
ft_void_t loadImageToMemory(ft_char8_t* fileName, ft_uint32_t destination, ft_uint8_t type);
ft_uint8_t Ft_Gpu_Rom_Font_WH(ft_uint8_t Char,ft_uint8_t font);
ft_uint16_t stringPixelWidth(const ft_char8_t* text,ft_uint8_t font);
ft_uint16_t unsignedNumberPixelWidth(ft_int16_t digits, ft_uint8_t font);
ft_void_t drawPayoutTable();
ft_void_t drawSpinColumns();
ft_void_t scroll();
ft_void_t updateIndex();
ft_void_t touched(ft_uint8_t touchVal);
ft_uint8_t nextRandomInt8();
ft_uint16_t getPoints();
ft_void_t jackpot();
ft_void_t startingAnimation();
ft_void_t infoScreen();
ft_uint8_t fontPixelHeight(ft_uint8_t font);
ft_void_t clearArray(ft_uint8_t* index, ft_uint16_t size, ft_uint8_t defaultValue);
ft_bool_t ableToSpin();
ft_void_t displayRewardText(ft_uint16_t points);
//ft_void_t displaySelectedIcon(ft_uint8_t index);
ft_bool_t ableToContinue();
ft_void_t coinsAnimation(ft_uint8_t);
ft_int32_t get_free_memory();
ft_void_t playSound(ft_uint8_t value);
ft_void_t lineBetButtons();
ft_uint16_t getlineBetPoints();


ft_uint8_t payoutTable[]={
	8,
	7,
	7,
	6,
	6,
	5,
	5,
	4,
	4,
	3,
	3,
	2,
	2,
	1,
	1
};



bet_lines_index linePosition[] = {

	{'t','t','t','t','t'},  //0
	{'t','t','t','m','b'},//1
	{'m','m','m','m','m'},//2
	{'m','m','m','m','t'},//3
	{'b','b','b','b','b'},//4
	{'b','b','b','m','t'},//5
	{'m','t','t','t','t'},//6
	{'t','m','b','m','t'},//7
	{'b','m','m','m','m'},//8
	{'t','m','m','m','m'},//9
	{'m','m','m','m','b'},//10
	{'b','m','t','m','b'}//11

};

#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
bet_lines betLines[]={
	{44,38,63,50,235,50,404,50,0x7e,0x1e,0x9c},     //1
	{46,67,63,55,235,55,404,170,0x15,0xb0,0x1a},    //2
	{47,100,63,102,235,102,404,102,0x03,0x46,0xDf}, //3
	{46,129,63,107,235,107,404,60,0xff,0x81,0xc0},  //4
	{48,160,63,170,235,170,404,170,0x65,0x37,0x00}, //5
	{49,189,63,165,235,165,404,60,0xe5,0x00,0x00},  //6
	{423,36,404,60,235,60,63,112,0x95,0xd0,0xfc},   //7
	{424,67,398,60,235,160,63,60,0x02,0x93,0x86},   //8
	{425,95,404,112,164,112,63,170,0xf9,0x73,0x06}, //9
	{424,126,404,117,159,117,59,58,0x96,0xf9,0x7b}, //10
	{424,157,404,170,299,122,63,122,0xc2,0x00,0x78},//11
	{424,186,400,180,235,75,83,170,0xff,0xff,0x14}, //12
};
#elif defined(DISPLAY_RESOLUTION_WVGA)
bet_lines betLines[]={
	{100,75,125,100,400,100,650,100,0x7e,0x1e,0x9c},     //1
	{100,125,125,108,400,108,650,300,0x15,0xb0,0x1a},    //2
	{100,190,200,190,400,190,650,190,0x03,0x46,0xDf}, //3
	{100,225,200,195,400,195,650,105,0xff,0x81,0xc0},  //4
	{100,275,125,300,400,300,650,300,0x65,0x37,0x00}, //5
	{100,320,125,308,400,308,650,108,0xe5,0x00,0x00},  //6
	{680,75,650,116,400,116,130,181,0x95,0xd0,0xfc},   //7
	{680,125,650,109,400,300,125,115,0x02,0x93,0x86},   //8
	{680,175,650,208,300,208,125,316,0xf9,0x73,0x06}, //9
	{680,220,650,216,300,216,125,116,0x96,0xf9,0x7b}, //10
	{680,270,650,275,500,222,125,222,0xc2,0x00,0x78},//11
	{680,320,625,300,400,125,175,300,0xff,0xff,0x14}, //12
};
#endif

#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_WVGA)

spinning_column columns[]={
	{6, {2,6,1,5,11,7,10,0,4,8,12,3,9,13},70,6,30,30},
	{5, {11,8,12,1,7,4,0,2,5,3,13,6,10,9},42,8,30,30},
	{9, {9,1,2,4,11,10,13,6,5,7,0,3,12,8},56,1,30,30},
	{11, {10,5,1,11,2,3,7,4,8,12,9,6,0,13},42,5,30,30},
	{10,{5,2,11,4,8,9,7,12,10,13,1,0,3,6},84,1,30,30},
};

#endif

#ifdef DISPLAY_RESOLUTION_QVGA
spinning_column columns[]={
	{6, {2,6,1,5,11,7,10,0,4,8,12,3,9,13},70,6,30,30},
	{5, {11,8,12,1,7,4,0,2,5,3,13,6,10,9},42,8,30,30},
	{9, {9,1,2,4,11,10,13,6,5,7,0,3,12,8},56,1,30,30},
};
#endif

#if defined(DISPLAY_RESOLUTION_WVGA)
	bitmap_header bitmapInfo[] = 
	{
		/*width,height,stride,memory offset */
		{105, 90,105*2,0}, // this one represents all other spinning icons
		{ 16, 16, 16*2,0},  //background icon
		{ 48, 51, 48*2,0},  //coin .bin file
		{ 48, 51, 48*2,0},
		{ 48, 51, 48*2,0},
		{ 48, 51, 48*2,0},
		{ 48, 51, 48*2,0},
		{ 48, 51, 48*2,0},
		{ 48, 51, 48*2,0},
		{105,270,  105,0},  //overlay
		{119,310,119*2,0} //outer overlay

	};

#elif defined(DISPLAY_RESOLUTION_QVGA) | defined(DISPLAY_RESOLUTION_WQVGA)
	bitmap_header bitmapInfo[] = 
	{
		/*width,height,stride,memory offset */
		{64,55,	64* 2,	0	}, // this one represents all other spinning icons
		{16,16, 16* 2,	0	},  //background icon
		{30,32,	30* 2,	0	},  //coin .bin file
		{30,32,	30* 2,	0	},
		{30,32,	30* 2,	0	},
		{30,32,	30* 2,	0	},
		{30,32,	30* 2,	0	},
		{30,32,	30* 2,	0	},
		{30,32,	30* 2,	0	},
		{64,165,64,  0   },  //overlay
		{72,190,72*2,0} //outer overlay

	};
#endif

ui_attributes UI;
jackpot_attributes JPot;
static ft_uint8_t sd_present=0;


/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;

#ifdef FT900_PLATFORM

FATFS FatFs;

FATFS            FatFs;				// FatFs work area needed for each volume
FIL 			 CurFile;
FRESULT          fResult;
SDHOST_STATUS    SDHostStatus;
#endif


ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;

#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif

DWORD get_fattime(void){
	return 0;
}

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

/*
//this function retrieves the current ram usage for the Arduino platform.  Negative number means stack overflow.
ft_int32_t get_free_memory()
{
#ifdef ARDUINO_PLATFORM
	extern ft_int32_t __heap_start, *__brkval;
	ft_int32_t v;
	return (ft_int32_t) &v - (__brkval == 0 ? (ft_int32_t) &__heap_start : (ft_int32_t) __brkval);
#endif
	return 0;
}
*/

#ifdef FT900_PLATFORM
#define FT900_TIMER_MAX_VALUE (65536)
ft_uint32_t tpprevval = 0,timercount,timervalue;

ft_int16_t timer_profileinit(ft_uint32_t *tcount,ft_uint32_t *tvalue)
{
	tpprevval = 0;
	*tcount = 0;
	*tvalue = 0;

	return 0;
}

/* api for start count */
ft_int16_t timer_profilecountstart(ft_uint32_t *tcount,ft_uint32_t *tvalue)
{
	//tpprevval = ft900_timer_get_value(2);
	 timer_read(timer_select_a,&tpprevval);
	//tpprevval =
	return 0;
}

ft_int16_t timer_profilecountend(ft_uint32_t *tcount,ft_uint32_t *tvalue)
{
	ft_uint32_t currtime;
	//currtime = ft900_timer_get_value(2);
	 timer_read(timer_select_a, &currtime);
	if(tpprevval > currtime)
	{
		/* loop back condition */
		*tvalue += FT900_TIMER_MAX_VALUE - tpprevval + currtime;
	}
	else
	{
		*tvalue += currtime - tpprevval;
	}

	*tcount += 1;

	return 0;
}

#endif


static ft_uint8_t currentLine=0;
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
static time_t lastMilli;
#endif
#if defined(FT900_PLATFORM)
ft_uint32_t curMilli=0;
#endif

ft_bool_t finished=FT_FALSE;

ft_bool_t displayTheLine(ft_uint8_t line){
#ifdef ARDUINO_PLATFORM
	static ft_uint32_t lastMilli=millis();
	ft_uint32_t curMilli;
	ft_bool_t nextLine=FT_FALSE;
	ft_uint8_t i,j;
	for(i=0;i<SPIN_COLUMNS;i++){
		if(JPot.lineBet[currentLine][i] < 254){
			nextLine=TRUE;
			break;
		}
	}
	
	if(nextLine){
		curMilli = millis();
		if((curMilli-lastMilli)>1000){
			currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);
			lastMilli=curMilli;
			return FT_FALSE;
		}
		else{
		return TRUE; 
		}
	}
	else{
		currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);
		return FT_FALSE;
	}
#endif

#if defined(FT900_PLATFORM)
	ft_bool_t nextLine=FT_FALSE;
		ft_uint8_t i,j;
		for(i=0;i<SPIN_COLUMNS;i++){
			if(JPot.lineBet[currentLine][i] < 254){
				nextLine=TRUE;
				break;
			}
		}

		if(nextLine){
			timer_profilecountend(&timercount,&timervalue);
			if((timervalue - curMilli)>10000){
				currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);

				if((timervalue - curMilli)>20000){
					timer_profilecountstart(&timercount,&timervalue);
					timer_profilecountend(&timercount,&timervalue);
					curMilli = timervalue;
				}
				else{
					curMilli = timervalue;
				}
				if((curMilli+10000) < curMilli){ //test whether the timer is approaching the 32bit number limit and reset the time counter
					curMilli=0;
					printf("Time counter reset.\n");
				}
				return FT_FALSE;
			}
			else{
				timer_profilecountstart(&timercount,&timervalue);
				return TRUE;
			}
		}
		else{
			currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);
			return FT_FALSE;
		}
#endif

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	time_t curMilli;
	ft_bool_t nextLine=FT_FALSE;
	ft_uint8_t i,j;
	for(i=0;i<SPIN_COLUMNS;i++){
		if(JPot.lineBet[currentLine][i] < 254){
			nextLine=TRUE;
			break;
		}
	}

	if(nextLine){
		curMilli = time(NULL);
		if((curMilli-lastMilli)>1){
			currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);
			lastMilli=curMilli;
			return FT_FALSE;
		}
		else{
		return TRUE; 
		}
	}
	else{
		currentLine=(currentLine+1) >= TOTAL_BET_LINES ? 0 : (currentLine+1);
		return FT_FALSE;
	}

#endif
}

ft_void_t drawbetlines(){
	ft_uint8_t i;
	Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(3 * 16) );
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
	if(JPot.displayRewardLine){
		if(displayTheLine(currentLine)){
				Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(betLines[currentLine].r,betLines[currentLine].g,betLines[currentLine].b));
				Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
#if defined(DISPLAY_RESOLUTION_QVGA) | defined(DISPLAY_RESOLUTION_WQVGA)
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[currentLine].x0,betLines[currentLine].y0,0,0));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[currentLine].x1,betLines[currentLine].y1,0,0));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[currentLine].x2,betLines[currentLine].y2,0,0));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[currentLine].x3,betLines[currentLine].y3,0,0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[currentLine].x0 *16,betLines[currentLine].y0 *16));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[currentLine].x1 *16,betLines[currentLine].y1 *16));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[currentLine].x2 *16,betLines[currentLine].y2 *16));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[currentLine].x3 *16,betLines[currentLine].y3 *16));
#endif
				Ft_App_WrCoCmd_Buffer(phost, END());
			}
	}
	else{
	for(i=0;i<JPot.selectedBetLine;i++){
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(betLines[i].r,betLines[i].g,betLines[i].b));
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
#if defined(DISPLAY_RESOLUTION_QVGA) | defined(DISPLAY_RESOLUTION_WQVGA)
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[i].x0,betLines[i].y0,0,0));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[i].x1,betLines[i].y1,0,0));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[i].x2,betLines[i].y2,0,0));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(betLines[i].x3,betLines[i].y3,0,0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[i].x0 *16,betLines[i].y0 *16));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[i].x1 *16,betLines[i].y1 *16));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[i].x2 *16,betLines[i].y2 *16));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(betLines[i].x3 *16,betLines[i].y3 *16));
#endif
			Ft_App_WrCoCmd_Buffer(phost, END());
	}
	}
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
}



/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */
const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
	255,255,255,2,//GPU instruction CLEAR_COLOR_RGB
	7,0,0,38, //GPU instruction CLEAR
	0,0,0,0,  //GPU instruction DISPLAY
};

ft_void_t playSound(ft_uint8_t value){
	Ft_Gpu_Hal_Wr16(phost,REG_SOUND,value);
	Ft_Gpu_Hal_Wr8(phost,REG_PLAY,0x01);
}


static ft_uint8_t sk=0, temp_tag=0;

/* Most buttons are responded on release but spinning button and payout table tracking are being handled seperately and differenly.  */
ft_uint8_t Read_Keys()
{
	static ft_uint8_t ret_tag=0, touchVal,temp_tag1;	
	ft_uint8_t i;
	touchVal = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
	JPot.pendown = (Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_DIRECT_XY)>>31) & 0x1;
	ret_tag = 0;

	if(JPot.spinning && !JPot.released && JPot.pendown){
		JPot.released=TRUE;
		for(i=0; i<SPIN_COLUMNS;i++){
			nextRandomInt8();
			columns[i].velocity =(BASE_VELOCITY +((nextRandomInt8()>>4) & 31L));	
		}
	}


	/* the respond-on-release sensing method does not apply to the payout table tracking and spin button*/
	if(touchVal==200 || touchVal==201){
		temp_tag = touchVal;
		return touchVal;
	}
	
	if(touchVal!=0)								// Allow if the Key is released
	{
		if(temp_tag!=touchVal)
		{
			temp_tag = touchVal;	
			sk = touchVal;										// Load the Read tag to temp variable	
		}  
	}
	else
	{
		if(temp_tag!=0)
		{
			ret_tag = temp_tag;
			temp_tag=0;
		}  
		sk = 0;
	}
	return ret_tag;
}



ft_void_t loadImageToMemory(ft_char8_t* fileName, ft_uint32_t destination, ft_uint8_t type){
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	FILE *fp;
	ft_uint32_t fileLen;
	ft_uint8_t pBuff[8192];
	if(type==LOADIMAGE){  //loadimage command takes destination address and options before the actual bitmap data
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
		Ft_Gpu_Hal_WrCmd32(phost,  destination);
		Ft_Gpu_Hal_WrCmd32(phost,  OPT_NODL);
	}
	else if(type==INFLATE){ //inflate command takes destination address before the actual bitmap
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_INFLATE);
		Ft_Gpu_Hal_WrCmd32(phost,  destination);
	}
	else if(type==LOAD){ //load bitmaps directly
	}
	fp = fopen(fileName, "rb+");

	if(fp){ 

		fseek(fp,0,SEEK_END);
		fileLen = ftell(fp);
		fseek(fp,0,SEEK_SET);
		while(fileLen > 0)
		{
			ft_uint32_t blocklen = fileLen>8192?8192:fileLen;		
			fread(pBuff,1,blocklen,fp);
			fileLen -= blocklen;
			if(LOAD== type)
			{
				Ft_Gpu_Hal_WrMem(phost,destination,pBuff, blocklen);//alignment is already taken care by this api
			}
			else
			{
				Ft_Gpu_Hal_WrCmdBuf(phost,pBuff, blocklen);//alignment is already taken care by this api
			}

			destination += blocklen;
		}
		fclose(fp);
	}else{
		printf("Unable to open file: %s\n",fileName);  
		//exit(1); 
	}
#endif

#ifdef ARDUINO_PLATFORM
	ft_uint8_t imbuff[512];  //the SD library requires a mandatory of 512 bytes of buffer during its operation. 
	//ft_uint16_t blocklen;
	byte file;
	Reader jpeg;

	if(type==LOADIMAGE){  //loadimage command takes destination address and options before the actual bitmap data
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
		Ft_Gpu_Hal_WrCmd32(phost,  destination);
		Ft_Gpu_Hal_WrCmd32(phost,  OPT_NODL);
	}
	else if(type==INFLATE){ //inflate command takes destination address before the actual bitmap
		Ft_Gpu_Hal_WrCmd32(phost,  CMD_INFLATE);
		Ft_Gpu_Hal_WrCmd32(phost,  destination);
	}
	else if(type==LOAD){ //load bitmaps directly
	}

	if(sd_present)
	{
		file = jpeg.openfile(fileName); 
	}

	if(file){
		while (jpeg.offset < jpeg.size)
		{
			ft_uint16_t n = min(512, jpeg.size - jpeg.offset);
			n = (n + 3) & ~3;   // force 32-bit alignment
			jpeg.readsector(imbuff);
			if(type==LOAD)
			{
				Ft_Gpu_Hal_WrMem(phost,destination,imbuff, n);//alignment is already taken care by this api
				destination += n;
			}
			else
			{
				Ft_Gpu_Hal_WrCmdBuf(phost,imbuff, n);//alignment is already taken care by this api
			}
		}
	}
#endif

#ifdef FT900_PLATFORM
	FIL InfSrc;
	ft_int32_t blocklen,filesize;
	ft_uint32_t g_random_buffer[512L];
	char tempName[14]="0:";

	strcat(tempName,fileName);
	fResult = f_open(&InfSrc, tempName, FA_READ | FA_OPEN_EXISTING);
	//fResult = f_open(&InfSrc, "0:\\overlayH.raw", FA_READ | FA_OPEN_EXISTING);
	if(fResult==FR_OK)
	{
		printf("Loading: %s\n",tempName);
		if(type==LOADIMAGE){  //loadimage command takes destination address and options before the actual bitmap data
			Ft_Gpu_Hal_WrCmd32(phost,  CMD_LOADIMAGE);
			Ft_Gpu_Hal_WrCmd32(phost,  destination);
			Ft_Gpu_Hal_WrCmd32(phost,  OPT_NODL);
		}
		else if(type==INFLATE){ //inflate command takes destination address before the actual bitmap
			Ft_Gpu_Hal_WrCmd32(phost,  CMD_INFLATE);
			Ft_Gpu_Hal_WrCmd32(phost,  destination);
		}
		else if(type==LOAD){ //load bitmaps directly
		}

		filesize = f_size(&InfSrc);
		//printf("   Size: %d",filesize);
		while(filesize>0)
		{
			fResult = f_read(&InfSrc, g_random_buffer, 512, &blocklen);	// read a chunk of src file
			filesize -= blocklen;
			if(type==LOAD){
				Ft_Gpu_Hal_WrMem(phost,destination,g_random_buffer, blocklen);
				destination += blocklen;
			}
			else{
				Ft_Gpu_Hal_WrCmdBuf(phost,g_random_buffer,blocklen);
			}

		}
		f_close(&InfSrc);

	}
	else{
		printf("Unable to open: %s", fileName);
	}
#endif
}




//static ft_uint32_t ramOffset=0;

ft_void_t jackpotSetup(){

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	ft_char8_t path[64];
	ft_char8_t homePath[] = "..\\..\\..\\Test\\";
#else
	ft_char8_t path[13];
#endif
	ft_uint32_t ramOffset=0;
	ft_char8_t fileName[3];
	ft_uint8_t i;
#if defined(DISPLAY_RESOLUTION_WQVGA)
	ft_char8_t bitmapExtention[] = "J.jpg";
	UI.spinButtonWidth=80;
#elif defined(DISPLAY_RESOLUTION_QVGA)
	ft_char8_t bitmapExtention[] = "J.jpg";
	UI.spinButtonWidth=50;
#elif defined(DISPLAY_RESOLUTION_WVGA)
	ft_char8_t bitmapExtention[] = "JH.jpg";
	UI.spinButtonWidth=100;

#endif


	UI.columnPixelHeight=NUM_OF_ICONS*ICON_HEIGHT;
	JPot.spinning=FT_FALSE;
	JPot.released=FT_FALSE;
	JPot.rewardedPoints=TRUE;
	JPot.showPayoutTable=FT_FALSE;
	JPot.reset=FT_FALSE;
	JPot.balance=INITIAL_BALANCE;
	JPot.spinColumnSelected=0;
	JPot.pendown=0;
	JPot.payoutTableShift=0;
	JPot.selectedBetMultiplier=0;
	JPot.selectedBetLine=12;
	UI.visibleIconsHeight=VISIBLE_ICONS * ICON_HEIGHT;
	UI.spinColumnXOffset=(FT_DispWidth>>1) - ((SPIN_COLUMNS*ICON_WIDTH + SPIN_COLUMNS*COLUMN_GAP)>>1);  //center the spinning columns at the middle of the screen
	UI.spinColumnYOffset= (((FT_DispHeight-STATUS_BAR_HEIGHT)>>1) - ((UI.visibleIconsHeight)>>1));   //center the spinning columns at the middle of the screen


	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	Ft_Gpu_CoCmd_Dlstart(phost);        // start
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
#if defined(DISPLAY_RESOLUTION_WVGA)
	Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,(FT_DispHeight>>1)-50,29,OPT_CENTER,"Loading bitmap...");
#else
	Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,FT_DispHeight>>1,29,OPT_CENTER,"Loading bitmap...");
#endif
	Ft_Gpu_CoCmd_Spinner(phost, (FT_DispWidth>>1),(FT_DispHeight>>1)+50,0,0);
	//Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	//Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

	#ifdef ARDUINO_PLATFORM
	if(!sd_present){
		while(1){
		Ft_Gpu_CoCmd_Dlstart(phost);        // start
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,FT_DispHeight>>1,29,OPT_CENTER,"Storage Device not Found");
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		}
	}
	#endif


	Ft_Gpu_CoCmd_Dlstart(phost);        // start
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));

	printf("Loading regular bitmaps\n");
	//load the bitmaps into memory, all spinning icons are assumed to be the same in all dimensions.
	for(i=0; i<NUM_OF_ICONS;i++){
		path[0]=fileName[0]='\0';
		Ft_Gpu_Hal_Dec2Ascii(fileName,i);
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		strcat(path, homePath);
#endif
		strcat(path, fileName);
		strcat(path, bitmapExtention);
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE((i)));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(ramOffset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,ICON_STRIDE,ICON_HEIGHT));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,ICON_WIDTH,ICON_HEIGHT));
		loadImageToMemory(path,ramOffset,LOADIMAGE);
		ramOffset+=(ICON_STRIDE*ICON_HEIGHT);
	}

	printf("Loading status bar\n");
	//load status bar bitmap by itself because the width is being repeated 
	path[0]=fileName[0]='\0';
	Ft_Gpu_Hal_Dec2Ascii(fileName,14);
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	strcat(path, homePath);
#endif
	strcat(path, fileName);
	strcat(path, bitmapExtention);
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE((14)));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(ramOffset));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,2,STATUS_BAR_HEIGHT));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,REPEAT,BORDER,FT_DispWidth,STATUS_BAR_HEIGHT));
#if defined(DISPLAY_RESOLUTION_WVGA)
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE_H(FT_DispWidth>>9,STATUS_BAR_HEIGHT>>9));	
#endif
	loadImageToMemory(path,ramOffset,LOADIMAGE);
	ramOffset+=(2*STATUS_BAR_HEIGHT);


	printf("Loading background bitmap\n");
	//load background bitmap into memory because the width and height are repeated. All bitmap handles have been exhausted, this icon will be drawn by specifying its souce, layout, and size. 
	path[0]=fileName[0]='\0';
	Ft_Gpu_Hal_Dec2Ascii(fileName,15);
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	strcat(path, homePath);
#endif
	strcat(path, fileName);
	strcat(path, bitmapExtention);
	bitmapInfo[1].Offset=ramOffset;
	loadImageToMemory(path,ramOffset, LOADIMAGE);
	ramOffset+=(bitmapInfo[1].Stride*bitmapInfo[1].Height);


	printf("loading coins\n");
	//load .bin coin bitmap files. 
	for(i=16;i<24;i++){
		path[0]=fileName[0]='\0';
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		strcat(path, homePath);
#endif
		Ft_Gpu_Hal_Dec2Ascii(fileName,i);
		strcat(path, fileName);
#if defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_QVGA)
		strcat(path, "J.bin");
#elif defined(DISPLAY_RESOLUTION_WVGA)
		strcat(path, "JH.bin");
#endif
		bitmapInfo[i-14].Offset=ramOffset;
		loadImageToMemory(path,ramOffset,INFLATE);
		ramOffset+=(bitmapInfo[i-14].Stride*bitmapInfo[i-14].Height);
	}


	printf("loading overlay\n");
	path[0]='\0';
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	strcat(path, homePath);
#endif

#if defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_QVGA)
	strcat(path, "overlay.raw");
#elif defined(DISPLAY_RESOLUTION_WVGA)
	strcat(path, "overlayH.raw");
#endif
	bitmapInfo[9].Offset=ramOffset;
	loadImageToMemory(path,ramOffset,LOAD);
	ramOffset+=(bitmapInfo[9].Stride*bitmapInfo[9].Height);

	printf("loading outer overlay\n");
	path[0]='\0';
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	strcat(path, homePath);
#endif

#if defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_QVGA)
	strcat(path, "outer.raw");
#elif defined(DISPLAY_RESOLUTION_WVGA)
	strcat(path, "outerH.raw");
#endif
	bitmapInfo[10].Offset=ramOffset;
	loadImageToMemory(path,ramOffset,LOAD);
	ramOffset+=(bitmapInfo[10].Stride*bitmapInfo[10].Height);

	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}



ft_void_t coinsAnimation(ft_uint8_t coinAmount){
	ft_schar8_t gravity=2;
	ft_uint8_t i;

	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

#if 0
	for(i=0; i<coinAmount; i++){ 
		if(spinCoins[i].index>=7){
			spinCoins[i].index=0;
		}
		if((spinCoins[i].xPos+bitmapInfo[spinCoins[i].index+2].Width<0) || (spinCoins[i].xPos>FT_DispWidth) || (spinCoins[i].yPos>FT_DispHeight)){
			continue;
		}
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(bitmapInfo[spinCoins[i].index+2].Offset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4,bitmapInfo[spinCoins[i].index+2].Stride,bitmapInfo[spinCoins[i].index+2].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,bitmapInfo[spinCoins[i].index+2].Width,bitmapInfo[spinCoins[i].index+2].Height));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(spinCoins[i].xPos * 16 ,spinCoins[i].yPos * 16));
		spinCoins[i].index+=1;
		if(spinCoins[i].index>=7){
			spinCoins[i].index=0;
		}

		spinCoins[i].xPos+=spinCoins[i].xVel;
		spinCoins[i].yPos+=spinCoins[i].yVel;
		spinCoins[i].yVel+=gravity;

		if((spinCoins[i].yPos+bitmapInfo[spinCoins[i].index+2].Height)> (FT_DispHeight-STATUS_BAR_HEIGHT) && !spinCoins[i].fall){
			spinCoins[i].yPos=FT_DispHeight-STATUS_BAR_HEIGHT-bitmapInfo[spinCoins[i].index+2].Height;
			spinCoins[i].yVel=-1 * ((spinCoins[i].yVel*3)/4);
			playSound(COIN_COLLISION_SOUND); //play sound when the coin hits the status bar
			if(spinCoins[i].yVel==0){
				spinCoins[i].fall=TRUE;
			}
		}
	}
#else
	for(i=0; i<coinAmount; i++){ 
		if(spinCoins[i].index>=7){
			spinCoins[i].index=0;
		}
		if((spinCoins[i].xPos+bitmapInfo[spinCoins[i].index+2].Width<0) || (spinCoins[i].xPos>FT_DispWidth) || (spinCoins[i].yPos>FT_DispHeight)){
			continue;
		}


		if((spinCoins[i].yPos+bitmapInfo[spinCoins[i].index+2].Height)> (FT_DispHeight-STATUS_BAR_HEIGHT) && !spinCoins[i].fall){
			spinCoins[i].yPos=FT_DispHeight-STATUS_BAR_HEIGHT-bitmapInfo[spinCoins[i].index+2].Height;
			spinCoins[i].yVel=-1 * ((spinCoins[i].yVel*3)/4);
			playSound(COIN_COLLISION_SOUND); //play sound when the coin hits the status bar
			if(spinCoins[i].yVel==0){
				spinCoins[i].fall=TRUE;
			}
		}

		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(bitmapInfo[spinCoins[i].index+2].Offset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4,bitmapInfo[spinCoins[i].index+2].Stride,bitmapInfo[spinCoins[i].index+2].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,bitmapInfo[spinCoins[i].index+2].Width,bitmapInfo[spinCoins[i].index+2].Height));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(spinCoins[i].xPos * 16 ,spinCoins[i].yPos * 16));
		spinCoins[i].index+=1;
		if(spinCoins[i].index>=7){
			spinCoins[i].index=0;
		}

		spinCoins[i].xPos+=spinCoins[i].xVel;
		spinCoins[i].yPos+=spinCoins[i].yVel;
		spinCoins[i].yVel+=gravity;

	}

#endif
}




ft_uint8_t Ft_Gpu_Rom_Font_WH(ft_uint8_t Char,ft_uint8_t font)
{
	ft_uint32_t ptr,Wptr;
	ft_uint8_t Width=0;
	ptr = Ft_Gpu_Hal_Rd32(phost,ROMFONT_TABLEADDRESS);					

	// read Width of the character
	Wptr = (ptr + (148L * (font- 16L)))+Char;	// (table starts at font 16)
	Width = Ft_Gpu_Hal_Rd8(phost,Wptr);
	return Width;
}

ft_uint8_t fontPixelHeight(ft_uint8_t font){

	ft_uint32_t ptr,hPtr;
	ft_uint8_t height=0;
	ptr = Ft_Gpu_Hal_Rd32(phost,ROMFONT_TABLEADDRESS);	


	// the height is at the 140th byte
	hPtr = (ptr + (148 * (font- 16)))+140;	// (table starts at font 16)
	height = Ft_Gpu_Hal_Rd8(phost,hPtr);
	return height;
	
	
}

ft_uint16_t stringPixelWidth(const ft_char8_t* text,ft_uint8_t font){
	ft_char8_t tempChar;
	ft_uint16_t length=0, index; 
	if(text==NULL){
		return 0;
	}

	if(text[0]=='\0'){
		return 0;
	}

	index=0;
	tempChar=text[index];
	while(tempChar!='\0'){
		length+=Ft_Gpu_Rom_Font_WH(tempChar, font);
		tempChar=text[++index];
	}

	return length;
}

//Numbers of the same font have the same width
ft_uint16_t unsignedNumberPixelWidth(ft_int16_t digits, ft_uint8_t font){
	return Ft_Gpu_Rom_Font_WH('1',font)*digits;
}








ft_void_t drawPayoutTable(){
#if defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_QVGA)
	ft_uint8_t  numFont=23, rateOfChange=2, payoutNumOffset=fontPixelHeight(numFont)>>1, anyFruitTextFont=25, topIndex, i, currentMultiplier;
#elif defined(DISPLAY_RESOLUTION_WVGA)
	ft_uint8_t  numFont=30, rateOfChange=3, /*payoutNumOffset=fontPixelHeight(numFont)>>1*/payoutNumOffset=ICON_HEIGHT/2, anyFruitTextFont=25, topIndex, i, currentMultiplier;
#endif
	static ft_uint8_t anyFruitTextLength, payoutNumWidth, anyFruitTextHeight, firstTime=1, multiplier=5;
	static ft_int16_t topPoint=0, tableSize;
	ft_int16_t startingX, startingY=0, payoutSetHeight=PAYOUT_TABLE_SIZE * ICON_HEIGHT;

	scroll();  //updates payout table offset

	if(firstTime){
		anyFruitTextLength= (ft_uint8_t)stringPixelWidth(ANY_FRUIT_TEXT, anyFruitTextFont);
		payoutNumWidth=(ft_uint8_t)unsignedNumberPixelWidth(3,numFont);
		anyFruitTextHeight=fontPixelHeight(anyFruitTextFont);
		tableSize = FT_DispHeight - STATUS_BAR_HEIGHT;

		firstTime=0;
	}

	topPoint+=JPot.payoutTableShift;

	if(JPot.payoutTableShift>0){
		JPot.payoutTableShift-=rateOfChange;
		JPot.payoutTableShift = JPot.payoutTableShift < 0 ? 0 : JPot.payoutTableShift;
	}
	else if(JPot.payoutTableShift<0){
		JPot.payoutTableShift+=rateOfChange;
		JPot.payoutTableShift = JPot.payoutTableShift > 0 ? 0 : JPot.payoutTableShift;
	}

	if(JPot.payoutTableShift != 0)
		printf("%d\n",JPot.payoutTableShift);

	if(topPoint<0){
		topPoint=(payoutSetHeight+topPoint);
		multiplier++;
		if(multiplier>5){
			multiplier=1;
		}
	}
	else if(topPoint>payoutSetHeight){
		topPoint=topPoint%payoutSetHeight;
		multiplier--;
		if(multiplier<1){
			multiplier=5;
		}

	}
	startingX=UI.spinColumnXOffset;
	topIndex = topPoint/ICON_HEIGHT;
	startingY=(topPoint%ICON_HEIGHT)*(-1);
	currentMultiplier=multiplier;

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(16));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_A(0xff));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
#if defined(DISPLAY_RESOLUTION_WQVGA) || (DISPLAY_RESOLUTION_QVGA)
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(UI.spinColumnXOffset,0, 0,0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(UI.spinColumnXOffset + ICON_WIDTH*3 + payoutNumWidth*2,tableSize, 0,0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(UI.spinColumnXOffset*16,0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((UI.spinColumnXOffset + ICON_WIDTH*3 + payoutNumWidth*2)*16,tableSize*16));
#endif

	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

	//a maximum of 5 icons can be drawn with a drawing table of 220 pixels in height.
	for(i=0;i<6;i++){
		if(topIndex>14){
			topIndex=0;
			currentMultiplier--;
			if(currentMultiplier<1){
				currentMultiplier=5;
			}
		}

		if(topIndex!=14){ 
			Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(topIndex));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(startingX*16,startingY*16));
			startingX+=(ICON_WIDTH + BUTTON_GAP);
		}
		else{ //fruit combination is at index 14
			Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ZERO));
			Ft_Gpu_CoCmd_Text(phost,startingX, startingY+((ICON_HEIGHT>>1)-(anyFruitTextHeight>>1)), anyFruitTextFont/*font*/, 0, ANY_FRUIT_TEXT);
			startingX += anyFruitTextLength + BUTTON_GAP;
			Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
		}

		//draw payout amount with the correct multipler
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ZERO));
		startingX+=BUTTON_GAP;
		Ft_Gpu_CoCmd_Text(phost,startingX,startingY+payoutNumOffset,numFont,0,"X");
		startingX+=20;
		Ft_Gpu_CoCmd_Number(phost, startingX, startingY+payoutNumOffset, numFont, 0, currentMultiplier);
		startingX+=ICON_WIDTH;
		if(currentMultiplier!=1 || topIndex==14){
			Ft_Gpu_CoCmd_Number(phost, startingX, startingY+payoutNumOffset, numFont, 0, payoutTable[topIndex] * currentMultiplier);
		}
		else{
			Ft_Gpu_CoCmd_Number(phost, startingX, startingY+payoutNumOffset, numFont, 0, 0);
		}
		Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));

		startingY+=ICON_HEIGHT;
		startingX=UI.spinColumnXOffset;
		topIndex++; 
	}


	Ft_Gpu_CoCmd_LoadIdentity(phost);
	Ft_Gpu_CoCmd_SetMatrix(phost);
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE((15)));
}

ft_void_t redrawColumnIcons(){
	ft_int16_t topIndex, bottomIndex, i, j, startingX=UI.spinColumnXOffset, startingY=UI.spinColumnYOffset, highLightingIndex;

	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

	for(i=0; i<SPIN_COLUMNS;i++){
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		topIndex=columns[i].curIndex;
		bottomIndex=(topIndex+2)%NUM_OF_ICONS;

		//draw icons from top to the bottom index
		while(1){
			//the drawn icon in each column has a brighter color
			if((columns[i].iconArray[topIndex]==JPot.lineBet[currentLine][i]) && JPot.displayRewardLine){

				Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX,(startingY),columns[i].iconArray[topIndex],0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(columns[i].iconArray[topIndex]));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(startingX*16,startingY*16));
#endif
				//icons in the winning line will have corresponding color box on top of it
				Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(betLines[currentLine].r,betLines[currentLine].g,betLines[currentLine].b));
					Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(16*2));
					Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINE_STRIP));
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX-1,startingY-1,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH+1,startingY-1,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH+1,startingY+ICON_HEIGHT+1,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX-1,startingY+ICON_HEIGHT+1,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX-1,startingY-1,0,0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX-1)*16,(startingY-1)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH+1)*16,(startingY-1)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH+1)*16,(startingY+ICON_HEIGHT+1)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX-1)*16,(startingY+ICON_HEIGHT+1)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX-1)*16,(startingY-1)*16));
#endif

					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
					Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
			}

			if(topIndex==bottomIndex){
				break;
			}
			startingY+=ICON_HEIGHT;
			topIndex=(topIndex+1)%NUM_OF_ICONS;
		}

		startingX+=(ICON_WIDTH+COLUMN_GAP);
		startingY=UI.spinColumnYOffset;
	}
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(15));
}


ft_void_t drawSpinColumns(){
	ft_int16_t topIndex, bottomIndex, i, j, startingX=UI.spinColumnXOffset, startingY=UI.spinColumnYOffset, highLightingIndex;
	static ft_schar8_t bounceOffset;

	updateIndex();
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	bounceOffset=0;
	JPot.stoppedColumns=0;
	for(i=0; i<SPIN_COLUMNS;i++){
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		topIndex=columns[i].curIndex;
		bottomIndex=(topIndex+2)%NUM_OF_ICONS;
		highLightingIndex=(columns[i].curIndex+1)%NUM_OF_ICONS;

		//bouncing effect happens after the column was stopped
		if(columns[i].velocity<=0){
			if(columns[i].bounceAmount>0){
				bounceOffset=columns[i].bounceOffset=columns[i].bounceOffset-5;
				if(bounceOffset<0){
					bounceOffset=columns[i].bounceOffset=columns[i].bounceAmount=columns[i].bounceAmount>>1;
				}
			}
			else{
				JPot.stoppedColumns++;
			}
		}
		else{
			playSound(SPINNING_SOUND);  //spinning sound for each column
		}


		//draw icons from top to the bottom index
		while(topIndex!=bottomIndex){
			//the drawn icon in each column has a brighter color
			if((topIndex==highLightingIndex) && (columns[i].velocity<=0)){
				//columns[i].drawnIndex=columns[i].iconArray[highLightingIndex];
				columns[i].drawnIndex=highLightingIndex;
				Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX,(startingY+bounceOffset),columns[i].iconArray[topIndex],0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
				Ft_App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(columns[i].iconArray[topIndex]));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(startingX*16,(startingY+bounceOffset)*16));
#endif
				//icons in the winning combination will have a red box on top of it.
				if(JPot.winningIndex[i]!=255 && JPot.rewardedPoints){
					//highlight box
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));
					Ft_App_WrCoCmd_Buffer(phost, LINE_WIDTH(16*2));
					Ft_App_WrCoCmd_Buffer(phost, BEGIN(LINES));
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
					//top line
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+2,startingY+bounceOffset,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH,startingY+bounceOffset,0,0));
					//left line
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+3,startingY+bounceOffset,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+3,startingY+ICON_HEIGHT+bounceOffset,0,0));
					//right line
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH-1,startingY+bounceOffset,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH-1,startingY+ICON_HEIGHT+bounceOffset,0,0));
					//bottom line
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX-1,startingY+ICON_HEIGHT+bounceOffset-1,0,0));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+ICON_WIDTH-1,startingY+ICON_HEIGHT+bounceOffset-1,0,0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
					//top line
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+2)*16,(startingY+bounceOffset)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH)*16,(startingY+bounceOffset)*16));
					//left line							
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+3)*16,(startingY+bounceOffset)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+3)*16,(startingY+ICON_HEIGHT+bounceOffset)*16));
					//right line						
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH-1)*16,(startingY+bounceOffset)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH-1)*16,(startingY+ICON_HEIGHT+bounceOffset)*16));
					//bottom line						
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX-1)*16,(startingY+ICON_HEIGHT+bounceOffset-1)*16));
					Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+ICON_WIDTH-1)*16,(startingY+ICON_HEIGHT+bounceOffset-1)*16));
#endif
					Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
					Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
				}
			}
			else{
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX,(startingY+bounceOffset),columns[i].iconArray[topIndex],0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
				Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(columns[i].iconArray[topIndex]));
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(startingX*16,(startingY+bounceOffset)*16));
#endif
			}
			startingY+=ICON_HEIGHT;
			topIndex=(topIndex+1)%NUM_OF_ICONS;
		}

#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX,(startingY+bounceOffset),columns[i].iconArray[topIndex],0));
#elif defined(DISPLAY_RESOLUTION_WVGA)
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(columns[i].iconArray[topIndex]));
		Ft_App_WrCoCmd_Buffer(phost, VERTEX2F(startingX*16,(startingY+bounceOffset)*16));
#endif

		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(15));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));

		//draw the overlay, which is exactly on top of the bitmaps
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(bitmapInfo[9].Offset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,bitmapInfo[9].Stride,bitmapInfo[9].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,bitmapInfo[9].Width,bitmapInfo[9].Height));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((startingX)*16,(UI.spinColumnYOffset)*16));

		//draw the outer overlay, some offsets
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(bitmapInfo[10].Offset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(ARGB4,bitmapInfo[10].Stride,bitmapInfo[10].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,bitmapInfo[10].Width,bitmapInfo[10].Height));
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((startingX-3)*16,(UI.spinColumnYOffset-12)*16));
#elif defined(DISPLAY_RESOLUTION_WVGA)
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((startingX-6)*16,(UI.spinColumnYOffset-20)*16));
#endif

		//selected columns indication: dots
		if((i+1)<=JPot.spinColumnSelected){
			Ft_App_WrCoCmd_Buffer(phost, POINT_SIZE(6 * 16) );
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS) );
#if defined(DISPLAY_RESOLUTION_WQVGA) | defined(DISPLAY_RESOLUTION_QVGA)
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+(ICON_WIDTH>>1), UI.spinColumnYOffset-6,0,0 ));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(startingX+(ICON_WIDTH>>1), startingY+ICON_HEIGHT+6,0,0 ));
#elif defined(DISPLAY_RESOLUTION_WVGA)
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+(ICON_WIDTH>>1))*16, (UI.spinColumnYOffset-6)*16));
			Ft_App_WrCoCmd_Buffer(phost, VERTEX2F((startingX+(ICON_WIDTH>>1))*16, (startingY+ICON_HEIGHT+6)*16));
#endif
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS) );
		}

		startingX+=(ICON_WIDTH+COLUMN_GAP);
		startingY=UI.spinColumnYOffset;
		bounceOffset=0;
	}
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
}


//payout table scrolling
ft_void_t scroll(){
	static ft_uint16_t curScreenY, preScreenY1, preScreenY2;
	static ft_bool_t continueTouch=FT_FALSE;
	if(!continueTouch && JPot.touchTag==200){
		continueTouch=TRUE;
		curScreenY = Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_SCREEN_XY);
		preScreenY2=preScreenY1 = curScreenY;
	}

	//scroll while pendown
	if(continueTouch && !JPot.pendown){
		preScreenY2=preScreenY1;
		curScreenY = Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_SCREEN_XY);
		if((curScreenY != 32768) && (preScreenY1 != 32768)){
			if(curScreenY != preScreenY1){
				JPot.payoutTableShift=(preScreenY1-curScreenY);
			}
		}
	}

	//scroll on release
	if(continueTouch && JPot.pendown){
		if(preScreenY2!= 32768 && curScreenY!=32768){
			JPot.payoutTableShift=(preScreenY2-curScreenY)<<1;
			if(JPot.payoutTableShift > 128)
				JPot.payoutTableShift = 128;
		}
		continueTouch=FT_FALSE;
	}
	preScreenY1=curScreenY;
}

//decrease the spinning column velocity afer the spin button has been released
ft_void_t updateIndex(){

	static ft_uint8_t i;
	ft_uint16_t pressure;

	if(JPot.spinning){
		//button has been released
		if(JPot.released){
			for(i=0; i<SPIN_COLUMNS;i++){
				if(columns[i].velocity>0){
					columns[i].velocity = (columns[i].velocity-1) <= 0 ? 0 : (columns[i].velocity-1);
					columns[i].curIndex=(columns[i].curIndex+i+1)%NUM_OF_ICONS;
				}
			}
		}
		//button is being pressed
		else if(JPot.touchTag==201){
			pressure=Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RZ);
			for(i=0; i<SPIN_COLUMNS;i++){
				columns[i].velocity = ((NORMAL_PRESSURE-pressure)*MAX_ADDITIONAL_VEL)/NORMAL_PRESSURE;
				//if the the touch was barely sensed
				if(columns[i].velocity<10){
					columns[i].velocity =(BASE_VELOCITY +(nextRandomInt8() & 31));
				}
				columns[i].curIndex=(columns[i].curIndex+1+(nextRandomInt8() & 3))%NUM_OF_ICONS;
			}
		}
		else{
			pressure=Ft_Gpu_Hal_Rd16(phost,REG_TOUCH_RZ);
			for(i=0; i<SPIN_COLUMNS;i++){
				columns[i].velocity = ((NORMAL_PRESSURE-pressure)*MAX_ADDITIONAL_VEL)/NORMAL_PRESSURE;
				//if the the touch was barely sensed
				if(columns[i].velocity<10){
					columns[i].velocity =(BASE_VELOCITY +(nextRandomInt8() & 31));
				}
				columns[i].curIndex=(columns[i].curIndex+1+(nextRandomInt8() & 3))%NUM_OF_ICONS;
			}
			JPot.released=TRUE;
		}
	}
}


ft_bool_t ableToSpin(){
	if(JPot.balance>=JPot.totalBetAmount && !JPot.spinning){
		return TRUE;
	}
	else{
		return FT_FALSE;
	}
}

ft_bool_t ableToContinue(){
	if(JPot.balance>=POINTS_PER_COLUMN){
		return TRUE;
	}
	else{
		return FT_FALSE;
	}
}

ft_void_t unsignedNumToAsciiArray(ft_char8_t *buf, ft_uint16_t num){

	ft_uint8_t temp, i, index=0,last;
	do{
		buf[index]=(num%10)+48;
		index++;
		num/=10;
	}while(num!=0);
	buf[index]='\0';

	last=index-1;
	for (i = 0; i < index/2; i++) {
		temp= buf[i];
		buf[i]=buf[last];
		buf[last] = temp;
		last--;
	}

}



ft_void_t touched(ft_uint8_t touchVal){
	static ft_uint8_t i,j;
	ft_uint8_t tempVal=0, betMax=10, lineBetMax=10;
	JPot.touchTag=touchVal;

	if(touchVal!=0 && touchVal!=200 && touchVal != 201){
		playSound(BUTTON_PRESS_SOUND);
	}

	//bet line selection
	if(touchVal>0 && touchVal<=12){
		if(!JPot.spinning){
		JPot.selectedBetLine=touchVal;
		JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
		JPot.displayRewardLine=FT_FALSE;
		}
		return;
	}

	//multiplier selection
	if(touchVal>=100 && touchVal<100+SPIN_COLUMNS){
		JPot.selectedBetMultiplier=touchVal-100;
		return;
	}


	//status bar buttons
	switch(touchVal){
	case 201: //spin button
		if(!JPot.pendown && ableToSpin() && !JPot.showPayoutTable){
			JPot.spinning=TRUE;
			JPot.released=FT_FALSE;
			JPot.rewardedPoints=FT_FALSE;
			JPot.balance-=JPot.totalBetAmount;
			if(ableToContinue()){
				JPot.reset=FT_FALSE;
			}
			for(i=0; i<SPIN_COLUMNS;i++){
				//columns[i].velocity = 64+(nextRandomInt8() & 7);
				columns[i].velocity = BASE_VELOCITY;
				columns[i].bounceAmount=columns[i].bounceOffset=BOUNCE_AMOUNT;
			}
		}
		break;
	case 202:  //column betting increase button
		if(!JPot.spinning && !JPot.showPayoutTable){
			if(JPot.spinColumnSelected+1<=SPIN_COLUMNS && JPot.balance>=POINTS_PER_COLUMN){
				JPot.spinColumnSelected++;
				JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
				if(ableToContinue()){
					JPot.reset=FT_FALSE;
				}
			}
		}
		break;
	case 203:  //column betting decrease button
		if(!JPot.spinning  && !JPot.showPayoutTable){
			if(JPot.spinColumnSelected-1>=1){
				JPot.spinColumnSelected--;
				JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
				if(ableToContinue()){
					JPot.reset=FT_FALSE;
				}
			}

		}
		break;
	case 204:  //line betting increase button
		if(!JPot.spinning  && !JPot.showPayoutTable){
			JPot.selectedBetLine = (JPot.selectedBetLine+1) > TOTAL_BET_LINES ? TOTAL_BET_LINES : (JPot.selectedBetLine+1);
			JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
			if(ableToContinue()){
					JPot.reset=FT_FALSE;
				}
			JPot.betChanged=TRUE;
		}

		break;
	case 205:  //line betting decrease
		if(!JPot.spinning  && !JPot.showPayoutTable){
			JPot.selectedBetLine = (JPot.selectedBetLine-1) < 0 ? 0 : (JPot.selectedBetLine-1);
			JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
			if(ableToContinue()){
				JPot.reset=FT_FALSE;
			}
			JPot.betChanged=TRUE;
		}
		break;
	case 206:  //reset game button
		if(!JPot.spinning  && !JPot.showPayoutTable){
		if(JPot.reset){
			JPot.balance=INITIAL_BALANCE;
			JPot.selectedBetLine=1;
			if(ableToContinue()){
				JPot.reset=FT_FALSE;
			}
		}
		else{
			JPot.selectedBetLine=1;
			JPot.spinColumnSelected=1;
			JPot.totalBetAmount=POINTS_PER_COLUMN;
			JPot.reset=TRUE;
		}
		}
		break;
	case 207: //payout table button
		if(ableToContinue()){
			JPot.reset=FT_FALSE;
		}
		if(JPot.showPayoutTable){
			JPot.showPayoutTable=FT_FALSE;
		}
		else{
			JPot.showPayoutTable=TRUE;
		}
		JPot.betChanged=TRUE;
		break;

	}

}

//xor shift random number generation
ft_uint8_t nextRandomInt8() {
	static ft_uint8_t x = 111;
	x ^= (x << 7);
	x ^= (x >> 5);
	return x ^= (x << 3);
}


ft_void_t clearArray(ft_uint8_t* index, ft_uint16_t size, ft_uint8_t defaultValue){
	ft_uint16_t i;
	for(i=0; i<size; i++){
		index[i]=defaultValue;
	}
}



ft_uint16_t getPoints(){
	ft_uint8_t i,j, lookingFor=5, fruitCount=0, fruitTableSize=7,tempMin, tempVal=0;
	ft_uint8_t fruitTable[] = {3,5,6,9,10,11,12};
	ft_uint8_t tempIndex[NUM_OF_ICONS];
	ft_uint16_t points=0;
	ft_bool_t finished=FT_FALSE;

	JPot.rewardedPoints=TRUE;
	clearArray(JPot.winningIndex,SPIN_COLUMNS,255);
	clearArray(tempIndex, NUM_OF_ICONS, 0);
	//count the winning indices
	for(i=0; i<JPot.spinColumnSelected; i++){
		//tempIndex[columns[i].drawnIndex]+=1;
		tempIndex[columns[i].iconArray[columns[i].drawnIndex]]+=1;
	}

	for(i=0; i<NUM_OF_ICONS;){
		//check for combination with the highest payout value first.
		if(tempIndex[i]>=lookingFor){
			for(j=0; j<JPot.spinColumnSelected; j++){
				//if(columns[j].drawnIndex==i){
				if(columns[j].iconArray[columns[j].drawnIndex]==i){
					JPot.winningIndex[j]=tempIndex[i];
				}
			}
			if(lookingFor>2){
				return payoutTable[i] * lookingFor;
			}
			else if(lookingFor==2){
				points+=payoutTable[i] * 2;
			}
		}
		i++;
		if(i==NUM_OF_ICONS){
			lookingFor--;
			i=0;
			if(lookingFor<2){
				if(points>0){
					return points; 
				}
				break;
			}
		}
	}


	//check for any fruits
	for(i=0; i<fruitTableSize; i++){
		for(j=0; j<JPot.spinColumnSelected; j++){
			if(fruitTable[i]==columns[j].iconArray[columns[j].drawnIndex]){
				JPot.winningIndex[j]=columns[j].iconArray[columns[j].drawnIndex];
				fruitCount++;
				tempVal++;
			}
		}
	}

	return fruitCount*payoutTable[14];
}






ft_void_t startingAnimation(){
	ft_uint8_t i;
	JPot.spinning=TRUE;
	JPot.released=TRUE;
	Ft_Gpu_Hal_Sleep(1000);

	for(i=0;i<SPIN_COLUMNS;i++){
		columns[i].bounceAmount=columns[i].bounceOffset=BOUNCE_AMOUNT;
	}

	while(1){
		Ft_Gpu_CoCmd_Dlstart(phost); 
		Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		updateIndex();
		drawSpinColumns();
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));

#if defined(DISPLAY_RESOLUTION_WQVGA)
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1, UI.spinColumnYOffset+UI.visibleIconsHeight+30, 31, OPT_CENTER, "JACKPOT by FTDI");
#elif defined(DISPLAY_RESOLUTION_QVGA)
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1, UI.spinColumnYOffset+UI.visibleIconsHeight+30, 30, OPT_CENTER, "JACKPOT by FTDI");
#elif defined(DISPLAY_RESOLUTION_WVGA)
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1, UI.spinColumnYOffset+UI.visibleIconsHeight+60, 31, OPT_CENTER, "JACKPOT by FTDI");
#endif

		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
		if(JPot.stoppedColumns == SPIN_COLUMNS){
			break;
		}
	}

	JPot.spinning=FT_FALSE;
	JPot.released=FT_FALSE;
	JPot.spinColumnSelected=SPIN_COLUMNS;

	for(i=0;i<8;i++){
		spinCoins[i].fall=FT_FALSE;
		spinCoins[i].xPos=FT_DispWidth>>1;
		spinCoins[i].yPos=FT_DispHeight/3;
		spinCoins[i].yVel=-2+ (nextRandomInt8() & 3)*-1;
		nextRandomInt8();
		if((nextRandomInt8()>>7) & 1){
			spinCoins[i].xVel=-4+ (nextRandomInt8() & 3)*-1;
		}
		else{
			spinCoins[i].xVel=4 + (nextRandomInt8() & 3);
		}
		spinCoins[i].index=(nextRandomInt8() & 7);
	}

	for(i=0;i<8;i++){
		playSound(COIN_REWARD_SOUND);
	}
}


#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
ft_uint8_t home_star_icon[] = {0x78,0x9C,0xE5,0x94,0xBF,0x4E,0xC2,0x40,0x1C,0xC7,0x7F,0x2D,0x04,0x8B,0x20,0x45,0x76,0x14,0x67,0xA3,0xF1,0x0D,0x64,0x75,0xD2,0xD5,0x09,0x27,0x17,0x13,0xE1,0x0D,0xE4,0x0D,0x78,0x04,0x98,0x5D,0x30,0x26,0x0E,0x4A,0xA2,0x3E,0x82,0x0E,0x8E,0x82,0xC1,0x38,0x62,0x51,0x0C,0x0A,0x42,0x7F,0xDE,0xB5,0x77,0xB4,0x77,0x17,0x28,0x21,0x26,0x46,0xFD,0x26,0xCD,0xE5,0xD3,0x7C,0xFB,0xBB,0xFB,0xFD,0xB9,0x02,0xCC,0xA4,0xE8,0x99,0x80,0x61,0xC4,0x8A,0x9F,0xCB,0x6F,0x31,0x3B,0xE3,0x61,0x7A,0x98,0x84,0x7C,0x37,0xF6,0xFC,0xC8,0xDD,0x45,0x00,0xDD,0xBA,0xC4,0x77,0xE6,0xEE,0x40,0xEC,0x0E,0xE6,0x91,0xF1,0xD2,0x00,0x42,0x34,0x5E,0xCE,0xE5,0x08,0x16,0xA0,0x84,0x68,0x67,0xB4,0x86,0xC3,0xD5,0x26,0x2C,0x20,0x51,0x17,0xA2,0xB8,0x03,0xB0,0xFE,0x49,0xDD,0x54,0x15,0xD8,0xEE,0x73,0x37,0x95,0x9D,0xD4,0x1A,0xB7,0xA5,0x26,0xC4,0x91,0xA9,0x0B,0x06,0xEE,0x72,0xB7,0xFB,0xC5,0x16,0x80,0xE9,0xF1,0x07,0x8D,0x3F,0x15,0x5F,0x1C,0x0B,0xFC,0x0A,0x90,0xF0,0xF3,0x09,0xA9,0x90,0xC4,0xC6,0x37,0xB0,0x93,0xBF,0xE1,0x71,0xDB,0xA9,0xD7,0x41,0xAD,0x46,0xEA,0x19,0xA9,0xD5,0xCE,0x93,0xB3,0x35,0x73,0x0A,0x69,0x59,0x91,0xC3,0x0F,0x22,0x1B,0x1D,0x91,0x13,0x3D,0x91,0x73,0x43,0xF1,0x6C,0x55,0xDA,0x3A,0x4F,0xBA,0x25,0xCE,0x4F,0x04,0xF1,0xC5,0xCF,0x71,0xDA,0x3C,0xD7,0xB9,0xB2,0x48,0xB4,0x89,0x38,0x20,0x4B,0x2A,0x95,0x0C,0xD5,0xEF,0x5B,0xAD,0x96,0x45,0x8A,0x41,0x96,0x7A,0x1F,0x60,0x0D,0x7D,0x22,0x75,0x82,0x2B,0x0F,0xFB,0xCE,0x51,0x3D,0x2E,0x3A,0x21,0xF3,0x1C,0xD9,0x38,0x86,0x2C,0xC6,0x05,0xB6,0x7B,0x9A,0x8F,0x0F,0x97,0x1B,0x72,0x6F,0x1C,0xEB,0xAE,0xFF,0xDA,0x97,0x0D,0xBA,0x43,0x32,0xCA,0x66,0x34,0x3D,0x54,0xCB,0x24,0x9B,0x43,0xF2,0x70,0x3E,0x42,0xBB,0xA0,0x95,0x11,0x37,0x46,0xE1,0x4F,0x49,0xC5,0x1B,0xFC,0x3C,0x3A,0x3E,0xD1,0x65,0x0E,0x6F,0x58,0xF8,0x9E,0x5B,0xDB,0x55,0xB6,0x41,0x34,0xCB,0xBE,0xDB,0x87,0x5F,0xA9,0xD1,0x85,0x6B,0xB3,0x17,0x9C,0x61,0x0C,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xED,0xC9,0xFC,0xDF,0x14,0x54,0x8F,0x80,0x7A,0x06,0xF5,0x23,0xA0,0x9F,0x41,0xF3,0x10,0x30,0x4F,0x41,0xF3,0x18,0x30,0xCF,0xCA,0xFC,0xFF,0x35,0xC9,0x79,0xC9,0x89,0xFA,0x33,0xD7,0x1D,0xF6,0x5E,0x84,0x5C,0x56,0x6E,0xA7,0xDA,0x1E,0xF9,0xFA,0xAB,0xF5,0x97,0xFF,0x2F,0xED,0x89,0x7E,0x29,0x9E,0xB4,0x9F,0x74,0x1E,0x69,0xDA,0xA4,0x9F,0x81,0x94,0xEF,0x4F,0xF6,0xF9,0x0B,0xF4,0x65,0x51,0x08};
#endif

ft_void_t infoScreen()
{
	const ft_char8_t* info[4];
	ft_uint16_t dloffset;
	info[0]="Jackpot Game";
	info[1]="to demonstrate: FT800 primitives";
	info[2]="& touch screen.";

#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
	Ft_Gpu_Hal_WrCmd32(phost,250*1024L);
	Ft_Gpu_Hal_WrCmdBuf(phost,home_star_icon,sizeof(home_star_icon));
	Ft_Gpu_CoCmd_Dlstart(phost);        // start
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(14));    // handle for background stars
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(250*1024L));      // Starting address in gram
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 16, 32));  // format 
	Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 32, 32  ));
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
#endif
	Ft_CmdBuffer_Index = 0;

	Ft_Gpu_CoCmd_Dlstart(phost); 
	Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,29,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
	Ft_Gpu_CoCmd_Calibrate(phost,0);
	Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
	Ft_Gpu_CoCmd_Swap(phost);
	Ft_App_Flush_Co_Buffer(phost);
	Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

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
	do
	{
		Ft_Gpu_CoCmd_Dlstart(phost);   
		Ft_Gpu_CoCmd_Append(phost,100000L,dloffset);
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
		// INFORMATION 
#ifdef DISPLAY_RESOLUTION_WVGA
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,40,30,OPT_CENTERX|OPT_CENTERY,info[0]);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,80,29,OPT_CENTERX|OPT_CENTERY,info[1]);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,120,29,OPT_CENTERX|OPT_CENTERY,info[2]);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,FT_DispHeight-30,29,OPT_CENTERX|OPT_CENTERY,"Click to play");

#else
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,20,28,OPT_CENTERX|OPT_CENTERY,info[0]);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,60,26,OPT_CENTERX|OPT_CENTERY,info[1]);
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,90,26,OPT_CENTERX|OPT_CENTERY,info[2]);  
		Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,FT_DispHeight-30,27,OPT_CENTERX|OPT_CENTERY,"Click to play");
#endif
		if(sk!='P'){
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		}
		else{
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));
			playSound(BUTTON_PRESS_SOUND);
		}
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));   
		Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(20*16));
		Ft_App_WrCoCmd_Buffer(phost,TAG('P'));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth/2)*16,(FT_DispHeight-60)*16));
		Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(180,35,35));
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-14,(FT_DispHeight-75),14,4));
#endif

#if defined(ARDUINO_PLATFORM) || defined(FT900_PLATFORM)
		Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
		Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(16*2));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-4,(FT_DispHeight-70),0,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)+10,(FT_DispHeight-60),0,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-4,(FT_DispHeight-50),0,0));
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-4,(FT_DispHeight-70),0,0));
#endif
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
	}while(Read_Keys()!='P');
}

//display fading out reward amount
ft_void_t displayRewardText(ft_uint16_t points){
	ft_uint8_t limit=10;
	static ft_uint8_t counter=0;
	static ft_uint16_t lastPoints=0;
	static ft_bool_t pointSet=FT_FALSE;

	if(points!=lastPoints && !pointSet){
		counter=255;
		lastPoints=points;
		pointSet=TRUE;
	}
	else if(points!=lastPoints && pointSet){
		counter=255;
		lastPoints=points;
		pointSet=TRUE;
	}

	if(counter>limit){
		if(!JPot.spinning && points>0){
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(counter));
			Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,ICON_HEIGHT*2 + (ICON_HEIGHT>>1) , 31, OPT_RIGHTX, "+ ");
			Ft_Gpu_CoCmd_Number(phost, FT_DispWidth>>1,ICON_HEIGHT*2 + (ICON_HEIGHT>>1) , 31, 0, points);
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		}
		else{
			lastPoints=0;
		}
		counter-=5;
		if(counter<=limit){
			lastPoints=points;
			pointSet=FT_FALSE;
		}
	}
}

//selected a different bet line button will display the corresponding indexed bitmap/text for a brief moment
ft_void_t displaySelectedIcon(ft_uint8_t index){
	ft_uint8_t limit=50;
	static ft_uint8_t counter=0, lastIndex=0;
	static ft_bool_t counterSet=FT_FALSE;

	if(index!=lastIndex && !counterSet){
		counter=255;
		lastIndex=index;
		counterSet=TRUE;
	}
	else if(index!=lastIndex && counterSet){
		counter=255;
		lastIndex=index;
		counterSet=TRUE;
	}

	if(counter>limit){
		if(!JPot.spinning){
			Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(counter));
			if(index!=14){
				Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(FT_DispWidth>>1,FT_DispHeight>>1,index,0));
			}
			else{
				Ft_Gpu_CoCmd_Text(phost,FT_DispWidth>>1,FT_DispHeight>>1,25, 0, ANY_FRUIT_TEXT);
			}
			Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
		}
		counter-=10;
		if(counter<=limit){
			lastIndex=index;
			counterSet=FT_FALSE;
		}
	}
}



ft_void_t lineBetButtons(){


#if defined(DISPLAY_RESOLUTION_WQVGA)
	ft_uint8_t i, buttonWidth=40, buttonHeight=18, font=22, tempIndex=0;
	ft_int16_t lineButtonStartingX = UI.spinColumnXOffset-50;
	ft_int16_t lineButtonStartingY = UI.spinColumnYOffset;
#elif defined(DISPLAY_RESOLUTION_QVGA)
	ft_uint8_t i, buttonWidth=35, buttonHeight=15, font=22, tempIndex=0;
	ft_int16_t lineButtonStartingX = 0;
	ft_int16_t lineButtonStartingY = UI.spinColumnYOffset+7;
#elif defined(DISPLAY_RESOLUTION_WVGA)  //needs to change for the larger screen size
	ft_uint8_t i, buttonWidth=65, buttonHeight=36, font=28, tempIndex=0;
	ft_int16_t lineButtonStartingX = UI.spinColumnXOffset-80;
	ft_int16_t lineButtonStartingY = UI.spinColumnYOffset;
#endif

	ft_char8_t name[4];
	ft_char8_t points[5];
	ft_char8_t value[10];

	ft_uint32_t normalButtonColor =0x8B8682;
	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	Ft_Gpu_CoCmd_FgColor(phost,0xB8860B);
	//line button
	for(i=0; i<TOTAL_BET_LINES;i++){
		name[0]='\0';
		Ft_Gpu_Hal_Dec2Ascii(name,i+1);
		Ft_App_WrCoCmd_Buffer(phost,TAG(i+1));
		if(temp_tag == (i+1)){
			Ft_Gpu_CoCmd_Button(phost,lineButtonStartingX, lineButtonStartingY, buttonWidth ,buttonHeight , font,OPT_FLAT, name);
		}
		else if(JPot.selectedBetLine == (i+1)){  //current line is selected
			Ft_Gpu_CoCmd_FgColor(phost,0x0000CD);
			Ft_Gpu_CoCmd_Button(phost,lineButtonStartingX, lineButtonStartingY, buttonWidth ,buttonHeight , font,OPT_FLAT, name);
			Ft_Gpu_CoCmd_FgColor(phost,0xB8860B);
		}
		else{
			Ft_Gpu_CoCmd_Button(phost,lineButtonStartingX, lineButtonStartingY, buttonWidth ,buttonHeight , font, 0, name);
		}


		lineButtonStartingY+=(buttonHeight+12);
		if(i==5){
#if defined(DISPLAY_RESOLUTION_WVGA)
			lineButtonStartingY=UI.spinColumnYOffset;
			lineButtonStartingX=UI.spinColumnXOffset + (SPIN_COLUMNS*ICON_WIDTH + SPIN_COLUMNS * COLUMN_GAP + 5);
#elif defined(DISPLAY_RESOLUTION_WQVGA)
			lineButtonStartingY=UI.spinColumnYOffset;
			lineButtonStartingX=UI.spinColumnXOffset + (SPIN_COLUMNS*ICON_WIDTH + SPIN_COLUMNS * COLUMN_GAP);
#elif defined(DISPLAY_RESOLUTION_QVGA)
			lineButtonStartingY=UI.spinColumnYOffset;
			lineButtonStartingX=UI.spinColumnXOffset + (SPIN_COLUMNS*ICON_WIDTH + SPIN_COLUMNS * COLUMN_GAP);
#endif 
		}
	}
}




ft_void_t statusBar(){
	static ft_bool_t firstTime=1;
	static ft_uint8_t betTextPixelLength,columnTextPixelLength, lineTextPixelLength,balanceTextPixelLength,resetTextPixelLength,payoutTextPixelLength;
	static ft_uint8_t count=0, payoutTableButtonHeight;
	static ft_uint8_t font=18, betLineFont=27;


	if(firstTime){
		balanceTextPixelLength = (ft_uint8_t)stringPixelWidth(BALANCE_TEXT, betLineFont);
		resetTextPixelLength = (ft_uint8_t)stringPixelWidth(RESET_TEXT, 22);
		payoutTextPixelLength = (ft_uint8_t)stringPixelWidth(PAYOUT_TEXT, 22);
#if defined(DISPLAY_RESOLUTION_QVGA)
		betLineFont=22;
		payoutTableButtonHeight=17;
#elif defined(DISPLAY_RESOLUTION_WQVGA)
		betLineFont=27;
		payoutTableButtonHeight=17;
#elif defined(DISPLAY_RESOLUTION_WVGA)
		betLineFont=30;
		payoutTableButtonHeight=38;
#endif
		betTextPixelLength = (ft_uint8_t)stringPixelWidth(BET_TEXT, betLineFont);
		lineTextPixelLength = (ft_uint8_t)stringPixelWidth(LINE_TEXT, betLineFont);
		columnTextPixelLength = (ft_uint8_t)stringPixelWidth(COLUMN_TEXT, betLineFont);
		firstTime=0;
	}

	Ft_App_WrCoCmd_Buffer(phost,TAG(0));

	Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
	Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ZERO));
	Ft_App_WrCoCmd_Buffer(phost, VERTEX2II(0,FT_DispHeight-STATUS_BAR_HEIGHT,14,0));
	Ft_App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));

	//betting column amount
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth>>1)+BUTTON_GAP, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, COLUMN_TEXT);
	Ft_Gpu_CoCmd_Number(phost, (FT_DispWidth>>1)+BUTTON_GAP + columnTextPixelLength , FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, JPot.spinColumnSelected);

	//total bet amount
#if defined(DISPLAY_RESOLUTION_QVGA)
	Ft_Gpu_CoCmd_Text(phost,resetTextPixelLength + BUTTON_GAP, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, BET_TEXT);
	Ft_Gpu_CoCmd_Number(phost, resetTextPixelLength + BUTTON_GAP*2 + betTextPixelLength , FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, JPot.totalBetAmount);
#elif defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_WVGA)
	Ft_Gpu_CoCmd_Text(phost,resetTextPixelLength + BUTTON_GAP +10, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, BET_TEXT);
	Ft_Gpu_CoCmd_Number(phost, resetTextPixelLength + BUTTON_GAP*2 + betTextPixelLength +8, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, JPot.totalBetAmount);
#endif

	//selected bet line and its bet amount
	Ft_Gpu_CoCmd_Text(phost,(FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1)*2 -2, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, LINE_TEXT);
	Ft_Gpu_CoCmd_Number(phost, (FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1)*2 -2 + lineTextPixelLength, FT_DispHeight-((STATUS_BAR_HEIGHT*3)>>2) - BUTTON_GAP*2, betLineFont, 0, JPot.selectedBetLine);

	//balance text and value
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(BUTTON_CURVATURE*16));

#if defined(DISPLAY_RESOLUTION_WVGA)
	Ft_Gpu_CoCmd_Text(phost,BUTTON_CURVATURE*2, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), betLineFont, 0, BALANCE_TEXT);
	Ft_Gpu_CoCmd_Number(phost, BUTTON_CURVATURE*10 + betTextPixelLength + 35, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), betLineFont, 0, JPot.balance);
#else
	Ft_Gpu_CoCmd_Text(phost,BUTTON_CURVATURE*2, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), betLineFont, 0, BALANCE_TEXT);
	Ft_Gpu_CoCmd_Number(phost, BUTTON_CURVATURE*10 + betTextPixelLength + 35, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), betLineFont, OPT_RIGHTX, JPot.balance);
#endif

	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));

	//payout table tracking box
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,0));
	Ft_App_WrCoCmd_Buffer(phost,LINE_WIDTH(16));
	Ft_App_WrCoCmd_Buffer(phost,BEGIN(RECTS));	
	Ft_App_WrCoCmd_Buffer(phost,TAG(200));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((UI.spinColumnXOffset)*16,0));
	Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16, (FT_DispHeight-STATUS_BAR_HEIGHT-BUTTON_GAP*2)*16));
	Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));



	//spin button
	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));
	Ft_Gpu_CoCmd_FgColor(phost,0xB8860B);
	Ft_App_WrCoCmd_Buffer(phost,TAG(201));
#if defined(DISPLAY_RESOLUTION_WQVGA)
	if(temp_tag==201 && !JPot.pendown)
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 30, OPT_FLAT, "Spin");
	else
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 30, 0, "Spin");
#elif defined(DISPLAY_RESOLUTION_QVGA)
	if(temp_tag==201 && !JPot.pendown)
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 29,OPT_FLAT, "Spin");
	else
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 29, 0, "Spin");
#elif defined(DISPLAY_RESOLUTION_WVGA)
	if(temp_tag==201 && !JPot.pendown)
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 30, OPT_FLAT, "Spin");
	else
		Ft_Gpu_CoCmd_Button(phost,FT_DispWidth-UI.spinButtonWidth-BUTTON_GAP, FT_DispHeight-STATUS_BAR_HEIGHT + BUTTON_GAP, UI.spinButtonWidth ,STATUS_BAR_HEIGHT-BUTTON_GAP*2 , 30, 0, "Spin");
#endif

	Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0));


	//column selection button increase
	Ft_App_WrCoCmd_Buffer(phost,TAG(202));
	if(temp_tag==202)
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)+BUTTON_GAP, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30, OPT_FLAT, "+");
	else
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)+BUTTON_GAP, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30,0, "+");

	//column selection button decrease
	Ft_App_WrCoCmd_Buffer(phost,TAG(203));
	if(temp_tag==203)
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)+BUTTON_GAP+2 + (UI.spinButtonWidth>>1), FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1 , 30,OPT_FLAT, "-");
	else
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)+BUTTON_GAP+2 + (UI.spinButtonWidth>>1), FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1 , 30, 0, "-");

	//coin selection button decrease
	Ft_App_WrCoCmd_Buffer(phost,TAG(205));
	if(temp_tag==205)
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1), FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30, OPT_FLAT, "-");
	else
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1), FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30, 0, "-");

	//coin selection button increase
	Ft_App_WrCoCmd_Buffer(phost,TAG(204));
	if(temp_tag==204)
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1)*2 -2, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30, OPT_FLAT, "+");
	else
		Ft_Gpu_CoCmd_Button(phost,(FT_DispWidth>>1)-BUTTON_GAP-(UI.spinButtonWidth>>1)*2 -2, FT_DispHeight-(STATUS_BAR_HEIGHT>>1), UI.spinButtonWidth>>1 ,STATUS_BAR_HEIGHT>>1, 30, 0, "+");

	//reset or new game button
	Ft_App_WrCoCmd_Buffer(phost,TAG(206));
	if(JPot.reset && !JPot.spinning){
		if(temp_tag==206){
			Ft_Gpu_CoCmd_Button(phost,count, FT_DispHeight-STATUS_BAR_HEIGHT+BUTTON_GAP, resetTextPixelLength+5,(STATUS_BAR_HEIGHT>>1)-BUTTON_GAP*2, 21, OPT_FLAT, "N.G.");
		}
		else{
			Ft_Gpu_CoCmd_Button(phost,count, FT_DispHeight-STATUS_BAR_HEIGHT+BUTTON_GAP, resetTextPixelLength+5,(STATUS_BAR_HEIGHT>>1)-BUTTON_GAP*2, 21, 0, "N.G.");
		}
	}
	else{
		if(temp_tag==206){
			Ft_Gpu_CoCmd_Button(phost,count, FT_DispHeight-STATUS_BAR_HEIGHT+BUTTON_GAP, resetTextPixelLength+5,(STATUS_BAR_HEIGHT>>1)-BUTTON_GAP*2, 22, OPT_FLAT, RESET_TEXT);
		}
		else{
			Ft_Gpu_CoCmd_Button(phost,count, FT_DispHeight-STATUS_BAR_HEIGHT+BUTTON_GAP, resetTextPixelLength+5,(STATUS_BAR_HEIGHT>>1)-BUTTON_GAP*2, 22, 0, RESET_TEXT);
		}
	}

	//payout table button
	Ft_App_WrCoCmd_Buffer(phost,TAG(207));
	if(JPot.showPayoutTable){
		if(temp_tag==207)
			Ft_Gpu_CoCmd_Button(phost,0, FT_DispHeight-STATUS_BAR_HEIGHT-payoutTableButtonHeight, payoutTextPixelLength+5,payoutTableButtonHeight-BUTTON_CURVATURE, 22, OPT_FLAT, "exit");
		else
			Ft_Gpu_CoCmd_Button(phost,0, FT_DispHeight-STATUS_BAR_HEIGHT-payoutTableButtonHeight, payoutTextPixelLength+5,payoutTableButtonHeight-BUTTON_CURVATURE, 22, 0, "exit");
	}
	else{
		if(temp_tag==207)
			Ft_Gpu_CoCmd_Button(phost,0, FT_DispHeight-STATUS_BAR_HEIGHT-payoutTableButtonHeight, payoutTextPixelLength+5,/*(STATUS_BAR_HEIGHT>>1)-BUTTON_GAP*2*/payoutTableButtonHeight-BUTTON_CURVATURE, 22,OPT_FLAT, PAYOUT_TEXT);
		else
			Ft_Gpu_CoCmd_Button(phost,0, FT_DispHeight-STATUS_BAR_HEIGHT-payoutTableButtonHeight, payoutTextPixelLength+5,payoutTableButtonHeight-BUTTON_CURVATURE, 22,0, PAYOUT_TEXT);
	}

	Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
}

ft_uint16_t fillLineIndexandPoints(){
	ft_uint8_t i,j,k,l,m,n,o,fruitCount=0,fruitTableSize=7,lookingFor=SPIN_COLUMNS,tempPoints, lastLookingFor;
	ft_uint8_t fruitTable[] = {3,5,6,9,10,11,12}, wonIndex[2];
	ft_bool_t finished=FT_FALSE,fruitFound=FT_FALSE,setVal;
	ft_uint8_t tempIndex[NUM_OF_ICONS];
	ft_uint16_t totalPoints=0;


		for(i=0;i<TOTAL_BET_LINES;i++){

		if(i>=JPot.selectedBetLine){
			for(n=0;n<SPIN_COLUMNS;n++){
				JPot.lineBet[i][n]=254;
			}
			continue;
		}

		finished=FT_FALSE;
		clearArray(tempIndex, NUM_OF_ICONS, 0);
		lookingFor=SPIN_COLUMNS;
		fruitCount=0;
		tempPoints=0;
		lastLookingFor=254;
		wonIndex[0]=254;
		wonIndex[1]=254;
		setVal=FT_FALSE;
		for(j=0;j<SPIN_COLUMNS;j++){
			if(linePosition[i].line[j] == 't'){
				JPot.lineBet[i][j] = (columns[j].drawnIndex-1) < 0 ? columns[j].iconArray[NUM_OF_ICONS-1] : columns[j].iconArray[columns[j].drawnIndex-1];
			}
			else if(linePosition[i].line[j] == 'm'){
				JPot.lineBet[i][j] = columns[j].iconArray[columns[j].drawnIndex];
			}
			else{
				JPot.lineBet[i][j] = (columns[j].drawnIndex+1) >= NUM_OF_ICONS ? columns[j].iconArray[0] : columns[j].iconArray[columns[j].drawnIndex+1];
			}
			tempIndex[JPot.lineBet[i][j]]++;
		}




		for(k=0; k<NUM_OF_ICONS;){
			//check for combination with the highest payout value first.
			if(tempIndex[k]>=lookingFor){
					if(setVal==FT_FALSE){
					for(o=0;o<NUM_OF_ICONS;o++){
						if(tempIndex[o]>=lookingFor){
							if(wonIndex[0]==254){  //for 5 columns
								wonIndex[0]=o;
							}
							else{
								wonIndex[1]=o;
							}
						}
					}
					setVal=FT_TRUE;
				}
				for(l=0; l<SPIN_COLUMNS; l++){
					if(JPot.lineBet[i][l] != wonIndex[0] && JPot.lineBet[i][l] != wonIndex[1]){
							JPot.lineBet[i][l]=254;
					}
				}
				if(lookingFor>2){
					totalPoints = payoutTable[k] * lookingFor;
					finished=TRUE;
				}
				else if(lookingFor==2){
					tempPoints+=payoutTable[k] * 2;
				}
			}
			if(finished){
				break;
			}
			k++;
			if(k==NUM_OF_ICONS){
				lookingFor--;
				k=0;
				if(lookingFor<2){
					break;
				}
			}
		}

		if(!finished && tempPoints>0){
			totalPoints+=tempPoints;
			finished=TRUE;
		}

		//check for any fruits
		if(!finished){
			for(m=0;m<SPIN_COLUMNS;m++){
				fruitFound=FT_FALSE;
				for(n=0;n<fruitTableSize;n++){
					if(JPot.lineBet[i][m] == fruitTable[n]){
						fruitFound=TRUE;
						fruitCount++;
					}
				}
				if(!fruitFound){
					JPot.lineBet[i][m]=254;
				}
			}
			totalPoints += fruitCount*payoutTable[14];
		}
	}
	return totalPoints;
}



ft_void_t jackpot(){
	ft_uint16_t earnedPoints=0;
	ft_uint8_t i, spinCoinAmount=8;
	#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
        lastMilli= time(NULL);
    #endif
	JPot.totalBetAmount = POINTS_PER_COLUMN * JPot.spinColumnSelected + JPot.selectedBetLine * POINTS_PER_COLUMN;
	while(1){


		Ft_App_WrCoCmd_Buffer(phost, CMD_DLSTART);
		Ft_App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0,0,0));
		Ft_App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));
		Ft_Gpu_Hal_Wr8(phost, REG_VOL_SOUND,50);
		touched(Read_Keys());

		Ft_App_WrCoCmd_Buffer(phost, COLOR_RGB(255,255,255));
		Ft_App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));


		//coin repeat background

		Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE((15)));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(bitmapInfo[1].Offset));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(RGB565,bitmapInfo[1].Width,bitmapInfo[1].Height));
		Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,REPEAT,REPEAT,FT_DispWidth,FT_DispHeight-STATUS_BAR_HEIGHT));
#if defined(DISPLAY_RESOLUTION_WVGA)
		Ft_App_WrCoCmd_Buffer(phost, BITMAP_SIZE_H(FT_DispWidth>>9,(FT_DispHeight-STATUS_BAR_HEIGHT)>>9));
#endif
		Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,0));



		if(JPot.showPayoutTable){
			drawPayoutTable();
		}
		else{
			drawSpinColumns();
			if(!JPot.spinning)
				drawbetlines();

			if(JPot.rewardedPoints){
				redrawColumnIcons();
			}
		}


		//reward points when all columns have stopped
		if(JPot.stoppedColumns==SPIN_COLUMNS){
			JPot.spinning=FT_FALSE;
			if(!JPot.rewardedPoints){
				earnedPoints=fillLineIndexandPoints();
				JPot.displayRewardLine=FT_TRUE;
#if defined(FT900_PLATFORM)
			timer_profilecountstart(&timercount,&timervalue);
			timer_profilecountend(&timercount,&timervalue);
			curMilli = timervalue;
#endif
				currentLine=0;
				earnedPoints+=getPoints();
				JPot.betChanged=FT_FALSE;
				if(earnedPoints>0){
					playSound(0x50);  //winning sound
					JPot.balance+=earnedPoints;
					spinCoinAmount=(earnedPoints>>4);
					if(spinCoinAmount>8){
						spinCoinAmount=8;
					}
					for(i=0;i<spinCoinAmount;i++){
						spinCoins[i].fall=FT_FALSE;
						spinCoins[i].xPos=FT_DispWidth>>1; 
						spinCoins[i].yPos=FT_DispHeight/3;
						spinCoins[i].yVel=-2+ (nextRandomInt8() & 3)*-1;
						nextRandomInt8();
						if(nextRandomInt8() & 1){
							spinCoins[i].xVel=-4+ (nextRandomInt8() & 3)*-1;
						}
						else{
							spinCoins[i].xVel=4 + (nextRandomInt8() & 3);
						}
						spinCoins[i].index=(nextRandomInt8() & 7);
					}

					for(i=0;i<spinCoinAmount;i++){
						playSound(COIN_REWARD_SOUND);
					}

				}

				//check the balance to see if the game can be continued. 
				if(!ableToContinue() && JPot.rewardedPoints){
					JPot.reset=TRUE;
				}
				else{
					JPot.reset=FT_FALSE;
				}
			}
		}

		statusBar();
		if(!JPot.showPayoutTable){
			lineBetButtons();
		}

		coinsAnimation(spinCoinAmount);


		displayRewardText(earnedPoints);

		//displaySelectedIcon(JPot.selectedBetLine);
		Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
		Ft_Gpu_CoCmd_Swap(phost);
		Ft_App_Flush_Co_Buffer(phost);
		Ft_Gpu_Hal_WaitCmdfifo_empty(phost);

		Ft_Gpu_Hal_Sleep(50);

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
        "Welcome to Gradient Example ... \r\n"
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
sys_enable(sys_device_timer_wdt);
timer_prescaler(10000);
timer_init(timer_select_a,              /* Device */
               5 ,        /* Initial Value */
               timer_direction_up,        /* Count Direction */
               timer_prescaler_select_on,   /* Prescaler Select */
			   timer_mode_continuous);
timer_start(timer_select_a);
sys_enable(sys_device_sd_card);
/*use for sd card*/
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
	    SDHOST_STATUS sd_status;
	  	if(sdhost_card_detect() == SDHOST_CARD_INSERTED)
	  	{
	  	printf("\n sd_status=%s",sdhost_card_detect());
	  	}

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
/* useful for timer */
ft_millis_init();
interrupt_enable_globally();
//printf("ft900 config done \n");
}
#endif

ft_void_t Ft_BootupConfig()
{
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

		/* Access address 0 to wake up the FT800 */
		Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);
		Ft_Gpu_Hal_Sleep(20);

		/* Set the clk to external clock */
#if (!defined(ME800A_HV35R) && !defined(ME810A_HV35R) && !defined(ME812AU_WH50R) && !defined(ME813AU_WH50C) && !defined(ME810AU_WH70R) && !defined(ME811AU_WH70C))
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



#if defined(MSVC_PLATFORM) || defined(FT900_PLATFORM)
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{
	ft_uint8_t Val;
	ft_uint8_t chipid;
#ifdef FT900_PLATFORM
	FT900_Config();
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
#ifdef FT900_PLATFORM
	printf("\n reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
	printf("\n reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
    printf("\n reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
	printf("\n reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));
#endif
    /*It is optional to clear the screen here*/
    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
    Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen. 
#ifdef ARDUINO_PLATFORM
	sd_present =  SD.begin(FT_SDCARD_CS); 
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0); 
#endif
	infoScreen();
	jackpotSetup();
	startingAnimation();
	jackpot();
	/* Close all the opened handles */
	Ft_Gpu_Hal_Close(phost);
	Ft_Gpu_Hal_DeInit();
#if defined(MSVC_PLATFORM) || defined(MSVC_FT800EMU)
	return 0;
#endif
}

void loop()
{
}



/* Nothing beyond this */















