#include "precomp.hpp"

#include "json_settings.hpp"
#include <nlohmann/json.hpp>

namespace atv
{

AppSettings loadSettings(const std::string& filePath)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    throw std::runtime_error("Cannot open settings file: " + filePath);
  }

  nlohmann::json j = nlohmann::json::parse(file);

  AppSettings s;

  if (j.contains("seed"))
  {
    s.seed = j["seed"].get<int>();
  }

  if (j.contains("size"))
  {
    auto sz = j["size"];
    s.size = cv::Size(sz[0].get<int>(), sz[1].get<int>());
  }

  if (j.contains("fps"))
  {
    s.fps = j["fps"].get<int>();
  }

  if (j.contains("knobs"))
  {
    auto& jk = j["knobs"];
    for (auto it = jk.begin(); it != jk.end(); ++it)
    {
      if (it.value().is_boolean())
      {
        s.knobs.setValue(it.key(), it.value().get<bool>() ? 1.0 : 0.0);
      }
      else if (it.value().is_number())
      {
        s.knobs.setValue(it.key(), it.value().get<double>());
      }
    }
  }

  if (j.contains("sources"))
  {
    for (const auto& src : j["sources"])
    {
      s.sources.push_back(src);
    }
  }

  if (j.contains("outputs"))
  {
    for (const auto& out : j["outputs"])
    {
      s.outputs.push_back(out);
    }
  }

  if (j.contains("channels"))
  {
    for (const auto& jch : j["channels"])
    {
      AppSettings::ChannelConfig ch;

      if (jch.contains("noise_level"))
      {
        ch.noise_level = jch["noise_level"].get<double>();
      }

      if (jch.contains("receptions"))
      {
        for (const auto& jrec : jch["receptions"])
        {
          AppSettings::ReceptionConfig rec;
          if (jrec.contains("source"))   rec.sourceIndex = jrec["source"].get<int>();
          if (jrec.contains("level"))    rec.level       = jrec["level"].get<double>();
          if (jrec.contains("multipath"))rec.multipath   = jrec["multipath"].get<double>();
          if (jrec.contains("ofs"))      rec.ofs         = jrec["ofs"].get<double>();
          if (jrec.contains("freqerr")) rec.freqerr     = jrec["freqerr"].get<double>();
          ch.receptions.push_back(rec);
        }
      }

      s.channels.push_back(ch);
    }
  }

  return s;
}

} // ::atv
