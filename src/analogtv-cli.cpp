/* xanalogtv-cli, Copyright © 2018-2023 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "precomp.hpp"

#include "analogtv.hpp"
#include "utils.hpp"
#include "source.hpp"
#include "output.hpp"
#include "control.hpp"

#include <chrono>

#include <opencv2/core.hpp>

//#include "wx/wx.h"

struct Params
{
  int         verbosity;
  int         seed;
  cv::Size    size;

  atv::ParametricString controlDescription;
  std::vector<atv::ParametricString> sources;
  std::vector<atv::ParametricString> outputs;
};


cv::Size getBestSize(const std::vector<std::shared_ptr<atv::Source>>& sources, cv::Size size)
{
  // get best size
  cv::Size outSize;
  int maxw = 0, maxh = 0;
  for (const auto& s : sources)
  {
    cv::Size sz = s->getImageSize();
    maxw = std::max(maxw, sz.width);
    maxh = std::max(maxh, sz.height);
  }
  outSize = (size.empty()) ? cv::Size(maxw, maxh) : size;
  /* can't be odd */
  outSize.width  &= ~1;
  outSize.height &= ~1;

  return outSize;
}


static void run(Params params)
{
  int seed = params.seed;
  if (params.seed == 0)
  {
    auto tp = std::chrono::high_resolution_clock::now().time_since_epoch();
    seed = tp.count();
  }
  cv::RNG rng(seed);

  std::vector<std::shared_ptr<atv::Source>> sources;
  for (const auto& s : params.sources)
  {
    sources.push_back(atv::Source::create(s));
  }

  atv::Log::write(2, "initialized " + std::to_string(sources.size()) + " sources");

  cv::Size outSize = getBestSize(sources, params.size);

  for (const auto& s : sources)
  {
    s->setOutSize(outSize);
    // randomly set ssavi (broken sync or something like this)
    s->setSsavi(rng() % 20 == 0);
  }

  std::vector<std::shared_ptr<atv::Output>> outputs;
  for (const auto& s : params.outputs)
  {
    outputs.emplace_back(atv::Output::create(s, outSize));
  }

  atv::Log::write(2, "initialized " + std::to_string(outputs.size()) + " outputs");

  atv::SetTopBox tv(seed, outSize.width, outSize.height);

  std::shared_ptr<atv::Control> control = atv::Control::create(params.controlDescription);
  control->setRNG(seed);

  control->createChannels(sources);

  control->run();

  while (true)
  {
    auto action = control->getNext();

    if (action.type == atv::Control::Operation::Type::QUIT)
    {
      break;
    }

    double curTime = control->getTime();

    int curInput = action.channel;

    bool switchChannel = (action.type == atv::Control::Operation::Type::SWITCH);

    tv.setKnobs(control->getKnobs());

    atv::Log::write(3, "Time: " + std::to_string(curTime));

    atv::ChanSetting& curChannel = control->chanSettings[curInput];
    for (size_t i = 0; i < curChannel.receptions.size(); i++)
    {
      atv::AnalogReception& rec = curChannel.receptions[i];
      curChannel.sources[i]->update(rec.input, curTime);
      /* Noisy image */
      rec.update(rng);
    }

    cv::Mat4b outBuffer = tv.draw(curChannel.noise_level, switchChannel, curChannel.receptions);

    // Send rendered frame to outputs
    for (const auto& o : outputs)
    {
      o->send(outBuffer);
    }
  }

  atv::Log::write(2, "Finish");
}


static const std::map<std::string, atv::CmdArgument> knownArgs =
{
    // name, exampleArgs, type, optional, help
    {"control",
      { "<file.json or param string>", atv::CmdArgument::Type::STRING, false,
        "control scenario file in JSON format or a parametric string specifying control type:\n"
        "  * JSON file containing prescripted instructions, overriding all other command line arguments (not implemented yet)\n"
        "  * :random is a random control with the following available parameters:\n"
        "    * duration: length of video in secs, 60 if not given\n"
        "    * powerup: if given, power-on animation is run at the beginning, and fade to black is done at the end\n"
        "    * fixsettings: if given, some TV settings are not random\n"
        "    * fps: frames per second, 30 if not given (not implemented properly yet)\n"
        "    Example control description: \":random:duration=60:fixsettings:powerup\"\n"
        "  * :gui use GUI to control everything manually (not implemented yet)" }},
    {"verbose",
      { "n",     atv::CmdArgument::Type::INT,  true,
        "level of verbosity from 0 to 5" }},
    {"size",
      { "width height", atv::CmdArgument::Type::LIST_INT, true,
        "use different size than maximum of given images\n"
        "Note: if no size is given and the only source is SMPTE bars generator then the output source size will be 320x240" }},
    {"seed",
      { "value", atv::CmdArgument::Type::INT, true,
        "random seed to start random generator or 0 to randomize by current date and time" }},
    {"in",
      { "src1 [src2 ... srcN]", atv::CmdArgument::Type::LIST_STRING, false,
        "signal sources such as still images, video files or special sources:\n"
        "  * Still image file name\n"
        "  * Video file name\n"
        "    Note: video files are detected by extension. Supported extensions are listed in source.cpp file\n"
        "    as knownVideoExtensions variable.\n"
        "  * :cam uses camera as a video source, params are:\n"
        "    * camera number in the system, 0 if not given\n"
        "    * timestamp: overlays timestamp over the image\n"
        "    Example camera descriptions: \":cam\" \":cam:0\" \":cam:0:timestamp\"\n"
        "  * :bars are SMPTE color bars, params are:\n"
        "    * Path to image to be used as an overlaid station logo (optional)\n"
        "    * timestamp: overlays timestamp over the image\n"
        "    Example bars descriptions: \":bars\" \":bars:/path/to/image\" \":bars:/path/to/image:timestamp\"\n"
        "  * :video is an alternative way to specify video source, params are:\n"
        "    * Path to video\n"
        "    * timestamp: overlays timestamp over the image\n"
        "    Example video descriptions: \":video:/path/to/video\" \":video:/path/to/video:timestamp\"\n"
        "  * :image is an alternative way to specify image source, params are:\n"
        "    * Path to image\n"
        "    * timestamp: overlays timestamp over the image\n"
        "    Example video descriptions: \":image:/path/to/image\" \":image:/path/to/image:timestamp\"" }},
    {"out",
      { "out1 [out2 ... outN]", atv::CmdArgument::Type::LIST_STRING, false,
        "resulting picture sinks such as video files or window:\n"
        "  * Video file name\n"
        "  * :highgui means output to window using OpenCV HighGUI module, stable FPS is not guaranteed\n"
        "Note: output is done simultaneously to all sinks" }}
};

static const std::string message =
"Shows images or videos like they are on an old TV screen\n"
"Based on analogtv hack written by Trevor Blackwell (https://tlb.org/)\n"
"from XScreensaver (https://www.jwz.org/xscreensaver/) by Jamie Zawinski (https://jwz.org/) and the team";


std::optional<Params> parseParams(int args, char** argv)
{
  std::map<std::string, atv::ArgType> usedArgs = atv::parseCmdArgs(knownArgs, args, argv);
  if (usedArgs.empty())
  {
    return { };
  }

  Params p;
  p.sources  = std::get<std::vector<atv::ParametricString>>(usedArgs.at("in"));
  p.outputs  = std::get<std::vector<atv::ParametricString>>(usedArgs.at("out"));
  p.controlDescription = std::get<atv::ParametricString>(usedArgs.at("control"));

  p.verbosity = 0;
  if (usedArgs.count("verbose"))
  {
    p.verbosity = std::get<int>(usedArgs.at("verbose"));
  }

  p.size = { };
  if (usedArgs.count("size"))
  {
    std::vector<int> l = std::get<std::vector<int>>(usedArgs.at("size"));
    if (l.size() != 2)
    {
      std::cout << "--size requires 2 integers" << std::endl;
      return { };
    }
    p.size = { l[0], l[1]};
    if (p.size.width < 64 || p.size.height < 64)
    {
      std::cout << "Image size should be bigger than 64x64" << std::endl;
      return { };
    }
  }

  p.seed = 0;
  if (usedArgs.count("seed"))
  {
    p.seed = std::get<int>(usedArgs.at("seed"));
  }

  return p;
}


int main (int argc, char **argv)
{
  char *s = strrchr (argv[0], '/');
  std::string progName(s ? s+1 : argv[0]);

  std::optional<Params> oparams = parseParams(argc, argv);
  if (!oparams)
  {
    showUsage(message, progName, knownArgs);
    return -1;
  }

  atv::Log::setProgName(progName);
  atv::Log::setVerbosity(oparams.value().verbosity);

  // Check that wxWidgets builds and works
  //wxPuts(wxT("TODO: implement a real GUI instead"));

  run(oparams.value());

  return 0;
}
