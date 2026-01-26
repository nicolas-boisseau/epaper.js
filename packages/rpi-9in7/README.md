# @epaperjs/rpi-9in7

ePaper.js support for Waveshare 9.7" e-Paper display with IT8951 controller.

## Specifications

- **Resolution**: 1200 × 825 pixels
- **Display size**: 202.8mm × 139.425mm
- **Operating voltage**: 5V
- **Interface**: USB/SPI/I80/I2C
- **Grayscale**: 2-16 levels (1-4 bit)
- **Display color**: black, white
- **Driver board**: IT8951

## Installation

```bash
npm install @epaperjs/rpi-9in7
```

## Prerequisites

This package requires the bcm2835 library to be installed on your Raspberry Pi:

```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71
./configure
make
sudo make install
```

## Hardware Connection

The IT8951 driver board connects to the Raspberry Pi via SPI:

| IT8951 | Raspberry Pi |
|--------|--------------|
| VCC    | 5V           |
| GND    | GND          |
| MISO   | GPIO 9 (MISO)|
| MOSI   | GPIO 10 (MOSI)|
| SCK    | GPIO 11 (SCLK)|
| CS     | GPIO 8 (CE0) |
| RST    | GPIO 17      |
| HRDY   | GPIO 24      |

## Usage

```typescript
import { Rpi9In7 } from '@epaperjs/rpi-9in7';
import { Orientation, ColorMode } from '@epaperjs/core';

const display = new Rpi9In7(Orientation.Horizontal, ColorMode.Gray16);

display.connect();
display.clear();

// Display a PNG image
const imageBuffer = fs.readFileSync('image.png');
await display.displayPng(imageBuffer);

display.sleep();
display.disconnect();
```

## Color Modes

- `ColorMode.Black` - Black and white (1-bit)
- `ColorMode.Gray4` - 4 grayscale levels (2-bit)
- `ColorMode.Gray16` - 16 grayscale levels (4-bit)

## VCOM Value

The IT8951 requires a VCOM value that is specific to each display panel. This value is usually printed on a sticker on the flexible cable of the e-paper. You can set it using:

```typescript
const display = new Rpi9In7(Orientation.Horizontal, ColorMode.Gray16, -2.0);
```

The default value is -2.0V if not specified.
