#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "esp_sntp.h"
#include "time.h"

bool masterSetupNTP(const char *ssid, const char *pwd);

time_t getTime();