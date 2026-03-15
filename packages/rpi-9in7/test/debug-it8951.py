#!/usr/bin/env python3
"""
Diagnostic script for Waveshare IT8951 e-Paper display.
Tests SPI communication and GPIO independently of Node.js.

Usage:
  sudo python3 test/debug-it8951.py

Requirements:
  sudo apt install python3-spidev python3-rpi.gpio

This script uses sysfs for CS (GPIO 8) and RPi.GPIO for RST/HRDY only,
to avoid the lgpio "GPIO busy" conflict that occurs when spi.open(0,0)
claims CE0 before RPi.GPIO can.
"""

import time
import sys
import os

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
CS_PIN   = 8    # CE0 — we drive this via sysfs to avoid lgpio busy conflict

# --------------------------------------------------------------------------
# Sysfs GPIO helpers for CS_PIN
# (RPi.GPIO / lgpio can't claim GPIO 8 after spi.open() claims it,
#  but sysfs /sys/class/gpio writes bypass that conflict.)
# --------------------------------------------------------------------------
def sysfs_export(pin):
    if not os.path.exists(f'/sys/class/gpio/gpio{pin}'):
        try:
            with open('/sys/class/gpio/export', 'w') as f:
                f.write(str(pin))
            time.sleep(0.1)
        except OSError:
            pass  # already exported
    try:
        with open(f'/sys/class/gpio/gpio{pin}/direction', 'w') as f:
            f.write('out')
    except OSError as e:
        print(f"  sysfs export GPIO{pin} failed: {e}")
        print("  --> The SPI driver may own this pin; trying to continue anyway.")

def sysfs_write(pin, val):
    try:
        with open(f'/sys/class/gpio/gpio{pin}/value', 'w') as f:
            f.write('1' if val else '0')
    except OSError:
        pass  # if it fails, kernel CS will handle it

def sysfs_unexport(pin):
    try:
        with open('/sys/class/gpio/unexport', 'w') as f:
            f.write(str(pin))
    except OSError:
        pass

# --------------------------------------------------------------------------
# Setup
# --------------------------------------------------------------------------
GPIO.setmode(GPIO.BCM)
GPIO.setup(RST_PIN,  GPIO.OUT, initial=GPIO.HIGH)
GPIO.setup(HRDY_PIN, GPIO.IN)

# Open SPI — kernel claims CE0 here
spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 1000000
spi.mode = 0
spi.no_cs = True   # Tell kernel not to touch CE0 automatically

# Now set up CS via sysfs (after spi.open, so we know the kernel has
# already initialised the pin — sysfs access should still work)
sysfs_export(CS_PIN)
sysfs_write(CS_PIN, 1)   # CS idle HIGH

print("=" * 60)
print("  IT8951 Hardware Diagnostic")
print("=" * 60)
print("  CS control: sysfs GPIO 8 (no_cs=True, manual CS)")
print("  SPI speed : 1 MHz")

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

print("    Waiting for HRDY HIGH after reset (max 5s)...")
t0 = time.time()
while GPIO.input(HRDY_PIN) == 0:
    if time.time() - t0 > 5:
        print("    TIMEOUT: HRDY never went HIGH!")
        GPIO.cleanup()
        spi.close()
        sys.exit(1)
    time.sleep(0.01)

elapsed = time.time() - t0
print(f"    HRDY went HIGH after {elapsed*1000:.0f} ms  ✓")

# --------------------------------------------------------------------------
# Helper: wait for HRDY (input only, no CS involved)
# --------------------------------------------------------------------------
def wait_hrdy(timeout=3.0):
    t0 = time.time()
    while GPIO.input(HRDY_PIN) == 0:
        if time.time() - t0 > timeout:
            print("      HRDY timeout!")
            return False
        time.sleep(0.001)
    return True

# --------------------------------------------------------------------------
# IT8951 SPI protocol (Waveshare reference implementation)
#
# Each transaction:
#   CS LOW  →  wait HRDY  →  xfer preamble (2 bytes)
#           →  wait HRDY  →  xfer command/data (2 bytes)  →  CS HIGH
#
# The TWO separate xfer2 calls with HRDY check between them is how
# Waveshare's own Python demos work.  CS must be held low across both,
# so we drive it manually via sysfs with no_cs=True on spidev.
# --------------------------------------------------------------------------
def write_command(cmd):
    wait_hrdy()
    sysfs_write(CS_PIN, 0)
    spi.xfer2([0x60, 0x00])
    wait_hrdy()
    spi.xfer2([(cmd >> 8) & 0xFF, cmd & 0xFF])
    sysfs_write(CS_PIN, 1)

def write_data(data):
    wait_hrdy()
    sysfs_write(CS_PIN, 0)
    spi.xfer2([0x00, 0x00])
    wait_hrdy()
    spi.xfer2([(data >> 8) & 0xFF, data & 0xFF])
    sysfs_write(CS_PIN, 1)

def read_data():
    wait_hrdy()
    sysfs_write(CS_PIN, 0)
    spi.xfer2([0x10, 0x00])
    wait_hrdy()
    spi.xfer2([0x00, 0x00])   # dummy
    wait_hrdy()
    rx = spi.xfer2([0x00, 0x00])
    sysfs_write(CS_PIN, 1)
    return (rx[0] << 8) | rx[1]

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
        print("    Most likely cause: MISO not wired or sysfs CS not working.")
        print("    Check ALL of these:")
        print("      1. MISO  (Pi pin 21 / GPIO 9)  → IT8951 HAT SDO/MISO")
        print("      2. MOSI  (Pi pin 19 / GPIO 10) → IT8951 HAT SDI/MOSI")
        print("      3. SCLK  (Pi pin 23 / GPIO 11) → IT8951 HAT SCK/CLK")
        print("      4. CS    (Pi pin 24 / GPIO 8)  → IT8951 HAT CS")
        print("      5. 5V    (Pi pin 2 or 4)       → IT8951 HAT VCC")
        print("      6. GND   (Pi pin 6)            → IT8951 HAT GND")
        print("    Quick MISO test: run 'gpio readall' and check GPIO 9 state.")
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
print("\n[6] Hardware loopback test (short Pi pin 19 MOSI ↔ pin 21 MISO, then run)...")
try:
    sysfs_write(CS_PIN, 0)
    resp = spi.xfer2([0xAB, 0xCD])
    sysfs_write(CS_PIN, 1)
    print(f"    Sent: [0xAB, 0xCD]  Received: {[hex(b) for b in resp]}")
    if resp == [0xAB, 0xCD]:
        print("    ✓ Loopback works — SPI bus OK, IT8951 MISO is the issue")
    elif all(b == 0 for b in resp):
        print("    ✗ All zeros — either no loopback wire, or MISO pin broken")
        print("      Short Pi pin 19 (MOSI) to pin 21 (MISO) and re-run to test SPI bus.")
    else:
        print(f"    Got {[hex(b) for b in resp]} — partial signal")
except Exception as e:
    print(f"    FAILED: {e}")

# --------------------------------------------------------------------------
# Cleanup
# --------------------------------------------------------------------------
sysfs_unexport(CS_PIN)
GPIO.cleanup()
spi.close()
print("\n" + "=" * 60)
print("  Diagnostic complete")
print("=" * 60)
print()
print("If Panel 0x0: check MISO wiring first (Pi pin 21 → HAT SDO).")
print("If loopback test also returns zeros: SPI not enabled (sudo raspi-config → Interfaces → SPI).")
print("If loopback works but Panel 0x0: MISO from IT8951 HAT not reaching Pi pin 21.")
