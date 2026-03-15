/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface for IT8951
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2026-01-26
* | Info        :   Configuration for IT8951 controller
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
#
******************************************************************************/
#include "DEV_Config.h"

/**
 * GPIO pins
**/
int IT8951_RST_PIN;
int IT8951_CS_PIN;
int IT8951_HRDY_PIN;

/**
 * GPIO read and write
**/
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    bcm2835_gpio_write(Pin, Value);
#elif USE_WIRINGPI_LIB
    digitalWrite(Pin, Value);
#elif USE_DEV_LIB
    SYSFS_GPIO_Write(Pin, Value);
#endif
#endif
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    UBYTE Read_value = 0;
#ifdef RPI
#ifdef USE_BCM2835_LIB
    Read_value = bcm2835_gpio_lev(Pin);
#elif USE_WIRINGPI_LIB
    Read_value = digitalRead(Pin);
#elif USE_DEV_LIB
    Read_value = SYSFS_GPIO_Read(Pin);
#endif
#endif
    return Read_value;
}

/**
 * SPI write
**/
void DEV_SPI_WriteByte(uint8_t Value)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    bcm2835_spi_transfer(Value);
#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, &Value, 1);
#elif USE_DEV_LIB
    DEV_HARDWARE_SPI_TransferByte(Value);
#endif
#endif
}

/**
 * SPI read
**/
UBYTE DEV_SPI_ReadByte(void)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    return bcm2835_spi_transfer(0x00);
#elif USE_WIRINGPI_LIB
    uint8_t value = 0x00;
    wiringPiSPIDataRW(0, &value, 1);
    return value;
#elif USE_DEV_LIB
    return DEV_HARDWARE_SPI_TransferByte(0x00);
#endif
#endif
    return 0;
}

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    char rData[Len];
    bcm2835_spi_transfernb((char *)pData, rData, Len);
#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, pData, Len);
#elif USE_DEV_LIB
    DEV_HARDWARE_SPI_Transfer(pData, Len);
#endif
#endif
}

void DEV_SPI_Read_nByte(uint8_t *pData, uint32_t Len)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    char wData[Len];
    memset(wData, 0x00, Len);
    bcm2835_spi_transfernb(wData, (char *)pData, Len);
#elif USE_WIRINGPI_LIB
    memset(pData, 0x00, Len);
    wiringPiSPIDataRW(0, pData, Len);
#elif USE_DEV_LIB
    memset(pData, 0x00, Len);
    DEV_HARDWARE_SPI_Transfer(pData, Len);
#endif
#endif
}

/**
 * GPIO Mode
**/
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    if(Mode == 0 || Mode == BCM2835_GPIO_FSEL_INPT) {
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_INPT);
    } else {
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_OUTP);
    }
#elif USE_WIRINGPI_LIB
    if(Mode == 0 || Mode == INPUT) {
        pinMode(Pin, INPUT);
        pullUpDnControl(Pin, PUD_UP);
    } else {
        pinMode(Pin, OUTPUT);
    }
#elif USE_DEV_LIB
    SYSFS_GPIO_Export(Pin);
    if(Mode == 0 || Mode == SYSFS_GPIO_IN) {
        SYSFS_GPIO_Direction(Pin, SYSFS_GPIO_IN);
    } else {
        SYSFS_GPIO_Direction(Pin, SYSFS_GPIO_OUT);
    }
#endif
#endif
}

/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    bcm2835_delay(xms);
#elif USE_WIRINGPI_LIB
    delay(xms);
#elif USE_DEV_LIB
    UDOUBLE i;
    for(i = 0; i < xms; i++) {
        usleep(1000);
    }
#endif
#endif
}

/**
 * delay x us
**/
void DEV_Delay_us(UDOUBLE xus)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    bcm2835_delayMicroseconds(xus);
#elif USE_WIRINGPI_LIB
    delayMicroseconds(xus);
#elif USE_DEV_LIB
    usleep(xus);
#endif
#endif
}

static int DEV_Equipment_Testing(void)
{
    FILE *fp;
    char issue_str[64];

    fp = fopen("/etc/issue", "r");
    if (fp == NULL) {
        Debug("Unable to open /etc/issue");
        return -1;
    }
    if (fread(issue_str, 1, sizeof(issue_str), fp) <= 0) {
        Debug("Unable to read from /etc/issue");
        return -1;
    }
    issue_str[sizeof(issue_str) - 1] = '\0';
    fclose(fp);

    printf("Current environment: ");
#ifdef RPI
    char systems[][9] = {"Raspbian", "Debian", "NixOS"};
    int detected = 0;
    for(int i = 0; i < 3; i++) {
        if (strstr(issue_str, systems[i]) != NULL) {
            printf("%s\n", systems[i]);
            detected = 1;
        }
    }
    if (!detected) {
        printf("not recognized\n");
        printf("Built for Raspberry Pi, but unable to detect environment.\n");
        return -1;
    }
#endif
    return 0;
}

void DEV_GPIO_Init(void)
{
#ifdef RPI
    IT8951_RST_PIN  = 17;   // Reset pin
    IT8951_HRDY_PIN = 24;   // Host Ready pin (input)
    // CS (GPIO 8 / CE0) is managed by the kernel spidev driver.
    // Do NOT configure it here — the kernel claims it on open().
    IT8951_CS_PIN   = -1;
#endif

    DEV_GPIO_Mode(IT8951_RST_PIN, 1);   // Output
    DEV_GPIO_Mode(IT8951_HRDY_PIN, 0);  // Input
    // CS is NOT configured here (kernel owns it)
}

/******************************************************************************
function:   Module Initialize
parameter:
Info:       Initialize GPIO and SPI for IT8951
******************************************************************************/
UBYTE DEV_Module_Init(void)
{
    printf("/***********************************/ \r\n");
    printf("IT8951 Module Init\r\n");
    if(DEV_Equipment_Testing() < 0) {
        return 1;
    }
#ifdef RPI
#ifdef USE_BCM2835_LIB
    if(!bcm2835_init()) {
        printf("bcm2835 init failed !!! \r\n");
        return 1;
    } else {
        printf("bcm2835 init success !!! \r\n");
    }

    DEV_GPIO_Init();

    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    // IT8951 can handle higher SPI speeds
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);  // ~7.8MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

#elif USE_WIRINGPI_LIB
    if(wiringPiSetupGpio() < 0) {
        printf("set wiringPi lib failed !!! \r\n");
        return 1;
    } else {
        printf("set wiringPi lib success !!! \r\n");
    }

    DEV_GPIO_Init();
    // Higher speed for IT8951
    wiringPiSPISetup(0, 12000000);

#elif USE_DEV_LIB
    printf("Write and read /dev/spidev0.0 \r\n");
    DEV_GPIO_Init();
    DEV_HARDWARE_SPI_begin("/dev/spidev0.0");
    // 2MHz is safe for init; can be increased after IT8951 is confirmed working
    DEV_HARDWARE_SPI_setSpeed(2000000);
    // Kernel CE0 management is kept active (SPI_NO_CS is NOT set).
    // Preamble + payload are always sent in a single ioctl buffer so that
    // CE0 is never de-asserted mid-transaction.
#endif

#endif
    printf("/***********************************/ \r\n");
    return 0;
}

/******************************************************************************
function:   Module exits
parameter:
Info:       Closes SPI and releases GPIO
******************************************************************************/
void DEV_Module_Exit(void)
{
#ifdef RPI
#ifdef USE_BCM2835_LIB
    DEV_Digital_Write(IT8951_RST_PIN, LOW);
    bcm2835_spi_end();
    bcm2835_close();
#elif USE_WIRINGPI_LIB
    DEV_Digital_Write(IT8951_RST_PIN, 0);
#elif USE_DEV_LIB
    DEV_HARDWARE_SPI_end();
    DEV_Digital_Write(IT8951_RST_PIN, 0);
#endif
#endif
}
