#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "EspNowLowPower.hpp"

// ---------------------------------------------------------------------------
// SETTINGS
// ---------------------------------------------------------------------------
uint8_t slaveAddress[] = {0xE8, 0x3D, 0xC1, 0x9D, 0x9A, 0xA0};

// How long the master sleeps between sync broadcasts (seconds).
// Adjust to your application's needs.
static const uint32_t SLEEP_DURATION_S = 30;

// ---------------------------------------------------------------------------
// Message structure (must match slave)
// ---------------------------------------------------------------------------
typedef struct {
    char payload[128];
} struct_message;

struct_message outgoingData;
struct_message incomingData;

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
void OnDataRecv(const esp_now_recv_info_t *recv_info,
                const uint8_t *data, int len) {
    memcpy(&incomingData, data, sizeof(incomingData));
    incomingData.payload[sizeof(incomingData.payload) - 1] = '\0';
    Serial.printf("[RX] %s\n", incomingData.payload);
}

void OnDataSent(const wifi_tx_info_t *tx_info,
                esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[TX] Error: packet lost!");
    } else {
        Serial.println("[TX] TIMESYNC delivered.");
    }
}

// ---------------------------------------------------------------------------
// setup() runs on every boot (including deep-sleep wakes)
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // 1. Reset (or continue) the session clock.
    //    On first boot RTC memory is 0 so this simply anchors t=0.
    //    On a deep-sleep wake it re-anchors to the stored epoch base,
    //    effectively continuing the clock from where it left off.
    resetSessionClock();
    Serial.printf("[Clock] Session time at boot: %u s\n", getSessionTime());

    // 2. Bring up ESP-NOW (WiFi radio required internally, no AP/STA join).
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();   // make sure we don't accidentally associate

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed - retrying after sleep.");
        enterDeepSleep(SLEEP_DURATION_S);
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // 3. Register slave peer.
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, slaveAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add peer - retrying after sleep.");
        enterDeepSleep(SLEEP_DURATION_S);
    }

    // 4. Build and send TIMESYNC.
    buildTimeSyncPayload(outgoingData.payload, sizeof(outgoingData.payload));
    Serial.printf("[TX] Sending: %s\n", outgoingData.payload);
    esp_now_send(slaveAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

    // 5. Brief wait for the send callback to fire before sleeping.
    delay(100);

    // 6. Go to deep sleep.  On wake, setup() runs again from the top.
    enterDeepSleep(SLEEP_DURATION_S);
}

// loop() is never reached because setup() always ends in deep sleep.
void loop() {}
