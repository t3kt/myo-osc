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
#include <cfloat>

//check for division by zero???
//--------------------------------------------------
//float ofMap(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp) {
//  
//  if (fabs(inputMin - inputMax) < FLT_EPSILON){
//    ofLogWarning("ofMath") << "ofMap(): avoiding possible divide by zero, check inputMin and inputMax: " << inputMin << " " << inputMax;
//    return outputMin;
//  }

static float mapValue(float value, Range inRange,
                      Range outRange, bool clamp) {
//  if (fabs(inRange.min - inRange.max) < FLT_EPSILON)
  float outVal = ((value - inRange.min) / (inRange.max - inRange.min) * (outRange.max - outRange.min) + outRange.min);
  if (clamp) {
    if (outRange.max < outRange.min) {
      if (outVal < outRange.max)
        return outRange.max;
      if (outVal > outRange.min)
        return outRange.min;
    } else {
      if (outVal < outRange.min)
        return outRange.min;
      if (outVal > outRange.max)
        return outRange.max;
    }
  }
  return outVal;
}

static float scale(float value, const OutputType& type) {
  if (type.scaling == Scaling::SCALE)
    return mapValue(value, type.inrange, type.outrange, false);
  else if(type.scaling == Scaling::CLAMP)
    return mapValue(value, type.inrange, type.outrange, true);
  else
    return value;
}

static myo::Vector3<float> scale(myo::Vector3<float> value, const OutputType& type) {
  if (type.scaling == Scaling::NONE)
    return value;
  return myo::Vector3<float>(scale(value.x(), type),
                             scale(value.y(), type),
                             scale(value.z(), type));
}

static myo::Quaternion<float> scale(myo::Quaternion<float> value, const OutputType& type) {
  if (type.scaling == Scaling::NONE)
    return value;
  return myo::Quaternion<float>(scale(value.x(), type),
                                scale(value.y(), type),
                                scale(value.z(), type),
                                scale(value.w(), type));
}

static int8_t scale(int8_t value, const OutputType& type) {
  if (type.scaling == Scaling::NONE)
    return value;
  return static_cast<int8_t>(scale(static_cast<float>(value), type));
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

void MyoOscGenerator::sendMessage(const OutputType& type, int8_t val) {
  send(beginMessage(type.path)
       << scale(val, type) << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
    logVal(scale(val, type));
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const OutputType& type, const int8_t* vals, int count) {
  auto p = beginMessage(type.path);
  for (int i = 0; i < count; ++i) {
    p << scale(vals[i], type);
  }
  send(p << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
    for (int i = 0; i < count; ++i) {
      logVal(scale(vals[i], type));
    }
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const OutputType& type, const char* val) {
  send(beginMessage(type.path)
       << val << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
    std::cout << "  " << std::right << val;
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const OutputType& type, myo::Vector3<float> vec) {
  vec = scale(vec, type);
  send(beginMessage(type.path)
       << vec.x() << vec.y() << vec.z() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
    logVector(vec);
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const OutputType& type, myo::Vector3<float> vec1, myo::Vector3<float> vec2) {
  vec1 = scale(vec1, type);
  vec2 = scale(vec2, type);
  send(beginMessage(type.path)
       << vec1.x() << vec1.y() << vec1.z()
       << vec2.x() << vec2.y() << vec2.z() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
    logVector(vec1);
    logVector(vec2);
    std::cout << std::endl;
  }
}

void MyoOscGenerator::sendMessage(const OutputType& type, myo::Quaternion<float> quat) {
  quat = scale(quat, type);
  send(beginMessage(type.path)
       << quat.x() << quat.y() << quat.z() << quat.w() << osc::EndMessage);
  if (settings.logOsc) {
    logPath(type.path);
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
  sendMessage(settings.accel, accel);
}

// units of deg/s
void MyoOscGenerator::onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro)
{
  if (!settings.gyro)
    return;
  sendMessage(settings.gyro, gyro);
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
    sendMessage(settings.orientationQuat, quat);
  
  if (settings.orientation) {
    auto ypr = quaternionToVector(quat);
    sendMessage(settings.orientation, ypr);
  }
}

// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
// making a fist, or not making a fist anymore.
void MyoOscGenerator::onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
{
  if (!settings.pose)
    return;
  
  sendMessage(settings.pose, pose.toString().c_str());
  
  // Vibrate the Myo whenever we've detected that the user has made a fist.
  if (pose == myo::Pose::fist) {
    myo->vibrate(myo::Myo::vibrationShort);
  }
}

void MyoOscGenerator::onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi) {
  if (!settings.rssi)
    return;
  sendMessage(settings.rssi, rssi);
}

void MyoOscGenerator::onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg) {
  if (!settings.emg)
    return;
  sendMessage(settings.emg, emg, 8);
}

// onArmSync() is called whenever Myo has recognized a setup gesture after someone has put it on their
// arm. This lets Myo know which arm it's on and which way it's facing.
void MyoOscGenerator::onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
{
  if (!settings.sync)
    return;
  sendMessage(settings.sync, (arm == myo::armLeft ? "L" : "R"));
}

// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
// when Myo is moved around on the arm.
void MyoOscGenerator::onArmUnsync(myo::Myo* myo, uint64_t timestamp)
{
  if (!settings.sync)
    return;
  sendMessage(settings.sync, "-");
}
