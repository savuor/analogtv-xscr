#include "precomp.hpp"

#include "gui_control.hpp"

namespace atv
{

GuiControl::GuiControl(double _fps, bool _randomizeSettings)
{
  this->fps = _fps;
  this->randomizeSettings = _randomizeSettings;
  //TODO: initialize GUI itself
}


const int MAX_MULTICHAN = 2;

// why const ref to sources does not work?
void GuiControl::createChannels(const std::vector<std::shared_ptr<atv::Source>> sources)
{
  size_t nChannels = std::max(sources.size() * 2, 6UL);

  this->chanSettings = { };
  for (size_t i = 0; i < nChannels; i++)
  {
    ChanSetting channelSetting;
    // noise: 0 to 0.2 or 0 to 5.0, default 0.04
    channelSetting.noise_level = 0.06;

    int last_station = 42;
    for (int stati = 0; stati < MAX_MULTICHAN; stati++)
    {
        int stationId;
        while (1)
        {
          stationId = this->rng() % (sources.size());
          // don't do ghost reception with the same station...
          if (stationId != last_station) break;
          // ...at least too often
          if (this->rng() % 10 == 0) break;
        }
        last_station = stationId;
        std::shared_ptr<atv::Source> source = sources[stationId];
        atv::AnalogReception rec;
        if (this->randomizeSettings)
        {
          rec.level = pow(this->rng.uniform(0.0, 1.0), 3.0) * 2.0 + 0.05;
          rec.ofs   = this->rng() % atv::ANALOGTV_SIGNAL_LEN;
          if (this->rng() % 3)
          {
            rec.multipath = this->rng.uniform(0.0, 1.0);
          }
          else
          {
            rec.multipath = 0.0;
          }
          if (stati > 0)
          {
            /* We only set a frequency error for ghosting stations,
              because it doesn't matter otherwise */
            rec.freqerr = this->rng.uniform(-1.0, 1.0) * 3.0;
          }
        }
        else
        {
          rec.level = 0.3;
          rec.ofs = 0;
          rec.multipath = 0.0;
          rec.freqerr = 0;
        }

        channelSetting.receptions.push_back(rec);
        channelSetting.sources.push_back(source);

        if (rec.level > 0.3) break;
        if (this->rng() % 4) break;
    }

    this->chanSettings.push_back(channelSetting);
  }

  //TODO: create GUI for channels
}


void GuiControl::rotateKnobsStart()
{
  // from analogtv_set_defaults()

  // values taken from analogtv-cli

  // tint: 0 to 360, default 5
  this->knobs.tint = 5;
  // color: 0 to 400, default 70
  // or 0 to +/- 500, need to check it
  this->knobs.color = 70 / 100.0;

  // brightness: -75 to 100, default 1.5 or 3.0
  this->knobs.brightness = 2 / 100.0;
  // contrast: 0 to 500, default 150
  this->knobs.contrast   = 150 / 100.0;
  this->knobs.height = 1.0;
  this->knobs.width  = 1.0;
  this->knobs.squish = 0.0;

  this->knobs.powerup = 1000.0;

  //tv.hashnoise_rpm = 0;
  //TODO: do we need both?
  this->knobs.useHashNoise = 0;
  this->knobs.enableHashNoise = 1;

  this->knobs.horizontalDesync = this->rng.uniform(-5.0, 5.0);
  this->knobs.squeezeBottom = this->rng.uniform(-1.0, 4.0);

  this->knobs.useFlutterHorizontalDesync = false;
  this->knobs.channelChangeCycles = 200000;

  if (this->randomizeSettings)
  {
    if (this->rng() % 4 == 0)
    {
      this->knobs.tint += pow(this->rng.uniform(-1.0, 1.0), 7) * 180.0;
    }
    if (1)
    {
      this->knobs.color += this->rng.uniform(0.0, 0.3) * ((this->rng() & 1) ? 1 : -1);
    }
    if (0) //if (darkp)
    {
      if (this->rng() % 4 == 0)
      {
        this->knobs.brightness += this->rng.uniform(0.0, 0.15);
      }
      if (this->rng() % 4 == 0)
      {
        this->knobs.contrast += this->rng.uniform(0.0, 0.2) * ((this->rng() & 1) ? 1 : -1);
      }
    }
  }
}


void GuiControl::rotateKnobsSwitch()
{
  if (this->randomizeSettings && !(this->rng() % 5))
  {
    if (this->rng() % 4 == 0) 
    {
      this->knobs.tint += pow(this->rng.uniform(-1.0, 1.0), 7) * 180.0 * ((this->rng() & 1) ? 1 : -1);
    }
    if (1)
    {
      this->knobs.color += this->rng.uniform(0.0, 0.3) * ((this->rng() & 1) ? 1 : -1);
    }
    if (0) //(darkp)
    {
      if (this->rng() % 4 == 0)
      {
        this->knobs.brightness += this->rng.uniform(0.0, 0.15);
      }
      if (this->rng() % 4 == 0)
      {
        this->knobs.contrast += this->rng.uniform(0.0, 0.2) * ((this->rng() & 1) ? 1 : -1);
      }
    }
  }
}


struct GuiControl::State
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
  virtual Control::Operation::Type getOperationType() const
  {
    switch (getType())
    {
      case Type::POWER_UP:
        return Control::Operation::Type::NONE;
      case Type::SHOW:
        return Control::Operation::Type::NONE;
      case Type::FADE_OUT:
        return Control::Operation::Type::NONE;
      case Type::SWITCH:
        return Control::Operation::Type::SWITCH;
      case Type::QUIT:
        return Control::Operation::Type::QUIT;
    }
    return Control::Operation::Type::NONE; // Default case, should not happen
  }

  virtual ~State() {}
  State(int _startFrame = 0, int _lastFrame = 0) :
    startFrame(_startFrame), lastFrame(_lastFrame)
  { }

  static std::shared_ptr<State> create(Type type, int startFrame, double fps, double lastBrightness = 0.0, int newChannel = 0);

  int startFrame;
  int lastFrame;
};

const double POWERUP_DURATION = 6.0;  /* Hardcoded in analogtv.c */
struct PowerUpState : public GuiControl::State
{
  Type getType() const override { return Type::POWER_UP; }
  Type nextType() const override { return Type::SHOW; }
  int startFrame;
  PowerUpState(int _startFrame, double fps) :
    State(_startFrame, _startFrame + POWERUP_DURATION * fps)
  { }
};

struct ShowState : public GuiControl::State
{
  Type getType()  const override { return Type::SHOW; }
  Type nextType() const override { return Type::SHOW; }
  ShowState(int _startFrame) :
    State(_startFrame, std::numeric_limits<int>::max())
  { }
};


const double POWERDOWN_DURATION = 1.0;  /* Only used here */
struct FadeOutState : public GuiControl::State
{
  Type getType()  const override { return Type::FADE_OUT; }
  Type nextType() const override { return Type::QUIT; }
  FadeOutState(int _startFrame, double fps, double _lastBrightness) :
    State(_startFrame, _startFrame + POWERDOWN_DURATION * fps),
    lastBrightness(_lastBrightness)
  { }
  double lastBrightness;
};

struct SwitchState : public GuiControl::State
{
  Type getType() const override  { return Type::SWITCH; }
  Type nextType() const override { return Type::SHOW; }
  SwitchState(int _startFrame, int _newChannel) :
    State(_startFrame, _startFrame + 1),
    newChannel(_newChannel)
  { }
  int newChannel;
};

struct QuitState : public GuiControl::State
{
  Type getType() const override  { return Type::QUIT; }
  Type nextType() const override { return Type::QUIT; }
  QuitState(int _startFrame) :
    State(_startFrame, std::numeric_limits<int>::max())
  { }
};

std::shared_ptr<GuiControl::State> GuiControl::State::create(Type type, int startFrame, double fps, double lastBrightness, int newChannel)
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


void GuiControl::startFadeOut()
{
  this->currentState = std::make_shared<FadeOutState>(this->frameCounter, this->fps, this->knobs.brightness);
}

void GuiControl::startSwitchChannel(int newChannel)
{
  this->currentState = std::make_shared<SwitchState>(this->frameCounter, newChannel);
}

void GuiControl::run()
{
  this->frameCounter = 0;
  this->channel = this->rng() % this->chanSettings.size();
  this->currentState = std::make_shared<PowerUpState>(0, this->fps);
  this->rotateKnobsStart();
  //TODO: update knobs GUI
}


/* Usable range is something like -0.75 to 1.0 */
static const double minBrightness = -1.5;

Control::Operation GuiControl::getNext()
{
  double curTime = this->frameCounter / this->fps;

  switch (currentState->getType())
  {
    case State::Type::POWER_UP:
    {
      this->knobs.powerup = curTime;
    }
    break;

    case State::Type::SWITCH:
    {
      this->channel = std::dynamic_pointer_cast<SwitchState>(currentState)->newChannel;
      this->rotateKnobsSwitch();
      atv::Log::write(2, std::to_string(curTime) + " sec: channel " + std::to_string(this->channel));
    }
    break;

    case State::Type::FADE_OUT:
    {
      /* Fade out, as there is no power-down animation. */
      double rate = (currentState->lastFrame - this->frameCounter) / this->fps / POWERDOWN_DURATION;
      double lastBrightness = std::dynamic_pointer_cast<FadeOutState>(currentState)->lastBrightness;
      this->knobs.brightness = minBrightness * (1.0 - rate) + lastBrightness * rate;
    }
    break;

    case State::Type::QUIT:
      break;

    case State::Type::SHOW:
      break;

    default:
      throw std::runtime_error("Unknown state type in GuiControl::getNext()");
      break;
  }

  //TODO: update knobs GUI

  Operation op;
  op.type = currentState->getOperationType();
  op.channel = this->channel;

  if (this->frameCounter >= currentState->lastFrame)
  {
    this->currentState = State::create(currentState->nextType(), this->frameCounter, this->fps);
  }

  this->frameCounter++;

  return op;
}

} // ::atv