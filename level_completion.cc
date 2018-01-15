#include <stdint.h>
#include <stdio.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <phosg/Filesystem.hh>

#include "level.hh"
#include "level_completion.hh"

using namespace std;

level_completion::level_completion() : state(NotAttempted),
    frames(0xFFFFFFFFFFFFFFFF), extra_items(0), extra_bombs(0),
    cleared_space(0), attenuated_space(0xFFFFFFFFFFFFFFFF) { }

level_completion::level_completion(const level_completion_v1& v1) :
    state(v1.state), frames(v1.frames), extra_items(v1.extra_items),
    extra_bombs(v1.extra_bombs), cleared_space(v1.cleared_space),
    attenuated_space(v1.attenuated_space), entropy(0xFFFFFFFFFFFFFFFF) { }

vector<level_completion> load_level_completion_state_v1(const string& filename) {
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f) {
    return vector<level_completion>();
  }

  fseek(f, 0, SEEK_END);
  uint64_t num = ftell(f) / sizeof(level_completion_v1);
  fseek(f, 0, SEEK_SET);

  vector<level_completion_v1> lc(num);
  fread(lc.data(), lc.size(), sizeof(level_completion_v1), f);
  fclose(f);

  vector<level_completion> ret;
  for (const auto& lcv1 : lc) {
    ret.emplace_back(lcv1);
  }

  return ret;
}

vector<level_completion> load_level_completion_state(const string& filename) {
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f) {
    return vector<level_completion>();
  }

  uint64_t version = 0;
  fread(&version, sizeof(version), 1, f);
  if (version != 2) {
    fclose(f);
    return vector<level_completion>();
  }

  fseek(f, 0, SEEK_END);
  uint64_t num = (ftell(f) - sizeof(version)) / sizeof(level_completion);
  fseek(f, sizeof(version), SEEK_SET);

  vector<level_completion> lc(num);
  fread(lc.data(), lc.size(), sizeof(level_completion), f);
  fclose(f);

  return lc;
}

void save_level_completion_state(const string& filename, const vector<level_completion>& lc) {
  FILE* f = fopen(filename.c_str(), "wb");
  if (!f) {
    throw runtime_error("can\'t open file");
  }

  uint64_t version = 2;
  fwrite(&version, sizeof(version), 1, f);

  fwrite(lc.data(), lc.size(), sizeof(level_completion), f);

  fclose(f);
}



class BitSerializer {
public:
  BitSerializer() : bit_offset(0) { }
  ~BitSerializer() = default;

  void write(bool v) {
    if (!(this->bit_offset & 7)) {
      this->serialized_data.resize(this->serialized_data.size() + 1, 0);
    }

    uint8_t* current = reinterpret_cast<uint8_t*>(
        &this->serialized_data[this->bit_offset >> 3]);
    if (v) {
      *current |= (0x80 >> (this->bit_offset & 7));
    }
    this->bit_offset++;
  }

  void write(const void* data, size_t size) {
    if (this->bit_offset & 7) {
      throw invalid_argument("raw data write not on byte boundary");
    }
    this->serialized_data.append(reinterpret_cast<const char*>(data), size);
    this->bit_offset += (size << 3);
  }

  const string& data() {
    return this->serialized_data;
  }

private:
  string serialized_data;
  size_t bit_offset;
};

class BitDeserializer {
public:
  BitDeserializer(const string& data) : serialized_data(data), bit_offset(0) { }
  ~BitDeserializer() = default;

  bool read() {
    if ((this->bit_offset >> 3) >= this->serialized_data.size()) {
      throw out_of_range("reached end of stream");
    }

    bool ret = (this->serialized_data[this->bit_offset >> 3] & (0x80 >> (this->bit_offset & 7))) ? true : false;
    this->bit_offset++;
    return ret;
  }

  string read(size_t size) {
    if (this->bit_offset & 7) {
      throw invalid_argument("raw data write not on byte boundary");
    }
    string ret = this->serialized_data.substr(this->bit_offset >> 3, size);
    this->bit_offset += (ret.size() << 3);
    return ret;
  }

private:
  string serialized_data;
  size_t bit_offset;
};

deque<struct player_actions> load_recording(const string& filename) {
  BitDeserializer des(load_file(filename));

  string version_str = des.read(sizeof(uint32_t));
  if (version_str != "\x45\x43\x45\x43") {
    throw invalid_argument("incorrect version");
  }

  string count_str = des.read(sizeof(uint32_t));
  if (count_str.size() != sizeof(uint32_t)) {
    throw invalid_argument("count is missing");
  }
  uint32_t count = *reinterpret_cast<const uint32_t*>(count_str.data());

  deque<struct player_actions> recording;
  for (uint32_t x = 0; x < count; x++) {
    struct player_actions actions;
    actions.drop_bomb = des.read();
    uint8_t impulse = des.read() << 2;
    impulse |= des.read() << 1;
    impulse |= des.read();
    actions.impulse = static_cast<enum player_impulse>(impulse);
    recording.emplace_back(actions);
  }

  return recording;
}

void save_recording(const string& filename, const deque<struct player_actions>& recording) {
  BitSerializer ser;

  uint32_t version = 0x43454345;
  ser.write(&version, sizeof(version));

  uint32_t count = recording.size();
  ser.write(&count, sizeof(count));

  for (const auto& actions : recording) {
    ser.write(actions.drop_bomb);
    uint8_t impulse = actions.impulse;
    ser.write((impulse >> 2) & 1);
    ser.write((impulse >> 1) & 1);
    ser.write(impulse & 1);
  }

  save_file(filename, ser.data());
}
