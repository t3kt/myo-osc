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

#ifndef __MYO_OSC_GENERATOR_H__

// stop oscpack sprintf warnings
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>

// add oscpack
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#define OUTPUT_BUFFER_SIZE 1024

#include "MyoOscSettings.h"

// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class MyoOscGenerator : public myo::DeviceListener {
public:
  explicit MyoOscGenerator(Settings settings);
  
  ~MyoOscGenerator() override;
  
  // units of g
  void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& accel) override;

  // units of deg/s
  void onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro) override;
  
  // onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
  // as a unit quaternion.
  void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat) override;
  
  // onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
  // making a fist, or not making a fist anymore.
  void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose) override;
  
  // onEmgData() is called whenever the Myo receives EMG data
  void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg) override;
  
  // onRssi() is called whenever the Myo receives a RSSI value
  // (received signal strength indication)
  void onRssi(myo::Myo* myo, uint64_t timestamp, int8_t rssi) override;
  
  // onArmSync() is called whenever Myo has recognized a setup gesture after someone has put it on their
  // arm. This lets Myo know which arm it's on and which way it's facing.
  void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection) override;
  
  // onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
  // it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
  // when Myo is moved around on the arm.
  void onArmUnsync(myo::Myo* myo, uint64_t timestamp) override;
  
  osc::OutboundPacketStream beginMessage(const std::string& message);
  
  void send(const osc::OutboundPacketStream& p);
  
  void sendMessage(const OutputType& type, int8_t val);
  void sendMessage(const OutputType& type, const int8_t* vals, int count);
  void sendMessage(const OutputType& type, const char* val);
  void sendMessage(const OutputType& type, myo::Vector3<float> vec);
  void sendMessage(const OutputType& type, myo::Vector3<float> vec1, myo::Vector3<float> vec2);
  void sendMessage(const OutputType& type, myo::Quaternion<float> quat);
  
  char buffer[OUTPUT_BUFFER_SIZE];
  UdpTransmitSocket* transmitSocket;
  Settings settings;
};

#endif // __MYO_OSC_GENERATOR_H__
