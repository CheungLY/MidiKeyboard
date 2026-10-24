#include <Adafruit_TinyUSB.h>
#include <Encoder.h>

// =====================================================
// USB MIDI
// =====================================================

Adafruit_USBD_MIDI usb_midi;

// =====================================================
// KEYS
// =====================================================

const uint8_t keyPins[8] =
{
  1,  // TOUCH1
  2,  // TOUCH2
  3,  // TOUCH3
  4,  // TOUCH4
  5,  // TOUCH5
  6,  // TOUCH6
  7,  // TOUCH7
  8   // TOUCH8
};

bool keyState[8];

const uint8_t noteMap[8] =
{
  60, // C4
  62, // D4
  64, // E4
  65, // F4
  67, // G4
  69, // A4
  71, // B4
  72  // C5
};

// =====================================================
// BUTTONS
// =====================================================

const int octaveDownPin = 13;   // SWITCH1
const int octaveUpPin   = 12;   // SWITCH2
const int programPin    = 11;   // SWITCH3

bool oldUp = false;
bool oldDown = false;
bool oldProgram = false;

// =====================================================
// ROTARY ENCODER
// =====================================================

const int encoderA = 10;
const int encoderB = 9;

Encoder encoder(encoderA, encoderB);

long lastEncoderValue = 0;

uint8_t volumeValue = 100;

// =====================================================
// PITCH BENDER
// =====================================================

const int pitchPin = RX;  // PitchBenderY

int lastPitch = 8192;

// =====================================================
// MIDI STATE
// =====================================================

int octaveOffset = 0;
uint8_t currentProgram = 0;

// =====================================================
// MIDI FUNCTIONS
// =====================================================

void sendNoteOn(uint8_t note)
{
  uint8_t msg[3] =
  {
    0x90,
    note,
    127
  };

  usb_midi.write(msg, 3);
}

void sendNoteOff(uint8_t note)
{
  uint8_t msg[3] =
  {
    0x80,
    note,
    0
  };

  usb_midi.write(msg, 3);
}

void sendControlChange(uint8_t cc,
                       uint8_t value)
{
  uint8_t msg[3] =
  {
    0xB0,
    cc,
    value
  };

  usb_midi.write(msg, 3);
}

void sendProgramChange(uint8_t program)
{
  uint8_t msg[2] =
  {
    0xC0,
    program
  };

  usb_midi.write(msg, 2);
}

void sendPitchBend(int value)
{
  value = constrain(value, 0, 16383);

  uint8_t lsb = value & 0x7F;
  uint8_t msb = (value >> 7) & 0x7F;

  uint8_t msg[3] =
  {
    0xE0,
    lsb,
    msb
  };

  usb_midi.write(msg, 3);
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  TinyUSBDevice.begin();

  for(int i=0;i<8;i++)
  {
    pinMode(keyPins[i], INPUT_PULLUP);
  }

  pinMode(octaveUpPin, INPUT_PULLUP);
  pinMode(octaveDownPin, INPUT_PULLUP);
  pinMode(programPin, INPUT_PULLUP);

  analogReadResolution(12);
}

// =====================================================
// KEYS
// =====================================================

void updateKeys()
{
  for(int i=0;i<8;i++)
  {
    bool pressed =
      !digitalRead(keyPins[i]);

    if(pressed && !keyState[i])
    {
      sendNoteOn(
        noteMap[i] + octaveOffset
      );
    }

    if(!pressed && keyState[i])
    {
      sendNoteOff(
        noteMap[i] + octaveOffset
      );
    }

    keyState[i] = pressed;
  }
}

// =====================================================
// BUTTONS
// =====================================================

void updateButtons()
{
  bool up =
    !digitalRead(octaveUpPin);

  bool down =
    !digitalRead(octaveDownPin);

  bool prog =
    !digitalRead(programPin);

  if(up && !oldUp)
  {
    octaveOffset += 12;

    if(octaveOffset > 48)
      octaveOffset = 48;
  }

  if(down && !oldDown)
  {
    octaveOffset -= 12;

    if(octaveOffset < -48)
      octaveOffset = -48;
  }

  if(prog && !oldProgram)
  {
    currentProgram++;

    if(currentProgram > 127)
      currentProgram = 0;

    sendProgramChange(currentProgram);
  }

  oldUp = up;
  oldDown = down;
  oldProgram = prog;
}

// =====================================================
// ENCODER
// =====================================================

void updateEncoder()
{
  long value =
    encoder.read() / 4;

  if(value != lastEncoderValue)
  {
    if(value > lastEncoderValue)
      volumeValue++;

    if(value < lastEncoderValue)
      volumeValue--;

    volumeValue =
      constrain(volumeValue,
                0,
                127);

    sendControlChange(
      7,
      volumeValue
    );

    lastEncoderValue = value;
  }
}

// =====================================================
// PITCH BEND
// =====================================================

void updatePitchBend()
{
  int raw =
    analogRead(pitchPin);

  int bend =
    map(raw,
        0,
        4095,
        0,
        16383);

  if(abs(bend - lastPitch) > 20)
  {
    sendPitchBend(bend);

    lastPitch = bend;
  }
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  updateKeys();

  updateButtons();

  updateEncoder();

  updatePitchBend();