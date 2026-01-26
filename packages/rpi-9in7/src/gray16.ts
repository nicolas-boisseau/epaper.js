import { PNG } from 'pngjs';

export type RGBAPixel = [r: number, g: number, b: number, a: number];

/**
 * Image options for Gray16 conversion
 */
export interface Gray16Options {
    rotate90Degrees?: boolean;
    rightToLeft?: boolean;
}

const defaultOptions: Required<Gray16Options> = {
    rotate90Degrees: false,
    rightToLeft: false,
};

/**
 * PNG Reader and converter for IT8951 displays
 * Supports 1bpp (B&W), 2bpp (4 gray), and 4bpp (16 gray) output
 */
export class Gray16 {
    private png?: PNG;
    private readonly inputPng: Buffer;

    constructor(pngInput: Buffer) {
        this.inputPng = pngInput;
    }

    private async parse(): Promise<PNG> {
        if (this.png) {
            return this.png;
        }

        return new Promise((resolve, reject) => {
            const png = new PNG({ filterType: -1 });
            png.parse(this.inputPng, (error, data) => {
                if (error) {
                    reject(error);
                } else {
                    this.png = data;
                    resolve(data);
                }
            });
        });
    }

    private getPixel(png: PNG, x: number, y: number): RGBAPixel {
        const idx = (png.width * y + x) << 2;
        return [png.data[idx], png.data[idx + 1], png.data[idx + 2], png.data[idx + 3]];
    }

    /**
     * Convert RGB pixel to grayscale (0-255)
     */
    private toGrayscale(pixel: RGBAPixel): number {
        const [r, g, b, a] = pixel;
        // Standard luminosity formula
        // If alpha is 0, treat as white
        if (a === 0) return 255;
        return Math.round(0.299 * r + 0.587 * g + 0.114 * b);
    }

    /**
     * Convert to 1bpp (black and white)
     * 8 pixels per byte, MSB first
     * @param options Conversion options
     * @returns Buffer with 1bpp packed data
     */
    public async to1bpp(options: Gray16Options = {}): Promise<Buffer> {
        const fullOpts = { ...defaultOptions, ...options };
        const png = await this.parse();
        const { height, width } = png;

        if (fullOpts.rotate90Degrees) {
            const devHeight = width;
            const devWidth = height;
            const bytesPerRow = Math.ceil(devWidth / 8);
            const outBuffer = Buffer.alloc(bytesPerRow * devHeight, 0xFF);

            for (let x = 0; x < width; x++) {
                for (let y = 0; y < height; y++) {
                    const outX = y;
                    const outY = devHeight - x - 1;
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    // Threshold at 128: < 128 = black (0), >= 128 = white (1)
                    if (gray < 128) {
                        const byteIdx = outY * bytesPerRow + Math.floor(outX / 8);
                        const bitIdx = 7 - (outX % 8);
                        outBuffer[byteIdx] &= ~(1 << bitIdx);
                    }
                }
            }
            return outBuffer;
        } else {
            const bytesPerRow = Math.ceil(width / 8);
            const outBuffer = Buffer.alloc(bytesPerRow * height, 0xFF);

            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    if (gray < 128) {
                        const byteIdx = y * bytesPerRow + Math.floor(x / 8);
                        const bitIdx = 7 - (x % 8);
                        outBuffer[byteIdx] &= ~(1 << bitIdx);
                    }
                }
            }
            return outBuffer;
        }
    }

    /**
     * Convert to 2bpp (4 gray levels)
     * 4 pixels per byte
     * Gray levels: 0 (black), 1, 2, 3 (white)
     * @param options Conversion options
     * @returns Buffer with 2bpp packed data
     */
    public async to2bpp(options: Gray16Options = {}): Promise<Buffer> {
        const fullOpts = { ...defaultOptions, ...options };
        const png = await this.parse();
        const { height, width } = png;

        if (fullOpts.rotate90Degrees) {
            const devHeight = width;
            const devWidth = height;
            const bytesPerRow = Math.ceil(devWidth / 4);
            const outBuffer = Buffer.alloc(bytesPerRow * devHeight, 0xFF);

            for (let x = 0; x < width; x++) {
                for (let y = 0; y < height; y++) {
                    const outX = y;
                    const outY = devHeight - x - 1;
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    // Convert 0-255 to 0-3 (2 bits)
                    const gray2bit = Math.floor(gray / 64);

                    const byteIdx = outY * bytesPerRow + Math.floor(outX / 4);
                    const pixelPos = 3 - (outX % 4);  // MSB first
                    const shift = pixelPos * 2;

                    // Clear the 2 bits and set new value
                    outBuffer[byteIdx] &= ~(0x03 << shift);
                    outBuffer[byteIdx] |= (gray2bit << shift);
                }
            }
            return outBuffer;
        } else {
            const bytesPerRow = Math.ceil(width / 4);
            const outBuffer = Buffer.alloc(bytesPerRow * height, 0xFF);

            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    // Convert 0-255 to 0-3 (2 bits)
                    const gray2bit = Math.floor(gray / 64);

                    const byteIdx = y * bytesPerRow + Math.floor(x / 4);
                    const pixelPos = 3 - (x % 4);  // MSB first
                    const shift = pixelPos * 2;

                    outBuffer[byteIdx] &= ~(0x03 << shift);
                    outBuffer[byteIdx] |= (gray2bit << shift);
                }
            }
            return outBuffer;
        }
    }

    /**
     * Convert to 4bpp (16 gray levels)
     * 2 pixels per byte
     * Gray levels: 0 (black) to 15 (white)
     * @param options Conversion options
     * @returns Buffer with 4bpp packed data
     */
    public async to4bpp(options: Gray16Options = {}): Promise<Buffer> {
        const fullOpts = { ...defaultOptions, ...options };
        const png = await this.parse();
        const { height, width } = png;

        if (fullOpts.rotate90Degrees) {
            const devHeight = width;
            const devWidth = height;
            const bytesPerRow = Math.ceil(devWidth / 2);
            const outBuffer = Buffer.alloc(bytesPerRow * devHeight, 0xFF);

            for (let x = 0; x < width; x++) {
                for (let y = 0; y < height; y++) {
                    const outX = y;
                    const outY = devHeight - x - 1;
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    // Convert 0-255 to 0-15 (4 bits)
                    const gray4bit = Math.floor(gray / 16);

                    const byteIdx = outY * bytesPerRow + Math.floor(outX / 2);

                    if (outX % 2 === 0) {
                        // High nibble (first pixel)
                        outBuffer[byteIdx] = (gray4bit << 4) | (outBuffer[byteIdx] & 0x0F);
                    } else {
                        // Low nibble (second pixel)
                        outBuffer[byteIdx] = (outBuffer[byteIdx] & 0xF0) | gray4bit;
                    }
                }
            }
            return outBuffer;
        } else {
            const bytesPerRow = Math.ceil(width / 2);
            const outBuffer = Buffer.alloc(bytesPerRow * height, 0xFF);

            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    const gray = this.toGrayscale(this.getPixel(png, x, y));

                    // Convert 0-255 to 0-15 (4 bits)
                    const gray4bit = Math.floor(gray / 16);

                    const byteIdx = y * bytesPerRow + Math.floor(x / 2);

                    if (x % 2 === 0) {
                        // High nibble (first pixel)
                        outBuffer[byteIdx] = (gray4bit << 4) | (outBuffer[byteIdx] & 0x0F);
                    } else {
                        // Low nibble (second pixel)
                        outBuffer[byteIdx] = (outBuffer[byteIdx] & 0xF0) | gray4bit;
                    }
                }
            }
            return outBuffer;
        }
    }
}
