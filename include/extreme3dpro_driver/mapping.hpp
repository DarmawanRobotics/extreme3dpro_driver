// Joystick state to velocity command (no ROS).
#pragma once

#include <string>

#include "extreme3dpro_driver/joystick.hpp"

namespace extreme3dpro
{

enum class ThrottleMode
{
  None,      // slider ignored
  Scale,     // slider scales every output, back = 0, forward = 1
  LinearZ,   // slider is linear.z
};

ThrottleMode parse_throttle_mode(const std::string & text);  // throws std::invalid_argument

struct TwistConfig
{
  int axis_x = 0;         // stick left/right -> linear.y
  int axis_y = 1;         // stick forward/back -> linear.x
  int axis_yaw = 2;       // stick twist -> angular.z
  int axis_throttle = 3;  // slider
  int deadman_button = 0; // -1: no deadman
  double deadzone = 0.05;
  double linear_scale = 1.0;
  double angular_scale = 1.0;
  ThrottleMode throttle = ThrottleMode::Scale;
};

struct Twist
{
  double linear_x = 0, linear_y = 0, linear_z = 0, angular_z = 0;
};

float apply_deadzone(float value, double deadzone);
float axis(const State & state, int index, double deadzone = 0.0);
bool deadman_held(const State & state, const TwistConfig & config);
// Zero unless connected and the deadman is held.
Twist compute_twist(const State & state, const TwistConfig & config);

}  // namespace extreme3dpro
