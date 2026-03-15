#!/usr/bin/env python3
"""
Diagnostic script for Waveshare IT8951 e-Paper display.
Tests SPI communication and GPIO independently of Node.js.

Usage:
  sudo python3 test/debug-it8951.py

Requirements:
  sudo apt install python3-spidev python3-rpi.gpio
"""

import time
import sys

try:
    import spidev
    import RPi.GPIO as GPIO
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with:  sudo apt install python3-spidev python3-rpi.gpio")
    sys.exit(1)

# --------------------------------------------------------------------------
# GPIO pins (BCM numbering)
# --------------------------------------------------------------------------
RST_PIN  = 17
HRDY_PIN = 24

# --------------------------------------------------------------------------
# Setup
# --------------------------------------------------------------------------
GPIO.setmode(GPIO.BCM)
GPIO.setup(RST_PIN,  GPIO.OUT, initial=GPIO.HIGH)
GPIO.setup(HRDY_PIN, GPIO.IN)

spi = spidev.SpiDev()
spi.open(0, 0)            # /dev/spidev0.0  (CE0 = GPIO 8)
spi.max_speed_hz = 1000000  # 1 MHz, very conservative
spi.mode = 0
spi.no_cs = True          # We manage CS manually via the CE0 line in spidev

print("=" * 60)
print("  IT8951 Hardware Diagnostic")
print("=" * 60)

# --------------------------------------------------------------------------
# Step 1: Check HRDY without reset
# --------------------------------------------------------------------------
print("\n[1] HRDY pin (GPIO 24) before reset:", GPIO.input(HRDY_PIN))

# --------------------------------------------------------------------------
# Step 2: Hardware reset
# --------------------------------------------------------------------------
print("\n[2] Performing hardware reset...")
GPIO.output(RST_PIN, GPIO.HIGH)
time.sleep(0.2)
GPIO.output(RST_PIN, GPIO.LOW)
time.sleep(0.02)
GPIO.output(RST_PIN, GPIO.HIGH)
time.sleep(0.5)

# Wait for HRDY
print("    Waiting for HRDY HIGH after reset (max 5s)...")
t0 = time.time()
while GPIO.input(HRDY_PIN) == 0:
    if time.time() - t0 > 5:
        print("    TIMEOUT: HRDY never went HIGH!")
        print("    --> Check wiring: HRDY should connect to GPIO 24")
        GPIO.cleanup()
        spi.close()
        sys.exit(1)
    time.sleep(0.01)

elapsed = time.time() - t0
print(f"    HRDY went HIGH after {elapsed*1000:.0f} ms  ✓")

# --------------------------------------------------------------------------
# Helper: wait for HRDY
# --------------------------------------------------------------------------
def wait_hrdy(timeout=3.0):
    t0 = time.time()
    while GPIO.input(HRDY_PIN) == 0:
        if time.time() - t0 > timeout:
            return False
        time.sleep(0.001)
    return True

# --------------------------------------------------------------------------
# Helper: IT8951 write command (manual CS via spi.no_cs + CE0 toggling)
# IT8951 SPI protocol:
#   Assert CS  → send 0x6000 (preamble) → wait HRDY → send command → deassert CS
# With spi.no_cs=True, CE0 is NOT toggled by the kernel.
# We use xfer2 which does NOT deassert CS between bytes (on most Pi kernels).
# --------------------------------------------------------------------------
def write_command(cmd):
    preamble = [0x60, 0x00]
    wait_hrdy()
    spi.xfer2(preamble)
    wait_hrdy()
    spi.xfer2([(cmd >> 8) & 0xFF, cmd & 0xFF])

def write_data(data):
    preamble = [0x00, 0x00]
    wait_hrdy()
    spi.xfer2(preamble)
    wait_hrdy()
    spi.xfer2([(data >> 8) & 0xFF, data & 0xFF])

def read_data():
    preamble = [0x10, 0x00]
    wait_hrdy()
    spi.xfer2(preamble)
    wait_hrdy()
    dummy = spi.xfer2([0x00, 0x00])   # dummy read
    wait_hrdy()
    data = spi.xfer2([0x00, 0x00])
    return (data[0] << 8) | data[1]

# --------------------------------------------------------------------------
# Step 3: IT8951_SYS_RUN (0x0001)
# --------------------------------------------------------------------------
print("\n[3] Sending SYS_RUN command (0x0001)...")
try:
    write_command(0x0001)
    time.sleep(0.1)
    print("    SYS_RUN sent  ✓")
except Exception as e:
    print(f"    FAILED: {e}")

# --------------------------------------------------------------------------
# Step 4: Get Device Info (0x0302)
# --------------------------------------------------------------------------
print("\n[4] Requesting Device Info (0x0302)...")
try:
    write_command(0x0302)
    words = []
    for i in range(20):
        w = read_data()
        words.append(w)

    panel_w = words[0]
    panel_h = words[1]
    buf_addr_l = words[2]
    buf_addr_h = words[3]
    fw = bytes([words[4] & 0xFF, (words[4] >> 8) & 0xFF,
                words[5] & 0xFF, (words[5] >> 8) & 0xFF,
                words[6] & 0xFF, (words[6] >> 8) & 0xFF,
                words[7] & 0xFF, (words[7] >> 8) & 0xFF]).decode('ascii', errors='?')

    print(f"\n    Panel size   : {panel_w} x {panel_h}")
    print(f"    Img buf addr : 0x{(buf_addr_h << 16 | buf_addr_l):08X}")
    print(f"    FW version   : {fw}")
    print(f"    Raw words    : {[hex(w) for w in words[:8]]}")

    if panel_w == 0 and panel_h == 0:
        print("\n    *** Panel 0x0 → IT8951 not responding ***")
        print("    Possible causes:")
        print("      - SPI wiring issue (MOSI/MISO/SCLK)")
        print("      - 5V power not supplied to IT8951 board")
        print("      - HRDY wired to wrong pin")
    else:
        print("\n    IT8951 communication OK  ✓")

except Exception as e:
    print(f"    FAILED: {e}")

# --------------------------------------------------------------------------
# Step 5: VCOM read
# --------------------------------------------------------------------------
print("\n[5] Reading VCOM value...")
try:
    write_command(0x0039)
    write_data(0)
    vcom_raw = read_data()
    vcom = -(vcom_raw / 1000.0)
    print(f"    VCOM = {vcom:.3f} V  (raw: {vcom_raw})")
    if vcom_raw == 0:
        print("    *** VCOM = 0 → communication not working ***")
except Exception as e:
    print(f"    FAILED: {e}")

# --------------------------------------------------------------------------
# Step 6: Raw SPI loopback test (no IT8951 — just checks SPI wiring)
# --------------------------------------------------------------------------
print("\n[6] Raw SPI echo test (send 0xAB 0xCD, expect echo if MISO connected)...")
try:
    resp = spi.xfer2([0xAB, 0xCD])
    print(f"    Sent: [0xAB, 0xCD]  Received: {[hex(b) for b in resp]}")
    if resp == [0xAB, 0xCD]:
        print("    Perfect loopback (MISO=MOSI shorted?)")
    elif resp == [0x00, 0x00]:
        print("    All zeros — MISO not connected or IT8951 not driving MISO")
    else:
        print("    Got non-zero, non-echo data — some communication happening")
except Exception as e:
    print(f"    FAILED: {e}")

# --------------------------------------------------------------------------
# Cleanup
# --------------------------------------------------------------------------
GPIO.cleanup()
spi.close()
print("\n" + "=" * 60)
print("  Diagnostic complete")
print("=" * 60)
