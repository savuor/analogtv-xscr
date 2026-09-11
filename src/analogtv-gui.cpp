#include "precomp.hpp"

#include "analogtv.hpp"
#include "utils.hpp"
#include "source.hpp"
#include "output.hpp"
#include "control.hpp"
#include "json_settings.hpp"
#include "control_window.hpp"

#include <chrono>

#include <QtWidgets/QApplication>
#include <QtCore/QTimer>


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

  atv::Knobs knobs = settings.knobs;
  atv::ControlWindow window(knobs);
  window.show();

  QObject::connect(&window, &atv::ControlWindow::knobChanged,
    [&knobs](const QString& name, double value)
    {
      knobs.setValue(name.toStdString(), value);
    });

  int currentChannel = 0;
  int frameCounter = 0;
  bool isPoweringDown = false;
  int powerDownLastFrame = std::numeric_limits<int>::max();
  double initialPowerDownBrightness = 0.0;
  const int powerDownDurationFrames = static_cast<int>(POWERDOWN_DURATION * settings.fps);

  QObject::connect(&window, &atv::ControlWindow::quitRequested, [&]()
  {
    if (!isPoweringDown)
    {
      isPoweringDown = true;
      powerDownLastFrame = frameCounter + powerDownDurationFrames;
      initialPowerDownBrightness = knobs.brightness;
    }
  });

  // Frame loop via QTimer
  const int powerUpLastFrame = static_cast<int>(POWERUP_DURATION * settings.fps);

  QTimer frameTimer;
  frameTimer.setInterval(1000 / settings.fps);
  QObject::connect(&frameTimer, &QTimer::timeout, [&]()
  {
    double curTime = static_cast<double>(frameCounter) / settings.fps;

    bool quit = false;
    if (isPoweringDown)
    {
      if (frameCounter >= powerDownLastFrame)
      {
        quit = true;
      }
      else
      {
        double rate = static_cast<double>(powerDownLastFrame - frameCounter) / powerDownDurationFrames;
        knobs.brightness = minBrightness * (1.0 - rate) + initialPowerDownBrightness * rate;
      }
    }
    else if (frameCounter < powerUpLastFrame)
    {
      knobs.powerup = curTime;
    }

    if(!quit)
    {
      tv.setKnobs(knobs);

      atv::ChanSetting& curChannel_s = channels[currentChannel];
      for (size_t i = 0; i < curChannel_s.receptions.size(); i++)
      {
        atv::AnalogReception& rec = curChannel_s.receptions[i];
        curChannel_s.sources[i]->update(rec.input, curTime);
        rec.update(rng);
      }

      cv::Mat4b outBuffer = tv.draw(curChannel_s.noise_level, false, curChannel_s.receptions);

      for (const auto& o : outputs)
      {
        o->send(outBuffer);
      }

      frameCounter++;
    }
    else
    {
      frameTimer.stop();
      app.quit();
    }
  });

  frameTimer.start();

  return app.exec();
}
