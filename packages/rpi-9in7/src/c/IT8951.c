/*****************************************************************************
* | File        :   IT8951.c
* | Author      :   Waveshare team
* | Function    :   IT8951 Controller driver for 9.7" e-Paper display
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2026-01-26
* | Info        :   IT8951 controller with 16 grayscale levels support
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
#include "IT8951.h"
#include <stdlib.h>
#include <string.h>

// Global device info
static IT8951DevInfo gDevInfo;
static UDOUBLE gMemAddr = 0;

//-----------------------------------------------------------
// Low-level SPI communication
//-----------------------------------------------------------

/**
 * Wait for the IT8951 to be ready (HRDY pin high).
 * Called BEFORE asserting CS for a new transaction.
 */
static void IT8951_WaitForReady(void)
{
    UDOUBLE timeout = 0;
    while(DEV_Digital_Read(IT8951_HRDY_PIN) == 0) {
        DEV_Delay_ms(1);
        if(++timeout > 5000) {
            printf("IT8951_WaitForReady timeout!\r\n");
            break;
        }
    }
}

/**
 * Write a command word to IT8951.
 * Preamble (0x6000) and command are sent as a single 4-byte SPI transfer
 * so that the kernel-managed CE0 covers the whole transaction.
 */
static void IT8951_WriteCommand(UWORD usCmd)
{
    uint8_t buf[4] = {
        0x60, 0x00,                         /* preamble: write command */
        (uint8_t)((usCmd >> 8) & 0xFF),
        (uint8_t)(usCmd & 0xFF)
    };
    IT8951_WaitForReady();
    DEV_SPI_Write_nByte(buf, 4);
}

/**
 * Write a data word to IT8951.
 * Preamble (0x0000) and data are sent as a single 4-byte SPI transfer.
 */
static void IT8951_WriteData(UWORD usData)
{
    uint8_t buf[4] = {
        0x00, 0x00,                          /* preamble: write data */
        (uint8_t)((usData >> 8) & 0xFF),
        (uint8_t)(usData & 0xFF)
    };
    IT8951_WaitForReady();
    DEV_SPI_Write_nByte(buf, 4);
}

/**
 * Read a data word from IT8951.
 * Preamble (0x1000) + dummy word + actual read, all in one transfer.
 */
static UWORD IT8951_ReadData(void)
{
    /* Total frame: 2-byte preamble + 2-byte dummy + 2-byte data = 6 bytes */
    uint8_t buf[6] = { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
    IT8951_WaitForReady();
    DEV_SPI_Write_nByte(buf, 6);   /* full-duplex: buf is overwritten with RX */
    return (UWORD)((buf[4] << 8) | buf[5]);
}

/**
 * Write N data words to IT8951.
 * Preamble (0x0000) + all words sent in ONE ioctl call so that
 * the kernel-managed CE0 stays asserted for the whole transaction.
 */
static void IT8951_WriteNData(UWORD *pwBuf, UDOUBLE ulSizeWordCnt)
{
    UDOUBLE totalBytes = 2 + ulSizeWordCnt * 2;  /* 2 preamble + 2*N data */
    uint8_t *buf = (uint8_t *)malloc(totalBytes);
    if(!buf) {
        printf("IT8951_WriteNData: malloc failed\r\n");
        return;
    }

    buf[0] = 0x00;
    buf[1] = 0x00;
    UDOUBLE i;
    for(i = 0; i < ulSizeWordCnt; i++) {
        buf[2 + i*2]     = (uint8_t)((pwBuf[i] >> 8) & 0xFF);
        buf[2 + i*2 + 1] = (uint8_t)(pwBuf[i] & 0xFF);
    }

    IT8951_WaitForReady();
    DEV_SPI_Write_nByte(buf, (uint32_t)totalBytes);
    free(buf);
}

/**
 * Read N data words from IT8951.
 * 2-byte preamble + 2-byte dummy + 2*N receive, all in one ioctl call.
 */
static void IT8951_ReadNData(UWORD *pwBuf, UDOUBLE ulSizeWordCnt)
{
    UDOUBLE totalBytes = 4 + ulSizeWordCnt * 2;  /* 2 preamble + 2 dummy + 2*N */
    uint8_t *buf = (uint8_t *)malloc(totalBytes);
    if(!buf) {
        printf("IT8951_ReadNData: malloc failed\r\n");
        return;
    }

    memset(buf, 0x00, totalBytes);
    buf[0] = 0x10;  /* read preamble */
    buf[1] = 0x00;

    IT8951_WaitForReady();
    DEV_SPI_Write_nByte(buf, (uint32_t)totalBytes);  /* full-duplex */

    /* Copy received words — bytes 0..3 are preamble+dummy (discard) */
    UDOUBLE i;
    for(i = 0; i < ulSizeWordCnt; i++) {
        pwBuf[i] = (UWORD)((buf[4 + i*2] << 8) | buf[4 + i*2 + 1]);
    }
    free(buf);
}

//-----------------------------------------------------------
// Register access
//-----------------------------------------------------------

/**
 * Write to a register
 */
static void IT8951_WriteReg(UWORD usRegAddr, UWORD usValue)
{
    IT8951_WriteCommand(IT8951_TCON_REG_WR);
    IT8951_WriteData(usRegAddr);
    IT8951_WriteData(usValue);
}

/**
 * Read from a register
 */
static UWORD IT8951_ReadReg(UWORD usRegAddr)
{
    IT8951_WriteCommand(IT8951_TCON_REG_RD);
    IT8951_WriteData(usRegAddr);
    return IT8951_ReadData();
}

//-----------------------------------------------------------
// System commands
//-----------------------------------------------------------

void IT8951_SystemRun(void)
{
    IT8951_WriteCommand(IT8951_TCON_SYS_RUN);
}

void IT8951_Standby(void)
{
    IT8951_WriteCommand(IT8951_TCON_STANDBY);
}

void IT8951_Sleep(void)
{
    IT8951_WriteCommand(IT8951_TCON_SLEEP);
}

//-----------------------------------------------------------
// Device info
//-----------------------------------------------------------

/**
 * Get device info from IT8951
 */
static void IT8951_GetSystemInfo(IT8951DevInfo *pDevInfo)
{
    UWORD buf[20];
    
    IT8951_WriteCommand(0x0302);  // Get Device Info command
    
    IT8951_ReadNData(buf, 20);
    
    pDevInfo->usPanelW = buf[0];
    pDevInfo->usPanelH = buf[1];
    pDevInfo->usImgBufAddrL = buf[2];
    pDevInfo->usImgBufAddrH = buf[3];
    
    // Copy firmware version string
    memcpy(pDevInfo->usFWVersion, &buf[4], 16);
    pDevInfo->usFWVersion[15] = '\0';
    
    // Copy LUT version string  
    memcpy(pDevInfo->usLUTVersion, &buf[12], 16);
    pDevInfo->usLUTVersion[15] = '\0';
}

IT8951DevInfo* IT8951_GetDevInfo(void)
{
    return &gDevInfo;
}

//-----------------------------------------------------------
// VCOM control
//-----------------------------------------------------------

void IT8951_SetVCOM(UWORD vcom)
{
    IT8951_WriteCommand(0x0039);  // VCOM command
    IT8951_WriteData(1);          // Set mode
    IT8951_WriteData(vcom);
}

UWORD IT8951_GetVCOM(void)
{
    IT8951_WriteCommand(0x0039);  // VCOM command
    IT8951_WriteData(0);          // Get mode
    return IT8951_ReadData();
}

//-----------------------------------------------------------
// Initialization
//-----------------------------------------------------------

/**
 * Hardware reset
 */
static void IT8951_Reset(void)
{
    DEV_Digital_Write(IT8951_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(IT8951_RST_PIN, 0);
    DEV_Delay_ms(20);
    DEV_Digital_Write(IT8951_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/**
 * Initialize IT8951 controller
 */
IT8951DevInfo IT8951_Init(void)
{
    printf("IT8951 Init\r\n");
    
    // Hardware reset
    IT8951_Reset();
    
    // Wait for ready
    IT8951_WaitForReady();
    
    // Run system
    IT8951_SystemRun();
    
    // Get device info
    IT8951_GetSystemInfo(&gDevInfo);
    
    printf("Panel %d x %d\r\n", gDevInfo.usPanelW, gDevInfo.usPanelH);
    printf("Firmware: %s\r\n", gDevInfo.usFWVersion);
    printf("LUT: %s\r\n", gDevInfo.usLUTVersion);
    
    // Calculate memory address
    gMemAddr = gDevInfo.usImgBufAddrL | ((UDOUBLE)gDevInfo.usImgBufAddrH << 16);
    printf("Image buffer address: 0x%08X\r\n", (unsigned int)gMemAddr);
    
    // Enable I80 packed write mode
    IT8951_WriteReg(IT8951_I80CPCR, 0x0001);
    
    return gDevInfo;
}

//-----------------------------------------------------------
// Image loading
//-----------------------------------------------------------

/**
 * Set target memory address for image load
 */
static void IT8951_SetTargetMemoryAddr(UDOUBLE ulImgBufAddr)
{
    UWORD usLow = (UWORD)(ulImgBufAddr & 0xFFFF);
    UWORD usHigh = (UWORD)((ulImgBufAddr >> 16) & 0xFFFF);
    
    IT8951_WriteReg(IT8951_LISAR, usLow);
    IT8951_WriteReg(IT8951_LISAR + 2, usHigh);
}

/**
 * Set up load image parameters and start load
 */
void IT8951_LoadImgStart(IT8951LdImgInfo *pLdImgInfo)
{
    UWORD usArg;
    
    // Set target address
    IT8951_SetTargetMemoryAddr(pLdImgInfo->ulStartFBAddr);
    
    // Build argument word
    usArg = (pLdImgInfo->usEndianType << 8) |
            (pLdImgInfo->usPixelFormat << 4) |
            pLdImgInfo->usRotate;
    
    // Send load image command
    IT8951_WriteCommand(IT8951_TCON_LD_IMG);
    IT8951_WriteData(usArg);
}

/**
 * Set up load image area parameters
 */
void IT8951_LoadImgAreaStart(IT8951LdImgInfo *pLdImgInfo, IT8951AreaInfo *pAreaInfo)
{
    UWORD usArg;
    
    // Set target address
    IT8951_SetTargetMemoryAddr(pLdImgInfo->ulStartFBAddr);
    
    // Build argument word
    usArg = (pLdImgInfo->usEndianType << 8) |
            (pLdImgInfo->usPixelFormat << 4) |
            pLdImgInfo->usRotate;
    
    // Send load image area command
    IT8951_WriteCommand(IT8951_TCON_LD_IMG_AREA);
    IT8951_WriteData(usArg);
    IT8951_WriteData(pAreaInfo->usX);
    IT8951_WriteData(pAreaInfo->usY);
    IT8951_WriteData(pAreaInfo->usWidth);
    IT8951_WriteData(pAreaInfo->usHeight);
}

/**
 * End image load operation
 */
void IT8951_LoadImgEnd(void)
{
    IT8951_WriteCommand(IT8951_TCON_LD_IMG_END);
}

/**
 * Write pixel data to IT8951
 */
void IT8951_WritePixelData(UWORD *pwBuf, UDOUBLE ulSizeWordCnt)
{
    IT8951_WriteNData(pwBuf, ulSizeWordCnt);
}

//-----------------------------------------------------------
// Display refresh
//-----------------------------------------------------------

/**
 * Wait for display engine to be ready
 */
void IT8951_WaitForDisplayReady(void)
{
    // Wait for LUT engine idle
    while(IT8951_ReadReg(IT8951_LUTAFSR) != 0) {
        DEV_Delay_ms(10);
    }
}

/**
 * Refresh a display area
 */
void IT8951_DisplayArea(UWORD usX, UWORD usY, UWORD usW, UWORD usH, UWORD usMode)
{
    IT8951_DisplayAreaBuf(usX, usY, usW, usH, usMode, gMemAddr);
}

/**
 * Refresh a display area from a specific buffer
 */
void IT8951_DisplayAreaBuf(UWORD usX, UWORD usY, UWORD usW, UWORD usH, UWORD usMode, UDOUBLE ulTargetMemAddr)
{
    IT8951_WriteCommand(0x0037);  /* USDEF_I80_CMD_DPY_BUF_AREA */
    IT8951_WriteData((UWORD)(ulTargetMemAddr & 0xFFFF));
    IT8951_WriteData((UWORD)((ulTargetMemAddr >> 16) & 0xFFFF));
    IT8951_WriteData(usMode);
    IT8951_WriteData(usX);
    IT8951_WriteData(usY);
    IT8951_WriteData(usW);
    IT8951_WriteData(usH);
    IT8951_WriteData(1);  /* enable waveform setting */
}

//-----------------------------------------------------------
// Clear display
//-----------------------------------------------------------

void IT8951_Clear(void)
{
    IT8951_Clear_Refresh(&gDevInfo);
}

void IT8951_Clear_Refresh(IT8951DevInfo *info)
{
    UWORD x, y;
    UWORD w = info->usPanelW;
    UWORD h = info->usPanelH;
    
    // Prepare load image info
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaInfo stAreaInfo;
    
    stLdImgInfo.ulStartFBAddr = gMemAddr;
    stLdImgInfo.usEndianType = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat = IT8951_8BPP;  // Use 8bpp for faster clear
    stLdImgInfo.usRotate = IT8951_ROTATE_0;
    
    stAreaInfo.usX = 0;
    stAreaInfo.usY = 0;
    stAreaInfo.usWidth = w;
    stAreaInfo.usHeight = h;
    
    // Load white image
    IT8951_LoadImgAreaStart(&stLdImgInfo, &stAreaInfo);
    
    // Allocate a line buffer
    UWORD lineSize = (w + 1) / 2;  // words per line for 8bpp
    UWORD *lineBuf = (UWORD *)malloc(lineSize * sizeof(UWORD));
    if (lineBuf == NULL) {
        printf("Memory allocation failed\r\n");
        IT8951_LoadImgEnd();
        return;
    }
    
    // Fill with white (0xFF)
    for(x = 0; x < lineSize; x++) {
        lineBuf[x] = 0xFFFF;
    }
    
    // Write line by line
    for(y = 0; y < h; y++) {
        IT8951_WritePixelData(lineBuf, lineSize);
    }
    
    free(lineBuf);
    
    IT8951_LoadImgEnd();
    
    // Wait for display ready then refresh
    IT8951_WaitForDisplayReady();
    IT8951_DisplayArea(0, 0, w, h, IT8951_MODE_INIT);
    IT8951_WaitForDisplayReady();
}

//-----------------------------------------------------------
// Display functions for different bit depths
//-----------------------------------------------------------

/**
 * Display 1bpp image (black and white)
 * Expands 1bpp packed input to 8bpp words for IT8951, sent in one batch.
 */
void IT8951_Display_1bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, 
                         UDOUBLE targetAddr, UBYTE isInvert)
{
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaInfo stAreaInfo;

    UWORD wordCount = (w + 1) / 2;             /* 2 pixels per word in 8bpp */
    UDOUBLE totalWords = (UDOUBLE)wordCount * h;
    UWORD inputBytesPerRow = (w + 7) / 8;

    UWORD *outBuf = (UWORD *)malloc(totalWords * sizeof(UWORD));
    if(!outBuf) {
        printf("IT8951_Display_1bpp: malloc failed\r\n");
        return;
    }

    /* Convert 1bpp packed → 8bpp (2 pixels per word, MSB = first pixel) */
    UDOUBLE outIdx = 0;
    for(UWORD row = 0; row < h; row++) {
        for(UWORD col = 0; col < w; col += 2) {
            UBYTE mask0 = 0x80 >> (col % 8);
            UBYTE pix0  = (image[row * inputBytesPerRow + col / 8] & mask0) ? 0xFF : 0x00;
            if(isInvert) pix0 = ~pix0;

            UWORD word = (UWORD)(pix0 << 8);

            if(col + 1 < w) {
                UBYTE mask1 = 0x80 >> ((col + 1) % 8);
                UBYTE pix1  = (image[row * inputBytesPerRow + (col + 1) / 8] & mask1) ? 0xFF : 0x00;
                if(isInvert) pix1 = ~pix1;
                word |= pix1;
            } else {
                word |= 0xFF;  /* white padding */
            }
            outBuf[outIdx++] = word;
        }
    }

    stLdImgInfo.ulStartFBAddr = targetAddr;
    stLdImgInfo.usEndianType  = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat = IT8951_8BPP;
    stLdImgInfo.usRotate      = IT8951_ROTATE_0;

    stAreaInfo.usX      = x;
    stAreaInfo.usY      = y;
    stAreaInfo.usWidth  = w;
    stAreaInfo.usHeight = h;

    IT8951_LoadImgAreaStart(&stLdImgInfo, &stAreaInfo);
    IT8951_WritePixelData(outBuf, totalWords);
    free(outBuf);
    IT8951_LoadImgEnd();

    IT8951_WaitForDisplayReady();
    IT8951_DisplayAreaBuf(x, y, w, h, IT8951_MODE_DU, targetAddr);
}

/**
 * Display 2bpp image (4 gray levels)
 */
void IT8951_Display_2bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, UDOUBLE targetAddr)
{
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaInfo stAreaInfo;
    
    stLdImgInfo.ulStartFBAddr = targetAddr;
    stLdImgInfo.usEndianType = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat = IT8951_2BPP;
    stLdImgInfo.usRotate = IT8951_ROTATE_0;
    
    stAreaInfo.usX = x;
    stAreaInfo.usY = y;
    stAreaInfo.usWidth = w;
    stAreaInfo.usHeight = h;
    
    IT8951_LoadImgAreaStart(&stLdImgInfo, &stAreaInfo);
    
    // 2bpp: 4 pixels per byte = 8 pixels per word
    UWORD wordsPerRow = (w + 7) / 8;
    UDOUBLE totalWords = (UDOUBLE)wordsPerRow * h;
    
    IT8951_WritePixelData((UWORD *)image, totalWords);
    
    IT8951_LoadImgEnd();
    
    IT8951_WaitForDisplayReady();
    IT8951_DisplayAreaBuf(x, y, w, h, IT8951_MODE_GL16, targetAddr);
}

/**
 * Display 4bpp image (16 gray levels)
 */
void IT8951_Display_4bpp(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h, UDOUBLE targetAddr)
{
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaInfo stAreaInfo;
    
    stLdImgInfo.ulStartFBAddr = targetAddr;
    stLdImgInfo.usEndianType = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat = IT8951_4BPP;
    stLdImgInfo.usRotate = IT8951_ROTATE_0;
    
    stAreaInfo.usX = x;
    stAreaInfo.usY = y;
    stAreaInfo.usWidth = w;
    stAreaInfo.usHeight = h;
    
    IT8951_LoadImgAreaStart(&stLdImgInfo, &stAreaInfo);
    
    // 4bpp: 2 pixels per byte = 4 pixels per word
    UWORD wordsPerRow = (w + 3) / 4;
    UDOUBLE totalWords = (UDOUBLE)wordsPerRow * h;
    
    IT8951_WritePixelData((UWORD *)image, totalWords);
    
    IT8951_LoadImgEnd();
    
    IT8951_WaitForDisplayReady();
    IT8951_DisplayAreaBuf(x, y, w, h, IT8951_MODE_GC16, targetAddr);
}

//-----------------------------------------------------------
// Full screen display functions
//-----------------------------------------------------------

void IT8951_Display_Full_1bpp(IT8951DevInfo *info, UBYTE *image)
{
    IT8951_Display_1bpp(image, 0, 0, info->usPanelW, info->usPanelH, gMemAddr, 0);
}

void IT8951_Display_Full_2bpp(IT8951DevInfo *info, UBYTE *image)
{
    IT8951_Display_2bpp(image, 0, 0, info->usPanelW, info->usPanelH, gMemAddr);
}

void IT8951_Display_Full_4bpp(IT8951DevInfo *info, UBYTE *image)
{
    IT8951_Display_4bpp(image, 0, 0, info->usPanelW, info->usPanelH, gMemAddr);
}
