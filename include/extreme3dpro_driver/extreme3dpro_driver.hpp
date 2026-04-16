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

class Extreme3DProDriver : public rclcpp::Node
{
public:
  explicit Extreme3DProDriver(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~Extreme3DProDriver() override;

private:
  // ── Parameters ──────────────────────────────────────────────
  std::string device_path_;
  double      publish_rate_;
  double      deadzone_;
  bool        autorepeat_;
  bool        publish_twist_;
  double      linear_scale_;
  double      angular_scale_;

  int axis_x_;
  int axis_y_;
  int axis_yaw_;
  int axis_throttle_;
  int deadman_btn_;

  // ── Publishers ──────────────────────────────────────────────
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr        joy_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr    twist_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr          deadman_pub_;
  rclcpp::TimerBase::SharedPtr                               timer_;

  // ── Internal state ─────────────────────────────────────────
  int  fd_;
  std::vector<float>   axes_;
  std::vector<int32_t> buttons_;
  bool                 fresh_;
  std::atomic<bool>    running_;
  std::thread          read_thread_;

  void declare_parameters();
  void load_parameters();
  bool open_device();
  void close_device();
  void read_loop();
  void timer_callback();
  void publish_joy();
  void publish_twist();
  float apply_deadzone(float value) const;
  static float normalise_axis(int16_t raw, int16_t min_val, int16_t max_val);
};

}  // namespace extreme3dpro_driver
