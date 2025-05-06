// Direct IR LED as Receiver for PAM signal
// Uses an IR LED in reverse bias as a photodiode
// Continuous signal processing without buffering

// Pin definitions
const int irReceiverPin = A0;   // Analog pin connected to reverse-biased IR LED
const int audioOutPin = 3;      // PWM output pin for audio
const int statusLedPin = 13;    // Status LED

// Sampling parameters
const unsigned long sampleRate = 8000;           // 8kHz sample rate (same as transmitter)
const unsigned long sampleInterval = 125;        // 125 microseconds per sample (1000000/8000)

// Signal processing parameters
const int thresholdValue = 20;        // Analog threshold to detect IR signal
const int baselineReadings = 100;     // Number of readings to establish baseline
const int signalTimeout = 50;         // How many samples without signal before declaring loss

// Runtime variables
unsigned long nextSampleTime = 0;
int ambientLevel = 0;          // Baseline ambient light level
int signalLossCount = 0;       // Counter for signal presence/absence
bool receivingSignal = false;  // Currently receiving valid signal
byte lastOutputValue = 128;    // Last output value (midpoint = silence)

void setup() {
  pinMode(audioOutPin, OUTPUT);
  pinMode(statusLedPin, OUTPUT);
  Serial.begin(115200);
  
  // Configure ADC for faster readings
  // Clear ADPS bits
  ADCSRA &= ~(bit(ADPS0) | bit(ADPS1) | bit(ADPS2));
  // Set ADC clock prescaler to 16 (1MHz with 16MHz system clock)
  ADCSRA |= bit(ADPS2);
  
  Serial.println(F("Calibrating ambient light level..."));
  calibrateAmbientLevel();
  
  Serial.println(F("Direct IR LED PAM Receiver ready"));
  Serial.print(F("Ambient level: "));
  Serial.println(ambientLevel);
  
  // Initialize audio output to midpoint (silence)
  analogWrite(audioOutPin, 128);
}

void loop() {
  unsigned long currentTime = micros();
  
  // Time to take a new sample?
  if (currentTime >= nextSampleTime) {
    // Process and output the sample immediately
    processSampleAndOutput();
    nextSampleTime = currentTime + sampleInterval;
  }
  
  // Optional: Add any other processing here that doesn't interfere with timing
}

void calibrateAmbientLevel() {
  long total = 0;
  
  // Take multiple readings to establish baseline
  for (int i = 0; i < baselineReadings; i++) {
    total += analogRead(irReceiverPin);
    delay(1);
  }
  
  ambientLevel = total / baselineReadings;
  digitalWrite(statusLedPin, LOW);
}

void processSampleAndOutput() {
  // Read the IR LED (acting as photodiode)
  int rawValue = analogRead(irReceiverPin);
  
  // Detect if we have signal (IR light causes voltage change)
  bool signalPresent = abs(rawValue - ambientLevel) > thresholdValue;
  
  if (signalPresent) {
    // Reset signal loss counter
    signalLossCount = 0;
    
    // If we weren't receiving before, start a new reception
    if (!receivingSignal) {
      receivingSignal = true;
      digitalWrite(statusLedPin, HIGH);
      
      // Optional: Serial debugging
       Serial.println(F("Signal detected"));
    }
    
    // Map the analog reading to an amplitude value (0-255)
    // The mapping is inverted because more IR light = lower voltage in reverse-biased LED
    int mappedValue = map(constrain(rawValue, ambientLevel - 150, ambientLevel), 
                        ambientLevel, ambientLevel - 150, 0, 255);
    
    // Output the sample immediately
    analogWrite(audioOutPin, mappedValue);
    lastOutputValue = mappedValue;
    
    // Optional: Debug output (uncomment if needed)
    // Static counter to avoid printing too often
     static int debugCounter = 0;
     if (++debugCounter >= 80) { // Print every 80 samples (10ms)
       Serial.println(mappedValue);
       debugCounter = 0;
     }
  }
  else {
    // Increment signal loss counter
    signalLossCount++;
    
    // If we've lost signal for too long, stop receiving
    if (signalLossCount > signalTimeout && receivingSignal) {
      receivingSignal = false;
      digitalWrite(statusLedPin, LOW);
      
      // Output silence (midpoint value)
      analogWrite(audioOutPin, 128);
      lastOutputValue = 128;
      
      // Optional: Serial debugging
      // Serial.println(F("Signal lost"));
    }
    else if (receivingSignal) {
      // We're still officially receiving, but this sample was missed
      // Can optionally hold the last value or fade toward silence
      // Hold last value approach:
      analogWrite(audioOutPin, lastOutputValue);
      
      // Fade to silence approach (uncomment to use):
       if (lastOutputValue > 128) lastOutputValue--;
       else if (lastOutputValue < 128) lastOutputValue++;
       analogWrite(audioOutPin, lastOutputValue);
    }
  }
}

// Optional calibration routine - can be called from loop if needed
void recalibrate() {
  digitalWrite(statusLedPin, HIGH);
  Serial.println(F("Recalibrating..."));
  calibrateAmbientLevel();
  Serial.print(F("New ambient level: "));
  Serial.println(ambientLevel);
}

// Optional: Add this function if you want to monitor signal levels
void monitorSignalLevels() {
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime > 500) {  // Print every 500ms
    int rawValue = analogRead(irReceiverPin);
    Serial.print(F("Raw: "));
    Serial.print(rawValue);
    Serial.print(F(" Diff: "));
    Serial.println(abs(rawValue - ambientLevel));
    lastPrintTime = currentTime;
  }
}