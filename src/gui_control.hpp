#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv.hpp"
#include "source.hpp"
#include "control.hpp"

namespace atv
{

struct GuiControl : public Control
{
  struct State;

  GuiControl() : GuiControl(/*fps*/ 60.0, /*randomizeSettings*/ false) {}

  GuiControl(double _fps, bool _randomizeSettings);

  void setRNG(uint64_t rngSeed) override
  {
    this->rng = cv::RNG(rngSeed);
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

  // to be called from the GUI
  void startFadeOut();
  void startSwitchChannel(int newChannel);

  Operation getNext() override;

  cv::RNG rng;
  bool randomizeSettings;
  double fps;

  // state
  int frameCounter;
  int channel;
  std::shared_ptr<State> currentState;

  Knobs knobs;
};


}