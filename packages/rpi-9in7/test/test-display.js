/**
 * Test script for Waveshare 9.7" IT8951 e-Paper display
 * 
 * Usage:
 *   node test/test-display.js [mode]
 * 
 * Modes:
 *   clear    - Just clear the display (default)
 *   bw       - Display a black & white test pattern
 *   gray4    - Display a 4-level grayscale test pattern
 *   gray16   - Display a 16-level grayscale test pattern (default)
 *   png      - Display a PNG file (pass path as 3rd argument)
 * 
 * Examples:
 *   node test/test-display.js clear
 *   node test/test-display.js gray16
 *   node test/test-display.js png /path/to/image.png
 */

const { createCanvas } = require('canvas');
const fs = require('fs');
const path = require('path');

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

const VCOM = -1.84;  // Your display VCOM value
const MODE = process.argv[2] || 'gray16';
const PNG_PATH = process.argv[3];

// --- Canvas-based test pattern generators ---

function generateBWPattern(width, height) {
    const { createCanvas } = requireCanvas();
    const canvas = createCanvas(width, height);
    const ctx = canvas.getContext('2d');

    // White background
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, width, height);

    // Black stripes
    ctx.fillStyle = 'black';
    for (let x = 0; x < width; x += 40) {
        ctx.fillRect(x, 0, 20, height);
    }

    // Text
    ctx.fillStyle = 'black';
    ctx.font = 'bold 60px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('ePaper.js - B&W Test', width / 2, height / 2);

    return canvas.toBuffer('image/png');
}

function generateGray4Pattern(width, height) {
    const canvas = requireCanvas().createCanvas(width, height);
    const ctx = canvas.getContext('2d');

    const levels = [0, 85, 170, 255];
    const blockWidth = Math.floor(width / 4);

    for (let i = 0; i < 4; i++) {
        const v = levels[i];
        ctx.fillStyle = `rgb(${v},${v},${v})`;
        ctx.fillRect(i * blockWidth, 0, blockWidth, height);
    }

    ctx.fillStyle = levels[0] < 128 ? 'white' : 'black';
    ctx.font = 'bold 50px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('ePaper.js - 4 Gray Levels', width / 2, height / 2);

    return canvas.toBuffer('image/png');
}

function generateGray16Pattern(width, height) {
    const canvas = requireCanvas().createCanvas(width, height);
    const ctx = canvas.getContext('2d');

    const blockWidth = Math.floor(width / 16);

    // Draw 16 gray level bars
    for (let i = 0; i < 16; i++) {
        const v = Math.round((i / 15) * 255);
        ctx.fillStyle = `rgb(${v},${v},${v})`;
        ctx.fillRect(i * blockWidth, 0, blockWidth, height);
    }

    // Labels
    for (let i = 0; i < 16; i++) {
        const v = Math.round((i / 15) * 255);
        ctx.fillStyle = v < 128 ? 'white' : 'black';
        ctx.font = 'bold 28px sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText(String(i), i * blockWidth + blockWidth / 2, height / 2);
    }

    ctx.fillStyle = 'black';
    ctx.font = 'bold 40px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('ePaper.js - 16 Gray Levels', width / 2, height - 40);

    return canvas.toBuffer('image/png');
}

function requireCanvas() {
    try {
        return require('canvas');
    } catch (e) {
        console.error('"canvas" module not found. Install it with:');
        console.error('  sudo apt install libcairo2-dev libpango1.0-dev libjpeg-dev libgif-dev librsvg2-dev');
        console.error('  npm install canvas');
        process.exit(1);
    }
}

// --- Main ---

async function main() {
    const display = new Rpi9In7(Orientation.Horizontal, getModeColorMode(), VCOM);

    console.log(`Connecting to display (VCOM=${VCOM}, mode=${MODE})...`);
    display.connect();

    console.log('Clearing display...');
    display.clear();

    if (MODE === 'clear') {
        console.log('Clear done.');
    } else if (MODE === 'png') {
        if (!PNG_PATH || !fs.existsSync(PNG_PATH)) {
            console.error(`PNG file not found: ${PNG_PATH}`);
            console.error('Usage: node test/test-display.js png /path/to/image.png');
            process.exit(1);
        }
        console.log(`Displaying PNG: ${PNG_PATH}`);
        const img = fs.readFileSync(PNG_PATH);
        await display.displayPng(img);
        console.log('Done.');
    } else {
        console.log(`Generating ${MODE} test pattern (1200x825)...`);
        let imgBuffer;
        if (MODE === 'bw') imgBuffer = generateBWPattern(1200, 825);
        if (MODE === 'gray4') imgBuffer = generateGray4Pattern(1200, 825);
        if (MODE === 'gray16') imgBuffer = generateGray16Pattern(1200, 825);

        console.log('Sending image to display...');
        await display.displayPng(imgBuffer);
        console.log('Done.');
    }

    console.log('Going to sleep...');
    display.sleep();
    display.disconnect();
}

function getModeColorMode() {
    switch (MODE) {
        case 'bw': return ColorMode.Black;
        case 'gray4': return ColorMode.Gray4;
        case 'png':
        case 'gray16':
        default: return ColorMode.Gray16;
    }
}

main().catch(err => {
    console.error('Error:', err);
    process.exit(1);
});
