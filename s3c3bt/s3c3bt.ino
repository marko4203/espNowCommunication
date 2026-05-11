#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "esp_sntp.h"
#include "time.h"
#include "EspNowLowPower.hpp"


//Wifi needed for NTP time sync, input SSID and password below
const char WIFI_SSID[] = "Network name";
const char WIFI_PASS[] = "Password";


// --- SETTINGS ---
// Replace with the MAC of the DESTINATION board
uint8_t slaveAddress[] = {0xE8,0x3D,0xC1,0x9D,0x9A,0xA0};


// Structure must match on both boards
typedef struct struct_message {
    char payload[128];
} struct_message;

struct_message outgoingData;
struct_message incomingData;

bool msgsent = false;

// --- CALLBACKS ---

// When data arrives via radio
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // Copy the data into our local structure
    memcpy(&incomingData, data, sizeof(incomingData));
    
    // Safety: ensure the payload is null-terminated
    incomingData.payload[sizeof(incomingData.payload) - 1] = '\0';
    
    //Serial.print("Received - ");
    Serial.println(incomingData.payload);
}
// When data is sent (status check)
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("Error: Packet lost in the air!");
    }
}

void setup() {
    Serial.begin(115200);
    /***************************SETTING UP TIMER VIA WIFI - NTP***************************************/
    while(!masterSetupNTP(WIFI_SSID,WIFI_PASS)){
        Serial.println("Error: Syncing time with NTP failed. WIFI or NTP server issue.");
        delay(5000);
    }

    /**************************************INITIALIZE ESP NOW****************************************/
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    
    /****************************REGISTERING SLAVE BOARD*********************************************/
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, slaveAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    /***************************TIME SYNC WIHT SLAVE**************************************************/
    snprintf(outgoingData.payload, sizeof(outgoingData.payload), "TIMESYNC %lld", (long long)getTime());
    esp_now_send(slaveAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
}

void loop() {
    // Check if you typed something in the Serial Monitor

    if (Serial.available() > 0) {
        // Read the characters until newline or buffer full
        outgoingData.payload[0] = 'S';
        outgoingData.payload[1] = '3';
        outgoingData.payload[2] = ':';
        outgoingData.payload[3] = ' ';

        // We subtract 5 from the max size to leave room for the 4-char prefix + null terminator
        int bytesRead = Serial.readBytesUntil('\n', &outgoingData.payload[4], sizeof(outgoingData.payload) - 5);
        outgoingData.payload[4 + bytesRead] = '\0';

        // Send over the air
        esp_now_send(slaveAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
        
        Serial.println(outgoingData.payload);
    }
}