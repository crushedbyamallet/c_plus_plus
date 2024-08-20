#pragma once
#include "GlobalDeclarations.h"
#include "Plugin.h"

/* Preprocessor directive: _CRT_SECURE_NO_WARNINGS */

typedef AEffect* (*pluginFuncPtr)(audioMasterCallback host);    // plugin's entry point

extern "C" {
    VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
}

int main()
{
    HMODULE modulePtr;

    modulePtr = LoadLibrary(pluginName);
    if (modulePtr == NULL) {
        printf("Failed trying to load VST from '%s', error %d.\n", pluginName, GetLastError());
        return 0;
    }

    pluginFuncPtr mainEntryPoint = (pluginFuncPtr)GetProcAddress(modulePtr, "VSTPluginMain");   // in some (older) plugins: main

    Plugin* p = new Plugin(mainEntryPoint(hostCallback));    // instantiate the plugin

    p->open();

    p->displayInfo();

    p->close();
    p = NULL;
    BOOL freeModuleResult = FreeLibrary(modulePtr);

    printf("\n-------- END --------\n");
    //system("pause");
}

extern "C" {
    VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
    {
        switch (opcode) {
            case audioMasterVersion:
                return kVstVersion;

            case audioMasterCurrentId:
                return effect->uniqueID;    // use the current plugin ID; needed by VST shell plugins to determine which sub-plugin to load

            case audioMasterIdle:
                return 1;   // ignore this call (as per Steinberg: "Give idle time to Host application, e.g. if plug-in editor is doing mouse tracking in a modal loop.") 

            case __audioMasterWantMidiDeprecated:
                return 1;

            case audioMasterGetCurrentProcessLevel:
                return kVstProcessLevelRealtime;

            default:
                printf("\nPlugin requested value of opcode %d.\n", opcode);
        }
#if TRACE
        printf("\nOpcode %d was requested by the plugin.\n", opcode);
#endif  // TRACE
    }
}
