; Copyright (c) Future Technology Devices International 2014



; THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED ``AS IS'' AND ANY EXPRESS
; OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
; FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED
; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
; THE POSSIBILITY OF SUCH DAMAGE.



Objective: 
==========
This ReadMe file contains the details of Refrigerator application release package. 

This package contains the FT800 Refrigerator application targeting Windows PC and Arduino platform.  

This application demonstrates the Refrigerator Demo drawn by using the bitmaps for background,menus and audio in sync with the menus and settings for the refrigerator based on FT800 platform.

Release package contents:
=========================
The folder structure is shown as below.


+---Bin
|   \---Msvc_win32
|          \--libMPSSE.a - MPSSE library file.
|   \---MSVC_Emulator
|          \--ft800emu.lib - MPSSE library file.               
|	      \--ft800emud.lib - MPSSE library file.               
+---Hdr
|   \---Arduino
|		\---\FT_DataTypes.h - Includes the FT800 datatypes 	
|						according to the platform.
|   		\---\FT_Platform.h - Includes Platform specific 
|						macros.
|   		\---\FT_CoPro_Cmds.h  - Includes the Coprocessor 	
|							commands.
|   		\---\FT_Gpu.h - Includes the GPU commands.
|   		\---\FT_Gpu_Hal.h - Includes the GPU HAL commands.
|   		\---\FT_Hal_Utils.h - Includes the HAL utilities.
|		\---\FT_Hal_I2C.h - Includes the HAL I2C utilities.
|		\---\sdcard.h - Includes the sdcard utilities.
|
|   \---Msvc_win32
|   		\---\ftd2xx.h - ftd2xx header file
|   		\---\libMPSSE_spi - MPSSE header file
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|   		\---\FT_Platform.h - Includes Platform specific 
|							macros.
|   \---MSVC_Emulator
|		\---\FT_Emulator.h
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|		\---\FT_EmulatorMain.h
|		\---\FT_Emulatorspi_i2c.h		
|   		\---\FT_Platform.h - Includes Platform specific 
|							macros.
|   \---\FT_CoPro_Cmds.h  - Includes the Coprocessor 	
|							commands.
|   \---\FT_Gpu.h - Includes the GPU commands.
|   \---\FT_Gpu_Hal.h - Includes the GPU HAL commands.
|   \---\FT_Hal_Utils.h - Includes the HAL utilities.
|
+---Project
|   \---Arduino
|   	\---FT_App_Refrigerator- project folder of Refrigerator Demo application based on arduino IDE  
| 		\---\FT_App_Refrigerator.ino - Sketch file of Refrigerator Demo application
| 		\---\FT_CoPro_Cmds.cpp - Coprocessor commands source file.
|   		\---\FT_CoPro_Cmds.h  - Includes the Coprocessor commands.
|  		\---\FT_DataTypes.h - Includes the FT800 datatypes.
|   		\---\FT_Gpu.h - Includes the Gpu commands.
|   		\---\FT_Gpu_Hal.cpp -  GPU HAL source.
|   		\---\FT_Gpu_Hal.h - Includes the Gpu Hal commands.
|   		\---\FT_Hal_Utils.h - Includes the hal utilities.
|   		\---\FT_Platform.h - Includes Platform specific commands.
|
|   \---MSVC_Emulator
|       \---FT_App_Refrigerator
|		\---FT_App_Refrigerator.sln– solution file of Refrigerator Demo Emulator application
|		\---FT_App_Refrigerator.vcxproj – project file of Refrigerator Demo Emulator application
|		\---FT_App_Refrigerator.vcxproj.filters
|
|   \---Msvc_win32
|       \---FT_App_Refrigerator – project folder of Refrigerator Demo application based on MSVC/PC platform
|		\---Ft_App_Refrigerator.sln – Solution file of Refrigerator Demo application.
|		\---Ft_App_Refrigerator.vcxproj– project file of Refrigerator Demo application.
|		\---Ft_App_Refrigerator.vcxproj.filters
|		\---Ft_App_Refrigerator.vcxproj.user
|
+---Src
|   	\---FT_CoPro_Cmds.c - Coprocessor commands source file.
|   	\---FT_Gpu_Hal.c - Gpu hal source commands file.
|   	\---FT_App_Refrigerator.c - Main file of Refrigerator application.
|   	\---FT_Emu_main.cpp	- Main file of Emulator
|
+--Test – folder containing input test files such as .wav, .jpg, .raw etc



Configuration Instructions:
===========================
This section contains details regarding various configurations supported by this software.

One display profile is supported by this software: (1) WQVGA (480x272) 
Two arduino platforms are supported by this software: (1) VM800P – FTDI specific module (2) Arduino pro

The above two configurations can be enabled/disabled via commenting/uncommenting macors in FT_Platform.h file. For MVSC/PC platform please look into .\FT_App_Refrigerator\Hdr\FT_Platform.h and for arduino platform please look into .\FT_App_Refrigerator\Project\Arduino\FT_App_Refrigerator\FT_Platform.h

By default WQVGA display profile is enabled. To enable QVGA, uncomment “#define SAMAPP_DISPLAY_QVGA” macro in respective FT_Platform.h file.The refrigerator application is displayed in WQVGA profile only.


For arduino platform please uncomment any one of the macros (1) “#define  FT_ATMEGA_328P” – FTDI specific module (2) “#define ARDUINO_PRO_328” – arduino pro. At any point of time only one macro should be uncommented/enabled.



Installation Instruction:
=========================

Unzip the package onto a respective project folder and open the solution/sketch file in the project folder and execute it. For MSVC/PC platform please execute .\FT_App_Refrigerator\Project\Msvc_win32\FT_App_Refrigerator\FT_App_Refrigerator.sln solution. For arduino platform please execute.\FT_App_Refrigerator\Project\Arduino\FT_App_Refrigerator\FT_App_Refrigerator.ino sketch.

The MSVC project file is compatible with Microsoft visual C++ 2010 Express.
The arduino project file is compatible with Arduino 1.0.5.

Reference Information:
======================
Please refer to AN_FT_App_Refrigerator for more information on application design, setup etc.
Please refer to FT800_Programmer_Guide for more information on programming.

Known issues\Limitations:
=============
1. Will work only on SPI mode.
2. The SPI host(Arduino, Windows PC) are assuming the data layout in memory as Little Endian format. 
3. For Arduino based platform, this application utilizes the timer using the millis API for the time display.
4. Only WQVGA display is enabled.
5. Only one MPSSE cable connected to the host PC is enumerated.

Extra Information:
==================
N.A


Release Notes (Version Significance):
=====================================
Version 3.1 - Addition of latest emulator library.
Version 3.0 - Support for Emulator.
Version 2.0 - Support for FT801 platform.
Version 1.0 - Final version based on the requirements.
Version 0.1 - intial draft of the release notes



