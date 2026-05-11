#include "EspNowLowPower.hpp"

bool masterSetupNTP(const char *ssid, const char *pwd){
  static const uint32_t NTP_TIMEOUT_MS = 10000;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);

  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED &&
    millis() - start < 15000){
    delay(300);
    Serial.print('.');
  }

  Serial.println();

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi connection failed.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }

  Serial.println("WiFi connected.");

  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com"); //cloudflare is a fallback option

  time_t now = 0;
  start = millis();

  while (now < 1000000000LL &&
    millis() - start < NTP_TIMEOUT_MS){

    time(&now);

    delay(200);
  }

  if(now < 1000000000LL){
    Serial.println("NTP sync failed.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }
  else{
    Serial.printf("NTP synced: %lld\n", (long long)now);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return true;
}


time_t getTime(){
  time_t t;
  time(&t);
  return t;
}