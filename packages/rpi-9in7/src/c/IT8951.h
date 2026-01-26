/*****************************************************************************
* | File        :   IT8951.h
* | Author      :   Waveshare team
* | Function    :   IT8951 Controller driver for 9.7" e-Paper display
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2026-01-26
* | Info        :   IT8951 controller with 16 grayscale levels support
*
* Display specifications:
*   Resolution: 1200 × 825
*   Grayscale: 2-16 levels (1-4 bit)
*   Interface: SPI
*
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#ifndef _IT8951_H_
#define _IT8951_H_

#include "DEV_Config.h"

// Display dimensions for 9.7" display
#define IT8951_9IN7_WIDTH   1200
#define IT8951_9IN7_HEIGHT  825

//-----------------------------------------------------------
// IT8951 SPI Command codes
//-----------------------------------------------------------
#define IT8951_TCON_SYS_RUN         0x0001
#define IT8951_TCON_STANDBY         0x0002
#define IT8951_TCON_SLEEP           0x0003
#define IT8951_TCON_REG_RD          0x0010
#define IT8951_TCON_REG_WR          0x0011
#define IT8951_TCON_MEM_BST_RD_T    0x0012
#define IT8951_TCON_MEM_BST_RD_S    0x0013
#define IT8951_TCON_MEM_BST_WR      0x0014
#define IT8951_TCON_MEM_BST_END     0x0015
#define IT8951_TCON_LD_IMG          0x0020
#define IT8951_TCON_LD_IMG_AREA     0x0021
#define IT8951_TCON_LD_IMG_END      0x0022

//-----------------------------------------------------------
// IT8951 Register addresses
//-----------------------------------------------------------
// I80 CPCR (Host Interface Communication Control Register)
#define IT8951_I80CPCR              0x0004

// System registers
#define IT8951_LISAR                0x0200  // Load Image Start Address Register (Low)
#define IT8951_LISAR_H              0x0202  // Load Image Start Address Register (High)

// Display Engine registers
#define IT8951_LUTAFSR              0x1224  // LUT Status Register (Display Engine)
#define IT8951_UP1SR                0x1234  // Update Parameter 1 Setting Register
#define IT8951_BGVR                 0x1250  // Background Voltage Register (VCOM)

// Memory Converter registers
#define IT8951_MCSR                 0x0200
#define IT8951_LISAR                0x0200

//-----------------------------------------------------------
// Waveform update modes
//-----------------------------------------------------------
#define IT8951_MODE_INIT            0   // Initialize display (clears ghosting)
#define IT8951_MODE_DU              1   // Direct Update (fastest, B&W only)
#define IT8951_MODE_GC16            2   // Grayscale Clearing 16 (16 gray levels, high quality)
#define IT8951_MODE_GL16            3   // Grayscale Low 16 (16 gray levels, low flashing)
#define IT8951_MODE_GLR16           4   // Grayscale Low Reagl 16
#define IT8951_MODE_GLD16           5   // Grayscale Low Delta 16
#define IT8951_MODE_DU4             6   // Direct Update 4 (4 gray levels)
#define IT8951_MODE_A2              7   // Animation mode (fastest, 2 levels)
#define IT8951_MODE_GCK16           8   // Grayscale Clearing Keep 16

//-----------------------------------------------------------
// Pixel format / bits per pixel
//-----------------------------------------------------------
#define IT8951_2BPP                 0   // 2 bits per pixel (4 gray levels)
#define IT8951_3BPP                 1   // 3 bits per pixel (8 gray levels)
#define IT8951_4BPP                 2   // 4 bits per pixel (16 gray levels)
#define IT8951_8BPP                 3   // 8 bits per pixel (256 gray levels, internal use)

//-----------------------------------------------------------
// Rotation
//-----------------------------------------------------------
#define IT8951_ROTATE_0             0
#define IT8951_ROTATE_90            1
#define IT8951_ROTATE_180           2
#define IT8951_ROTATE_270           3

//-----------------------------------------------------------
// Endianness
//-----------------------------------------------------------
#define IT8951_LDIMG_L_ENDIAN       0
#define IT8951_LDIMG_B_ENDIAN       1

//-----------------------------------------------------------
// Data structures
//-----------------------------------------------------------

// Device information structure returned by IT8951
typedef struct {
    UWORD usPanelW;
    UWORD usPanelH;
    UWORD usImgBufAddrL;
    UWORD usImgBufAddrH;
    char usFWVersion[16];
    char usLUTVersion[16];
} IT8951DevInfo;

// Load image area information
typedef struct {
    UWORD usEndianType;     // Little or Big Endian
    UWORD usPixelFormat;    // Pixel Format (2, 3, 4, 8 bpp)
    UWORD usRotate;         // Rotation (0, 90, 180, 270)
    UDOUBLE ulStartFBAddr;  // Start address of source frame buffer
    UWORD usX;              // X position
    UWORD usY;              // Y position
    UWORD usWidth;          // Width
    UWORD usHeight;         // Height
} IT8951LdImgInfo;

// Area information for display update
typedef struct {
    UWORD usX;
    UWORD usY;
    UWORD usWidth;
    UWORD usHeight;
} IT8951AreaInfo;

//-----------------------------------------------------------
// Function declarations
//-----------------------------------------------------------

// Initialization
IT8951DevInfo IT8951_Init(void);
void IT8951_SystemRun(void);
void IT8951_Standby(void);
void IT8951_Sleep(void);

// VCOM control
void IT8951_SetVCOM(UWORD vcom);
UWORD IT8951_GetVCOM(void);

// Display operations
void IT8951_Clear(void);
void IT8951_Clear_Refresh(IT8951DevInfo *info);

// Image loading
void IT8951_LoadImgStart(IT8951LdImgInfo *pLdImgInfo);
void IT8951_LoadImgAreaStart(IT8951LdImgInfo *pLdImgInfo, IT8951AreaInfo *pAreaInfo);
void IT8951_LoadImgEnd(void);

// Image data transfer
void IT8951_WritePixelData(UWORD *pwBuf, UDOUBLE ulSizeWordCnt);

// Display refresh
void IT8951_DisplayArea(UWORD usX, UWORD usY, UWORD usW, UWORD usH, UWORD usMode);
void IT8951_DisplayAreaBuf(UWORD usX, UWORD usY, UWORD usW, UWORD usH, UWORD usMode, UDOUBLE ulTargetMemAddr);

// High-level functions for different bit depths
void IT8951_Display_1bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, 
                         UDOUBLE targetAddr, UBYTE isInvert);
void IT8951_Display_2bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, UDOUBLE targetAddr);
void IT8951_Display_4bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, UDOUBLE targetAddr);

// Full screen display functions
void IT8951_Display_Full_1bpp(IT8951DevInfo *info, UBYTE *image);
void IT8951_Display_Full_2bpp(IT8951DevInfo *info, UBYTE *image);
void IT8951_Display_Full_4bpp(IT8951DevInfo *info, UBYTE *image);

// Wait for display ready
void IT8951_WaitForDisplayReady(void);

// Get device info
IT8951DevInfo* IT8951_GetDevInfo(void);

#endif
