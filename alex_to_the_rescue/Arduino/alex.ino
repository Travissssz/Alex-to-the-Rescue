#include <serialize.h>
#include <stdarg.h>
#include <math.h>
#include "packet.h"
#include "constants.h"

/*
  GLOBAL STATE AND CONFIGURATION
*/

volatile TDirection dir;

// Encoder Constants
#define COUNTS_PER_REV      3
#define WHEEL_CIRC          20.42

// Robot Physical Dimensions (cm)
#define ALEX_LENGTH         16
#define ALEX_BREADTH        5

// Encoder Tick Counters
volatile unsigned long leftForwardTicks;
volatile unsigned long rightForwardTicks;
volatile unsigned long leftReverseTicks;
volatile unsigned long rightReverseTicks;

// Turn Counters (Revolutions)
volatile unsigned long leftForwardTicksTurns;
volatile unsigned long rightForwardTicksTurns;
volatile unsigned long leftReverseTicksTurns;
volatile unsigned long rightReverseTicksTurns;

// Distance Tracking
volatile unsigned long forwardDist;
volatile unsigned long reverseDist;
unsigned long deltaDist;
unsigned long newDist;
unsigned long deltaTicks;
unsigned long targetTicks;
float alexDiagonal = 0.0;
float alexCirc = 0.0;


/*
  HARDWARE PIN DEFINITIONS
*/

// Buzzer
#define buzzerPin           45

// Color Sensor (TCS3200)
#define S0                  48
#define S1                  49
#define S2                  50
#define S3                  51
#define sensorOut           52

// Ultrasonic Sensor
#define trig                46
#define echo                47

// Global Sensor Readings
volatile unsigned long red;
volatile unsigned long green;
volatile unsigned long blue;
char colour;
long duration;
uint32_t dist;


/*
  ULTRASONIC SENSOR LOGIC
*/

void setupultra() {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
}

uint32_t ultrareading() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  duration = pulseIn(echo, HIGH);
  dist = duration * 0.034 / 2;      // Speed of sound conversion to cm
  return dist;
}


/*
  COLOR SENSOR LOGIC
*/

void setupcoloursensor() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  // Frequency-scaling set to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
}

void readcolours() {
  // RED channel
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  red = pulseIn(sensorOut, LOW);
  red = map(red, 244, 799, 255, 0);
  delay(100);

  // GREEN channel
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  green = pulseIn(sensorOut, LOW);
  green = map(green, 258, 791, 255, 0);
  delay(100);

  // BLUE channel
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  blue = pulseIn(sensorOut, LOW);
  blue = map(blue, 207, 640, 255, 0);
  delay(100);
}

char detectcolours() {
  int mapred, mapgreen, mapblue;

  // Quantization Logic (0, 1, or 2)
  mapred   = (red <= 100)   ? 0 : (red <= 200)   ? 1 : 2;
  mapgreen = (green <= 100) ? 0 : (green <= 200) ? 1 : 2;
  mapblue  = (blue <= 100)  ? 0 : (blue <= 200)  ? 1 : 2;

  // Color Mapping Table
  if (mapred == 1 && mapgreen == 2 && mapblue == 2) return 'B'; // Blue
  if (mapred == 1 && mapgreen == 1 && mapblue == 1) return 'G'; // Green
  if (mapred == 2 && mapgreen == 0 && mapblue == 0) return 'R'; // Red
  if (mapred == 2 && mapgreen == 1 && mapblue == 1) return 'O'; // Orange
  if (mapred == 2 && mapgreen == 2 && mapblue == 1) return 'Y'; // Yellow
  
  return 'X'; // Unknown
}


/*
  AUDIO FEEDBACK (BUZZER)
*/

void setupbuzzer() {
  pinMode(buzzerPin, OUTPUT);
}

void soundBuzzer() {
  // Sequence of tones for notification/celebration
  tone(buzzerPin, 1108.73, 169); delay(169);
  tone(buzzerPin, 987.77, 169);  delay(169);
  tone(buzzerPin, 1108.73, 508); delay(508);
  tone(buzzerPin, 739.99, 1016); delay(1016);
  tone(buzzerPin, 0, 254);       delay(254);
  // ... (Sequence continued)
}


/*
  COMMUNICATION PROTOCOL (PI TO ARDUINO)
*/

void sendStatus() {
  TPacket statusPacket;
  statusPacket.packetType = PACKET_TYPE_RESPONSE;
  statusPacket.command = RESP_STATUS;
  
  statusPacket.params[0] = leftForwardTicks;
  statusPacket.params[1] = rightForwardTicks;
  statusPacket.params[2] = leftReverseTicks;
  statusPacket.params[3] = rightReverseTicks;
  statusPacket.params[4] = leftForwardTicksTurns;
  statusPacket.params[5] = rightForwardTicksTurns;
  statusPacket.params[6] = leftReverseTicksTurns;
  statusPacket.params[7] = rightReverseTicksTurns;
  statusPacket.params[8] = forwardDist;
  statusPacket.params[9] = reverseDist;
  
  sendResponse(&statusPacket);
}

void sendColour() {
  readcolours();
  TPacket colourPacket;
  colourPacket.packetType = PACKET_TYPE_RESPONSE;
  colourPacket.command = RESP_COLOUR;
  colourPacket.params[0] = red;
  colourPacket.params[1] = green;
  colourPacket.params[2] = blue;
  sendResponse(&colourPacket);
}

void sendDist() {
  uint32_t ultrasonicDist = ultrareading();
  TPacket distancePacket;
  distancePacket.packetType = PACKET_TYPE_RESPONSE;
  distancePacket.command = RESP_DIST;
  distancePacket.params[0] = ultrasonicDist;
  sendResponse(&distancePacket);
  sendOK();
}


/*
  LOW-LEVEL INTERRUPTS AND BARE-METAL SETUP
*/

void enablePullups() {
  // Bare-metal: Enable pull-ups on pins 18 (PD3) and 19 (PD2)
  DDRD &= ~0b00001100;              // Set as inputs
  PORTD |= 0b00001100;              // Enable pull-ups
}

void setupEINT() {
  // Configure INT2 and INT3 for falling edge trigger
  EIMSK |= 0b00001100;
  EICRA |= 0b10100000;
}

ISR(INT2_vect) { rightISR(); }
ISR(INT3_vect) { leftISR(); }

void leftISR() {
  if (dir == FORWARD) {
    leftForwardTicks++;
    forwardDist = (unsigned long)((float)leftForwardTicks / COUNTS_PER_REV * WHEEL_CIRC);
  } else if (dir == BACKWARD) {
    leftReverseTicks++;
    reverseDist = (unsigned long)((float)leftReverseTicks / COUNTS_PER_REV * WHEEL_CIRC);
  } else if (dir == LEFT) {
    leftReverseTicksTurns++;
  } else if (dir == RIGHT) {
    leftForwardTicksTurns++;
  }
}

void rightISR() {
  if (dir == FORWARD)  rightForwardTicks++;
  else if (dir == BACKWARD) rightReverseTicks++;
  else if (dir == LEFT)     rightForwardTicksTurns++;
  else if (dir == RIGHT)    rightForwardTicksTurns++;
}


/*
  CORE EXECUTION (SETUP & LOOP)
*/

void setup() {
  alexDiagonal = sqrt((ALEX_LENGTH * ALEX_LENGTH) + (ALEX_BREADTH * ALEX_BREADTH));
  alexCirc = PI * alexDiagonal;

  cli();                            // Disable interrupts during setup
  setupEINT();
  setupSerial();
  enablePullups();
  initializeState();
  setupcoloursensor();
  setupultra();
  setupbuzzer();
  sei();                            // Re-enable interrupts
}

void loop() {
  TPacket recvPacket;
  TResult result = readPacket(&recvPacket);

  if (result == PACKET_OK) {
    handlePacket(&recvPacket);
  } else if (result == PACKET_BAD) {
    sendBadPacket();
  } else if (result == PACKET_CHECKSUM_BAD) {
    sendBadChecksum();
  }
}