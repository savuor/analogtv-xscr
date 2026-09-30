#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv.hpp"
#include "source.hpp"
#include "control.hpp"

namespace atv
{

struct RandomControl : public Control
{
  //TODO: delegate construction
  RandomControl() {}

  RandomControl(bool _fixSettings, double _fps, double _duration, bool _powerUpDown)
  {
    this->fixSettings = _fixSettings;

    this->fps = _fps;
    this->duration = _duration;
    this->usePowerUpDown = _powerUpDown;
  }

  void setRNG(uint64_t rngSeed) override
  {
    this->rng = cv::RNG(rngSeed);
  }


  bool overrideSettings() const override
  {
    return false;
  }

  // not used, returns a default size
  cv::Size getSize() const override
  {
    return cv::Size(640, 480);
  }

  std::vector<nlohmann::json> getSources() const override
  {
    return {};
  }

  std::vector<nlohmann::json> getOutputs() const override
  {
    return {};
  }

  // why const ref to sources does not work?
  void createChannels(const std::vector<std::shared_ptr<atv::Source>> sources) override;

  Knobs getKnobs() override
  {
    return knobs;
  }

  void rotateKnobsStart();

  void rotateKnobsSwitch();

  void run() override;

  double getTime() override
  {
    return this->frameCounter / this->fps;
  }

  double getFps() const override
  {
    return this->fps;
  }

  Operation getNext() override;

  cv::RNG rng;

  bool fixSettings;

  double duration;
  double fps;
  bool usePowerUpDown;

  // state
  int frameCounter;
  int channel;
  int lastFrame;
  int channelLastFrame;
  int fadeOutFirstFrame;
  int powerUpLastFrame;

  // for fading out
  double lastBrightness;

  Knobs knobs;
};


}