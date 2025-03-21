//
// Created by mimose on 25-3-21.
//

#include "kernel.h"
#include <circle/timer.h>

CKernel::CKernel(void)
{

}

CKernel::~CKernel(void)
{

}

bool CKernel::Initialize(void)
{
    return true;
}

TShutdownMode CKernel::Run(void)
{
    for (unsigned int i = 0; i < 15; i++)
    {
        m_ActLED.On();
        CTimer::SimpleMsDelay(200);
        m_ActLED.Off();
        CTimer::SimpleMsDelay(500);
    }
    return SHUTDOWN_REBOOT;
}