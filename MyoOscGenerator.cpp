// Myo-OSC, by Samy Kamkar
// outputs all data from the Myo armband over OSC
// usage: myo-osc osc.ip.address osc.port
//
// http://samy.pl - code@samy.pl - 03/03/2014
//
// also makes use of oscpack from http://www.rossbencina.com/code/oscpack
//
// some code originally from hello-myo:
// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.


#include "MyoOscGenerator.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <iomanip>
#include <fstream>

static bool readOutputTypeJson(const picojson::value& obj, OutputType* out) {
  if (obj.is<picojson::null>()) {
    out->enabled = false;
    return true;
  }
  if (obj.is<bool>()) {
    out->enabled = obj.get<bool>();
    return true;
  }
  if (obj.is<std::string>()) {
    out->path = obj.get<std::string>();
    out->enabled = !out->path.empty();
    return true;
  }
  if (obj.is<picojson::object>()) {
    out->enabled = obj.get("enabled").evaluate_as_boolean();
    if (out->enabled) {
      auto pathval = obj.get("path");
      if (!pathval.is<picojson::null>()) {
        if (pathval.is<std::string>())
          out->path = pathval.get<std::string>();
        else
          return false;
      }
    }
    return true;
  }
  return false;
}

static bool readBoolJson(const picojson::value& val, bool* out) {
  if (val.is<picojson::null>())
    return true;
  if (val.is<bool>()) {
    *out = val.get<bool>();
    return true;
  }
  return false;
}

static bool readStringJson(const picojson::value& val, std::string* out) {
  if (val.is<picojson::null>())
    return true;
  if (val.is<std::string>()) {
    *out = val.get<std::string>();
    return true;
  }
  return false;
}

static bool readIntJson(const picojson::value& val, int* out) {
  if (val.is<picojson::null>())
    return true;
  if (val.is<double>()) {
    *out = static_cast<int>(val.get<double>());
    return true;
  }
  return false;
}

bool Settings::readJson(std::istream &input, Settings* settings) {
  picojson::value obj;
  std::string err = picojson::parse(obj, input);
  if (!err.empty()) {
    std::cerr << "Error parsing JSON: " << err << std::endl;
    return false;
  }
  if (!readOutputTypeJson(obj.get("accel"), &settings->accel) ||
      !readOutputTypeJson(obj.get("gyro"), &settings->gyro) ||
      !readOutputTypeJson(obj.get("orientation"), &settings->orientation) ||
      !readOutputTypeJson(obj.get("orientationQuat"), &settings->orientationQuat) ||
      !readOutputTypeJson(obj.get("pose"), &settings->pose) ||
      !readOutputTypeJson(obj.get("emg"), &settings->emg) ||
      !readOutputTypeJson(obj.get("sync"), &settings->sync) ||
      !readOutputTypeJson(obj.get("rssi"), &settings->rssi) ||
      !readBoolJson(obj.get("console"), &settings->console) ||
      !readBoolJson(obj.get("logOsc"), &settings->logOsc) ||
      !readStringJson(obj.get("host"), &settings->hostname) ||
      !readIntJson(obj.get("port"), &settings->port))
    return false;
  return true;
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

static void logPath(const std::string& path) {
  std::cout << std::setw(20) << std::setfill(' ') << std::left << (path + ":");
}

static void logVal(float val) {
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << val;
}

static void logVal(int8_t val) {
  std::cout << "  " << std::setw(10) << std::right << (int)val;
}

static void logVector(const myo::Vector3<float>& vec) {
  logVal(vec.x());
  logVal(vec.y());
  logVal(vec.z());
}

static void logQuaterion(const myo::Quaternion<float>& quat) {
  logVal(quat.x());
  logVal(quat.y());
  logVal(quat.z());
  logVal(quat.w());
}

void MyoOscGenerator::sendMessage(const std::string &path, int8_t val) {
  send(beginMessage(path)
       << val << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    logVal(val);
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const std::string &path, const int8_t* vals, int count) {
  auto p = beginMessage(path);
  for (int i = 0; i < count; ++i) {
    p << vals[i];
  }
  send(p << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    for (int i = 0; i < count; ++i) {
      logVal(vals[i]);
    }
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const std::string &path, const char* val) {
  send(beginMessage(path)
       << val << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    std::cout << "  " << std::right << val;
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const std::string& path, const myo::Vector3<float>& vec) {
  send(beginMessage(path)
       << vec.x() << vec.y() << vec.z() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    logVector(vec);
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const std::string& path, const myo::Vector3<float>& vec1, const myo::Vector3<float>& vec2) {
  send(beginMessage(path)
       << vec1.x() << vec1.y() << vec1.z()
       << vec2.x() << vec2.y() << vec2.z() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    logVector(vec1);
    logVector(vec2);
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const std::string& path, const myo::Quaternion<float>& quat) {
  send(beginMessage(path)
       << quat.x() << quat.y() << quat.z() << quat.w() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    logQuaterion(quat);
    std::cout << std::endl;
  }
}

MyoOscGenerator::MyoOscGenerator(Settings settings)
: settings(settings)
{
  transmitSocket = new UdpTransmitSocket(IpEndpointName(settings.hostname.c_str(), settings.port));
}

MyoOscGenerator::~MyoOscGenerator() {
  if (transmitSocket != nullptr) {
    delete transmitSocket;
  }
}

osc::OutboundPacketStream MyoOscGenerator::beginMessage(const std::string& message) {
  osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);
  p << osc::BeginMessage(message.c_str());
  return p;
}

void MyoOscGenerator::send(const osc::OutboundPacketStream& p) {
  transmitSocket->Send(p.Data(), p.Size());
}

// units of g
void MyoOscGenerator::onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel)
{
  if (!settings.accel)
    return;
  sendMessage(settings.accel.path, accel);
}

// units of deg/s
void MyoOscGenerator::onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro)
{
  if (!settings.gyro)
    return;
  sendMessage(settings.gyro.path, gyro);
}

static myo::Vector3<float> quaternionToVector(const myo::Quaternion<float>& quat) {
  // Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
  float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
                    1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));
  float pitch = asin(2.0f * (quat.w() * quat.y() - quat.z() * quat.x()));
  float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
                     1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
  return myo::Vector3<float>(yaw, pitch, roll);
}

// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
// as a unit quaternion.
void MyoOscGenerator::onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
{
  if (!settings.orientation && !settings.orientationQuat)
    return;
  
  if (settings.orientationQuat)
    sendMessage(settings.orientationQuat.path, quat);
  
  if (settings.orientation) {
    auto ypr = quaternionToVector(quat);
    sendMessage(settings.orientation.path, ypr);
  }
}

// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
// making a fist, or not making a fist anymore.
void MyoOscGenerator::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
{
  if (!settings.pose)
    return;
  
  sendMessage(settings.pose.path, pose.toString().c_str());
  
  // Vibrate the Myo whenever we've detected that the user has made a fist.
  if (pose == myo::Pose::fist) {
    myo->vibrate(myo::Myo::vibrationShort);
  }
}

void MyoOscGenerator::onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi) {
  if (!settings.rssi)
    return;
  sendMessage(settings.rssi.path, rssi);
}

void MyoOscGenerator::onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg) {
  if (!settings.emg)
    return;
  sendMessage(settings.emg.path, emg, 8);
}

// onArmSync() is called whenever Myo has recognized a setup gesture after someone has put it on their
// arm. This lets Myo know which arm it's on and which way it's facing.
void MyoOscGenerator::onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
{
  if (!settings.sync)
    return;
  sendMessage(settings.sync.path, (arm == myo::armLeft ? "L" : "R"));
}

// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
// when Myo is moved around on the arm.
void MyoOscGenerator::onArmUnsync(myo::Myo* myo, uint64_t timestamp)
{
  if (!settings.sync)
    return;
  sendMessage(settings.sync.path, "-");
}
