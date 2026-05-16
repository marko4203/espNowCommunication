#pragma once

#include <esp_now.h>
#include <WiFi.h>       // still required by esp_now internally
#include <esp_sleep.h>
#include "time.h"

// ---------------------------------------------------------------------------
// Local epoch management
//
// Instead of wall-clock Unix time we keep a simple "session epoch":
// the master resets it to 0 on every boot and broadcasts that anchor
// to the slave.  Both sides then advance the clock purely from
// esp_timer_get_time() (microsecond counter that runs during light-sleep
// but is reset on deep-sleep).  We store the anchor in RTC memory so it
// survives deep-sleep wakes on both boards.
// ---------------------------------------------------------------------------

// Call once on master boot to reset the session epoch to 0.
void resetSessionClock();

// Returns seconds elapsed since the session epoch was last reset.
uint32_t getSessionTime();

// Encode a TIMESYNC packet payload (written into `buf`, length `bufLen`).
// The value sent is always 0 – the slave simply resets its own counter.
void buildTimeSyncPayload(char *buf, size_t bufLen);

// Call on the slave when a "TIMESYNC <val>" payload is received.
// Resets the slave's session clock anchor to `val` (normally 0).
void applyTimeSync(uint32_t val);

// Enter deep sleep for `seconds`.  Never returns – chip reboots on wake.
void enterDeepSleep(uint32_t seconds);
