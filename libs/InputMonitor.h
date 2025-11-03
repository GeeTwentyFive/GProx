#ifndef _INPUTMONITOR_H
#define _INPUTMONITOR_H


#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "libuiohook/uiohook.h"


#define INPUTMONITOR__MAX_BINDINGS 256


typedef enum {
        INPUTMONITOR__TARGET_KEYBOARD,
        INPUTMONITOR__TARGET_MOUSE
} InputMonitor__target_type;

typedef struct {

        InputMonitor__target_type targetType;
        unsigned short target;
        int *OUT_state;

} _InputMonitor_binding;


static int _InputMonitor_init = 0;

static pthread_t _InputMonitor_monitorThreadID = 0;

static _InputMonitor_binding _InputMonitor_bindings[INPUTMONITOR__MAX_BINDINGS];
static int _InputMonitor_setBindings = 0;


void _InputMonitor_InputHookCallback(uiohook_event *event) {

        for (int i = 0; i < _InputMonitor_setBindings; i++) {

                // Mouse

                if (_InputMonitor_bindings[i].targetType == INPUTMONITOR__TARGET_MOUSE) {

                        if (event->type == EVENT_MOUSE_PRESSED) {
                                if (event->data.mouse.button == _InputMonitor_bindings[i].target)
                                        *(_InputMonitor_bindings[i].OUT_state) = 1;
                        }

                        else if (event->type == EVENT_MOUSE_RELEASED) {
                                if (event->data.mouse.button == _InputMonitor_bindings[i].target)
                                        *(_InputMonitor_bindings[i].OUT_state) = 0;
                        }

                }


                // Keyboard

                else if (_InputMonitor_bindings[i].targetType == INPUTMONITOR__TARGET_KEYBOARD) {

                        if (event->type == EVENT_KEY_PRESSED) {
                                if (event->data.keyboard.keycode == _InputMonitor_bindings[i].target)
                                        *(_InputMonitor_bindings[i].OUT_state) = 1;
                        }

                        else if (event->type == EVENT_KEY_RELEASED) {
                                if (event->data.keyboard.keycode == _InputMonitor_bindings[i].target)
                                        *(_InputMonitor_bindings[i].OUT_state) = 0;
                        }

                }

        }

}

void* _InputMonitor_MonitorThread(void*) {

        int status = hook_run();
        if (status != UIOHOOK_SUCCESS) {
                printf("ERROR (0x%X): libuiohook - hook_run() - Failed to start input hook\nFILE: %s, LINE: %s\n", status, __FILE__, __LINE__);
                exit(1);
        }

}

int _InputMonitor_Init() {

        hook_set_dispatch_proc(_InputMonitor_InputHookCallback);


        if (_InputMonitor_monitorThreadID != 0) {
                pthread_cancel(_InputMonitor_monitorThreadID);
        }


        int status = pthread_create(&_InputMonitor_monitorThreadID, NULL, _InputMonitor_MonitorThread, NULL);
        if (status != 0) {
                printf("ERROR (%i): pthread - pthread_create() - Failed to create input monitoring thread\nFILE: %s, LINE: %s\n", status, __FILE__, __LINE__);
                return 0;
        }

        _InputMonitor_init = 1;
        return 1;

}

        // PUBLIC FUNCTIONS

//
// Asynchronously monitors state of "target" key/click.
// Sets *OUT_state to 1 or 0 if "target" key/click is pressed or released respectively.
//
// "target" is one of libuiohook's key/click definitions.
// Ex.: VC_A, MOUSE_BUTTON1, etc.
//
int InputMonitor__Bind(InputMonitor__target_type targetType, unsigned short target, int *OUT_state) {

        if (!_InputMonitor_init) {
                if (!_InputMonitor_Init()) {
                        puts("ERROR: Failed to initialize InputMonitor");
                }
        }



        if (_InputMonitor_setBindings+1 >= INPUTMONITOR__MAX_BINDINGS) {
                printf("ERROR: Reached InputMonitor's binding limit. To increase: Increase value of INPUTMONITOR__MAX_BINDINGS (current: %u).", INPUTMONITOR__MAX_BINDINGS);
                return 0;
        }

        _InputMonitor_bindings[_InputMonitor_setBindings].targetType = targetType;
        _InputMonitor_bindings[_InputMonitor_setBindings].target = target;
        _InputMonitor_bindings[_InputMonitor_setBindings].OUT_state = OUT_state;
        _InputMonitor_setBindings++;



        return 1;

}


void InputMonitor__ClearBindings() { _InputMonitor_setBindings = 0; }


#endif // _INPUTMONITOR_H
