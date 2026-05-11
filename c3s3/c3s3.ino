#include <esp_now.h>
#include <WiFi.h>

// --- SETTINGS ---
// Replace with the MAC of the DESTINATION board
uint8_t peerAddress[] = {0x44,0x1B,0xF6,0xD3,0xFC,0x9C}; 

// Structure must match on both boards
typedef struct struct_message {
    char payload[240]; // Buffer for your text
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

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Register the other board
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    Serial.println("Bridge Ready. Type something in Serial Monitor!");
}

void loop() {
    // Check if you typed something in the Serial Monitor

    if (Serial.available() > 0) {
        // Read the characters until newline or buffer full
        outgoingData.payload[0] = 'C';
        outgoingData.payload[1] = '3';
        outgoingData.payload[2] = ':';
        outgoingData.payload[3] = ' ';

        // We subtract 5 from the max size to leave room for the 4-char prefix + null terminator
        int bytesRead = Serial.readBytesUntil('\n', &outgoingData.payload[4], sizeof(outgoingData.payload) - 5);
        
        // 3. Null-terminate based on the total length (prefix length + bytes read)
        int totalLen = 4 + bytesRead;
        outgoingData.payload[totalLen] = '\0';

        // Send over the air
        esp_now_send(peerAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
        
        Serial.println(outgoingData.payload);
    }
}