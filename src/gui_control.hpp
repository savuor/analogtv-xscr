#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv.hpp"
#include "source.hpp"
#include "control.hpp"

namespace atv
{

const double POWERUP_DURATION = 6.0;  /* Hardcoded in analogtv.c */
const double POWERDOWN_DURATION = 1.0;  /* Only used here */

struct GuiControl : public Control
{
  struct State
  {
    enum class Type
    {
      POWER_UP, // n frames -> SHOW
      SHOW,
      FADE_OUT, // n frames -> QUIT
      SWITCH,   // 0 frames -> SHOW
      QUIT
    };

    virtual Type getType() const = 0;
    virtual Type nextType() const = 0;
    virtual Operation::Type getOperationType() const
    {
      switch (getType())
      {
        case Type::POWER_UP:
          return Operation::Type::NONE;
        case Type::SHOW:
          return Operation::Type::NONE;
        case Type::FADE_OUT:
          return Operation::Type::NONE;
        case Type::SWITCH:
          return Operation::Type::SWITCH;
        case Type::QUIT:
          return Operation::Type::QUIT;
      }
      return Operation::Type::NONE; // Default case, should not happen
    }

    virtual ~State() {}
    State(int _startFrame = 0, int _lastFrame = 0) :
      startFrame(_startFrame), lastFrame(_lastFrame)
    { }

    static std::shared_ptr<State> create(Type type, int startFrame, double fps, double lastBrightness = 0.0, int newChannel = 0)
    {
      switch (type)
      {
        case Type::POWER_UP:
          return std::make_shared<PowerUpState>(startFrame, fps);
        case Type::SHOW:
          return std::make_shared<ShowState>(startFrame);
        case Type::FADE_OUT:
          return std::make_shared<FadeOutState>(startFrame, fps, lastBrightness);
        case Type::SWITCH:
          return std::make_shared<SwitchState>(startFrame, newChannel);
        case Type::QUIT:
          return std::make_shared<QuitState>(startFrame);
      }
      return nullptr;
    }

    int startFrame;
    int lastFrame;
  };

  struct PowerUpState : public State
  {
    Type getType() const override { return Type::POWER_UP; }
    Type nextType() const override { return Type::SHOW; }
    int startFrame;
    PowerUpState(int _startFrame, double fps) :
      State(_startFrame, _startFrame + POWERUP_DURATION * fps)
    { }
  };

  struct ShowState : public State
  {
    Type getType()  const override { return Type::SHOW; }
    Type nextType() const override { return Type::SHOW; }
    ShowState(int _startFrame) :
      State(_startFrame, std::numeric_limits<int>::max())
    { }
  };

  struct FadeOutState : public State
  {
    Type getType()  const override { return Type::FADE_OUT; }
    Type nextType() const override { return Type::QUIT; }
    FadeOutState(int _startFrame, double fps, double _lastBrightness) :
      State(_startFrame, _startFrame + POWERDOWN_DURATION * fps),
      lastBrightness(_lastBrightness)
    { }
    double lastBrightness;
  };

  struct SwitchState : public State
  {
    Type getType() const override  { return Type::SWITCH; }
    Type nextType() const override { return Type::SHOW; }
    SwitchState(int _startFrame, int _newChannel) :
      State(_startFrame, _startFrame + 1),
      newChannel(_newChannel)
    { }
    int newChannel;
  };

  struct QuitState : public State
  {
    Type getType() const override  { return Type::QUIT; }
    Type nextType() const override { return Type::QUIT; }
    QuitState(int _startFrame) :
      State(_startFrame, std::numeric_limits<int>::max())
    { }
  };

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

  void run() override
  {
    this->frameCounter = 0;
    this->channel = this->rng() % this->chanSettings.size();
    this->currentState = std::make_shared<PowerUpState>(0, this->fps);
    this->rotateKnobsStart();
  }

  double getTime() override
  {
    return this->frameCounter / this->fps;
  }

  // to be called from the GUI
  void startFadeOut()
  {
    this->currentState = std::make_shared<FadeOutState>(this->frameCounter, this->fps, this->knobs.brightness);
  }

  void startSwitchChannel(int newChannel)
  {
    this->currentState = std::make_shared<SwitchState>(this->frameCounter, newChannel);
  }

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