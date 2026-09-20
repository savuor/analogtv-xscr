#include "precomp.hpp"

#include "recorded_control.hpp"

#include <nlohmann/json.hpp>

namespace atv
{

namespace
{
constexpr double powerUpDuration = 6.0;
constexpr double powerDownDuration = 1.0;
constexpr double minBrightness = -1.5;
constexpr double renderPowerUpDuration = 6.0;
}

RecordedControl::RecordedControl(const std::string& filePath)
  : settings(), sequence(), knobs(), rng(), fps(30.0), frameCounter(0), sequenceIndex(0),
    sequenceFrame(0), currentChannel(0), powerUpFrames(0), powerDownFrames(0),
    poweringUp(false), poweringDown(false), firstFrame(false), targetBrightness(0.0)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    throw std::runtime_error("Cannot open control file: " + filePath);
  }

  settings = nlohmann::json::parse(file);
  fps = settings.value("fps", 30.0);
  if (fps <= 0.0)
  {
    throw std::runtime_error("Recorded control FPS must be positive");
  }

  if (!settings.contains("sequence") || !settings["sequence"].is_array() || settings["sequence"].empty())
  {
    throw std::runtime_error("Recorded control requires a non-empty sequence");
  }

  for (const auto& item : settings["sequence"])
  {
    if (!item.is_object() || !item.contains("channel") || !item.contains("duration"))
    {
      throw std::runtime_error("Each recorded sequence item requires channel and duration");
    }

    double duration = item["duration"].get<double>();
    if (duration <= 0.0)
    {
      throw std::runtime_error("Recorded sequence durations must be positive");
    }

    SequenceEntry entry;
    entry.channel = item["channel"].get<int>();
    entry.durationFrames = std::max(1, static_cast<int>(std::llround(duration * fps / 1000.0)));
    entry.knobs = item.value("knobs", nlohmann::json::object());
    entry.channelSettings = item;
    sequence.push_back(std::move(entry));
  }
}

void RecordedControl::setRNG(uint64_t rngSeed)
{
  rng = cv::RNG(rngSeed);
}

void RecordedControl::applyKnobs(const nlohmann::json& values)
{
  if (!values.is_object())
  {
    throw std::runtime_error("Recorded knob settings must be an object");
  }

  for (auto it = values.begin(); it != values.end(); ++it)
  {
    if (it.value().is_boolean())
    {
      knobs.setValue(it.key(), it.value().get<bool>() ? 1.0 : 0.0);
    }
    else if (it.value().is_number())
    {
      knobs.setValue(it.key(), it.value().get<double>());
    }
  }
}

void RecordedControl::applyChannelSettings(int channel, const nlohmann::json& values)
{
  if (values.contains("noise_level"))
  {
    chanSettings.at(channel).setValue("noise_level", values["noise_level"].get<double>());
  }

  if (values.contains("receptions"))
  {
    if (!values["receptions"].is_array())
    {
      throw std::runtime_error("Recorded receptions must be an array");
    }

    auto& receptions = chanSettings.at(channel).receptions;
    if (values["receptions"].size() > receptions.size())
    {
      throw std::runtime_error("Recorded reception override exceeds channel receptions");
    }

    for (size_t i = 0; i < values["receptions"].size(); ++i)
    {
      if (!values["receptions"][i].is_object())
      {
        throw std::runtime_error("Recorded reception settings must be objects");
      }
      for (auto it = values["receptions"][i].begin(); it != values["receptions"][i].end(); ++it)
      {
        if (it.value().is_number())
        {
          receptions[i].setValue(it.key(), it.value().get<double>());
        }
      }
    }
  }
}

void RecordedControl::createChannels(const std::vector<std::shared_ptr<atv::Source>> sources)
{
  if (!settings.contains("channels") || !settings["channels"].is_array())
  {
    throw std::runtime_error("Recorded control requires channels");
  }

  for (const auto& item : settings["channels"])
  {
    ChanSetting channel;
    channel.noise_level = item.value("noise_level", channel.noise_level);

    for (const auto& reception : item.value("receptions", nlohmann::json::array()))
    {
      int sourceIndex = reception.value("source", -1);
      if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size()))
      {
        throw std::runtime_error("Recorded reception refers to an invalid source");
      }

      AnalogReception rec;
      for (auto it = reception.begin(); it != reception.end(); ++it)
      {
        if (it.key() != "source" && it.value().is_number())
        {
          rec.setValue(it.key(), it.value().get<double>());
        }
      }
      channel.receptions.push_back(rec);
      channel.sources.push_back(sources.at(sourceIndex));
    }
    chanSettings.push_back(std::move(channel));
  }

  for (const auto& entry : sequence)
  {
    if (entry.channel < 0 || entry.channel >= static_cast<int>(chanSettings.size()))
    {
      throw std::runtime_error("Recorded sequence refers to an invalid channel");
    }
  }
}

void RecordedControl::applySequenceEntry(size_t index)
{
  currentChannel = sequence.at(index).channel;
  applyKnobs(sequence.at(index).knobs);
  applyChannelSettings(currentChannel, sequence.at(index).channelSettings);
}

Knobs RecordedControl::getKnobs()
{
  return knobs;
}

void RecordedControl::run()
{
  if (chanSettings.empty())
  {
    throw std::runtime_error("Recorded control requires at least one channel");
  }

  applyKnobs(settings.value("knobs", nlohmann::json::object()));
  targetBrightness = knobs.brightness;
  knobs.brightness = minBrightness;
  knobs.powerup = 0.0;

  frameCounter = 0;
  sequenceIndex = 0;
  sequenceFrame = 0;
  currentChannel = sequence.front().channel;
  applySequenceEntry(sequenceIndex);
  targetBrightness = knobs.brightness;
  knobs.brightness = minBrightness;
  knobs.powerup = 0.0;

  powerUpFrames = static_cast<int>(powerUpDuration * fps);
  powerDownFrames = static_cast<int>(powerDownDuration * fps);
  poweringUp = true;
  poweringDown = false;
  firstFrame = true;
}

Control::Operation RecordedControl::getNext()
{
  if (poweringUp)
  {
    double rate = powerUpFrames == 0 ? 1.0 : static_cast<double>(frameCounter) / powerUpFrames;
    rate = std::min(rate, 1.0);
    knobs.brightness = minBrightness + (targetBrightness - minBrightness) * rate;
    knobs.powerup = renderPowerUpDuration * rate;
    if (frameCounter >= powerUpFrames)
    {
      poweringUp = false;
      sequenceFrame = 0;
      knobs.brightness = targetBrightness;
      knobs.powerup = renderPowerUpDuration;
    }
  }
  else if (poweringDown)
  {
    double rate = powerDownFrames == 0 ? 1.0 : static_cast<double>(frameCounter) / powerDownFrames;
    knobs.brightness = targetBrightness + (minBrightness - targetBrightness) * std::min(rate, 1.0);
    if (frameCounter >= powerDownFrames)
    {
      return {Operation::Type::QUIT, currentChannel};
    }
  }
  else if (sequenceFrame >= sequence.at(sequenceIndex).durationFrames)
  {
    ++sequenceIndex;
    sequenceFrame = 0;
    if (sequenceIndex >= sequence.size())
    {
      poweringDown = true;
      frameCounter = 0;
      return getNext();
    }

    int previousChannel = currentChannel;
    applySequenceEntry(sequenceIndex);
    targetBrightness = knobs.brightness;
    return {previousChannel == currentChannel ? Operation::Type::NONE : Operation::Type::SWITCH, currentChannel};
  }

  ++frameCounter;
  if (!poweringUp && !poweringDown)
  {
    ++sequenceFrame;
  }
  bool switchChannel = firstFrame;
  firstFrame = false;
  return {switchChannel ? Operation::Type::SWITCH : Operation::Type::NONE, currentChannel};
}

double RecordedControl::getTime()
{
  return static_cast<double>(frameCounter) / fps;
}

double RecordedControl::getFps() const
{
  return fps;
}

} // ::atv