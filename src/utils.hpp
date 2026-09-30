#pragma once

#include "precomp.hpp"

namespace atv
{

struct Log
{
    // TODO: variadic
    static void write(int level, const std::string& s);
    static void setVerbosity(int n);
    static int  getVerbosity();
    static void setProgName(const std::string& s);
};

cv::Mat drawTime(double time);

cv::Mat loadImage(const std::string& fname);

std::optional<int> parseInt(const std::string &s);

// splits string by a delimiting char, start and end of source string:
// ":asdf:qwer:" by ":" -> ["", "asdf", "qwer", ""]
// "asdf:qwer"   by ":" -> ["asdf", "qwer"]
std::vector<std::string> split(const std::string& s, char d);

// transforms a list of strings to key-value pairs:
// ["a", "k1=v1", "k2=v2", ""] -> {"a":"", "k1":"v1", "k2":"v2"}
// empty strings are omitted
// more than one '=' character in a string is forbidden
std::map<std::string, std::string> parseKeyValues(const std::vector<std::string>& tokens);

struct ParametricString
{
  std::string className;
  std::vector<std::string> varArgs;
  std::map<std::string, std::string> kvArgs;

  static ParametricString parse(const std::string& desc);
};

struct CmdArgument
{
  enum class Type
  {
    BOOL, INT, LIST_INT, STRING, LIST_STRING
  };

  Type type;
  bool optional;
  std::string exampleArgs;
  std::vector<std::string> help;

  CmdArgument(const std::string& _exampleArgs, const Type& _type,
              bool _optional, const std::string& _help) :
    type(_type),
    optional(_optional),
    exampleArgs(_exampleArgs),
    help()
  {
    size_t p0 = 0;
    for (size_t i = 0; i < _help.length(); i++)
    {
      if (_help.at(i) == '\n')
      {
        help.push_back(_help.substr(p0, i - p0));
        p0 = i+1;
      }
    }
    if (p0 < _help.length())
    {
      help.push_back(_help.substr(p0, _help.length() - p0 - 1));
    }
  }
};

typedef std::variant<bool, int, ParametricString, std::vector<int>, std::vector<ParametricString>> ArgType;
std::map<std::string, ArgType> parseCmdArgs(const std::map<std::string, CmdArgument>& knownArgs, int nArgs, char** argv);

void showUsage(const std::string& message, const std::string& appName, const std::map<std::string, CmdArgument>& knownArgs);


enum class PropertyType
{
  Double,
  Int,
  Bool,
};

// for GUI creation
enum class ControlType
{
  DoubleSpin,
  IntSpin,
  CheckBox,
  Dial
};

struct PropertyInfo
{
  PropertyType type;
  ControlType controlType;
  double min;
  double max;
  double defaultValue;
  std::string description;
  void* value;
};


struct Properties : std::map<std::string, PropertyInfo>
{
  Properties() = default;

  Properties(const Properties&) = default;
  Properties(Properties&&) = default;

  Properties& operator=(const Properties&) = default;
  Properties& operator=(Properties&&) = default;

  ~Properties() = default;

  Properties(const std::initializer_list<std::pair<const std::string, PropertyInfo>>& init) :
    std::map<std::string, PropertyInfo>(init)
  {
    initDefault();
  }

  void initDefault() const
  {
    for (const auto& p : *this)
    {
      if (p.second.type == PropertyType::Double)
        *(static_cast<double*>(p.second.value)) = p.second.defaultValue;
      else if (p.second.type == PropertyType::Bool)
        *(static_cast<bool*>(p.second.value)) = (std::abs(p.second.defaultValue) > std::numeric_limits<double>::epsilon());
      else if (p.second.type == PropertyType::Int)
        *(static_cast<int*>(p.second.value)) = static_cast<int>(p.second.defaultValue);
    }
  }

  std::pair<double, double> getRange(const std::string& param) const
  {
    auto it = this->find(param);
    if (it != this->end())
      return {it->second.min, it->second.max};
    else
      throw std::runtime_error("Parameter not found: " + param);
  }

  double getDefault(const std::string& param) const
  {
    auto it = this->find(param);
    if (it != this->end())
      return it->second.defaultValue;
    else
      throw std::runtime_error("Parameter not found: " + param);
  }

  std::string getDescription(const std::string& param) const
  {
    auto it = this->find(param);
    if (it != this->end())
      return it->second.description;
    else
      throw std::runtime_error("Parameter not found: " + param);
  }

  ControlType getControlType(const std::string& param) const
  {
    auto it = this->find(param);
    if (it != this->end())
      return it->second.controlType;
    else
      throw std::runtime_error("Parameter not found: " + param);
  }

  void setValue(const std::string& param, double value) const
  {
    auto it = this->find(param);
    if (it != this->end())
    {
      if (!it->second.value)
        throw std::runtime_error("Parameter has no associated value: " + param);
      else
      {
        if (it->second.type == PropertyType::Double)
          *(static_cast<double*>(it->second.value)) = value;
        else if (it->second.type == PropertyType::Bool)
          *(static_cast<bool*>(it->second.value)) = (std::abs(value) > std::numeric_limits<double>::epsilon());
        else if (it->second.type == PropertyType::Int)
          *(static_cast<int*>(it->second.value)) = static_cast<int>(value);
      }
    }
    else
      throw std::runtime_error("Parameter not found: " + param);
  }

  double getValue(const std::string& param) const
  {
    auto it = this->find(param);
    if (it != this->end() && it->second.value)
    {
      if (!it->second.value)
        throw std::runtime_error("Parameter has no associated value: " + param);
      else
      {
        if (it->second.type == PropertyType::Double)
          return *(static_cast<double*>(it->second.value));
        else if (it->second.type == PropertyType::Bool)
          return static_cast<bool>(*(static_cast<bool*>(it->second.value)));
        else if (it->second.type == PropertyType::Int)
          return static_cast<int>(*(static_cast<int*>(it->second.value)));
        else return 0.0;
      }
    }
    else
      throw std::runtime_error("Parameter not found: " + param);
  }
};



} // ::atv
