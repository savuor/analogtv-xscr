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
  double fps;
  int frameCounter;

  size_t sequenceIndex;
  int sequenceLastFrame;
  bool powerUp;

  // for the cases of turn OFF/ON events we will need this
  int turnOnFrame;
  int currentChannel;
  int powerDownFrames;
  int powerDownStartFrame;
  double targetBrightness;

  int lastFrame;
};

} // ::atv