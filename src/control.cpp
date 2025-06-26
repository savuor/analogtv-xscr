#include "precomp.hpp"

#include "control.hpp"
#include "random_control.hpp"

namespace atv
{

std::shared_ptr<Control> Control::create(const atv::ParametricString& desc)
{
  std::shared_ptr<Control> control;

  if (!desc.className.empty())
  {
    // should be like ":random" or ":random:p=1:q=2:b"
    if (desc.className == "random")
    {
      const double defaultDuration = 60.0;
      double duration = defaultDuration;
      if (desc.kvArgs.count("duration"))
      {
        duration = atv::parseInt(desc.kvArgs.at("duration")).value_or(defaultDuration);
      }

      bool powerUpDown = desc.kvArgs.count("powerup");
      bool fixSettings = desc.kvArgs.count("fixsettings");

      const int defaultFps = 30;
      int fps = defaultFps;
      if (desc.kvArgs.count("fps"))
      {
        fps = atv::parseInt(desc.kvArgs.at("fps")).value_or(defaultFps);
      }

      control = std::make_shared<RandomControl>(fixSettings, fps, duration, powerUpDown);
    }
    else if (desc.className == "gui")
    {
      throw std::runtime_error("GUI control is not implemented yet");
    }
    else
    {
      throw std::runtime_error("Unknown source type: " + desc.className);
    }
  }
  else
  {
    // TODO: load json with settings from desc.varArgs[0]
    throw std::runtime_error("JSON loading is not implemented yet");
  }

  return control;
}

} // ::atv