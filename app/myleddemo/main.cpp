//
// Created by mimose on 25-3-21.
//

#include "kernel.h"
#include <circle/startup.h>

int main(void)
{
    CKernel kernel;
    if (!kernel.Initialize())
    {
        halt();
        return EXIT_HALT;
    }

    TShutdownMode mode = kernel.Run();

    switch (mode)
    {
    case SHUTDOWN_REBOOT:
            reboot();
            return EXIT_REBOOT;
    case SHUTDOWN_HALT:
    case SHUTDOWN_NONE:
    default:
            halt();
            return EXIT_HALT;
    }
    return EXIT_HALT;
}