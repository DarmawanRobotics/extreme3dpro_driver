#pragma once

#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Bool.h>

#include <string>
#include <vector>
#include <atomic>
#include <thread>

namespace extreme3dpro_driver
{

class Extreme3DProDriver
{
public:
  Extreme3DProDriver(ros::NodeHandle & nh, ros::NodeHandle & pnh);
  ~Extreme3DProDriver();

private:
  ros::NodeHandle nh_, pnh_;

  std::string device_path_;
  double      publish_rate_, deadzone_, linear_scale_, angular_scale_;
  bool        autorepeat_, publish_twist_;
  int         axis_x_, axis_y_, axis_yaw_, axis_throttle_, deadman_btn_;

  ros::Publisher joy_pub_, twist_pub_, deadman_pub_;
  ros::Timer     timer_;

  int                  fd_;
  std::vector<float>   axes_;
  std::vector<int32_t> buttons_;
  bool                 fresh_;
  std::atomic<bool>    running_;
  std::thread          read_thread_;

  void  load_parameters();
  bool  open_device();
  void  close_device();
  void  read_loop();
  void  timer_callback(const ros::TimerEvent &);
  void  publish_joy();
  void  publish_twist();
  float apply_deadzone(float value) const;
  static float normalise_axis(int16_t raw, int16_t min_val, int16_t max_val);
};

}  // namespace extreme3dpro_driver
