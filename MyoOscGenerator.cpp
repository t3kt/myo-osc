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

static void logVector(const myo::Vector3<float>& vec) {
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << vec.x();
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << vec.y();
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << vec.z();
}

static void logQuaterion(const myo::Quaternion<float>& quat) {
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << quat.x();
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << quat.y();
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << quat.z();
  std::cout << "  " << std::setw(10) << std::right << std::setprecision(2) << quat.w();
}

static void logVal(int8_t val) {
  std::cout << "  " << std::setw(10) << std::right << (int)val;
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
      logVal((int)vals[i]);
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

void MyoOscGenerator::sendMessage(const std::string& path, const myo::Quaternion<float>& quat1, const myo::Vector3<float>& vec2) {
  send(beginMessage(path)
       << quat1.x() << quat1.y() << quat1.z() << quat1.w()
       << vec2.x() << vec2.y() << vec2.z() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(path);
    logQuaterion(quat1);
    logVector(vec2);
    std::cout << std::endl;
  }
}

MyoOscGenerator::MyoOscGenerator(Settings settings)
: settings(settings), onArm(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
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
//  if (settings.logOsc) {
//    std::cout << "  " << p.Data() << std::endl;
//  }
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

// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
// as a unit quaternion.
void MyoOscGenerator::onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
{
  if (!settings.orientation)
    return;
  using std::atan2;
  using std::asin;
  using std::sqrt;
  
  // Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
  float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
                     1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
  float pitch = asin(2.0f * (quat.w() * quat.y() - quat.z() * quat.x()));
  float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
                    1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));
  
  sendMessage(settings.orientation.path, quat, myo::Vector3<float>(roll, pitch, yaw));
  
  // Convert the floating point angles in radians to a scale from 0 to 20.
  roll_w = static_cast<int>((roll + (float)M_PI)/(M_PI * 2.0f) * 18);
  pitch_w = static_cast<int>((pitch + (float)M_PI/2.0f)/M_PI * 18);
  yaw_w = static_cast<int>((yaw + (float)M_PI)/(M_PI * 2.0f) * 18);
}

// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
// making a fist, or not making a fist anymore.
void MyoOscGenerator::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
{
  if (!settings.pose)
    return;
  currentPose = pose;
  
  sendMessage(settings.pose.path, currentPose.toString().c_str());
  
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
  onArm = true;
  whichArm = arm;
  
  sendMessage(settings.sync.path, (whichArm == myo::armLeft ? "L" : "R"));
}

// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
// when Myo is moved around on the arm.
void MyoOscGenerator::onArmUnsync(myo::Myo* myo, uint64_t timestamp)
{
  if (!settings.sync)
    return;
  onArm = false;
  sendMessage(settings.sync.path, "-");
}

// We define this function to print the current values that were updated by the on...() functions above.
void MyoOscGenerator::print()
{
  if (!settings.console)
    return;
  // Clear the current line
  std::cout << '\r';
  
  // Print out the orientation. Orientation data is always available, even if no arm is currently recognized.
  std::cout << '[' << std::string(roll_w, '*') << std::string(18 - roll_w, ' ') << ']'
  << '[' << std::string(pitch_w, '*') << std::string(18 - pitch_w, ' ') << ']'
  << '[' << std::string(yaw_w, '*') << std::string(18 - yaw_w, ' ') << ']';
  
  if (onArm) {
    // Print out the currently recognized pose and which arm Myo is being worn on.
    
    // Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
    // output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
    // that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
    std::string poseString = currentPose.toString();
    
    std::cout << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
    << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
  } else {
    // Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
    std::cout << "[?]" << '[' << std::string(14, ' ') << ']';
  }
  
  std::cout << std::flush;
}
