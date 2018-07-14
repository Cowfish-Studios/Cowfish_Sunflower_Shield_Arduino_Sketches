#ifndef _SAMPLEAPP_H_
#define _SAMPLEAPP_H_




#define SAMAPP_Abs_MeterDial_Size146x146_rgb (19005 )

#define SAMAPP_Abs_BaseDial_Size146x146_L1 (1308)


#define SAMAPP_Relative_MeterDial_Size146x146_rgb (18064)

#if defined(FT900_PLATFORM)
extern ft_uint8_t SAMAPP_Abs_Meter_Dial_rgb[];

extern ft_uint8_t SAMAPP_Abs_Base_Dial_L1[];

extern ft_uint8_t SAMAPP_Relative_Meter_Dial_rgb[];

#else
extern FT_PROGMEM ft_prog_uchar8_t SAMAPP_Abs_Meter_Dial_rgb[];

extern FT_PROGMEM ft_prog_uchar8_t SAMAPP_Abs_Base_Dial_L1[];

extern FT_PROGMEM ft_prog_uchar8_t SAMAPP_Relative_Meter_Dial_rgb[];
#endif

ft_int16_t SAMAPP_qsin(ft_uint16_t a);
ft_int16_t SAMAPP_qcos(ft_uint16_t a);
ft_void_t SAMAPP_BootupConfig();


ft_void_t Abs_Meter_Dial();

ft_void_t Relative_Meter_Dial();



#endif /* _SAMPLEAPP_H_ */

/* Nothing beyond this */








