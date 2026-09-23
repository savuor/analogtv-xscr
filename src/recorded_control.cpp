#include "precomp.hpp"

#include "recorded_control.hpp"

#include <nlohmann/json.hpp>

namespace atv
{

namespace
{

constexpr double minBrightness = -1.5;

}

RecordedControl::RecordedControl(const std::string& filePath)
  : settings(), sequence(), knobs(), fps(30.0), frameCounter(0),
    sequenceIndex(0),
    sequenceLastFrame(0),
    powerUp(true),
    turnOnFrame(0),
    currentChannel(0),
    powerDownFrames(0),
    powerDownStartFrame(0),
    targetBrightness(0.0),
    lastFrame(0)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    throw std::runtime_error("Cannot open control file: " + filePath);
  }

  this->settings = nlohmann::json::parse(file);
  this->fps = this->settings.value("fps", 30.0);
  if (this->fps <= 0.0)
  {
    throw std::runtime_error("Recorded control FPS must be positive");
  }

  if (!this->settings.contains("sequence") ||
      !this->settings["sequence"].is_array() ||
       this->settings["sequence"].empty())
  {
    throw std::runtime_error("Recorded control requires a non-empty sequence");
  }

  for (const auto& item : this->settings["sequence"])
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
    entry.durationFrames = std::max(1, static_cast<int>(std::llround(duration * this->fps / 1000.0)));
    entry.knobs = item.value("knobs", nlohmann::json::object());
    entry.channelSettings = item;
    this->sequence.push_back(std::move(entry));
  }
}

void RecordedControl::setRNG(uint64_t /* rngSeed */)
{ }

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
      this->knobs.setValue(it.key(), it.value().get<bool>() ? 1.0 : 0.0);
    }
    else if (it.value().is_number())
    {
      this->knobs.setValue(it.key(), it.value().get<double>());
    }
  }
}

void RecordedControl::applyChannelSettings(int channel, const nlohmann::json& values)
{
  if (values.contains("noise_level"))
  {
    this->chanSettings.at(channel).setValue("noise_level", values["noise_level"].get<double>());
  }

  if (values.contains("receptions"))
  {
    if (!values["receptions"].is_array())
    {
      throw std::runtime_error("Recorded receptions must be an array");
    }

    auto& receptions = this->chanSettings.at(channel).receptions;
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
  if (!this->settings.contains("channels") || !this->settings["channels"].is_array())
  {
    throw std::runtime_error("Recorded control requires channels");
  }

  for (const auto& item : this->settings["channels"])
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
    this->chanSettings.push_back(std::move(channel));
  }

  for (const auto& entry : this->sequence)
  {
    if (entry.channel < 0 || entry.channel >= static_cast<int>(chanSettings.size()))
    {
      throw std::runtime_error("Recorded sequence refers to an invalid channel");
    }
  }
}

void RecordedControl::applySequenceEntry(size_t index)
{
  this->currentChannel = this->sequence.at(index).channel;
  this->applyKnobs(this->sequence.at(index).knobs);
  this->applyChannelSettings(this->currentChannel, this->sequence.at(index).channelSettings);
}

Knobs RecordedControl::getKnobs()
{
  return this->knobs;
}

void RecordedControl::run()
{
  if (this->chanSettings.empty())
  {
    throw std::runtime_error("Recorded control requires at least one channel");
  }

  this->applyKnobs(this->settings.value("knobs", nlohmann::json::object()));
  this->knobs.timeSinceStart = 0.0;

  this->frameCounter = 0;
  this->turnOnFrame = 0;
  this->sequenceIndex = 0;
  this->currentChannel = this->sequence.front().channel;
  this->applySequenceEntry(this->sequenceIndex);
  this->sequenceLastFrame = this->sequence.at(this->sequenceIndex).durationFrames;

  this->powerUp = this->settings.value("powerup", true);

  this->powerDownFrames = static_cast<int>(this->settings.value("powerDownDuration", 0.0) * this->fps);
  int fullLengthFrames = 0;
  for (const auto& entry : this->sequence)
  {
    fullLengthFrames += entry.durationFrames;
  }
  this->powerDownStartFrame = fullLengthFrames;
  this->lastFrame = fullLengthFrames + this->powerDownFrames;
}

Control::Operation RecordedControl::getNext()
{
  Operation op;
  op.type = Operation::Type::NONE;

  if (this->frameCounter >= this->lastFrame)
  {
    op.type = Operation::Type::QUIT;
  }
  else if (this->frameCounter >= this->powerDownStartFrame)
  {
    // empty power down period is accounted in previous if block
    double rate = static_cast<double>(this->frameCounter - this->powerDownStartFrame) / this->powerDownFrames;
    this->knobs.brightness = this->targetBrightness + (minBrightness - this->targetBrightness) * std::min(rate, 1.0);
  }
  else if (this->frameCounter >= this->sequenceLastFrame)
  {
    ++this->sequenceIndex;
    this->sequenceLastFrame = this->frameCounter + this->sequence.at(this->sequenceIndex).durationFrames;

    int previousChannel = this->currentChannel;
    this->applySequenceEntry(this->sequenceIndex);
    if (previousChannel != this->currentChannel)
    {
      op.type = Operation::Type::SWITCH;
    }
  }

  // to ignore powerup effects
  knobs.timeSinceStart = this->powerUp ? static_cast<double>(this->frameCounter - this->turnOnFrame) / this->fps : 1000.0;

  ++this->frameCounter;

  op.channel = this->currentChannel;
  return op;
}

double RecordedControl::getTime()
{
  return static_cast<double>(this->frameCounter) / this->fps;
}

double RecordedControl::getFps() const
{
  return this->fps;
}

} // ::atv