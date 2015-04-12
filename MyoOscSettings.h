// MyoOscSettings.h

#ifndef __MYO_OSC_SETTINGS_H__

#include <iostream>
#include <string>

#include <myo/myo.hpp>

struct Range {
  float min;
  float max;
  Range() : min(0), max(1) { }
};

enum class Scaling {
  NONE,
  SCALE,
  CLAMP
};

struct OutputType {
  
  bool enabled;
  std::string path;
  Range inrange;
  Range outrange;
  Scaling scaling;
  
  OutputType()
  : enabled(false), path("")
  , scaling(Scaling::NONE)
  , inrange(), outrange() { }
  
  OutputType(bool en, std::string p)
  : enabled(en), path(p)
  , scaling(Scaling::NONE)
  , inrange(), outrange() { }
  
  operator bool() const { return enabled; }
};

std::ostream& operator<<(std::ostream& os, const OutputType& type);

struct Settings {
  OutputType accel;
  OutputType gyro;
  OutputType orientation;
  OutputType orientationQuat;
  OutputType pose;
  OutputType emg;
  OutputType sync;
  OutputType rssi;
  
  bool console;
  bool logOsc;
  
  std::string hostname;
  int port;
  
  static bool readJson(std::istream& input, Settings* settings);
  static bool readJsonFile(const char* filename, Settings* settings);
};

std::ostream& operator<<(std::ostream& os, const Settings& settings);

#endif // __MYO_OSC_SETTINGS_H__
