#pragma once

#include "control.hpp"

namespace atv
{

class RecordedControl : public Control
{
public:
  explicit RecordedControl(const std::string& filePath);

  void setRNG(uint64_t rngSeed) override;
  void createChannels(const std::vector<std::shared_ptr<atv::Source>> sources) override;
  Knobs getKnobs() override;
  void run() override;
  Operation getNext() override;
  double getTime() override;
  double getFps() const override;

private:
  struct SequenceEntry
  {
    int channel;
    int durationFrames;
    nlohmann::json knobs;
    nlohmann::json channelSettings;
  };

  void applyKnobs(const nlohmann::json& values);
  void applyChannelSettings(int channel, const nlohmann::json& values);
  void applySequenceEntry(size_t index);

  nlohmann::json settings;
  std::vector<SequenceEntry> sequence;
  Knobs knobs;
  cv::RNG rng;
  double fps;
  int frameCounter;
  size_t sequenceIndex;
  int sequenceFrame;
  int currentChannel;
  int powerUpFrames;
  int powerDownFrames;
  bool poweringUp;
  bool poweringDown;
  bool firstFrame;
  double targetBrightness;
};

} // ::atv