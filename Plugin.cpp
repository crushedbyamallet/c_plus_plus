#include "Plugin.h"

Plugin::Plugin(AEffect* effect)
{
    e = effect;
    dispatcher = configurePlugin();
}

AEffectDispatcherProc Plugin::configurePlugin() {

    // Check the plugin's magic number.
    // If incorrect, then the file either was not loaded properly, is not a real VST plugin, or is otherwise corrupt.
    if (e->magic != kEffectMagic) {
        printf("Plugin's magic number is bad.\n");
        return NULL;
    }

    // Create dispatcher handle.
    return (AEffectDispatcherProc)(e->dispatcher);
}

#pragma region - lifecycle - 

void Plugin::open() {

    dispatcher(e, effOpen, 0, 0, NULL, 0.0f);  

    dispatcher(e, effSetSampleRate, 0, 0, NULL, 44100.0);   // hard-coded for now
    dispatcher(e, effSetBlockSize, 0, 512, NULL, 0.0f);   // hard-coded for now

    resume();
}

void Plugin::suspend() {

    dispatcher(e, effMainsChanged, 0, 0, NULL, 0.0f);
    dispatcher(e, effStopProcess, 0, 0, NULL, 0.0f);
}

void Plugin::resume() {

    dispatcher(e, effMainsChanged, 0, 1, NULL, 0.0f);
    dispatcher(e, effStartProcess, 0, 0, NULL, 0.0f);
}

void Plugin::close() {

    suspend();

    dispatcher(e, effClose, 0, 0, NULL, 0.0f);
}

#pragma endregion

#pragma region - information -

void Plugin::displayInfo() {

    unsigned int nameBufferSize = kVstMaxVendorStrLen + 1;  // the largest buffer size we need for the names (+1 for '\0')
    char* nameBuffer = (char*)malloc(sizeof(char) * nameBufferSize);

    dispatcher(e, effGetVendorString, 0, 0, nameBuffer, 0.0f);
    printf("Vendor: %s\n", nameBuffer);

    VstInt32 vendorVersion = (VstInt32)dispatcher(e, effGetVendorVersion, 0, 0, NULL, 0.0f);
    printf("Version: %d\n", vendorVersion);

    memset(nameBuffer, 0, sizeof(char) * nameBufferSize);    // clear the buffer (actual size required: kVstMaxProductStrLen)
    dispatcher(e, effGetProductString, 0, 0, nameBuffer, 0);
    printf("Name of Product: %s\n", nameBuffer);

    VstInt32 uniqueId = e->uniqueID;
    printf("Unique ID: %d\n", uniqueId);

    memset(nameBuffer, 0, sizeof(char) * nameBufferSize);    // clear the buffer (actual size required: kVstMaxEffectNameLen)
    dispatcher(e, effGetEffectName, 0, 0, nameBuffer, 0);
    printf("Name of Effect: %s\n", nameBuffer);

    if (e->flags & effFlagsIsSynth) printf("The Plugin is a Synthesizer.\n");

    printf("Category: ");
    VstInt32 pluginCategory = (VstInt32)dispatcher(e, effGetPlugCategory, 0, 0, NULL, 0.0f); // strangely enough this query returns either kPlugCategSynth or kPlugCategUnknown only so the rest of the switch statement is ineffective
    switch (pluginCategory) {
        case kPlugCategUnknown:
            printf("Unknown, category not implemented.\n");
            break;
        case kPlugCategEffect:
            printf("Simple Effect.\n");
            break;
        case kPlugCategSynth:
            printf("VST Instrument (Synths, samplers, etc.)\n");
            break;
        case kPlugCategAnalysis:
            printf("Scope, Tuner, etc.\n");
            break;
        case kPlugCategMastering:
            printf("Dynamics, etc.\n");
            break;
        case kPlugCategSpacializer:
            printf("Panners, etc.\n");
            break;
        case kPlugCategRoomFx:
            printf("Delays and Reverbs.\n");
            break;
        case kPlugSurroundFx:
            printf("Dedicated surround processor.\n");
            break;
        case kPlugCategRestoration:
            printf("Denoiser, etc.\n");
            break;
        case kPlugCategOfflineProcess:
            printf("Offline Process.\n");
            break;
        case kPlugCategShell:
            printf("Plug-in is container of other plug-ins (shell plug-in).\n");
            break;
        case kPlugCategGenerator:
            printf("Tone Generator.\n");
            break;
        default:
            printf("Plugin type: other, category %d.\n", pluginCategory);
            break;
    }

    printf("GUI Editor: %s\n", e->flags & effFlagsHasEditor ? "Yes" : "No");

    VstInt32 numInputs = e->numInputs;
    printf("Number of inputs: %d\n", numInputs);
    VstInt32 numOutputs = e->numOutputs;
    printf("Number of outputs: %d\n", numOutputs);

    VstInt32 numPrograms = e->numPrograms;
    printf("Number of programs: %d\n", numPrograms);
    for (VstInt32 progIndex = 0; progIndex < numPrograms; progIndex++)
    {
        memset(nameBuffer, 0, sizeof(char) * nameBufferSize);   // clear the name buffer (actual size required: kVstMaxProgNameLen)
        if (dispatcher(e, effGetProgramNameIndexed, progIndex, 0, nameBuffer, 0))
            printf("  prog. %03d: %s\n", progIndex + 1, nameBuffer);
    }

    // Display the parameters. Note: unlike for everything else, the parameters will only be available when the plugin is open.
    AudioEffect* audioEffect = (AudioEffect*)e->object;   // get a pointer to the audio effect object in the plugin
    AudioEffectX* audioEffectX = (AudioEffectX*)e->object;
    printf("\nNumber of Parameters = %d\n", e->numParams);
    for (VstInt32 i = 0; i < e->numParams; i++) {
        // display each parameter in this format: name, displayed value, internal value
        memset(nameBuffer, 0, sizeof(char) * nameBufferSize);   // clear the name buffer (actual size required: kVstMaxParamStrLen)
        audioEffect->getParameterName(i, nameBuffer);
        //dispatcher(e, effGetParamName, i, 0, nameBuffer, 0.0f);    // an alternative way to get the parameter name
        printf("\n  %d: '%s' ", i, nameBuffer);
        memset(nameBuffer, 0, sizeof(char) * nameBufferSize);   // reuse the name buffer
        audioEffect->getParameterDisplay(i, nameBuffer);
        printf("disp.: %s ", nameBuffer);
        memset(nameBuffer, 0, sizeof(char) * nameBufferSize);   // reuse the name buffer
        audioEffect->getParameterLabel(i, nameBuffer);
        float value = e->getParameter(e, i);    // the current value of the parameter
        printf("%s val.: %f \n", nameBuffer, value);
    }

    free(nameBuffer);

    displayCanDo();
}

void Plugin::displayCanDo() {

    const char* canDoString[] = { "sendVstEvents", "sendVstMidiEvent", "receiveVstEvents", "receiveVstMidiEvent", "receiveVstTimeInfo", "startStopProcess", "bypass" };

    printf("\n'Can Do'-s:\n===========\n");

    for (int i = 0; i < sizeof(canDoString) / sizeof(*canDoString); i++) {
        VstIntPtr result = dispatcher(e, effCanDo, 0, 0, (void*)canDoString[i], 0.0f);

        const char* resultString;
        switch ((short)result) {
            case -1:
                resultString = "No";
                break;
            case 0:
                resultString = "Don't know";
                break;
            case 1:
                resultString = "Yes";
                break;
            default:
                resultString = "Undefined response";
        }
        printf("  %s: %s\n", canDoString[i], resultString);
    }
}

#pragma endregion
