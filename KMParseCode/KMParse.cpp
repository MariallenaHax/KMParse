#include <iostream>
#include "KMParseCode/Engine.h"

Tarsa mainengine;

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include "../resource.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#ifndef NDEBUG
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    mainengine.Start();
    return 0;
}

#else

int main(int argc, char** argv)
{
        mainengine.Start();
        return 0;
}


#endif
