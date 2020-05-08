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
#if LE_CONFIG_WDOG_ENABLE_EXTERNAL
static int wdogFd = -1;
#endif


//--------------------------------------------------------------------------------------------------
/**
 * SIGTERM event handler that will stop the watchdog device
 */
//--------------------------------------------------------------------------------------------------
static void SigTermEventHandler(int sigNum)
{
#if LE_CONFIG_WDOG_ENABLE_EXTERNAL
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
    LE_FATAL("Watchdog expired. Restart device.");
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
#if LE_CONFIG_WDOG_ENABLE_EXTERNAL
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
#if LE_CONFIG_WDOG_ENABLE_EXTERNAL
    if (LE_CONFIG_WDOG_PA_MODULE[0] != '\0')
    {
        char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
        snprintf(systemCmd, sizeof(systemCmd), "/sbin/modprobe %s", LE_CONFIG_WDOG_PA_MODULE);
        if (system(systemCmd) < 0)
        {
            LE_WARN("Unable to load linux softdog driver.");
        }
    }

    // Spin until watchdog opens.
    int i;

    for (i = 0; i < MAX_WDOG_OPEN_TRIES; ++i)
    {
        wdogFd = open(LE_CONFIG_WDOG_PA_DEVICE, O_WRONLY);
        if (wdogFd < 0)
        {
            LE_WARN("Failed to open watchdog device; retrying...");
            le_thread_Sleep(1);
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
