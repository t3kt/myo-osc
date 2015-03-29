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

std::ostream& operator<<(std::ostream& os, const Settings& settings) {
  return os << "Settings<\n"
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

osc::OutboundPacketStream MyoOscGenerator::beginMessage(const char* message) {
  osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);
  p << osc::BeginMessage(message);
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
  a_x = accel.x();
  a_y = accel.y();
  a_z = accel.z();
  
  send(beginMessage("/myo/accel")
       << accel.x() << accel.y() << accel.z() << osc::EndMessage);
}

// units of deg/s
void MyoOscGenerator::onGyroscopeData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& gyro)
{
  if (!settings.gyro)
    return;
  g_x = gyro.x();
  g_y = gyro.y();
  g_z = gyro.z();
  
  send(beginMessage("/myo/gyro")
       << g_x << g_y << g_z << osc::EndMessage);
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
  
  send(beginMessage("/myo/orientation")
       << quat.x() << quat.y() << quat.z() << quat.w()
       << roll << pitch << yaw << osc::EndMessage);
  
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
  
  send(beginMessage("/myo/pose")
       << currentPose.toString().c_str() << osc::EndMessage);
  
  // Vibrate the Myo whenever we've detected that the user has made a fist.
  if (pose == myo::Pose::fist) {
    myo->vibrate(myo::Myo::vibrationShort);
  }
}

void MyoOscGenerator::onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi) {
  if (!settings.rssi)
    return;
  send(beginMessage("/myo/rssi")
       << rssi
       << osc::EndMessage);
}

void MyoOscGenerator::onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg) {
  if (!settings.emg)
    return;
  send(beginMessage("/myo/emg")
       << emg[0] << emg[1] << emg[2] << emg[3]
       << emg[4] << emg[5] << emg[6] << emg[7]
       << osc::EndMessage);
}

// onArmSync() is called whenever Myo has recognized a setup gesture after someone has put it on their
// arm. This lets Myo know which arm it's on and which way it's facing.
void MyoOscGenerator::onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
{
  if (!settings.sync)
    return;
  onArm = true;
  whichArm = arm;
  
  send(beginMessage("/myo/onarm")
       << (whichArm == myo::armLeft ? "L" : "R") << osc::EndMessage);
}

// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
// when Myo is moved around on the arm.
void MyoOscGenerator::onArmUnsync(myo::Myo* myo, uint64_t timestamp)
{
  if (!settings.sync)
    return;
  onArm = false;
  send(beginMessage("/myo/onarmlost")
       << osc::EndMessage);
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
