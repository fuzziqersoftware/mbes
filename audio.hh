#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

void init_al();
void exit_al();

class generated_sound {
public:
  virtual ~generated_sound();

  void play();

protected:
  generated_sound(float seconds, uint32_t sample_rate = 44100);

  void create_al_objects();

  float seconds;
  uint32_t sample_rate;

  ALuint buffer_id;
  ALuint source_id;
  size_t num_samples;
  int16_t* samples;
};

class sine_wave : public generated_sound {
public:
  sine_wave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

protected:
  float frequency;
  float volume;
};

class square_wave : public generated_sound {
public:
  square_wave(float frequency, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

protected:
  float frequency;
  float volume;
};

class white_noise : public generated_sound {
public:
  white_noise(float seconds, float volume = 1.0, uint32_t sample_rate = 44100);

protected:
  float volume;
};

class split_noise : public generated_sound {
public:
  split_noise(int split_distance, float seconds, float volume = 1.0,
      uint32_t sample_rate = 44100);

protected:
  int split_distance;
  float volume;
};
