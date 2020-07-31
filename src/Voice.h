#include <Audio.h>

struct Voice {
  AudioMixer4 oscModMixer1;
  AudioMixer4 oscModMixer2;
  AudioMixer4 pwmMixer1;
  AudioMixer4 pwmMixer2;
  AudioSynthWaveformModulated osc1;
  AudioSynthWaveformModulated osc2;
  AudioMixer4 oscMixer;
  AudioMixer4 filterModMixer;
  AudioFilterStateVariable filter;
  AudioMixer4 filterMixer;
  AudioEffectEnvelope env;
  byte note;
  float freq;
  bool playing;
};