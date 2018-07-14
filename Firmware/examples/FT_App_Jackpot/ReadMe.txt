; Copyright (c) Future Technology Devices International 2015



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
This ReadMe file contains the details of FT_App_Jackpot application release package. 

This application demonstrates primitives and touch screen based on FT800 platform.


Release package contents:
=========================
The folder structure is shown as below.


+---Bin
|   \---Msvc_win32
|          \--libMPSSE.a - MPSSE library file.
|          \--ftd2xx.lib - FTD2XX library.
|          \--LibFT4222.lib - FT4222 library file.
|   \---MSVC_Emulator
|          \--ft8xxemu.lib - Emulator library file.               
+---Hdr

|
|   \---FT90x
|   		\---\FT_DataTypes.h - datatypes header file
|   		\---\FT_Platform - platform specific header file
|
|   \---Msvc_win32
|   		\---\ftd2xx.h - ftd2xx header file
|   		\---\libMPSSE_spi - MPSSE header file
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|   		\---\FT_Platform.h - Includes Platform specific macros.
|
|                \---\LibFT4222.h - LibFT4222 header file 
|   \---MSVC_Emulator
|		\---\FT_Emulator.h
|		\---\FT_DataTypes.h - Includes the FT800 	
|					datatypes according to the platform.
|		\---\FT_EmulatorMain.h
|		\---\FT_Emulatorspi_i2c.h		
|   		\---\FT_Platform.h - Includes Platform specific macros.
|
|   \---\FT_CoPro_Cmds.h  - Includes the Coprocessor 	
|							commands.
|   \---\FT_Gpu.h - Includes the GPU commands.
|   \---\FT_Gpu_Hal.h - Includes the GPU HAL commands.
|   \---\FT_Hal_Utils.h - Includes the HAL utilities.
|
+---Project
|   \---Arduino
|   	\---FT_App_Jackpot
		\---\FT_App_Jackpot.ino - Main file of Jackpot Demo
|   		\---\FT_CoPro_Cmds.h  - Includes the Coprocessor commands.
|		\---\FT_CoPro_Cmds.cpp - Coprocessor commands source code.
|  		\---\FT_DataTypes.h - Includes the FT800 datatypes.
|   		\---\FT_Gpu.h - Includes the Gpu commands.
|   		\---\FT_Gpu_Hal.h - Includes the Gpu Hal commands.
|		\---\FT_Gpu_Hal.cpp -Gpu Hal commands source code.
|   		\---\FT_Hal_Utils.h - Includes the hal utilities.
|   		\---\FT_Platform.h - Includes Platform specific commands.
|   		\---\sdcard.h - Required to access the sd card.
|
|   \---FT90x
|       \---FT_App_Metaball - project folder of Metaball Demo application based on FT90x IDE  
|       	\---.cproject - cproject for metaball application
|       	\---.project
|
|   \---MSVC_Emulator
|       \---FT_App_Jackpot
|		\---Ft_App_Jackpot.sln– solution file of Jackpot Demo Emulator application
|		\---Ft_App_Jackpot.vcxproj – project file of Jackpot Demo Emulator application
|		\---Ft_App_Jackpot.vcxproj.filters
|
|   \---Msvc_win32
|       \---FT_App_Jackpot - project folder of Jackpot Demo application based on MSVC/PC platform
|		\---Ft_App_Jackpot.sln– solution file of Jackpot Demo application
|		\---Ft_App_Jackpot.vcxproj – project file of Jackpot Demo application
|		\---Ft_App_Jackpot.vcxproj.filters
|
+---Src
|   	\---FT_CoPro_Cmds.c - Coprocessor commands source file.
|   	\---FT_Gpu_Hal.c - Gpu hal source commands file.
|   	\---FT_Jackpot.c - Main file of Jackpot Demo.
|   	\---FT_Emu_main.cpp	- Main file of Emulator
|
+--Test – folder containing input test files such as .wav, .jpg, .raw etc
|
|--ReadMe.txt - this file


Configuration Instructions:
===========================
This section contains details regarding various configurations supported by this package.

Module related configurations are part of respective FT_Platform.h files.

For MSVC_win32 platform modules supported are (1) VM800B43_50 (2) VM801B43_50 (3)VM810C50. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\Msvc_win32\FT_Platform.h file.

For MSVC_Emulator platform modules supported are (1) VM800B43_50  (2) VM801B43_50. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\MSVC_Emulator\FT_Platform.h file.

For FT90x platform, mcu modules supported are (1)MM900EV1A (2) MM900EV2A (3) MM900EV3A (4)MM900EV_LITE. EVE modules supported are (1)ME800A_HV35R (2)ME810A_HV35R (3)ME813A_WV7C. Uncomment only one macro definition at any instance of time and comment rest others in \FT_App_Metaball\Hdr\FT90x\FT_Platform.h file.

For arduino platform modules supported are (1)VM800P43_50  (2)VM801P43_50 (3)VM800B43_50 (4)VM800B35 (5)VM801B43_50.Uncomment only one macro definition at any instance of time and comment rest others in FT_App_Metaball\Project\Arduino\FT_App_Metaball\FT_Platform.h file. 

Any arduino module can be wired to (1)VM800B43_50 (2)VM801B43_50 modules for experimentations ex: Arduino pro

For any of the custom configuration other than the modules mention above, comment all the module macros and enable/disable the respective macros for resolution, chip, spi channels, ILI9488 etc.

Installation Instruction:
=========================

Unzip the package onto a respective project folder and open the solution/sketch file in the project folder and execute it. 
For MSVC/PC platform please execute .\FT_Jackpot\Project\Msvc_win32\FT_Jackpot\FT_Jackpot.sln solution. 
For Arduino platform please execute.\FT_Jackpot\Project\Arduino\FT_Jackpot\FT_Jackpot.ino sketch.
For MVSC/PC Emulator platform please execute .\FT_Jackpot\Project\MSVC_Emulator\FT_Jackpot\FT_Jackpot.sln solution. 
For FT90x platform please import the project files in the .\FT_Jackpot\Project\FT90x\FT_Jackpot\ directory to the FTDI eclipse IDE.

The MSVC project file is compatible with Microsoft visual C++ version 2010.
The arduino project file is compatible with Arduino 1.0.5.
The MSVC emulator project is compatible with Microsoft Visual C++ version 2012 or above.
Reference Information:
======================
Please refer to AN_306 FT800 Jackpot Application Notes for more information on application design, setup etc.
Please refer to FT800_Programmer_Guide for more information on programming FT800.
Please refer to the FT90x installation apps notes in the FTDI website.

Known issues:
=============
1. This application contains only SPI interface to FT800.
2. FT8xx Emulator touch works fine for FT801, FT81x platforms.
3. The SPI host(Arduino, Windows PC) are assuming the data layout in memory as Little Endian format. 

Extra Information:
==================
N.A


Release Notes (Version Significance):
=====================================
Version 4.3 - Updated to version 3.2 of HAL library
Version 4.2 - Addition of the newest emulator library.
Version 4.1 - Added support for VM810C50 module for msvc platform.
Version 4.0 - Added support for FT81x series and FT90x series.
Version 3.2 - Added support for FT900 platform
Version 3.1 - Jun/23/2014 - Removed REG_ROTATE
Version 3.0 - Support for Emulator.
Version 2.0 - Support for FT801 platform.
Version 1.1.2 - Final version based on the requirements.




