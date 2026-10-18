#include "extreme3dpro_driver/mapping.hpp"

#include <cmath>
#include <stdexcept>

namespace extreme3dpro
{

ThrottleMode parse_throttle_mode(const std::string & text)
{
  if (text == "none") {
    return ThrottleMode::None;
  }
  if (text == "scale") {
    return ThrottleMode::Scale;
  }
  if (text == "linear_z") {
    return ThrottleMode::LinearZ;
  }
  throw std::invalid_argument("throttle_mode must be none, scale or linear_z, not '" + text + "'");
}

float apply_deadzone(float value, double deadzone)
{
  if (deadzone <= 0.0) {
    return value;
  }
  const float dz = static_cast<float>(deadzone);
  if (dz >= 1.0f || std::fabs(value) <= dz) {
    return 0.0f;
  }
  float sign = value > 0.0f ? 1.0f : -1.0f;
  return sign * (std::fabs(value) - dz) / (1.0f - dz);
}

float axis(const State & state, int index, double deadzone)
{
  if (index < 0 || index >= static_cast<int>(state.axes.size())) {
    return 0.0f;
  }
  return apply_deadzone(state.axes[index], deadzone);
}

bool deadman_held(const State & state, const TwistConfig & config)
{
  if (!state.connected) {
    return false;
  }
  if (config.deadman_button < 0) {
    return true;
  }
  return config.deadman_button < static_cast<int>(state.buttons.size()) &&
         state.buttons[config.deadman_button] != 0;
}

Twist compute_twist(const State & state, const TwistConfig & config)
{
  Twist t;
  if (!deadman_held(state, config)) {
    return t;
  }
  // Linux: stick forward and right are negative / positive, twist right is positive,
  // slider forward is -1. ROS: x forward, y left, z counter-clockwise.
  double scale = 1.0;
  float slider = axis(state, config.axis_throttle);
  if (config.throttle == ThrottleMode::Scale) {
    scale = (1.0 - slider) / 2.0;
  } else if (config.throttle == ThrottleMode::LinearZ) {
    t.linear_z = -slider * config.linear_scale;
  }
  t.linear_x = -axis(state, config.axis_y, config.deadzone) * config.linear_scale * scale;
  t.linear_y = -axis(state, config.axis_x, config.deadzone) * config.linear_scale * scale;
  t.angular_z = -axis(state, config.axis_yaw, config.deadzone) * config.angular_scale * scale;
  return t;
}

}  // namespace extreme3dpro
