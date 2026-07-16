#include "psp_input.h"
#include <pspkernel.h>

SceCtrlData PspInput::currentState{};
SceCtrlData PspInput::previousState{};

void PspInput::Init()
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void PspInput::Poll()
{
    previousState = currentState;
    sceCtrlReadBufferPositive(&currentState, 1);
}

bool PspInput::IsButtonPressed(unsigned int button)
{
    return (currentState.Buttons & button) != 0;
}

bool PspInput::IsButtonJustPressed(unsigned int button)
{
    return ((currentState.Buttons & button) != 0) && ((previousState.Buttons & button) == 0);
}
