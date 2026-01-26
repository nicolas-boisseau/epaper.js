/*****************************************************************************
* | File        :   IT8951_node.cc
* | Author      :   Waveshare team / ePaper.js
* | Function    :   Node.js N-API bindings for IT8951 controller
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2026-01-26
* | Info        :   N-API bindings for 9.7" e-Paper display
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
#include "napi.h"
extern "C" {
    #include "DEV_Config.h"
    #include "IT8951.h"
}

// Global device info storage
static IT8951DevInfo g_DevInfo;
static bool g_Initialized = false;

/**
 * Initialize the device hardware (SPI, GPIO)
 */
Napi::Number DEV_Init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    uint8_t result = DEV_Module_Init();
    return Napi::Number::New(env, result);
}

/**
 * Initialize the IT8951 controller
 * @param vcom - VCOM voltage value (float, e.g., -2.0)
 * @returns Object with device info
 */
Napi::Object Init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // Get VCOM parameter (default -2.0V if not specified)
    float vcom = -2.0f;
    if (info.Length() > 0 && info[0].IsNumber()) {
        vcom = info[0].As<Napi::Number>().FloatValue();
    }
    
    // Initialize IT8951
    g_DevInfo = IT8951_Init();
    g_Initialized = true;
    
    // Set VCOM (convert float to UWORD, e.g., -2.0 -> 2000)
    if (vcom < 0) {
        UWORD vcomValue = (UWORD)(-vcom * 1000);
        IT8951_SetVCOM(vcomValue);
    }
    
    // Create result object
    Napi::Object result = Napi::Object::New(env);
    result.Set("width", Napi::Number::New(env, g_DevInfo.usPanelW));
    result.Set("height", Napi::Number::New(env, g_DevInfo.usPanelH));
    result.Set("imgBufAddrL", Napi::Number::New(env, g_DevInfo.usImgBufAddrL));
    result.Set("imgBufAddrH", Napi::Number::New(env, g_DevInfo.usImgBufAddrH));
    result.Set("fwVersion", Napi::String::New(env, g_DevInfo.usFWVersion));
    result.Set("lutVersion", Napi::String::New(env, g_DevInfo.usLUTVersion));
    
    return result;
}

/**
 * Display 1bpp (black and white) image
 */
Napi::Value Display_1bpp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    Napi::Buffer<uint8_t> jsBuffer = info[0].As<Napi::Buffer<uint8_t>>();
    IT8951_Display_Full_1bpp(&g_DevInfo, jsBuffer.Data());
    IT8951_WaitForDisplayReady();
    
    return env.Undefined();
}

/**
 * Display 2bpp (4 gray levels) image
 */
Napi::Value Display_2bpp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    Napi::Buffer<uint8_t> jsBuffer = info[0].As<Napi::Buffer<uint8_t>>();
    IT8951_Display_Full_2bpp(&g_DevInfo, jsBuffer.Data());
    IT8951_WaitForDisplayReady();
    
    return env.Undefined();
}

/**
 * Display 4bpp (16 gray levels) image
 */
Napi::Value Display_4bpp(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    Napi::Buffer<uint8_t> jsBuffer = info[0].As<Napi::Buffer<uint8_t>>();
    IT8951_Display_Full_4bpp(&g_DevInfo, jsBuffer.Data());
    IT8951_WaitForDisplayReady();
    
    return env.Undefined();
}

/**
 * Display image with specified bits per pixel
 * @param buffer - Image data
 * @param bpp - Bits per pixel (1, 2, or 4)
 */
Napi::Value Display(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Buffer and bpp number expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    Napi::Buffer<uint8_t> jsBuffer = info[0].As<Napi::Buffer<uint8_t>>();
    int bpp = info[1].As<Napi::Number>().Int32Value();
    
    switch (bpp) {
        case 1:
            IT8951_Display_Full_1bpp(&g_DevInfo, jsBuffer.Data());
            break;
        case 2:
            IT8951_Display_Full_2bpp(&g_DevInfo, jsBuffer.Data());
            break;
        case 4:
            IT8951_Display_Full_4bpp(&g_DevInfo, jsBuffer.Data());
            break;
        default:
            Napi::Error::New(env, "Invalid bpp value. Must be 1, 2, or 4").ThrowAsJavaScriptException();
            return env.Undefined();
    }
    
    IT8951_WaitForDisplayReady();
    return env.Undefined();
}

/**
 * Clear the display
 */
Napi::Value Clear(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    IT8951_Clear();
    return env.Undefined();
}

/**
 * Put display to sleep
 */
Napi::Value Sleep(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    IT8951_Sleep();
    return env.Undefined();
}

/**
 * Exit and cleanup hardware resources
 */
Napi::Value DEV_Exit(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    DEV_Module_Exit();
    g_Initialized = false;
    return env.Undefined();
}

/**
 * Get device information
 */
Napi::Object GetDeviceInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    Napi::Object result = Napi::Object::New(env);
    
    if (g_Initialized) {
        result.Set("width", Napi::Number::New(env, g_DevInfo.usPanelW));
        result.Set("height", Napi::Number::New(env, g_DevInfo.usPanelH));
        result.Set("imgBufAddrL", Napi::Number::New(env, g_DevInfo.usImgBufAddrL));
        result.Set("imgBufAddrH", Napi::Number::New(env, g_DevInfo.usImgBufAddrH));
        result.Set("fwVersion", Napi::String::New(env, g_DevInfo.usFWVersion));
        result.Set("lutVersion", Napi::String::New(env, g_DevInfo.usLUTVersion));
    } else {
        result.Set("error", Napi::String::New(env, "Device not initialized"));
    }
    
    return result;
}

/**
 * Set VCOM value
 * @param vcom - VCOM voltage (negative float, e.g., -2.0)
 */
Napi::Value SetVCOM(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "VCOM voltage number expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    float vcom = info[0].As<Napi::Number>().FloatValue();
    if (vcom < 0) {
        UWORD vcomValue = (UWORD)(-vcom * 1000);
        IT8951_SetVCOM(vcomValue);
    }
    
    return env.Undefined();
}

/**
 * Get current VCOM value
 * @returns VCOM voltage as negative float
 */
Napi::Number GetVCOM(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    UWORD vcomRaw = IT8951_GetVCOM();
    float vcom = -((float)vcomRaw / 1000.0f);
    return Napi::Number::New(env, vcom);
}

/**
 * Setup N-API module exports
 */
Napi::Object SetupNapi(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "dev_init"),
                Napi::Function::New(env, DEV_Init));
    exports.Set(Napi::String::New(env, "init"),
                Napi::Function::New(env, Init));
    exports.Set(Napi::String::New(env, "display_1bpp"),
                Napi::Function::New(env, Display_1bpp));
    exports.Set(Napi::String::New(env, "display_2bpp"),
                Napi::Function::New(env, Display_2bpp));
    exports.Set(Napi::String::New(env, "display_4bpp"),
                Napi::Function::New(env, Display_4bpp));
    exports.Set(Napi::String::New(env, "display"),
                Napi::Function::New(env, Display));
    exports.Set(Napi::String::New(env, "clear"),
                Napi::Function::New(env, Clear));
    exports.Set(Napi::String::New(env, "sleep"),
                Napi::Function::New(env, Sleep));
    exports.Set(Napi::String::New(env, "dev_exit"),
                Napi::Function::New(env, DEV_Exit));
    exports.Set(Napi::String::New(env, "get_device_info"),
                Napi::Function::New(env, GetDeviceInfo));
    exports.Set(Napi::String::New(env, "set_vcom"),
                Napi::Function::New(env, SetVCOM));
    exports.Set(Napi::String::New(env, "get_vcom"),
                Napi::Function::New(env, GetVCOM));

    return exports;
}

NODE_API_MODULE(waveshare9in7, SetupNapi)
