#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/bool.hpp>

#include <string>
#include <vector>
#include <atomic>
#include <thread>

namespace extreme3dpro_driver
{

// ──────────────────────────────────────────────────────────────
// Logitech Extreme 3D Pro axis / button layout  (Linux evdev)
// ──────────────────────────────────────────────────────────────
//  Axis 0  –  X        (roll,   left/right)       –1024 … 1023
//  Axis 1  –  Y        (pitch,  forward/back)      –1024 … 1023
//  Axis 2  –  Rz       (twist / yaw)               –128 …  127
//  Axis 3  –  Throttle (slider, bottom)              0  …  255
//  Axis 4  –  Hat X    (–1, 0, +1)
//  Axis 5  –  Hat Y    (–1, 0, +1)
//
//  Buttons 0‒11 (trigger = 0, thumb = 1, 3‒11 base buttons)
// ──────────────────────────────────────────────────────────────

struct JoystickEvent
{
  uint32_t time;   // timestamp (ms)
  int16_t  value;  // axis / button value
  uint8_t  type;   // JS_EVENT_BUTTON | JS_EVENT_AXIS  (| JS_EVENT_INIT)
  uint8_t  number; // axis / button index
};

class Extreme3DProDriver : public rclcpp::Node
{
public:
  explicit Extreme3DProDriver(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~Extreme3DProDriver() override;

private:
  // ── Parameters ──────────────────────────────────────────────
  std::string device_path_;
  double      publish_rate_;      // Hz
  double      deadzone_;          // 0‥1
  bool        autorepeat_;
  bool        publish_twist_;     // also publish cmd_vel?
  double      linear_scale_;      // m/s  per full‑deflection
  double      angular_scale_;     // rad/s per full‑deflection

  // ── Axis / button mapping indices (configurable) ───────────
  int axis_x_;       // roll  → twist.linear.y  (strafe)
  int axis_y_;       // pitch → twist.linear.x  (fwd/back)
  int axis_yaw_;     // yaw   → twist.angular.z
  int axis_throttle_;// throttle → twist.linear.z
  int deadman_btn_;  // deadman switch (trigger default)

  // ── Publishers ──────────────────────────────────────────────
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr        joy_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr    twist_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr          deadman_pub_;
  rclcpp::TimerBase::SharedPtr                               timer_;

  // ── Internal state ─────────────────────────────────────────
  int  fd_;                          // file descriptor
  std::vector<float>   axes_;        // normalised –1 … +1
  std::vector<int32_t> buttons_;     // 0 / 1
  bool                 fresh_;       // new data since last pub?
  std::atomic<bool>    running_;
  std::thread          read_thread_;

  // ── Methods ────────────────────────────────────────────────
  void declare_parameters();
  void load_parameters();
  bool open_device();
  void close_device();
  void read_loop();                  // blocking, runs in thread
  void timer_callback();             // publish at fixed rate
  void publish_joy();
  void publish_twist();
  float apply_deadzone(float value) const;

  /// Normalise raw axis value to –1 … +1
  static float normalise_axis(int16_t raw, int16_t min_val, int16_t max_val);
};

}  // namespace extreme3dpro_driver
