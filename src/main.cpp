#include <Arduino.h>
#include <Audio.h>
#include <MIDI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <Wire.h>

#include <Constants.h>
#include <Hardware.h>
#include <MIDICC.h>
#include <Voice.h>

// Methods
void noteOn(byte channel, byte note, byte velocity);
void noteOff(byte channel, byte note, byte velocity);
void controlChange(byte channel, byte control, byte value);
void pitchBendControl(byte channel, int bend);
int findVoice(byte note);
int findAvailableVoice();
void freeVoices();
void playNote(byte note, byte velocity);
void stopNote(byte note);
void voicePlay(int voice, byte note, byte velocity);
void voiceStop(int voice);
float getFrequency(byte note);

// GUItool: begin automatically generated code

AudioSynthWaveformDc     constant1DC;
AudioSynthWaveformDc     pitchBend;

AudioSynthWaveform       pitchLFO;
AudioSynthWaveform       filterLFO;
AudioSynthWaveform       pwmLFO;

AudioSynthWaveformDc     pwmConstant;

AudioMixer4              mainMixer;         //xy=386,409

AudioOutputAnalogStereo  dacs1;          //xy=681,380
AudioConnection*         patchCords[TOTAL_VOICES * 22];
AudioConnection          patchCord1(mainMixer, 0, dacs1, 0);
AudioConnection          patchCord2(mainMixer, 0, dacs1, 1);

// GUItool: end automatically generated code

// ENVELOPE
int   global_adsr_attack = 0;
int   global_adsr_decay = 50;
float global_adsr_sustain = 0.75f;
int   global_adsr_release = 500;

// OSCILLATORS
float global_osc_1_gain = 0.75f;
float global_osc_2_gain = 0.75f;

int   global_osc_1_waveform = WAVEFORM_SAWTOOTH;
int   global_osc_2_waveform = WAVEFORM_SAWTOOTH;

int   global_osc_2_octave = 0;

float global_detune_factor = 1.0f;

// FILTER
float global_filter_frequency = 1.0f;
float global_filter_resonance = 0.707f;
float global_filter_octave = 2.0f;

byte  global_filter_mode = 0;

float global_filter_lp_gain = 1.0f;
float global_filter_bp_gain = 0.0f;
float global_filter_hp_gain = 0.0f;

// FILTER LFO
float global_filter_lfo_freq = 0.0f;
float global_filter_lfo_amount = 0.0f;
int   global_filter_lfo_waveform = WAVEFORM_SINE;

// PWM LFO
float global_pwm_lfo_freq = 0.0f;
float global_pwm_lfo_amount = 0.0f;
float global_pwm_lfo_waveform = WAVEFORM_SINE;

// PITCH LFO
float global_pitch_lfo_freq = 0.0f;

Voice voices[TOTAL_VOICES];

MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, MIDI);

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);

  MIDI.setHandleNoteOn(noteOn);
  MIDI.setHandleNoteOff(noteOff);
  MIDI.setHandleControlChange(controlChange);
  MIDI.setHandlePitchBend(pitchBendControl);

  DDRD = B11111111;
  PORTD = B00000000;
  Serial.begin(115200);
  AudioMemory(120);
  usbMIDI.setHandleNoteOn(noteOn);
  usbMIDI.setHandleNoteOff(noteOff);
  usbMIDI.setHandleControlChange(controlChange);

  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].osc1.begin(global_osc_1_waveform);
    voices[i].osc1.amplitude(0.75);
    voices[i].osc1.frequencyModulation(2);

    voices[i].osc2.begin(global_osc_2_waveform);
    voices[i].osc2.amplitude(0.75);
    voices[i].osc2.frequencyModulation(2);

    voices[i].oscMixer.gain(0, global_osc_1_gain);
    voices[i].oscMixer.gain(1, global_osc_2_gain);

    voices[i].filter.frequency(global_filter_frequency);
    voices[i].filter.resonance(global_filter_resonance);

    voices[i].filterMixer.gain(0, global_filter_lp_gain);
    voices[i].filterMixer.gain(1, global_filter_bp_gain);
    voices[i].filterMixer.gain(2, global_filter_hp_gain);

    voices[i].env.attack(global_adsr_attack);
    voices[i].env.decay(global_adsr_decay);
    voices[i].env.sustain(global_adsr_sustain);
    voices[i].env.release(global_adsr_release);

    voices[i].oscModMixer1.gain(0, 1.0f);
    voices[i].oscModMixer1.gain(1, 1.0f);

    voices[i].oscModMixer2.gain(0, 1.0f);
    voices[i].oscModMixer2.gain(1, 1.0f);

    patchCords[i] =     new AudioConnection(voices[i].osc1, 0, voices[i].oscMixer, 0);
    patchCords[i + 1] = new AudioConnection(voices[i].osc2, 0, voices[i].oscMixer, 1);
    patchCords[i + 2] = new AudioConnection(voices[i].oscMixer, 0, voices[i].filter, 0);
    patchCords[i + 3] = new AudioConnection(voices[i].filter, 0, voices[i].filterMixer, 0);
    patchCords[i + 4] = new AudioConnection(voices[i].filter, 1, voices[i].filterMixer, 1);
    patchCords[i + 5] = new AudioConnection(voices[i].filter, 2, voices[i].filterMixer, 2);
    patchCords[i + 6] = new AudioConnection(voices[i].filterMixer, 0, voices[i].env, 0);
    patchCords[i + 7] = new AudioConnection(voices[i].env, 0, mainMixer, i);
    patchCords[i + 8] = new AudioConnection(voices[i].filterModMixer, 0, voices[i].filter, 1);
    patchCords[i + 9] = new AudioConnection(filterLFO, 0, voices[i].filterModMixer, 0);
    patchCords[i + 10] = new AudioConnection(pwmConstant, 0, voices[i].pwmMixer1, 0);
    patchCords[i + 11] = new AudioConnection(pwmConstant, 0, voices[i].pwmMixer2, 0);
    patchCords[i + 12] = new AudioConnection(pwmLFO, 0, voices[i].pwmMixer1, 1);
    patchCords[i + 13] = new AudioConnection(pwmLFO, 0, voices[i].pwmMixer2, 1);
    patchCords[i + 14] = new AudioConnection(voices[i].pwmMixer1, 0, voices[i].osc1, 1);
    patchCords[i + 15] = new AudioConnection(voices[i].pwmMixer2, 0, voices[i].osc2, 1);
    patchCords[i + 16] = new AudioConnection(pitchLFO, 0, voices[i].oscModMixer1, 1);
    patchCords[i + 17] = new AudioConnection(pitchLFO, 0, voices[i].oscModMixer2, 1);
    patchCords[i + 18] = new AudioConnection(pitchBend, 0, voices[i].oscModMixer1, 0);
    patchCords[i + 19] = new AudioConnection(pitchBend, 0, voices[i].oscModMixer2, 0);
    patchCords[i + 20] = new AudioConnection(voices[i].oscModMixer1, 0, voices[i].osc1, 0);
    patchCords[i + 21] = new AudioConnection(voices[i].oscModMixer2, 0, voices[i].osc2, 0);
  }

  pitchLFO.amplitude(1.0f);
  pitchLFO.frequency(global_pitch_lfo_freq);
  pitchLFO.begin(WAVEFORM_SINE);

  filterLFO.amplitude(1.0f);
  filterLFO.frequency(global_filter_lfo_freq);
  filterLFO.begin(WAVEFORM_SINE);

  pwmLFO.amplitude(global_pwm_lfo_amount);
  pwmLFO.frequency(global_pwm_lfo_freq);
  pwmLFO.begin(WAVEFORM_SINE);

  mainMixer.gain(0, 0.5);
  mainMixer.gain(1, 0.5);
  mainMixer.gain(2, 0.5);
  mainMixer.gain(3, 0.5);
}

void updateOsc1Waveform() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].osc1.begin(global_osc_1_waveform);
  }
}

void updateOsc2Waveform() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].osc2.begin(global_osc_2_waveform);
  }
}

void setFrequencies() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].osc2.frequency(NOTEFREQS[voices[i].note + global_osc_2_octave] * global_detune_factor);
  }
}

void setOsc1Gain() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscMixer.gain(0, global_osc_1_gain);
  }
}

void setOsc2Gain() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].oscMixer.gain(1, global_osc_2_gain);
  }
}

void setFilterFrequency() {
  if (global_filter_frequency > 2500) {
    global_filter_octave = 2.0f;
  } else if (global_filter_frequency < 60) {
    global_filter_octave = 7.0f;
  } else {
    global_filter_octave = 2.0f + ((2560 - global_filter_frequency) / 500);
  }

  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].filter.frequency(global_filter_frequency);
    voices[i].filter.octaveControl(global_filter_octave);
  }
}

void setFilterResonance() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].filter.resonance(global_filter_resonance);
  }
}

void updateFilterLFOWaveform() {
  filterLFO.begin(global_filter_lfo_waveform);
}

void updateFilterLFOFrequency() {
  filterLFO.frequency(global_filter_lfo_freq);
}

void updateFilterLFOAmount() {
  filterLFO.amplitude(global_filter_lfo_amount);
}

void updatePWMLFOWaveform() {
  pwmLFO.begin(global_pwm_lfo_waveform);
}

void updatePWMLFOFrequency() {
  pwmLFO.frequency(global_pwm_lfo_freq);
}

void updatePWMLFOAmount() {
  pwmLFO.amplitude(global_pwm_lfo_amount);
  pwmConstant.amplitude(global_pwm_lfo_amount);
}

void setFilterMode(byte mode) {
  switch (mode) {
    case 0: // LP
      global_filter_lp_gain = 1.0f;
      global_filter_bp_gain = 0;
      global_filter_hp_gain = 0;
      break;

    case 1: // BP
      global_filter_lp_gain = 0;
      global_filter_bp_gain = 1.0f;
      global_filter_hp_gain = 0;
      break;

    case 2: // HP
      global_filter_lp_gain = 0;
      global_filter_bp_gain = 0;
      global_filter_hp_gain = 1.0f;
      break;
  }

  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].filterMixer.gain(0, global_filter_lp_gain);
    voices[i].filterMixer.gain(1, global_filter_bp_gain);
    voices[i].filterMixer.gain(2, global_filter_hp_gain);
  }
}

void updateAmpEnvAttack() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].env.attack(global_adsr_attack);
  }
}

void updateAmpEnvDecay() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].env.decay(global_adsr_decay);
  }
}

void updateAmpEnvSustain() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].env.sustain(global_adsr_sustain);
  }
}

void updateAmpEnvRelease() {
  for (int i = 0; i < TOTAL_VOICES; i++) {
    voices[i].env.release(global_adsr_release);
  }
}

void readMux() {
  for (int channel = 0; channel < MUX_CHANNELS; channel++) {
    PORTD = channel;
    float mux1value = analogRead(A14) >> 3;
    float mux2value = analogRead(A15) >> 3;
    switch (channel)
    {
      case MUX_OSC_1_GAIN:
        controlChange(0, CC_OSC_1_GAIN, mux1value);
        break;

      case MUX_OSC_2_GAIN:
        controlChange(0, CC_OSC_2_GAIN, mux1value);
        break;

      case MUX_OSC_2_OCTAVE:
        controlChange(0, CC_OSC_2_OCTAVE, mux1value);
        break;

      case MUX_OSC_2_DETUNE:
        controlChange(0, CC_OSC_2_DETUNE, mux1value);
        break;

      case MUX_FILTER_CUTOFF:
        controlChange(0, CC_FILTER_CUTOFF, mux1value);
        break;

      case MUX_FILTER_RESONANCE:
        controlChange(0, CC_FILTER_RESONANCE, mux1value);
        break;

      case MUX_OSC_1_WAVEFORM:
        controlChange(0, CC_OSC_1_WAVEFORM, mux1value);
        break;

      case MUX_OSC_2_WAVEFORM:
        controlChange(0, CC_OSC_2_WAVEFORM, mux1value);
        break;

      case MUX_AMP_ADSR_ATTACK:
        controlChange(0, CC_AMP_ADSR_ATTACK, mux1value);
        break;

      case MUX_AMP_ADSR_DECAY:
        controlChange(0, CC_AMP_ADSR_DECAY, mux1value);
        break;

      case MUX_AMP_ADSR_SUSTAIN:
        controlChange(0, CC_AMP_ADSR_SUSTAIN, mux1value);
        break;

      case MUX_AMP_ADSR_RELEASE:
        controlChange(0, CC_AMP_ADSR_RELEASE, mux1value);
        break;

      case MUX_FILTER_LFO_LEVEL:
        controlChange(0, CC_FILTER_LFO_LEVEL, mux1value);
        break;

      case MUX_FILTER_LFO_RATE:
        controlChange(0, CC_FILTER_LFO_RATE, mux1value);
        break;

      case MUX_PWM_LEVEL:
        controlChange(0, CC_PWM_LEVEL, mux1value);
        break;

      case MUX_PWM_RATE:
        controlChange(0, CC_PWM_RATE, mux1value);
        break;
    }

    switch (channel)
    {
      case MUX2_FILTER_LFO_WAVEFORM:
        controlChange(0, CC_FILTER_LFO_WAVEFORM, mux2value);
        break;

      case MUX2_PWM_LFO_WAVEFORM:
        controlChange(0, CC_PWM_LFO_WAVEFORM, mux2value);
        break;
    }
  }
}

void loop() {
  MIDI.read();
  readMux();
  usbMIDI.read();
}

void noteOn(byte channel, byte note, byte velocity) {
  if (note >= 0 && note <= 127) {
    playNote(note, velocity);
  }
}

void noteOff(byte channel, byte note, byte velocity) {
  if (note >= 0 && note <= 127) {
    stopNote(note);
  }
}

void pitchBendControl(byte channel, int bend) {
  pitchBend.amplitude(bend * 0.5 * PITCH_BEND_RANGE * DIV12 * DIV8192);
}

int getWaveform(byte value) {
  int result = -1;
  if (value < 20) {
    result = WAVEFORM_SAWTOOTH;
  } else if (value < 47) {
    result = WAVEFORM_SQUARE;
  } else if (value < 85) {
    result = WAVEFORM_TRIANGLE;
  } else {
    result = WAVEFORM_PULSE;
  }

  return result;
}

int getLFOWaveform(byte value) {
  int result = -1;

  if (value < 36) {
    result = WAVEFORM_SINE;
  } else if (value < 80) {
    result = WAVEFORM_SAWTOOTH;
  } else {
    result = WAVEFORM_TRIANGLE;
  }

  return result;
}

void controlChange(byte channel, byte control, byte value) {
  switch (control) {
    case CC_OSC_1_GAIN:
      global_osc_1_gain = LINEAR[value];
      setOsc1Gain();
      break;

    case CC_OSC_2_GAIN:
      global_osc_2_gain = LINEAR[value];
      setOsc2Gain();
      break;

    case CC_OSC_2_DETUNE:
      global_detune_factor = 1 - (MAX_DETUNE * LINEAR[value]);
      setFrequencies();
      break;

    case CC_FILTER_CUTOFF:
      global_filter_frequency = FILTERFREQS[value];
      setFilterFrequency();
      break;

    case CC_FILTER_RESONANCE:
      global_filter_resonance = (13.9f * POWER[value]) + 1.1f;
      setFilterResonance();
      break;

    case CC_OSC_1_WAVEFORM:
      global_osc_1_waveform = getWaveform(value);
      updateOsc1Waveform();
      break;

    case CC_OSC_2_WAVEFORM:
      global_osc_2_waveform = getWaveform(value);
      updateOsc2Waveform();
      break;

    case CC_OSC_2_OCTAVE:
      if (value < 26) {
        global_osc_2_octave = 24;
      } else if (value < 52) {
        global_osc_2_octave = 12;
      } else if (value < 78) {
        global_osc_2_octave = 0;
      } else if (value < 104) {
        global_osc_2_octave = -12;
      } else {
        global_osc_2_octave = -24;
      }
      setFrequencies();
      break;

    case CC_FILTER_MODE:
      setFilterMode(value);
      break;

    case CC_AMP_ADSR_ATTACK:
      global_adsr_attack = ENVTIMES[value];
      updateAmpEnvAttack();
      break;

    case CC_AMP_ADSR_DECAY:
      global_adsr_decay = ENVTIMES[value];
      updateAmpEnvDecay();
      break;

    case CC_AMP_ADSR_SUSTAIN:
      global_adsr_sustain = LINEAR[value];
      updateAmpEnvSustain();
      break;

    case CC_AMP_ADSR_RELEASE:
      global_adsr_release = ENVTIMES[value];
      updateAmpEnvRelease();
      break;

    case CC_FILTER_LFO_RATE:
      global_filter_lfo_freq = MAX_LFO_FREQ * LINEAR[value];
      updateFilterLFOFrequency();
      break;

    case CC_FILTER_LFO_LEVEL:
      global_filter_lfo_amount = LINEAR[value];
      updateFilterLFOAmount();
      break;

    case CC_PWM_RATE:
      global_pwm_lfo_freq = MAX_LFO_FREQ * LINEAR[value];
      updatePWMLFOFrequency();
      break;

    case CC_PWM_LEVEL:
      global_pwm_lfo_amount = LINEAR[value];
      updatePWMLFOAmount();
      break;

    case CC_FILTER_LFO_WAVEFORM:
      global_filter_lfo_waveform = getLFOWaveform(value);
      updateFilterLFOWaveform();
      break;

    case CC_PWM_LFO_WAVEFORM:
      global_pwm_lfo_waveform = getLFOWaveform(value);
      updatePWMLFOWaveform();
      break;
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
  float vel = velocity * DIV127;

  voices[voice].note = note;

  voices[voice].osc1.frequency(NOTEFREQS[note]);
  voices[voice].osc1.amplitude(vel);

  voices[voice].osc2.frequency(NOTEFREQS[note + global_osc_2_octave] * global_detune_factor);
  voices[voice].osc2.amplitude(vel);

  voices[voice].env.attack(global_adsr_attack);
  voices[voice].env.noteOn();

  voices[voice].playing = true;
}

void voiceStop(int voice) {
  voices[voice].env.noteOff();
  voices[voice].playing = false;
}