/**
 * Test script for Waveshare 9.7" IT8951 e-Paper display
 * Uses only pngjs (already a dependency) — no canvas required.
 *
 * Usage:
 *   node test/test-display.js [mode]
 *
 * Modes:
 *   clear    - Just clear the display
 *   bw       - Black & white test pattern
 *   gray4    - 4-level grayscale test pattern
 *   gray16   - 16-level grayscale test pattern (default)
 *   png      - Display a PNG file (pass path as 3rd argument)
 *
 * Examples:
 *   node test/test-display.js clear
 *   node test/test-display.js gray16
 *   node test/test-display.js png /path/to/image.png
 */

'use strict';

const fs = require('fs');
const { PNG } = require('pngjs');

// Dynamically require the compiled module
let Rpi9In7, Orientation, ColorMode;
try {
    ({ Rpi9In7 } = require('../lib/rpi9In7'));
    ({ Orientation, ColorMode } = require('../../core/lib/device/displayDevice'));
} catch (e) {
    console.error('Failed to load modules. Make sure you ran "pnpm build" from the repo root.');
    console.error(e.message);
    process.exit(1);
}

const VCOM = -1.84;
const WIDTH = 1200;
const HEIGHT = 825;
const MODE = process.argv[2] || 'gray16';
const PNG_PATH = process.argv[3];

// ---------------------------------------------------------------------------
// PNG generators using pngjs (pure JS, no native deps)
// ---------------------------------------------------------------------------

/**
 * Create a PNG buffer from a pixel-fill callback: (x, y) => { r, g, b }
 */
function makePNG(width, height, fillFn) {
    const png = new PNG({ width, height, filterType: -1 });
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const idx = (width * y + x) * 4;
            const { r, g, b } = fillFn(x, y);
            png.data[idx] = r;
            png.data[idx + 1] = g;
            png.data[idx + 2] = b;
            png.data[idx + 3] = 255;
        }
    }
    return PNG.sync.write(png);
}

/** Black & white checkerboard + horizontal black bar in the middle */
function generateBWPattern(w, h) {
    return makePNG(w, h, (x, y) => {
        if (y > h / 3 && y < (2 * h) / 3) {
            return { r: 0, g: 0, b: 0 };
        }
        const v = (Math.floor(x / 60) + Math.floor(y / 60)) % 2 === 0 ? 255 : 0;
        return { r: v, g: v, b: v };
    });
}

/** 4 vertical bands: black, dark gray, light gray, white */
function generateGray4Pattern(w, h) {
    const levels = [0, 85, 170, 255];
    return makePNG(w, h, (x) => {
        const band = Math.min(Math.floor((x / w) * 4), 3);
        const v = levels[band];
        return { r: v, g: v, b: v };
    });
}

/** 16 vertical bands from black (left) to white (right) */
function generateGray16Pattern(w, h) {
    return makePNG(w, h, (x) => {
        const band = Math.min(Math.floor((x / w) * 16), 15);
        const v = Math.round((band / 15) * 255);
        return { r: v, g: v, b: v };
    });
}

// --- Main ---

async function main() {
    const display = new Rpi9In7(Orientation.Horizontal, getModeColorMode(), VCOM);

    console.log(`\n=== Waveshare 9.7" IT8951 test ===`);
    console.log(`Mode: ${MODE}  |  VCOM: ${VCOM}\n`);

    console.log('Connecting...');
    display.connect();

    console.log('Clearing display...');
    display.clear();

    if (MODE === 'clear') {
        console.log('Clear done.');
    } else if (MODE === 'png') {
        if (!PNG_PATH || !fs.existsSync(PNG_PATH)) {
            console.error(`PNG file not found: ${PNG_PATH || '(no path given)'}`);
            console.error('Usage: node test/test-display.js png /path/to/image.png');
            process.exit(1);
        }
        console.log(`Displaying: ${PNG_PATH}`);
        const img = fs.readFileSync(PNG_PATH);
        await display.displayPng(img);
        console.log('Done.');
    } else {
        console.log(`Generating ${MODE} test pattern (${WIDTH}x${HEIGHT})...`);
        let imgBuffer;
        if (MODE === 'bw') imgBuffer = generateBWPattern(WIDTH, HEIGHT);
        else if (MODE === 'gray4') imgBuffer = generateGray4Pattern(WIDTH, HEIGHT);
        else imgBuffer = generateGray16Pattern(WIDTH, HEIGHT);

        console.log('Sending to display...');
        await display.displayPng(imgBuffer);
        console.log('Done.');
    }

    console.log('Sleeping...');
    display.sleep();
    display.disconnect();
    console.log('Disconnected.\n');
}

function getModeColorMode() {
    switch (MODE) {
        case 'bw': return ColorMode.Black;
        case 'gray4': return ColorMode.Gray4;
        default: return ColorMode.Gray16;
    }
}

main().catch(err => {
    console.error('Error:', err);
    process.exit(1);
});
