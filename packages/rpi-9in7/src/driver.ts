/**
 * IT8951 Driver Interface for Waveshare 9.7" e-Paper Display
 * 
 * The IT8951 is a high-performance controller that supports displays up to
 * 2048x2048 resolution with 16 grayscale levels.
 */

export interface IT8951DeviceInfo {
    width: number;
    height: number;
    imgBufAddrL: number;
    imgBufAddrH: number;
    fwVersion: string;
    lutVersion: string;
}

export interface Driver {
    /**
     * Initialize the hardware (SPI, GPIO)
     */
    dev_init(): number;

    /**
     * Initialize the IT8951 controller
     * @param vcom The VCOM voltage value (negative, e.g., -2.0)
     * @returns Device info containing width, height, and buffer addresses
     */
    init(vcom: number): IT8951DeviceInfo;

    /**
     * Display image in 1-bit black/white mode (fastest)
     * @param buffer Image buffer (1 bit per pixel, packed)
     */
    display_1bpp(buffer: Buffer): void;

    /**
     * Display image in 2-bit grayscale mode (4 levels)
     * @param buffer Image buffer (2 bits per pixel, packed)
     */
    display_2bpp(buffer: Buffer): void;

    /**
     * Display image in 4-bit grayscale mode (16 levels)
     * @param buffer Image buffer (4 bits per pixel, packed)
     */
    display_4bpp(buffer: Buffer): void;

    /**
     * Display image with auto mode detection
     * @param buffer Image buffer
     * @param bpp Bits per pixel (1, 2, or 4)
     */
    display(buffer: Buffer, bpp: number): void;

    /**
     * Clear the display to white
     */
    clear(): void;

    /**
     * Put the display into sleep mode
     */
    sleep(): void;

    /**
     * Exit and cleanup hardware resources
     */
    dev_exit(): void;

    /**
     * Get the device information
     */
    get_device_info(): IT8951DeviceInfo;

    /**
     * Set the VCOM voltage
     * @param vcom VCOM voltage (negative value, e.g., -2.0)
     */
    set_vcom(vcom: number): void;

    /**
     * Get the current VCOM voltage
     * @returns VCOM voltage as negative float
     */
    get_vcom(): number;
}
