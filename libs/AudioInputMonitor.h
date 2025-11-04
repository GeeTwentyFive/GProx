#ifndef _AUDIOINPUTMONITOR_H
#define _AUDIOINPUTMONITOR_H



#include <stdio.h>

// #define MINIAUDIO_IMPLEMENTATION
// #define MA_NO_DECODING
// #define NA_NO_ENCODING
// #define MA_NO_WAV
// #define MA_NO_FLAC
// #define MA_NO_MP3
// #define MA_NO_RESOURCE_MANAGER
// #define MA_NO_NODE_GRAPH
// #define MA_NO_GENERATION
#include "miniaudio.h"







typedef void(AudioInputMonitor__DataCallback)(const void *pData, ma_uint32 frameCount);







static ma_device _AudioInputMonitor_device;

static AudioInputMonitor__DataCallback *_AudioInputMonitor_DataCallback;







void _AudioInputMonitor_MiniaudioDataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {

        (*_AudioInputMonitor_DataCallback)(pInput, frameCount);

}







        // PUBLIC FUNCTIONS

int AudioInputMonitor__Monitor(ma_format format, ma_uint32 channels, ma_uint32 sampleRate, AudioInputMonitor__DataCallback *OUT_dataCallback) {

        ma_device_uninit(&_AudioInputMonitor_device);



        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
        deviceConfig.capture.format = format;
        deviceConfig.capture.channels = channels;
        deviceConfig.sampleRate = sampleRate;
        deviceConfig.dataCallback = _AudioInputMonitor_MiniaudioDataCallback;

        _AudioInputMonitor_DataCallback = OUT_dataCallback;

        int status = ma_device_init(NULL, &deviceConfig, &_AudioInputMonitor_device);
        if (status != MA_SUCCESS) {
                printf("ERROR (%i): miniaudio - ma_device_init() - Failed to initialize miniaudio input device\nFILE: %s, LINE: %d\n", status, __FILE__, __LINE__);
                return 1;
        }



        status = ma_device_start(&_AudioInputMonitor_device);
        if (status != MA_SUCCESS) {
                printf("ERROR (%i): miniaudio - ma_device_start() - Failed to start miniaudio input device\nFILE: %s, LINE: %d\n", status, __FILE__, __LINE__);
                return 1;
        }



        return 0;

}



#endif // _AUDIOINPUTMONITOR_H
