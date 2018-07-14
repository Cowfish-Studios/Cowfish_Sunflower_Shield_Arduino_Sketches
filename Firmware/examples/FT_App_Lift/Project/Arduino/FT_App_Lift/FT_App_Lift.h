#ifndef _SAMPLEAPP_H_
#define _SAMPLEAPP_H_

/*The font data contains 22 chinese characters, which is generated by fnt_cvt utility from SimFang true type font file of Windows*/
extern FT_PROGMEM ft_prog_uchar8_t SAMApp_ChineseFont_MetricBlock[];
extern FT_PROGMEM ft_prog_uchar8_t  SAMApp_ChineseFont_FontBmpData[];


#define FT_APP_MATIRX_PRECITION (1)
#define FT_APP_MATRIX_GPU_PRECITION (256)
/* Macros for lift application */
#define LIFTAPP_DIR_NONE (0)
#define LIFTAPP_DIR_DOWN (1)
#define LIFTAPP_DIR_UP (2)

#define FT_APP_GRAPHICS_FLIP_RIGHT (1)
#define FT_APP_GRAPHICS_FLIP_BOTTOM (2)
/* Structure for lift demo implementation */
typedef struct S_LiftAppBallsLinear
{
	ft_int16_t xOffset;
	ft_int16_t yOffset;
	ft_char8_t dx;
	ft_char8_t dy;
}S_LiftAppBallsLinear_t;

/* Structure for lift application font */
#define LIFTAPPCHARRESIZEPRECISION (256)
#define LIFTAPPMAXCHARS (12)
#define LIFTAPPMAXFILENAMEARRAY (16)//macro used for file name array
#define LIFTAPPCHUNKSIZE (512L) //sdcard sector size
#define LIFTAPPAUDIOBUFFERTOTALSIZE (64*1024) //total buffer size of audio buffer allocated
/* Audio atate machine */
#define LIFTAPPAUDIOSTATE_NONE (0)
#define LIFTAPPAUDIOSTATE_INIT (1)
#define LIFTAPPAUDIOSTATE_DOWNLOAD (2)
#define LIFTAPPAUDIOSTATE_PLAYBACK (3)
#define LIFTAPPAUDIOSTATE_STOP (4)

typedef struct S_LiftAppFont
{
	ft_uint32_t MaxWidth;
	ft_uint32_t MaxHeight;
	ft_uint32_t Stride;
	ft_uint8_t Format;
	ft_uint8_t Width[LIFTAPPMAXCHARS];
}S_LiftAppFont_t;

/* bitmap transformation matrix - tbd - change the below to fixed point arthematic */
typedef struct Ft_App_GraphicsMatrix
{
	float a;
	float b;
	float c;
	float d;
	float e;
	float f;
}Ft_App_GraphicsMatrix_t;
/* Transform functionality */
typedef struct Ft_App_PostProcess_Transform
{
	/* all the below units are in terms of 256 units - signed 8.8 format */
	/* a and d are used for scaling, c and f are used for offest position */
	ft_int32_t Transforma;
	ft_int32_t Transformb;
	ft_int32_t Transformc;
	ft_int32_t Transformd;
	ft_int32_t Transforme;
	ft_int32_t Transformf;
	/* Spectial effects, mirror effect,  */
	ft_uint32_t SpecialEffect;
}Ft_App_PostProcess_Transform_t;


typedef struct S_LiftAppRate
{
	ft_uint16_t IttrCount;//total number of counts since starting
	ft_uint16_t CurrTime;//current time since starting
}S_LiftAppRate_t;

typedef struct S_LiftAppTrasParams
{
	ft_int16_t FloorNumChangeRate;//change of floor number in terms of units
	ft_int16_t ResizeRate;//resize rate which includes from org to resize and resize to orginal
	ft_int16_t ResizeDimMin;//dimension of min resize in terms of 8 bits precision
	ft_int16_t ResizeDimMax;//dimension of max resize in terms of 8 bits precision
	ft_int16_t FloorNumStagnantRate;//Floor being stagnant in terms of units
	ft_int16_t MaxFloorNum;//make sure 0 is not considered in this usecase
	ft_int16_t MinFloorNum;//make sure 0 is not considered in this usecase
}S_LiftAppTrasParams_t;


/* context of lift application */
typedef struct S_LiftAppCtxt
{
	S_LiftAppTrasParams_t SLATransPrms;
	S_LiftAppRate_t SLARate;
	ft_char8_t ArrowDir;//0 means no direction, 1 means going down, 2 means going up
	ft_char8_t CurrFloorNum;//not more less than -128 and more than 127
	ft_char8_t NextFloorNum;//Next floor number from this floor - need to handle same floor number also
	ft_int16_t CurrFloorNumChangeRate;
	ft_int16_t CurrFloorNumStagnantRate;
	ft_int16_t CurrFloorNumResizeRate;
	ft_int16_t CurrArrowResizeRate;
        /* Audio playback management */
	ft_int32_t AudioCurrAddr;
	ft_int32_t AudioCurrFileSz;
	//ft_int32_t AudioTotBuffSz;
	ft_char8_t AudioFileNames[LIFTAPPMAXFILENAMEARRAY];
  	ft_char8_t AudioFileIdx;
	ft_char8_t AudioState;
	ft_char8_t AudioPlayFlag;
        ft_char8_t opt_orientation;
}S_LiftAppCtxt_t;




ft_void_t SAMAPP_fadeout();
ft_void_t SAMAPP_fadein();
ft_int16_t SAMAPP_qsin(ft_uint16_t a);
ft_int16_t SAMAPP_qcos(ft_uint16_t a);
/* Sample app APIs for graphics primitives */



/* Sample app APIs for widgets */

ft_void_t SAMAPP_CoPro_Widget_Calibrate();
ft_void_t SAMAPP_CoPro_Loadimage();
ft_void_t SAMAPP_Aud_Music_Player_Streaming();
ft_void_t SAMAPP_Aud_Music_Player();
ft_void_t Loadimage_display();
ft_void_t FT_LiftApp_Portrait();
float linear(float p1,float p2,ft_uint16_t t,ft_uint16_t rate);
ft_void_t display_logo();
ft_void_t SAMAPP_Touch();
ft_void_t SAMAPP_Sound();
//ft_uint32_t play_music(ft_char8_t *pFileName,ft_uint32_t dstaddr,ft_uint8_t Option,ft_uint32_t Buffersz);
ft_uint32_t play_music(Reader &r /*ft_char8_t *pFileName*/,ft_uint32_t dstaddr,ft_uint8_t Option,ft_uint32_t Buffersz);
ft_void_t font_display(ft_int16_t BMoffsetx, ft_int16_t BMoffsety,ft_uint32_t string_length,ft_char8_t *string_display);

ft_void_t SAMAPP_PowerMode();
ft_void_t SAMAPP_BootupConfig();
ft_int16_t Ft_App_TrsMtrxTranslate(Ft_App_GraphicsMatrix_t *pMatrix,float x,float y,float *pxres,float *pyres);
ft_int16_t Ft_App_UpdateTrsMtrx(Ft_App_GraphicsMatrix_t *pMatrix,Ft_App_PostProcess_Transform_t *ptrnsmtrx);
ft_int16_t Ft_App_TrsMtrxLoadIdentity(Ft_App_GraphicsMatrix_t *pMatrix);
ft_int16_t Ft_App_TrsMtrxRotate(Ft_App_GraphicsMatrix_t *pMatrix,ft_int32_t th);

ft_int32_t Ft_App_LoadRawAndPlay(ft_char8_t *filename,ft_int32_t DstAddr,ft_uint8_t Option, ft_int32_t Buffersz);
ft_int32_t FT_LiftApp_FTransition(S_LiftAppCtxt_t *pLACtxt,ft_int32_t dstaddr/*, ft_uint8_t opt_landscape*/);
ft_int32_t play_audio(Reader &aud_reader);
ft_void_t Load_afile(ft_uint32_t add, Reader &r,ft_uint8_t noofsectors);
ft_void_t Playback(ft_uint8_t play,ft_uint8_t vol);
ft_void_t Ft_Play_Sound(ft_uint8_t sound,ft_uint8_t vol,ft_uint8_t midi);
ft_void_t Init_Audio();
//ft_int32_t FT_LiftApp_FTransition_portrait(S_LiftAppCtxt_t *pLACtxt,ft_int32_t dstaddr);
ft_int32_t Ft_App_AudioPlay(S_LiftAppCtxt_t *pCtxt,ft_int32_t DstAddr,ft_int32_t BuffChunkSz);
ft_int32_t test_sdcard();

#endif /* _SAMPLEAPP_H_ */

/* Nothing beyond this */
