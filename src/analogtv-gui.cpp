#include "precomp.hpp"

#include "analogtv.hpp"
#include "utils.hpp"
#include "source.hpp"
#include "output.hpp"
#include "control.hpp"
#include "json_settings.hpp"
#include "control_window.hpp"

#include <QtWidgets/QApplication>

static cv::Size getBestSize(const std::vector<std::shared_ptr<atv::Source>>& sources, cv::Size size)
{
  int maxw = 0, maxh = 0;
  for (const auto& s : sources)
  {
    cv::Size sz = s->getImageSize();
    maxw = std::max(maxw, sz.width);
    maxh = std::max(maxh, sz.height);
  }
  cv::Size outSize = (size.empty()) ? cv::Size(maxw, maxh) : size;
  outSize.width  &= ~1;
  outSize.height &= ~1;
  return outSize;
}


const double POWERUP_DURATION = 6.0;
const double POWERDOWN_DURATION = 1.0;
const double minBrightness = -1.5;


int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: analogtv-gui <settings.json>" << std::endl;
    return 1;
  }

  // Constructed first (and so destroyed last) so that OpenCV's Qt-based highgui
  // backend, used by "outputs", always has a live QApplication session.
  QApplication app(argc, argv);

  std::string jsonPath = argv[1];

  atv::AppSettings settings = atv::loadSettings(jsonPath);

  int seed = settings.seed;
  if (seed == 0)
  {
    auto tp = std::chrono::high_resolution_clock::now().time_since_epoch();
    seed = static_cast<int>(tp.count());
  }
  cv::RNG rng(seed);

  // Create sources
  std::vector<std::shared_ptr<atv::Source>> sources;
  for (const auto& s : settings.sourceStrings)
  {
    sources.push_back(atv::Source::create(atv::ParametricString::parse(s)));
  }

  cv::Size outSize = getBestSize(sources, settings.size);

  for (const auto& s : sources)
  {
    s->setOutSize(outSize);
    s->setSsavi(rng() % 20 == 0);
  }

  // Build channels from config
  std::vector<atv::ChanSetting> channels;
  for (const auto& chCfg : settings.channels)
  {
    atv::ChanSetting ch;
    ch.noise_level = chCfg.noise_level;

    for (const auto& recCfg : chCfg.receptions)
    {
      atv::AnalogReception rec;
      rec.setValue("ofs",       recCfg.ofs);
      rec.setValue("level",     recCfg.level);
      rec.setValue("multipath", recCfg.multipath);
      rec.setValue("freqerr",   recCfg.freqerr);
      ch.receptions.push_back(rec);
      ch.sources.push_back(sources.at(recCfg.sourceIndex));
    }

    channels.push_back(ch);
  }

  // Create outputs
  std::vector<std::shared_ptr<atv::Output>> outputs;
  for (const auto& s : settings.outputStrings)
  {
    outputs.push_back(atv::Output::create(atv::ParametricString::parse(s), outSize));
  }

  // Create TV
  atv::SetTopBox tv(seed, outSize.width, outSize.height);

  int currentChannel = 0;
  atv::ChanSetting* currentChannelPtr = &channels[currentChannel];

  atv::Knobs knobs = settings.knobs;
  atv::ControlWindow window(knobs, *currentChannelPtr, sources, static_cast<int>(channels.size()));
  window.show();

  QObject::connect(&window, &atv::ControlWindow::knobChanged,
    [&knobs](const QString& name, double value)
    {
      knobs.setValue(name.toStdString(), value);
    });

  QObject::connect(&window, &atv::ControlWindow::chanParamChanged,
    [&currentChannelPtr](const QString& name, double value)
    {
      currentChannelPtr->setValue(name.toStdString(), value);
    });

  QObject::connect(&window, &atv::ControlWindow::receptionParamChanged,
    [&currentChannelPtr](int index, const QString& name, double value)
    {
      currentChannelPtr->receptions.at(index).setValue(name.toStdString(), value);
    });

  QObject::connect(&window, &atv::ControlWindow::receptionSourceChanged,
    [&currentChannelPtr, &sources](int index, int sourceIndex)
    {
      currentChannelPtr->sources.at(index) = sources.at(sourceIndex);
    });

  bool pendingChannelSwitch = false;
  QObject::connect(&window, &atv::ControlWindow::channelSwitchRequested,
    [&currentChannel, &currentChannelPtr, &channels, &pendingChannelSwitch, &window](int channelIndex)
    {
      if (channelIndex >= 0 && channelIndex < static_cast<int>(channels.size()) && channelIndex != currentChannel)
      {
        currentChannel = channelIndex;
        currentChannelPtr = &channels[currentChannel];
        pendingChannelSwitch = true;
        window.setChannel(*currentChannelPtr);
      }
    });

  int frameCounter = 0;

  bool isOn = false; // TV is off by default
  bool isPoweringUp = false;
  bool isPoweringDown = false;
  int transitionStartFrame = 0;
  double transitionStartBrightness = minBrightness;
  const double originalBrightness = knobs.brightness;

  knobs.brightness = minBrightness;
  knobs.powerup = 0.0;

  const int powerUpDurationFrames = static_cast<int>(POWERUP_DURATION * settings.fps);
  const int powerDownDurationFrames = static_cast<int>(POWERDOWN_DURATION * settings.fps);

  QObject::connect(&window, &atv::ControlWindow::powerToggled, [&](bool on)
  {
    if (on && !isOn)
    {
      isOn = true;
      isPoweringUp = true;
      isPoweringDown = false;
      transitionStartFrame = frameCounter;
      transitionStartBrightness = knobs.brightness;
    }
    else if (!on && isOn)
    {
      isOn = false;
      isPoweringDown = true;
      isPoweringUp = false;
      transitionStartFrame = frameCounter;
      transitionStartBrightness = knobs.brightness;
    }
  });

  bool done = false;
  QObject::connect(&window, &atv::ControlWindow::quitRequested, [&]()
  {
    done = true;
  });

  cv::Mat4b outBuffer(outSize);

  const auto frameInterval = std::chrono::milliseconds(1000 / settings.fps);
  const auto loopStart = std::chrono::steady_clock::now();

  while (!done)
  {
    app.processEvents();

    // real elapsed time, kept flowing regardless of the TV's power state
    double curTime = static_cast<double>(frameCounter) / settings.fps;

    if (isPoweringUp)
    {
      int elapsed = frameCounter - transitionStartFrame;
      if (elapsed >= powerUpDurationFrames)
      {
        isPoweringUp = false;
        knobs.brightness = originalBrightness;
        knobs.powerup = POWERUP_DURATION;
      }
      else
      {
        double rate = static_cast<double>(elapsed) / powerUpDurationFrames;
        knobs.brightness = transitionStartBrightness + (originalBrightness - transitionStartBrightness) * rate;
        knobs.powerup = static_cast<double>(elapsed) / settings.fps;
      }
    }
    else if (isPoweringDown)
    {
      int elapsed = frameCounter - transitionStartFrame;
      if (elapsed >= powerDownDurationFrames)
      {
        isPoweringDown = false;
        knobs.brightness = minBrightness;
      }
      else
      {
        double rate = static_cast<double>(elapsed) / powerDownDurationFrames;
        knobs.brightness = transitionStartBrightness + (minBrightness - transitionStartBrightness) * rate;
      }
    }

    tv.setKnobs(knobs);

    bool switchChannel = pendingChannelSwitch;
    pendingChannelSwitch = false;

    // while fully off, don't touch sources or render a frame, just send a black one; curTime keeps flowing regardless
    if (isOn || isPoweringDown)
    {
      atv::ChanSetting& curChannel = channels[currentChannel];
      for (size_t i = 0; i < curChannel.receptions.size(); i++)
      {
        atv::AnalogReception& rec = curChannel.receptions[i];
        curChannel.sources[i]->update(rec.input, curTime);
        rec.update(rng);
      }

      outBuffer = tv.draw(curChannel.noise_level, switchChannel, curChannel.receptions);
    }
    else
    {
      outBuffer.setZero();
    }

    for (const auto &o : outputs)
    {
      o->send(outBuffer);
    }

    frameCounter++;

    // wait for the next scheduled slot; if a frame took longer than frameInterval, skip ahead
    // instead of trying to catch up on missed slots one by one
    auto elapsed = std::chrono::steady_clock::now() - loopStart;
    long long nextSlot = elapsed / frameInterval + 1;
    std::this_thread::sleep_until(loopStart + nextSlot * frameInterval);
  }

  return 0;
}
