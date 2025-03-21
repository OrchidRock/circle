//
// Created by mimose on 25-3-21.
//

#ifndef KERNEL_H
#define KERNEL_H

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/types.h>

enum TShutdownMode
{
    SHUTDOWN_NONE,
    SHUTDOWN_HALT,
    SHUTDOWN_REBOOT,
};

class CKernel
{
public:
    CKernel();
    ~CKernel();

    boolean Initialize(void);
    TShutdownMode Run(void);

private:
    CActLED    m_ActLED;
};

#endif //KERNEL_H
