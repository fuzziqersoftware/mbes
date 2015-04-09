#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include "audio.hh"

const char* al_err_str(ALenum err) {
  switch(err) {
    case AL_NO_ERROR:
      return "AL_NO_ERROR";
    case AL_INVALID_NAME:
      return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
      return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
      return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
      return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
      return "AL_OUT_OF_MEMORY";
  }
  return "unknown";
}

#define __al_check_error(file,line) \
  do { \
    for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError()) \
      fprintf(stderr, "AL error %s at %s:%d\n", al_err_str(err), file, line); \
  } while(0)

#define al_check_error() \
    __al_check_error(__FILE__, __LINE__)


void init_al() {
  const char *defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  ALCdevice* dev = alcOpenDevice(defname);
  ALCcontext* ctx = alcCreateContext(dev, NULL);
  alcMakeContextCurrent(ctx);
}

void exit_al() {
  ALCcontext* ctx = alcGetCurrentContext();
  ALCdevice* dev = alcGetContextsDevice(ctx);

  alcMakeContextCurrent(NULL);
  alcDestroyContext(ctx);
  alcCloseDevice(dev);
}



generated_sound::generated_sound(float seconds, uint32_t sample_rate) :
    seconds(seconds), sample_rate(sample_rate) {

  alGenBuffers(1, &this->buffer_id);
  al_check_error();

  this->num_samples = this->seconds * this->sample_rate;
  this->samples = new short[this->num_samples];
}

generated_sound::~generated_sound() {
  // unbind buffer from source
  alDeleteSources(1, &this->source_id);
  alDeleteBuffers(1, &this->buffer_id);
  delete[] this->samples;
}

void generated_sound::create_al_objects() {
  alBufferData(this->buffer_id, AL_FORMAT_MONO16, this->samples,
      this->num_samples * sizeof(int16_t), this->sample_rate);
  al_check_error();

  alGenSources(1, &this->source_id);
  alSourcei(this->source_id, AL_BUFFER, this->buffer_id);
  al_check_error();
}

void generated_sound::play() {
  alSourcePlay(this->source_id);
}



sine_wave::sine_wave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : generated_sound(seconds, sample_rate),
    frequency(frequency), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    this->samples[x] = 32760 * sin((2.0f * M_PI * this->frequency) / this->sample_rate * x) * this->volume;
  }
  this->create_al_objects();
}

square_wave::square_wave(float frequency, float seconds, float volume,
    uint32_t sample_rate) : generated_sound(seconds, sample_rate),
    frequency(frequency), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    if ((uint64_t)((2 * this->frequency) / this->sample_rate * x) & 1)
      this->samples[x] = 32760 * this->volume;
    else
      this->samples[x] = -32760 * this->volume;
  }
  this->create_al_objects();
}

white_noise::white_noise(float seconds, float volume, uint32_t sample_rate) :
    generated_sound(seconds, sample_rate), volume(volume) {
  for (size_t x = 0; x < this->num_samples; x++) {
    this->samples[x] = rand() - RAND_MAX / 2;
  }
  this->create_al_objects();
}

split_noise::split_noise(int split_distance, float seconds, float volume,
    bool fade_out, uint32_t sample_rate) : generated_sound(seconds, sample_rate),
    split_distance(split_distance), volume(volume) {

  for (size_t x = 0; x < this->num_samples; x += split_distance) {
    this->samples[x] = rand() - RAND_MAX / 2;
  }

  for (size_t x = 0; x < this->num_samples; x++) {
    if ((x % split_distance) == 0)
      continue;

    int first_x = (x / split_distance) * split_distance;
    int second_x = first_x + split_distance;
    float x1p = (float)(x - first_x) / (second_x - first_x);
    if (second_x >= this->num_samples)
      this->samples[x] = 0;
    else
      this->samples[x] = x1p * this->samples[first_x] + (1 - x1p) * this->samples[second_x];
  }

  // TODO generalize
  if (fade_out) {
    for (size_t x = 0; x < this->num_samples; x++) {
      this->samples[x] *= (float)(this->num_samples - x) / this->num_samples;
    }
  }
  this->create_al_objects();
}
