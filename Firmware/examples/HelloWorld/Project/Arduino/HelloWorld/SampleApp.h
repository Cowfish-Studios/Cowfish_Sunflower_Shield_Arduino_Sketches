#ifndef _SAMPLEAPP_H_
#define _SAMPLEAPP_H_
#include "FT_Platform.h"

ft_void_t Ft_App_WrCoCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd);
ft_void_t Ft_App_WrDlCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd);
ft_void_t Ft_App_WrCoStr_Buffer(Ft_Gpu_Hal_Context_t *phost,const ft_char8_t *s);
ft_void_t Ft_App_Flush_DL_Buffer(Ft_Gpu_Hal_Context_t *phost);
ft_void_t Ft_App_Flush_Co_Buffer(Ft_Gpu_Hal_Context_t *phost);

#endif /* _SAMPLEAPP_H_ */

/* Nothing beyond this */
