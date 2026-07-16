#pragma once

#include <pspctrl.h>

class PspInput
{
public:
    static void Init();
    static void Poll();
    static bool IsButtonPressed(unsigned int button);
    static bool IsButtonJustPressed(unsigned int button);

private:
    static SceCtrlData currentState;
    static SceCtrlData previousState;
};
