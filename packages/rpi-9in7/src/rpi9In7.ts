import { ColorMode, DisplayDevice, Orientation } from '@epaperjs/core';
import { ImageOptions } from '@epaperjs/core/src/image/imageOptions';
import bindings from 'bindings';
import { Driver, IT8951DeviceInfo } from './driver';
import { Gray16 } from './gray16';

/**
 * Waveshare 9.7" e-Paper display with IT8951 controller
 * 
 * Specifications:
 * - Resolution: 1200 × 825
 * - Grayscale: 2-16 levels (1-4 bit)
 * - Interface: SPI
 * - Controller: IT8951
 */
export class Rpi9In7 implements DisplayDevice {
    public readonly height: number;
    public readonly width: number;
    private readonly driver: Driver;
    private deviceInfo: IT8951DeviceInfo | null = null;

    /**
     * Create a new Rpi9In7 display instance
     * @param orientation Display orientation (horizontal or vertical)
     * @param colorMode Color mode (Black, Gray4, or Gray16)
     * @param vcom VCOM voltage value (negative, e.g., -2.0). Check your display's label.
     */
    constructor(
        public readonly orientation: Orientation = Orientation.Horizontal,
        public readonly colorMode: ColorMode = ColorMode.Gray16,
        private readonly vcom: number = -2.0
    ) {
        const supportedColorModes = [ColorMode.Black, ColorMode.Gray4, ColorMode.Gray16];
        if (!supportedColorModes.includes(colorMode)) {
            throw new Error(`Only color modes: [${supportedColorModes}] are supported`);
        }

        this.driver = bindings('waveshare9in7.node');

        // Native resolution: 1200 × 825
        this.height = this.orientation === Orientation.Horizontal ? 825 : 1200;
        this.width = this.orientation === Orientation.Horizontal ? 1200 : 825;
    }

    /**
     * Connect to the display and initialize
     */
    public connect(): void {
        this.driver.dev_init();
        this.wake();
    }

    /**
     * Disconnect from the display
     */
    public disconnect(): void {
        this.sleep();
        this.driver.dev_exit();
    }

    /**
     * Wake up the display and initialize
     */
    public wake(): void {
        this.deviceInfo = this.driver.init(this.vcom);
        console.log(`IT8951 initialized: ${this.deviceInfo.width}x${this.deviceInfo.height}`);
        console.log(`Firmware: ${this.deviceInfo.fwVersion}`);
        console.log(`LUT: ${this.deviceInfo.lutVersion}`);
    }

    /**
     * Clear the display to white
     */
    public clear(): void {
        this.driver.clear();
    }

    /**
     * Put the display to sleep
     */
    public sleep(): void {
        this.driver.sleep();
    }

    /**
     * Display a PNG image on the e-paper
     * @param img PNG image buffer
     * @param options Display options
     */
    public async displayPng(img: Buffer, options?: ImageOptions): Promise<void> {
        switch (this.colorMode) {
            case ColorMode.Gray16:
                await this.displayPngGray16(img, options);
                break;
            case ColorMode.Gray4:
                await this.displayPngGray4(img, options);
                break;
            case ColorMode.Black:
            default:
                await this.displayPngBW(img, options);
                break;
        }
    }

    /**
     * Display black and white image (1bpp)
     */
    private async displayPngBW(img: Buffer, options?: ImageOptions): Promise<void> {
        const converter = new Gray16(img);
        const bwBuffer = await converter.to1bpp({
            ...options,
            rotate90Degrees: this.orientation === Orientation.Vertical,
        });
        this.driver.display_1bpp(bwBuffer);
    }

    /**
     * Display 4 gray level image (2bpp)
     */
    private async displayPngGray4(img: Buffer, options?: ImageOptions): Promise<void> {
        const converter = new Gray16(img);
        const grayBuffer = await converter.to2bpp({
            ...options,
            rotate90Degrees: this.orientation === Orientation.Vertical,
        });
        this.driver.display_2bpp(grayBuffer);
    }

    /**
     * Display 16 gray level image (4bpp)
     */
    private async displayPngGray16(img: Buffer, options?: ImageOptions): Promise<void> {
        const converter = new Gray16(img);
        const grayBuffer = await converter.to4bpp({
            ...options,
            rotate90Degrees: this.orientation === Orientation.Vertical,
        });
        this.driver.display_4bpp(grayBuffer);
    }

    /**
     * Get the device information from IT8951
     */
    public getDeviceInfo(): IT8951DeviceInfo | null {
        return this.deviceInfo;
    }

    /**
     * Set the VCOM voltage
     * @param vcom VCOM voltage (negative value, e.g., -2.0)
     */
    public setVCOM(vcom: number): void {
        this.driver.set_vcom(vcom);
    }

    /**
     * Get the current VCOM voltage
     * @returns VCOM voltage as negative float
     */
    public getVCOM(): number {
        return this.driver.get_vcom();
    }
}
