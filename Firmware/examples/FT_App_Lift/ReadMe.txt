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
This ReadMe file contains the details of Lift application release package. 

This application demonstrates the lift simulation Demo drawn by using the bitmaps for fonts, arrow and audio in sync with the change in floor number with date and time display based on FT800 platform.

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
|   \---FT90X
|		\---\FT_DataTypes.h
|  		\---\FT_Platform.h
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
|   	\---FT_App_Lift- project folder of Lift Demo application based on arduino IDE  
| 		\---\FT_App_Lift.ino - Sketch file of Lift Demo application
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
|       \---FT_App_Lift
|		\---FT_App_Lift.sln– solution file of Lift Demo Emulator application
|		\---FT_App_Lift.vcxproj – project file of Lift Demo Emulator application
|		\---FT_App_Lift.vcxproj.filters
|
|   \---FT90x
|        \---FT_App_Lift
|		\---.cproject
|		\---.project
|
|   \---Msvc_win32
|       \---FT_App_Lift – project folder of Lift Demo application based on MSVC/PC platform
|		\---Ft_App_Lift.sln – Solution file of Lift Demo application.
|		\---Ft_App_Lift.vcxproj– project file of Lift Demo application.
|		\---Ft_App_Lift.vcxproj.filters
|
+---Src
|   	\---FT_CoPro_Cmds.c - Coprocessor commands source file.
|   	\---FT_Gpu_Hal.c - Gpu hal source commands file.
|   	\---FT_App_Lift.c - Main file of Lift application.
|	\---FT_Emu_main.cpp	- Main file of Emulator
|
+--Test – folder containing input test files such as .wav, .jpg, .raw etc
|
|--ReadMe.txt - this file



Configuration Instructions:
===========================
This section contains details regarding various configurations supported by this software.

The configurations can be enabled/disabled via commenting/uncommenting macors in FT_Platform.h file. 
For MVSC/PC platform please look into .\FT_App_Lift\Hdr\Msvc_win32\FT_Platform.h 
For arduino platform please look into .\FT_App_Lift\Project\Arduino\FT_App_Lift\FT_Platform.h
For FT90x platform please look into .\FT_App_Lift\Hdr\FT90x\FT_Platform.h
For MVSC/PC Emulator platform please look into .\FT_App_Lift\Hdr\MSVC_Emulator\FT_Platform.h 




Installation Instruction:
=========================

Unzip the package onto a respective project folder and open the solution/sketch file in the project folder and execute it. 
For MSVC/PC platform please execute .\FT_App_Lift\Project\Msvc_win32\FT_App_Lift\FT_App_Lift.sln solution. 
For Arduino platform please execute.\FT_App_Lift\Project\Arduino\FT_App_Lift\FT_App_Lift.ino sketch.
For MVSC/PC Emulator platform please execute .\FT_App_Lift\Project\MSVC_Emulator\FT_App_Lift\FT_App_Lift.sln solution. 
For FT90x platform please import the project files in the .\FT_App_Lift\Project\FT90x\FT_App_Lift\ directory to the FTDI eclipse IDE.

The MSVC project file is compatible with Microsoft visual C++ version 2010.
The arduino project file is compatible with Arduino 1.0.5.
The MSVC emulator project is compatible with Microsoft Visual C++ version 2012 or above.
Reference Information:
======================
Please refer to AN_FT_App_Lift for more information on application design, setup etc.
Please refer to FT800_Programmer_Guide for more information on programming.
Please refer to the FT90x installation apps notes in the FTDI website.

Known issues\Limitations:
=============
1. This application contains only SPI interface to FT800.
2. The SPI host(Arduino, Windows PC) are assuming the data layout in memory as Little Endian format. 
3. For Arduino based platform, this application utilizes the timer using the millis API for the time display.
4. Only WQVGA display is enabled.
5. Only one MPSSE cable connected to the host PC is enumerated.

Extra Information:
==================
N.A


Release Notes (Version Significance):
=====================================
Version 2.3 - Updated to version 3.2 of HAL library
Version 2.2 - Addition of the latest emulator library
Version 2.1 - Added support for VM810C50 module for msvc platform.
Version 2.0 - Added support for FT81x series and FT90x series.
Version 1.0 - Final version based on the requirements.
Version 0.1 - intial draft of the release notes



