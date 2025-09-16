
// ==== Pin config (adjust to your board) ====
constexpr uint8_t LED_CLIENT_PIN = LED_BUILTIN;   // LED #2 (Client status)

// If your LED_BUILTIN is active-low, set these to false.
constexpr bool LED_CLIENT_ACTIVE_HIGH = false;

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

// ---- Instantiate LED ----
Led ledClient{LED_CLIENT_PIN, LED_CLIENT_ACTIVE_HIGH};

static inline LedMode desiredClientMode() {
  if (BLEmanager->clientIsConnected) return LedMode::On;
  if (BLEmanager->isScanning())      return LedMode::Blink;
  return LedMode::Off;
}

// ---- LED Task ----
void ledTask(void* parameter) {
  (void)parameter;

  ledClient.begin();

  for (;;) {
    // Update modes
    ledClient.setMode(desiredClientMode());

    // Tick blinkers
    ledClient.tick();

    // Run every 50 ms (fast enough for smooth blinking, cheap on CPU)
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
