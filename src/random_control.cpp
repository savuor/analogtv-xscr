#include "precomp.hpp"

#include "random_control.hpp"

namespace atv
{

const int MAX_MULTICHAN = 2;

const double POWERUP_DURATION = 6.0;  /* Hardcoded in analogtv.c */
const double POWERDOWN_DURATION = 1.0;  /* Only used here */


// why const ref to sources does not work?
void RandomControl::createChannels(const std::vector<std::shared_ptr<atv::Source>> sources)
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
        if (this->fixSettings)
        {
          rec.level = 0.3;
          rec.ofs = 0;
          rec.multipath = 0.0;
          rec.freqerr = 0;
        }
        else
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

        channelSetting.receptions.push_back(rec);
        channelSetting.sources.push_back(source);

        if (rec.level > 0.3) break;
        if (this->rng() % 4) break;
    }

    this->chanSettings.push_back(channelSetting);
  }
}


void RandomControl::rotateKnobsStart()
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

  if (!this->fixSettings)
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

  void RandomControl::rotateKnobsSwitch()
  {
    if (!this->fixSettings && !(this->rng() % 5))
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

void RandomControl::run()
{
  this->channel = this->rng() % this->chanSettings.size();
  // for fading out
  this->lastBrightness = -std::numeric_limits<double>::max();

  this->frameCounter = 0;
  this->lastFrame = this->fps * this->duration;
  this->powerUpLastFrame = POWERUP_DURATION * this->fps;
  this->fadeOutFirstFrame = (this->duration - POWERDOWN_DURATION) * this->fps;

  // for channel switching
  this->channelLastFrame = 0;
}


Control::Operation RandomControl::getNext()
{
  Operation op;
  op.channel = this->channel;
  op.type = Operation::Type::NONE;

  double curTime = this->frameCounter / this->fps;
  // power up -> switch channels -> power down

  bool canSwitchChannels = true;
  if (this->usePowerUpDown)
  {
    // don't switch channels when powering up / fading out
    if (this->frameCounter < this->powerUpLastFrame)
    {
      this->knobs.powerup = curTime;
      canSwitchChannels = false;
    }
    else if (this->frameCounter >= this->fadeOutFirstFrame)
    {
      /* Usable range is something like -0.75 to 1.0 */
      static const double minBrightness = -1.5;

      // initialize fading out
      if (this->lastBrightness <= -10.0) // some big value
      {
        this->lastBrightness = this->knobs.brightness;
      }

      /* Fade out, as there is no power-down animation. */
      double rate = (this->duration - curTime) / POWERDOWN_DURATION;
      this->knobs.brightness = minBrightness * (1.0 - rate) + this->lastBrightness * rate;

      canSwitchChannels = false;
    }
  }

  if (canSwitchChannels)
  {
    // channel switch is allowed
    if (this->frameCounter >= this->channelLastFrame)
    {
      /* 1 - 7 sec */
      this->channelLastFrame = frameCounter + this->fps * (1 + this->rng.uniform(0.0, 6.0));

      this->channel = this->rng() % this->chanSettings.size();

      atv::Log::write(2, std::to_string(curTime) + " sec: channel " + std::to_string(this->channel));

      /* Turn the knobs every now and then */
      this->rotateKnobsSwitch();

      op.type = Operation::Type::SWITCH;
    }
  }

  if (this->frameCounter >= this->lastFrame)
  {
    op.type = Operation::Type::QUIT;
  }

  this->frameCounter++;

  op.channel = this->channel;
  return op;
}

} // ::atv