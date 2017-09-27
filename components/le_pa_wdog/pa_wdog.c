/*
 * @file pa_wdog.c
 *
 * External watchdog bridge for standard Linux watchdog.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_wdog.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of modprobe system command.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SYSTEM_CMD_LENGTH 200

//--------------------------------------------------------------------------------------------------
/**
 * Number of times to retry opening the hardware watchdog
 */
//--------------------------------------------------------------------------------------------------
#define MAX_WDOG_OPEN_TRIES     8

//--------------------------------------------------------------------------------------------------
/**
 * File descriptor of the external watchdog.
 */
//--------------------------------------------------------------------------------------------------
#ifdef PA_WDOG_DEVICE
static int wdogFd = -1;
#endif


//--------------------------------------------------------------------------------------------------
/**
 * SIGTERM event handler that will stop the watchdog device
 */
//--------------------------------------------------------------------------------------------------
static void SigTermEventHandler(int sigNum)
{
#ifdef PA_WDOG_DEVICE
    if (write(wdogFd, "V", 1) != 1)
    {
        LE_WARN("Failed to write to watchdog.");
    }

    if (close(wdogFd) < 0)
    {
        LE_WARN("Could not close watchdog device.");
    }

    wdogFd = -1;

    LE_INFO("Watchdog stopped");
#endif
    exit(-sigNum);
}


//--------------------------------------------------------------------------------------------------
/**
 * Shutdown action to take if a service is not kicking
 */
//--------------------------------------------------------------------------------------------------
void pa_wdog_Shutdown
(
    void
)
{
    LE_FATAL("Watchdog expired. Restart device.")
}


//--------------------------------------------------------------------------------------------------
/**
 * Kick the external watchdog
 */
//--------------------------------------------------------------------------------------------------
void pa_wdog_Kick
(
    void
)
{
#ifdef PA_WDOG_DEVICE
    if (wdogFd != -1)
    {
        if (write(wdogFd, "k", 1) != 1)
        {
            LE_WARN("Failed to kick watchdog.");
        }
    }
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize PA watchdog
 **/
//--------------------------------------------------------------------------------------------------
void pa_wdog_Init
(
    void
)
{
    // Initialize external linux watchdog
#ifdef PA_WDOG_DEVICE
#  ifdef PA_WDOG_MODULE
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
    snprintf(systemCmd, sizeof(systemCmd), "/sbin/modprobe %s", STRINGIZE(PA_WDOG_MODULE));
    if (system(systemCmd) < 0)
    {
        LE_WARN("Unable to load linux softdog driver.");
    }
#  endif

    // Spin until watchdog opens.
    int i;

    for (i = 0; i < MAX_WDOG_OPEN_TRIES; ++i)
    {
        wdogFd = open(STRINGIZE(PA_WDOG_DEVICE), O_WRONLY);
        if (wdogFd < 0)
        {
            LE_WARN("Failed to open watchdog device; retrying...");
            sleep(1);
        }
        else
        {
            break;
        }
    }

    if (i >= MAX_WDOG_OPEN_TRIES)
    {
        LE_FATAL("Failed to open hardware watchdog");
    }
#else
    LE_WARN("No watchdog configured, continuing without watchdog");
#endif

    // Kick the watchdog immediately once.
    pa_wdog_Kick();
}


COMPONENT_INIT
{
    // Block signal
    le_sig_Block(SIGTERM);

    // Setup the signal event handler.
    le_sig_SetEventHandler(SIGTERM, SigTermEventHandler);
}
