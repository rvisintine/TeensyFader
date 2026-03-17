// USB MIDI Crossfader — Teensy 4.0 Firmware
// Reads a Mini Innofader Plus on analog pin A0 and sends MIDI CC messages.
// RGB LED (WS2812B) shows power status and responds to incoming MIDI.
// Configuration via serial terminal (channel, cc, led cc saved to EEPROM).
//
// Build: Arduino IDE + Teensyduino
//   Board: Teensy 4.0
//   USB Type: Serial + MIDI
//
// Requires: Adafruit NeoPixel library (install via Library Manager)

#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

// --- EEPROM addresses ---
const int EEPROM_MAGIC   = 0;
const int EEPROM_CHANNEL = 1;
const int EEPROM_CC      = 2;
const int EEPROM_LED_CC  = 3;
const byte EEPROM_MAGIC_VALUE = 0xCF;

// --- Defaults ---
const int DEFAULT_CHANNEL = 4;
const int DEFAULT_CC      = 20;
const int DEFAULT_LED_CC  = 69;  // incoming CC to control LED color

// --- Options ---
const bool HEARTBEAT_ENABLED = true; // set to false to disable onboard LED heartbeat

// --- Pins ---
const int FADER_PIN = A0;
const int LED_PIN   = 2;      // WS2812B data pin (any digital pin works)
const int ONBOARD_LED_PIN = 13; // Teensy 4.0 onboard LED (orange, single color)

// --- LED color presets (matched by incoming CC value ranges) ---
// CC 0-15: red (default/power)    CC 16-31: green
// CC 32-47: blue                  CC 48-63: yellow
// CC 64-79: cyan                  CC 80-95: magenta
// CC 96-111: white                CC 112-127: off
const int NUM_PRESETS = 8;
const uint32_t COLOR_PRESETS[] = {
  0xFF0000,  // 0:  red
  0x00FF00,  // 1:  green
  0x0000FF,  // 2:  blue
  0xFFFF00,  // 3:  yellow
  0x00FFFF,  // 4:  cyan
  0xFF00FF,  // 5:  magenta
  0xFFFFFF,  // 6:  white
  0x000000   // 7:  off
};

// --- NeoPixel ---
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- Runtime config (loaded from EEPROM) ---
int midiChannel;
int midiCC;
int ledCC;

// --- Smoothing / jitter suppression ---
const int SMOOTH_WINDOW = 4;
const int CHANGE_THRESHOLD = 1;

int readings[SMOOTH_WINDOW];
int readIndex = 0;
long readTotal = 0;
int lastSentValue = -1;

// --- Serial input buffer ---
char serialBuf[32];
int serialPos = 0;

void setLED(uint32_t color) {
  led.setPixelColor(0, color);
  led.show();
}

void loadConfig() {
  if (EEPROM.read(EEPROM_MAGIC) == EEPROM_MAGIC_VALUE) {
    midiChannel = constrain(EEPROM.read(EEPROM_CHANNEL), 1, 16);
    midiCC = constrain(EEPROM.read(EEPROM_CC), 0, 127);
    ledCC = constrain(EEPROM.read(EEPROM_LED_CC), 0, 127);
  } else {
    midiChannel = DEFAULT_CHANNEL;
    midiCC = DEFAULT_CC;
    ledCC = DEFAULT_LED_CC;
    saveConfig();
  }
}

void saveConfig() {
  EEPROM.update(EEPROM_MAGIC, EEPROM_MAGIC_VALUE);
  EEPROM.update(EEPROM_CHANNEL, midiChannel);
  EEPROM.update(EEPROM_CC, midiCC);
  EEPROM.update(EEPROM_LED_CC, ledCC);
}

void printConfig() {
  Serial.println("--- MIDI Crossfader ---");
  Serial.print("  channel: ");
  Serial.println(midiChannel);
  Serial.print("  cc:      ");
  Serial.println(midiCC);
  Serial.print("  ledcc:   ");
  Serial.println(ledCC);
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  channel <1-16>    MIDI channel");
  Serial.println("  cc <0-127>        fader CC number");
  Serial.println("  ledcc <0-127>     incoming CC for LED color");
  Serial.println("  status            print config");
  Serial.println();
  Serial.println("LED colors (by incoming CC value):");
  Serial.println("  0-15: red    16-31: green   32-47: blue");
  Serial.println("  48-63: yellow  64-79: cyan  80-95: magenta");
  Serial.println("  96-111: white  112-127: off");
}

void processCommand(const char* cmd) {
  int value;

  if (sscanf(cmd, "channel %d", &value) == 1) {
    if (value >= 1 && value <= 16) {
      midiChannel = value;
      saveConfig();
      Serial.print("Channel set to ");
      Serial.println(midiChannel);
    } else {
      Serial.println("Error: channel must be 1-16");
    }
  } else if (sscanf(cmd, "ledcc %d", &value) == 1) {
    if (value >= 0 && value <= 127) {
      ledCC = value;
      saveConfig();
      Serial.print("LED CC set to ");
      Serial.println(ledCC);
    } else {
      Serial.println("Error: ledcc must be 0-127");
    }
  } else if (sscanf(cmd, "cc %d", &value) == 1) {
    if (value >= 0 && value <= 127) {
      midiCC = value;
      saveConfig();
      Serial.print("CC set to ");
      Serial.println(midiCC);
    } else {
      Serial.println("Error: cc must be 0-127");
    }
  } else if (strncmp(cmd, "status", 6) == 0) {
    printConfig();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    printConfig();
  }
}

void handleIncomingMIDI() {
  // Process all pending MIDI messages
  while (usbMIDI.read()) {
    if (usbMIDI.getType() == usbMIDI.ControlChange &&
        usbMIDI.getChannel() == midiChannel &&
        usbMIDI.getData1() == ledCC) {
      int value = usbMIDI.getData2();
      int preset = (value * NUM_PRESETS) / 128; // map 0-127 to preset index
      preset = constrain(preset, 0, NUM_PRESETS - 1);
      setLED(COLOR_PRESETS[preset]);
    }
  }
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(12); // Teensy 4.0: 12-bit ADC (0-4095)

  pinMode(ONBOARD_LED_PIN, OUTPUT);

  led.begin();
  led.setBrightness(30); // keep it subtle — adjust 0-255

  loadConfig();
  setLED(COLOR_PRESETS[0]); // red on boot

  for (int i = 0; i < SMOOTH_WINDOW; i++) {
    readings[i] = analogRead(FADER_PIN);
    readTotal += readings[i];
  }

  delay(500);
  printConfig();
}

void loop() {
  // --- Read fader ---
  readTotal -= readings[readIndex];
  readings[readIndex] = analogRead(FADER_PIN);
  readTotal += readings[readIndex];
  readIndex = (readIndex + 1) % SMOOTH_WINDOW;

  int smoothed = readTotal / SMOOTH_WINDOW;
  int mapped = map(smoothed, 0, 4095, 0, 127);
  mapped = constrain(mapped, 0, 127);

  if (lastSentValue < 0 || abs(mapped - lastSentValue) > CHANGE_THRESHOLD) {
    usbMIDI.sendControlChange(midiCC, mapped, midiChannel);
    lastSentValue = mapped;
  }

  // --- Handle serial commands ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialPos > 0) {
        serialBuf[serialPos] = '\0';
        processCommand(serialBuf);
        serialPos = 0;
      }
    } else if (serialPos < (int)sizeof(serialBuf) - 1) {
      serialBuf[serialPos++] = c;
    }
  }

  // --- Handle incoming MIDI (LED control) ---
  handleIncomingMIDI();

  // --- Onboard LED heartbeat (~2 second breathing cycle) ---
  if (HEARTBEAT_ENABLED) {
    int brightness = (millis() / 8) % 256;
    if (brightness > 127) brightness = 255 - brightness;
    analogWrite(ONBOARD_LED_PIN, brightness);
  }

  delay(1);
}
