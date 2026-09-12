#pragma once

#include "precomp.hpp"

#include "utils.hpp"
#include "analogtv.hpp"
#include "source.hpp"

namespace atv
{

struct ChanSetting
{
  ChanSetting() :
    receptions(),
    sources(),
    noise_level(0),
    chanParams {
      {"noise_level", {atv::ParamType::Double, 0.0, 5.0, 0.04, "Channel noise level", &noise_level}}
    }
  { }

  // chanParams holds a pointer to this->noise_level, so a naive compiler-generated copy would
  // leave it pointing at the source object; rebuild it via the default constructor instead.
  ChanSetting(const ChanSetting& other)
    : ChanSetting()
  {
    *this = other;
  }

  ChanSetting& operator=(const ChanSetting& other)
  {
    if (this != &other)
    {
      receptions = other.receptions;
      sources = other.sources;
      noise_level = other.noise_level;
      // chanParams pointers already point to this->noise_level, no need to reinit
    }
    return *this;
  }

  //TODO: join them into one vector
  std::vector<atv::AnalogReception> receptions;
  std::vector<std::shared_ptr<atv::Source>> sources;
  double noise_level; // noise: 0 to 0.2 or 0 to 5.0, default 0.04

  std::map<std::string, ParamInfo> chanParams;

  std::pair<double, double> getRange(const std::string& param) const
  {
    auto it = chanParams.find(param);
    if (it != chanParams.end()) return {it->second.min, it->second.max};
    return {0.0, 0.0};
  }

  void setValue(const std::string& param, double value)
  {
    auto it = chanParams.find(param);
    if (it != chanParams.end() && it->second.value)
    {
      *(static_cast<double*>(it->second.value)) = value;
    }
  }
};

struct Control
{
  struct Operation
  {
    enum class Type
    {
      QUIT, SWITCH, NONE
    };

    Type type;
    int channel;
  };

  static std::shared_ptr<Control> create(const atv::ParametricString& desc);

  virtual void setRNG(uint64_t rngSeed) = 0;

  // why const ref to sources does not work?
  virtual void createChannels(const std::vector<std::shared_ptr<atv::Source>> sources) = 0;

  virtual Knobs getKnobs() = 0;

  virtual void run() = 0;

  virtual Operation getNext() = 0;

  virtual double getTime() = 0;

  std::vector<ChanSetting> chanSettings;
};


} // ::atv