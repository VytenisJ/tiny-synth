#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Methods
void noteOn(byte channel, byte note, byte velocity);
void noteOff(byte channel, byte note, byte velocity);
void controlChange(byte channel, byte control, byte value);
int findVoice(byte note);
int findAvailableVoice();
void freeVoices();
void playNote(byte note, byte velocity);
void stopNote(byte note);
void voicePlay(int voice, byte note, byte velocity);
void voiceStop(int voice);
float getFrequency(byte note);
void readHardwareInput();

#define TOTAL_VOICES 4

#define ATTACK_PIN 14
#define SAW_AMOUNT 15
#define TRIANGLE_AMOUNT 16

const float DIV127 = 0.00787;

// GUItool: begin automatically generated code
//AudioSynthWaveform       waveform2;      //xy=127,436
//AudioSynthWaveform       waveform1;      //xy=129,380
AudioMixer4              mixer1;         //xy=386,409
AudioFilterStateVariable filter1;        //xy=600,412
AudioEffectEnvelope      envelope1;      //xy=551,419
AudioOutputAnalogStereo  dacs1;          //xy=681,380
//AudioConnection          patchCord1(waveform2, 0, mixer1, 1);
//AudioConnection          patchCord2(waveform1, 0, mixer1, 0);
//AudioConnection          patchCord3(mixer1, envelope1);
AudioConnection*         patchCords[TOTAL_VOICES * 5];
AudioConnection          filterPatchCord(mixer1, 0, filter1, 0);
AudioConnection          patchCord4(filter1, 0, dacs1, 0);
AudioConnection          patchCord5(filter1, 0, dacs1, 1);
// GUItool: end automatically generated code

int global_attack = 0;
float oscPrimaryAmplitude = 0.0;
float oscSecondaryAmplitude = 0.0;
float detuneFactor = 1;
int oscPrimaryWaveform = WAVEFORM_SAWTOOTH;
int oscSecondaryWaveform = WAVEFORM_SAWTOOTH;

struct voice_info {
  AudioSynthWaveform oscPrimary;
  AudioSynthWaveform oscSecondary;
  AudioEffectEnvelope envPrimary;
  AudioEffectEnvelope envSecondary;
  AudioMixer4 mixer;
  byte note;
  float freq;
  bool playing;
};

voice_info voices[TOTAL_VOICES];

int voicesPlaying = 0;
int voicesStopped = 0;
int evictVoice = 0;

void setup() {
  Serial.begin(115200);
  AudioMemory(120);
  usbMIDI.setHandleNoteOn(noteOn);
  usbMIDI.setHandleNoteOff(noteOff);
  usbMIDI.setHandleControlChange(controlChange);

  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscPrimary.begin(oscPrimaryWaveform);
    voices[i].oscPrimary.amplitude(oscPrimaryAmplitude);
    voices[i].oscPrimary.pulseWidth(0.15);

    voices[i].oscSecondary.begin(oscSecondaryWaveform);
    voices[i].oscSecondary.amplitude(oscSecondaryAmplitude);
    voices[i].oscSecondary.pulseWidth(0.15);

    voices[i].envPrimary.attack(global_attack);
    voices[i].envPrimary.decay(50);
    voices[i].envPrimary.sustain(0.75);
    voices[i].envPrimary.release(500);

    voices[i].envSecondary.attack(global_attack);
    voices[i].envSecondary.decay(50);
    voices[i].envSecondary.sustain(0.75);
    voices[i].envSecondary.release(500);

    patchCords[i * 5] = new AudioConnection(voices[i].oscPrimary, voices[i].envPrimary);
    patchCords[i * 5 + 1] = new AudioConnection(voices[i].oscSecondary, voices[i].envSecondary);
    patchCords[i * 5 + 2] = new AudioConnection(voices[i].envPrimary, 0, voices[i].mixer, 0);
    patchCords[i * 5 + 3] = new AudioConnection(voices[i].envSecondary, 0, voices[i].mixer, 1);
    patchCords[i * 5 + 4] = new AudioConnection(voices[i].mixer, 0, mixer1, i);
  }
}

void setVoices() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscPrimary.begin(WAVEFORM_SAWTOOTH);
    voices[i].oscPrimary.amplitude(oscPrimaryAmplitude);
    voices[i].oscPrimary.pulseWidth(0.15);

    voices[i].oscSecondary.begin(WAVEFORM_TRIANGLE);
    voices[i].oscSecondary.amplitude(oscSecondaryAmplitude);
    voices[i].oscSecondary.pulseWidth(0.15);

    voices[i].envPrimary.attack(global_attack);
    voices[i].envPrimary.decay(50);
    voices[i].envPrimary.sustain(0.75);
    voices[i].envPrimary.release(500);

    voices[i].envSecondary.attack(global_attack);
    voices[i].envSecondary.decay(50);
    voices[i].envSecondary.sustain(0.75);
    voices[i].envSecondary.release(500);
  }
}

void setWaveforms() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscPrimary.begin(oscPrimaryWaveform);
    voices[i].oscSecondary.begin(oscSecondaryWaveform);
  }
}

void setFrequencies() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscSecondary.frequency(voices[i].freq * detuneFactor);
  }
}

void loop() {
  readHardwareInput();
  usbMIDI.read();
}

void noteOn(byte channel, byte note, byte velocity) {
  if (note > 23 && note < 108) {
    //voicePlay(0, note, velocity);
    playNote(note, velocity);
  }
  Serial.printf("\n**** NoteOn: channel==%hhu,note==%hhu,playing==%hhu,stopped==%hhu ****", channel, note, voicesPlaying, voicesStopped);
}

void noteOff(byte channel, byte note, byte velocity) {
  if (note > 23 && note < 108) {
    stopNote(note);
  }
  Serial.printf("\n**** NoteOff: channel==%hhu,note==%hhu,playing==%hhu,stopped==%hhu ****", channel, note, voicesPlaying, voicesStopped);
}

void controlChange(byte channel, byte control, byte value) {
  float filterFreq = 1;
  float filterRes = 0.7;

  switch (control) {
    case 101:
      detuneFactor = 1 - (0.05 * (value * DIV127));
      setFrequencies();
      break;

    case 102:
      filterFreq = 10000 * value * DIV127;
      filter1.frequency(filterFreq);
      break;

    case 103:
      filterRes = (4.3 * (value * DIV127)) + 0.7;
      filter1.resonance(filterRes);
      break;

    case 104:
      switch (value) {
        case 0:
          oscPrimaryWaveform = WAVEFORM_SAWTOOTH;
          break;

        case 1:
          oscPrimaryWaveform = WAVEFORM_SQUARE;
          break;

        case 2:
          oscPrimaryWaveform = WAVEFORM_TRIANGLE;
          break;
        
        case 3:
          oscPrimaryWaveform = WAVEFORM_PULSE;
          break;
      }
      setWaveforms();
      break;

    case 105:
      switch (value) {
        case 0:
          oscSecondaryWaveform = WAVEFORM_SAWTOOTH;
          break;

        case 1:
          oscSecondaryWaveform = WAVEFORM_SQUARE;
          break;

        case 2:
          oscSecondaryWaveform = WAVEFORM_TRIANGLE;
          break;
        
        case 3:
          oscSecondaryWaveform = WAVEFORM_PULSE;
          break;
      }
      setWaveforms();
      break;
  }
}

int findVoice(byte note) {
  int i;
  int voicesUsed = voicesStopped + voicesPlaying;
  for (i = voicesStopped; i < voicesUsed && !(voices[i].note == note); ++i);
  if (i == voicesUsed) {
    return TOTAL_VOICES;
  }

  voice_info temp = voices[i];
  voices[i] = voices[voicesStopped];
  voices[voicesStopped] = temp;
  --voicesPlaying;

  return voicesStopped++;
}

int findAvailableVoice() {
  int i;
  int voicesUsed = voicesStopped + voicesPlaying;

  if (voicesUsed < TOTAL_VOICES) {
    for (i = voicesUsed; i < TOTAL_VOICES; ++i);
    if (i < TOTAL_VOICES) {
      voice_info temp = voices[i];
      voices[i] = voices[voicesUsed];
      voices[voicesUsed] = temp;
    }
    i = voicesUsed;
    voicesPlaying++;
  } else {
    if (voicesStopped) {
      i = evictVoice % voicesStopped;
      voice_info temp = voices[i];
      voicesStopped--;
      voices[i] = voices[voicesStopped];
      voices[voicesStopped] = temp;
      voicesPlaying++;
      i = voicesStopped;
    } else {
      i = evictVoice;
    }
  }

  evictVoice++;
  evictVoice %= TOTAL_VOICES;
  
  return i;
}

void freeVoices() {
  for (int i = 0; i < voicesStopped; i++) {
    if (!voices[i].envPrimary.isActive()) {
      voice_info temp = voices[i];
      --voicesStopped;
      voices[i] = voices[voicesStopped];
      int voicesUsed = voicesStopped + voicesPlaying;
      voices[voicesStopped] = voices[voicesUsed];
      voices[voicesUsed] = temp;
    }
  }
}

void playNote(byte note, byte velocity) {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    if (!voices[i].playing) {
      voicePlay(i, note, velocity);
      break;
    }
  }
}

void stopNote(byte note) {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    if (voices[i].note == note) {
      voiceStop(i);
      break;
    }
  }
}

void voicePlay(int voice, byte note, byte velocity) {
  float freq = getFrequency(note);
  float vel = velocity * DIV127;

  voices[voice].note = note;
  voices[voice].freq = freq;

  voices[voice].oscPrimary.frequency(freq);
  voices[voice].oscPrimary.amplitude(oscPrimaryAmplitude);

  voices[voice].oscSecondary.frequency(freq * detuneFactor);
  voices[voice].oscSecondary.amplitude(oscSecondaryAmplitude);

  voices[voice].envPrimary.attack(global_attack);
  voices[voice].envPrimary.noteOn();

  voices[voice].envSecondary.attack(global_attack);
  voices[voice].envSecondary.noteOn();
  voices[voice].playing = true;
}

void voiceStop(int voice) {
  voices[voice].envPrimary.noteOff();
  voices[voice].envSecondary.noteOff();
  voices[voice].playing = false;
}

float getFrequency(byte note) {
  return (float) 440.0 * (float) pow(2.0, (note - 69) / 12.0);
}

void readHardwareInput() {
  int attackPot = analogRead(ATTACK_PIN);
  int sawPot = analogRead(SAW_AMOUNT);
  int trianglePot = analogRead(TRIANGLE_AMOUNT);

  global_attack = map(attackPot, 0, 1023, 0, 3000);
  oscPrimaryAmplitude = (float)(map(sawPot, 0, 1023, 0, 100) / 100.0);
  oscSecondaryAmplitude = (float)(map(trianglePot, 0, 1023, 0, 100) / 100.0);

  Serial.printf("\n**** GLOBAL ATTACK: pot==%hhu,calculated==%hhu", attackPot, global_attack);
  Serial.printf("  **** GLOBAL SAW AMPLITUDE: pot==%hhu,calculated==%.2f", sawPot, oscPrimaryAmplitude);
  Serial.printf("  **** GLOBAL TRIANGLE AMPLITUDE: pot==%hhu,calculated==%.2f", trianglePot, oscSecondaryAmplitude);
}