/*
*   Tested with Seeed Studio XIAO ESP32-S3 with 2 additional leds connected !!
*/

// ==== Pin config (adjust to your board) ====
constexpr uint8_t LED_SERVER_PIN = A8;   // LED #1 (server status) XIAO ESP32-S3 Sense
constexpr uint8_t LED_CLIENT_PIN = A5;   // LED #2 (client status) XIAO ESP32-S3 Sense

// If your LEDs are active-low, set these to false.
constexpr bool LED_SERVER_ACTIVE_HIGH = true;
constexpr bool LED_CLIENT_ACTIVE_HIGH = true;

// ==== Blink timing ====
constexpr uint32_t BLINK_INTERVAL_MS = 300;

// ---------------------- LED helper -------------------------
enum class LedMode : uint8_t { Off, On, Blink };

struct Led {
  uint8_t pin;
  bool activeHigh;

  LedMode mode = LedMode::Off;
  bool level = false;
  uint32_t lastToggleMs = 0;
  uint32_t blinkIntervalMs = BLINK_INTERVAL_MS;

  void begin() {
    pinMode(pin, OUTPUT);
    writeLevel(false); // start off
  }

  void setMode(LedMode m) {
    if (mode == m) return;
    mode = m;

    switch (mode) {
      case LedMode::Off:  writeLevel(false); break;
      case LedMode::On:   writeLevel(true);  break;
      case LedMode::Blink:
        lastToggleMs = millis();
        writeLevel(true);
        break;
    }
  }

  void tick() {
    if (mode != LedMode::Blink) return;
    uint32_t now = millis();
    if (now - lastToggleMs >= blinkIntervalMs) {
      lastToggleMs = now;
      writeLevel(!level);
    }
  }

private:
  void writeLevel(bool on) {
    level = on;
    if (activeHigh) {
      digitalWrite(pin, on ? HIGH : LOW);
    } else {
      digitalWrite(pin, on ? LOW : HIGH);
    }
  }
};

// ---- Instantiate LEDs ----
Led ledServer{LED_SERVER_PIN, LED_SERVER_ACTIVE_HIGH};
Led ledClient{LED_CLIENT_PIN, LED_CLIENT_ACTIVE_HIGH};

// ---- Decide LED modes based on BLE state ----
static inline LedMode desiredServerMode() {
  if (BLEmanager->serverIsConnected) return LedMode::On;
  if (BLEmanager->isAdvertising())   return LedMode::Blink;
  return LedMode::Off;
}

static inline LedMode desiredClientMode() {
  if (BLEmanager->clientIsConnected) return LedMode::On;
  if (BLEmanager->isScanning())      return LedMode::Blink;
  return LedMode::Off;
}

// ---- LED Task ----
void ledTask(void* parameter) {
  (void)parameter;

  ledServer.begin();
  ledClient.begin();

  for (;;) {
    // Update modes
    ledServer.setMode(desiredServerMode());
    ledClient.setMode(desiredClientMode());

    // Tick blinkers
    ledServer.tick();
    ledClient.tick();

    // Run every 50 ms (fast enough for smooth blinking, cheap on CPU)
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
