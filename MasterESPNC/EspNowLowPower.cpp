#include "EspNowLowPower.hpp"
#include <esp_timer.h>
#include <Arduino.h>

// ---------------------------------------------------------------------------
// RTC memory survives deep-sleep.  We keep:
//   rtcEpochBase  – the session-second value that was set when
//                   esp_timer_get_time() was last anchored.
//   rtcTimerBase  – the esp_timer value (µs) at that same moment.
//
// getSessionTime() = rtcEpochBase + (now_us - rtcTimerBase) / 1 000 000
//
// On every fresh power-on both values are 0, so the session clock starts
// at 0.  On a deep-sleep wake they hold whatever was written before sleep,
// letting the clock continue seamlessly even though esp_timer resets.
// ---------------------------------------------------------------------------
RTC_DATA_ATTR static uint32_t rtcEpochBase = 0;
RTC_DATA_ATTR static int64_t  rtcTimerBase = 0;   // µs, at last anchor

// ---------------------------------------------------------------------------

void resetSessionClock() {
    rtcEpochBase = 0;
    rtcTimerBase = esp_timer_get_time();  // anchor to "now"
    Serial.println("[Clock] Session clock reset to 0.");
}

uint32_t getSessionTime() {
    int64_t elapsed_us = esp_timer_get_time() - rtcTimerBase;
    if (elapsed_us < 0) elapsed_us = 0;   // guard against timer wrap
    return rtcEpochBase + (uint32_t)(elapsed_us / 1000000LL);
}

void buildTimeSyncPayload(char *buf, size_t bufLen) {
    // We always broadcast 0 so the slave resets its own counter.
    snprintf(buf, bufLen, "TIMESYNC 0");
}

void applyTimeSync(uint32_t val) {
    // Re-anchor the slave clock to `val` (expected to be 0 from master).
    rtcEpochBase = val;
    rtcTimerBase = esp_timer_get_time();
    Serial.printf("[Clock] Session clock set to %u.\n", val);
}

void enterDeepSleep(uint32_t seconds) {
    Serial.printf("[Sleep] Entering deep sleep for %u s...\n", seconds);
    Serial.flush();
    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    esp_deep_sleep_start();
    // Never reaches here.
}
