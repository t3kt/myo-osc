// MyoOscSettings.cpp

#include "MyoOscSettings.h"

#include "picojson.h"

#include <fstream>
#include <exception>

static inline bool isnull(const picojson::value& val) {
  return val.is<picojson::null>();
}

namespace reader {
  using picojson::value;
  using picojson::array;
  using picojson::object;
  
  static void readBool(const value& val, bool * out) {
    if (isnull(val))
      return;
    if (val.is<bool>()) {
      *out = val.get<bool>();
      return;
    }
    throw std::invalid_argument("Invalid bool value: " + val.serialize());
  }
  static void readString(const value& val, std::string* out) {
    if (isnull(val))
      return;
    if (val.is<std::string>()) {
      *out = val.get<std::string>();
      return;
    }
    throw std::invalid_argument("Invalid string value: " + val.serialize());
  }
  
  template <typename T>
  static void readNumber(const value& val, T* out) {
    if (isnull(val))
      return;
    if (val.is<double>()) {
      *out = static_cast<T>(val.get<double>());
      return;
    }
    throw std::invalid_argument("Invalid number value: " + val.serialize());
  }
  
  static void readRange(const value& val, Range* out) {
    if (isnull(val))
      return;
    if (val.is<array>()) {
      const auto& arr = val.get<array>();
      if (arr.size() != 2)
        throw std::invalid_argument("Invalid array length for range value: " + val.serialize());
      readNumber(arr[0], &out->min);
      readNumber(arr[1], &out->max);
      return;
    }
    if (val.is<object>()) {
      readNumber(val.get("min"), &out->min);
      readNumber(val.get("max"), &out->max);
      return;
    }
    throw std::invalid_argument("Invalid range value: " + val.serialize());
  }
  
  static void readScaling(const value& val, Scaling* out) {
    if (isnull(val))
      return;
    if (val.is<std::string>()) {
      const auto& str = val.get<std::string>();
      if (str == "none")
        *out = Scaling::NONE;
      else if (str == "scale")
        *out = Scaling::SCALE;
      else if (str == "clamp")
        *out = Scaling::CLAMP;
      else
        throw std::invalid_argument("Invalid scaling value: " + val.serialize());
      return;
    }
    if (val.is<double>()) {
      auto ival = static_cast<int>(val.get<double>());
      switch (ival) {
        case (int)Scaling::NONE:
        case (int)Scaling::SCALE:
        case (int)Scaling::CLAMP:
          *out = static_cast<Scaling>(ival);
          return;
      }
    }
    throw std::invalid_argument("Invalid scaling value: " + val.serialize());
  }
  
  static void readOutputType(const value& val, OutputType* out) {
    if (isnull(val)) {
      out->enabled = false;
      return;
    }
    if (val.is<bool>()) {
      out->enabled = val.get<bool>();
      return;
    }
    if (val.is<std::string>()) {
      out->path = val.get<std::string>();
      out->enabled = true;
      return;
    }
    if (val.is<object>()) {
      auto enval = val.get("enabled");
      if (enval.is<bool>())
        out->enabled = enval.get<bool>();
      else if (!enval.is<picojson::null>())
        throw std::invalid_argument("Invalid OutputType enabled value: " + enval.serialize());
      else
        out->enabled = true;
      if (out->enabled) {
        auto pathval = val.get("path");
        if (!isnull(pathval)) {
          if (pathval.is<std::string>())
            out->path = pathval.get<std::string>();
          else
            throw std::invalid_argument("Invalid OutputType path value: " + pathval.serialize());
        }
        auto inval = val.get("in");
        auto outval = val.get("out");
        if (!isnull(inval) ||
            !isnull(outval)) {
          out->scaling = Scaling::SCALE;
          readScaling(val.get("scale"), &out->scaling);
          readRange(inval, &out->inrange);
          readRange(outval, &out->outrange);
        }
      }
      return;
    }
    throw std::invalid_argument("Invalid OutputType value: " + val.serialize());
  }
  
  static void readSettings(const value& val, Settings* out) {
    if (isnull(val))
      return;
    readOutputType(val.get("accel"), &out->accel);
    readOutputType(val.get("gyro"), &out->gyro);
    readOutputType(val.get("orientation"), &out->orientation);
    readOutputType(val.get("orientationQuat"), &out->orientation);
    readOutputType(val.get("pose"), &out->pose);
    readOutputType(val.get("emg"), &out->emg);
    readOutputType(val.get("sync"), &out->sync);
    readOutputType(val.get("rssi"), &out->rssi);
    readBool(val.get("console"), &out->console);
    readBool(val.get("logOsc"), &out->logOsc);
    readString(val.get("host"), &out->hostname);
    readNumber(val.get("port"), &out->port);
  }
}

bool Settings::readJson(std::istream &input, Settings* settings) {
  picojson::value obj;
  std::string err = picojson::parse(obj, input);
  if (!err.empty()) {
    std::cerr << "Error parsing JSON: " << err << std::endl;
    return false;
  }
  try {
    reader::readSettings(obj, settings);
    return true;
  } catch (const std::exception& ex) {
    std::cerr << "Error reading JSON: " << ex.what() << std::endl;
    return false;
  }
}

bool Settings::readJsonFile(const char* filename, Settings* settings) {
  std::ifstream filein(filename);
  if (filein.bad())
    return false;
  bool ok = readJson(filein, settings);
  filein.close();
  return ok;
}

std::ostream& operator<<(std::ostream& os, const OutputType& type) {
  if (type)
    os << type.path;
  else
    os << "(none)";
  if (type.scaling == Scaling::SCALE ||
      type.scaling == Scaling::CLAMP) {
    os << " [" << type.inrange.min << ", " << type.inrange.max << "] -> ["
    << type.outrange.min << ", " << type.outrange.max << "]";
    if (type.scaling == Scaling::CLAMP)
      os << " (clamp)";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  static const std::string none("(none)");
  return os << std::boolalpha << "Settings<\n"
  << "  hostname: " << settings.hostname << "\n"
  << "  port: " << settings.port << "\n"
  << "  accel: " << settings.accel << "\n"
  << "  gyro: " << settings.gyro << "\n"
  << "  orientation: " << settings.orientation << "\n"
  << "  pose: " << settings.pose << "\n"
  << "  emg: " << settings.emg << "\n"
  << "  sync: " << settings.sync << "\n"
  << "  rssi: " << settings.rssi << "\n"
  << "  console: " << settings.console << "\n"
  << ">\n";
}
