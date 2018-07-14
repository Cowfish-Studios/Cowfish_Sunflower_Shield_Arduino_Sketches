; Copyright (c) Future Technology Devices International 2013


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
This package contains the FT80x and FT81x sample applications targeting Windows PC, Arduino platform, EVE Emulator and FT90x platform.  
The sample application is created for tutorial purposes for smooth learning of FT8xx. The users are encouraged to read the source codes first and change the functions that they would like to experiment.



Release package contents:
=========================
The folder structure is shown as below.
+---Bin
|   \---Msvc_win32
|   	\--libMPSSE.a - MPSSE library file.
|          \--ftd2xx.lib - FTD2XX library.
|          \--LibFT4222.lib - FT4222 library file.
|   \---MSVC_Emulator
|       \--ft8xxemu.lib - FT800 Emulator library in release mode
|     
|
+---Hdr
|   \---FT90x
|   		\---\FT_DataTypes.h - datatypes header file
|   		\---\FT_Platform - platform specific header file
|
|   \---Msvc_win32
|   		\---\ftd2xx.h - ftd2xx header file
|   		\---\libMPSSE_spi - MPSSE header file
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|   		\---\FT_Platform.h - Includes Platform specific 
|							macros.
|                \---\LibFT4222.h - LibFT4222 header file 
|   \---MSVC_Emulator
|		\---\FT_Emulator.h
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|		\---\FT_EmulatorMain.h
|   		\---\FT_Platform.h - Includes Platform specific 
|							macros.
|   \---\FT_CoPro_Cmds.h  - Includes the Coprocessor 	
|							commands.
|   \---\FT_Gpu.h - Includes the GPU commands.
|   \---\FT_Gpu_Hal.h - Includes the GPU HAL commands.
|   \---\FT_Hal_Utils.h - Includes the HAL utilities.
|   \---\FT_ILI9488.h includes ILI9488 Bootup config and intialzation
|   \---\SampleApp.h - Includes the Sample application resources
|
+---Project
|   \---Arduino
|       \---SampleApp
| 		\---SampleApp.ino - Sketch file of sample application
|   	    	\---SampleApp.h - Header file for sample application
|   	    	\---SampleApp_RawData.cpp - The source file for data used in Sample Application
|    		\---FT_CoPro_Cmds.cpp - Coprocessor commands source file.
|   		\---FT_CoPro_Cmds.h  - Includes the Coprocessor commands.
|  	     	\---FT_DataTypes.h - Includes the FT800 datatypes.
|   		\---FT_Gpu.h - Includes the Gpu commands.
|   		\---FT_Gpu_Hal.cpp - GPU HAL source.
|   		\---FT_Gpu_Hal.h - Includes the GPU HAL commands.
|   		\---FT_Hal_Utils.h - Includes the HAL utilities.
|   		\---FT_Platform.h - Includes Platform specific commands.
|		\---sdcard.h - Includes SD card header file
|		\---FT_Hal_I2C.cpp - Includes I2c source file	
|		\---FT_Hal_I2C.h - Includes I2c header file
|   \---Msvc_win32
|       \---SampleApp 
|           \---SampleApp.sln - Sample Application solution file for MSVC IDE
|		    \---SampleApp.vcxproj - Sample Application project file for MSVC IDE 
|
|   \---MSVC_Emulator
|       \---SampleApp 
|            \---SampleApp.sln - Sample Application solution file for MSVC IDE(FT800 emulator specific)
|	     \---SampleApp.vcxproj - Sample Application project file for MSVC IDE(FT800 emulator specific) 
+---Src
|   	\---FT_CoPro_Cmds.c - Coprocessor commands source file.
|   	\---FT_Gpu_Hal.c - Gpu HAL source file.
|   	\---FT800Emu_main.cpp - The C wrapper of SPI/I2C interface of FT800 emulator and the main() function.
|       \---SampleApp.c - The API implementation of FT800 sample application
|       \-- SampleApp_RawData.c - The data definition used in sampleApp.c
|       \-- FT_ILI9488.c - ILI Driver Source file
|
+--Test - folder containing input test files such as .wav, .jpg, .raw etc
---Readme.txt - This file


Configuration Instructions:
===========================
This section contains details regarding various configurations supported by this package.

Module related configurations are part of respective FT_Platform.h files.

For MSVC_win32 platform modules supported are (1) VM800B43_50 (2) VM800B35 (3) VM801B43_50 (4)VM810C50. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\Msvc_win32\FT_Platform.h file.

For MSVC_Emulator platform modules supported are (1) VM800B43_50 (2) VM800B35 (3) VM801B43_50. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\MSVC_Emulator\FT_Platform.h file.

For FT90x platform, mcu modules supported are (1)MM900EV1A (2) MM900EV2A (3) MM900EV3A (4)MM900EV_LITE. EVE modules supported are (1)ME800A_HV35R (2)ME810A_HV35R (3)ME813A_WV7C. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\FT90x\FT_Platform.h file.

For arduino platform modules supported are (1)VM800P43_50 (2)VM800P35 (3)VM801P43_50 (4)VM800B43_50 (5)VM800B35 (6)VM801B43_50.Uncomment only one macro definition at any instance of time and comment rest others in FT_App_Metaball\Project\Arduino\FT_App_Metaball\FT_Platform.h file. 

Any arduino module can be wired to (1)VM800B43_50 (2)VM800B35 (3)VM801B43_50 modules for experimentations ex: Arduino pro

For any of the custom configuration other than the modules mention above, comment all the module macros and enable/disable the respective macros for resolution, chip, spi channels, ILI9488 etc.

Installation Instruction:
=========================
The MSVC project file is compatible with Microsoft visual C++ 2012 Express.
Please open the respective sampleApp.sln under project for MSVC(MPSSE) platform and MSVC FT800 platform.

The Arduino sketch file is compatible with Arduino 1.0.5.


Reference Information:
======================
1. AN_245_VM800CB_SampleApp_PC_Introduction.pdf
2. AN_246_VM800CB_SampleApp_Arduino_Introduction.pdf

Extra Information:
==================
1. Sampel Applications for Windows PC and Arduino platform shares share the same code base. But Windows PC has 
more APIs definition since it has no storage limitation. 



Known issues and limitations:
=============================
1. Will work only on SPI mode.
2. The SPI host(Arduino, Windows PC) are assuming the data layout in memory as Little Endian format. 
3. Cannot compile and build MSVC project using the version under Microsoft visual C++ 2012 Express.
4. FT8xx Emulator touch works fine for FT801, FT81x platforms.


Release Notes (Version Significance):
=====================================
1.0.   First formal release
1.0.2  Fix the issue in "Ft_Gpu_CoCmd_Button" 
1.0.3  Add the support of FT800 Emulator, remove Doc folder , new readme.txt
1.0.4  Change the output directory of executable for FT800 Emulator based project
1.0.5  Update emulator library with Audio enabled
1.0.6 - Jun/23/2014 - Added multitouch sets and FT801 emulation support. 
2.0    Migration to FT81x platforms (Hardware and Emulator)
3.0    Added FT90x platform project.
3.1    Added support for VM810C50 module for msvc platform.
3.2 - Addition of the latest emulator library and PALETTE8 example
3.3 - Updated to version 3.2 of HAL library