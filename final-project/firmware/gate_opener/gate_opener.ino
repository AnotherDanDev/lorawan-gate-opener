/*
 * LoRaWAN Gate Opener
 * Arduino Uno + Dragino LA66 LoRaWAN shield
 *
 * Joins The Things Network (OTAA), sends periodic heartbeat uplinks so TTN
 * has a window to deliver queued downlinks, and pulses a relay wired across
 * a gate remote's button when it receives payload 0x01 on FPort 1.
 *
 * Based on Dragino's "Join-TTN-network" example for the LA66 shield.
 * Copy secrets.example.h to secrets.h and fill in your TTN device keys.
 */
#include <SoftwareSerial.h>
#include "secrets.h"

// ——— CONFIGURATION ———
// Pins for LA66 UART connection
#define LA66_RX_PIN     10  // Arduino RX ← LA66 TX
#define LA66_TX_PIN     11  // Arduino TX → LA66 RX

// Pin driving the relay wired across the gate remote's button
#define RELAY_PIN        8

// Heartbeat and polling intervals (in milliseconds)
const unsigned long HEARTBEAT_INTERVAL_MS = 600000UL;  // e.g. 10 min
const unsigned long POLL_INTERVAL_MS      = 10000UL;   // e.g. 10 s
// ——— END CONFIGURATION ———

SoftwareSerial loraSerial(LA66_RX_PIN, LA66_TX_PIN);

// Buffer for incoming AT responses
char            buf[128];
uint8_t         bufPos       = 0;

// State tracking
bool            joinedNetwork = false;
unsigned long   lastHeartbeat = 0;
unsigned long   lastPoll      = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {;}
  loraSerial.begin(9600);
  loraSerial.listen();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.println(F("\n=== LA66 Gate Opener ==="));

  // 1) Reset the module
  Serial.println(F("Resetting module (ATZ)..."));
  loraSerial.println(F("ATZ"));
  delay(2000);

  // 2) Configure OTAA credentials
  Serial.println(F("Configuring OTAA keys..."));
  loraSerial.println(F("AT+NJM=1"));   // OTAA join mode
  delay(100);
  loraSerial.println(F("AT+DEUI=" LA66_DEVEUI));    // DevEUI
  delay(100);
  loraSerial.println(F("AT+APPEUI=" LA66_APPEUI));  // AppEUI
  delay(100);
  loraSerial.println(F("AT+APPKEY=" LA66_APPKEY));  // AppKey
  delay(100);

  // 3) Join TTN
  Serial.println(F("Joining TTN (AT+JOIN)..."));
  loraSerial.println(F("AT+JOIN"));
}

void loop() {
  // ——— 1) Read and handle all incoming AT responses ———
  while (loraSerial.available()) {
    char c = loraSerial.read();

    // End‑of‑line received?
    if (c == '\r' || c == '\n') {
      if (bufPos == 0) continue;
      buf[bufPos] = '\0';
      Serial.print(F("LA66> ")); Serial.println(buf);

      // a) Network joined?
      if (strncmp(buf, "JOINED", 6) == 0) {
        joinedNetwork = true;
        Serial.println(F("** Joined TTN! Switching to Class C..."));
        loraSerial.println(F("AT+CLASS=C"));
      }
      // b) First OK after Class C → enable reply mode + initial heartbeat
      else if (joinedNetwork && lastHeartbeat == 0 && strstr(buf, "OK") != NULL) {
        // Enable immediate downlink replies
        Serial.println(F("-> Enabling immediate-REPLY mode (AT+RPL=4)"));
        loraSerial.println(F("AT+RPL=4"));
        delay(100);

        // Send first heartbeat uplink (payload 0x00 on FPort 1)
        Serial.println(F("-> Sending first heartbeat (0x00)"));
        loraSerial.println(F("AT+MSGHEX=1:00"));
        lastHeartbeat = millis();
      }
      // c) Module indicates a downlink is pending?
      else if (strstr(buf, "Run AT+RECVB") != NULL) {
        Serial.println(F("-> Fetching downlink (AT+RECVB?)"));
        loraSerial.println(F("AT+RECVB?"));
      }
      // d) Direct downlink data line (e.g. "1: 01")
      else if (buf[0] >= '1' && buf[0] <= '9' && buf[1] == ':') {
        int fport = buf[0] - '0';
        // Parse first byte of payload
        uint8_t cmd = strtoul(buf + 2, NULL, 16);
        Serial.print(F("Downlink on FPort ")); Serial.print(fport);
        Serial.print(F(", cmd=0x")); Serial.println(cmd, HEX);

        if (fport == 1 && cmd == 0x01) {
          Serial.println(F("** Triggering gate relay **"));
          digitalWrite(RELAY_PIN, HIGH);
          delay(1000);
          digitalWrite(RELAY_PIN, LOW);
          Serial.println(F("Relay off."));
        }
      }

      // Reset buffer for next line
      bufPos = 0;
    }
    else {
      // Accumulate characters
      if (bufPos < sizeof(buf) - 1) {
        buf[bufPos++] = c;
      }
    }
  }

  // ——— 2) Periodic heartbeat uplink ———
  if (joinedNetwork && lastHeartbeat > 0) {
    unsigned long now = millis();
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
      Serial.println(F("-> Sending periodic heartbeat (0x00)"));
      loraSerial.println(F("AT+MSGHEX=1:00"));
      lastHeartbeat = now;
    }
  }

  // ——— 3) Regularly poll for buffered downlinks ———
  if (joinedNetwork && lastPoll > 0 && millis() - lastPoll >= POLL_INTERVAL_MS) {
    Serial.println(F("-> Polling for downlink (AT+RECVB?)"));
    loraSerial.println(F("AT+RECVB?"));
    lastPoll = millis();
  }
  // Initialize lastPoll on first run after join
  else if (joinedNetwork && lastPoll == 0 && lastHeartbeat > 0) {
    lastPoll = millis();
  }

  // nothing else here — waiting for heartbeats & downlinks
}
