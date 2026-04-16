#include "extreme3dpro_driver/extreme3dpro_driver.h"

#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace extreme3dpro_driver
{

Extreme3DProDriver::Extreme3DProDriver(ros::NodeHandle & nh, ros::NodeHandle & pnh)
: nh_(nh), pnh_(pnh), fd_(-1), fresh_(false), running_(false)
{
  load_parameters();

  joy_pub_    = nh_.advertise<sensor_msgs::Joy>("joy", 10);
  deadman_pub_= nh_.advertise<std_msgs::Bool>("deadman", 10);
  if (publish_twist_)
    twist_pub_ = nh_.advertise<geometry_msgs::Twist>("cmd_vel", 10);

  if (!open_device()) {
    ROS_FATAL("Cannot open joystick '%s'. Is the Extreme 3D Pro connected?",
              device_path_.c_str());
    ros::shutdown();
    return;
  }

  running_ = true;
  read_thread_ = std::thread(&Extreme3DProDriver::read_loop, this);

  timer_ = nh_.createTimer(
    ros::Duration(1.0 / publish_rate_),
    &Extreme3DProDriver::timer_callback, this);

  ROS_INFO("extreme3dpro_driver started — device: %s  rate: %.0f Hz  deadzone: %.2f",
           device_path_.c_str(), publish_rate_, deadzone_);
}

Extreme3DProDriver::~Extreme3DProDriver()
{
  running_ = false;
  if (read_thread_.joinable()) read_thread_.join();
  close_device();
}

void Extreme3DProDriver::load_parameters()
{
  pnh_.param<std::string>("device",        device_path_,   "/dev/input/js0");
  pnh_.param<double>     ("publish_rate",  publish_rate_,  50.0);
  pnh_.param<double>     ("deadzone",      deadzone_,      0.05);
  pnh_.param<bool>       ("autorepeat",    autorepeat_,    true);
  pnh_.param<bool>       ("publish_twist", publish_twist_, true);
  pnh_.param<double>     ("linear_scale",  linear_scale_,  1.0);
  pnh_.param<double>     ("angular_scale", angular_scale_, 1.0);
  pnh_.param<int>        ("axis_x",        axis_x_,        0);
  pnh_.param<int>        ("axis_y",        axis_y_,        1);
  pnh_.param<int>        ("axis_yaw",      axis_yaw_,      2);
  pnh_.param<int>        ("axis_throttle", axis_throttle_, 3);
  pnh_.param<int>        ("deadman_btn",   deadman_btn_,   0);
}

bool Extreme3DProDriver::open_device()
{
  fd_ = open(device_path_.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd_ < 0) {
    ROS_ERROR("open(%s): %s", device_path_.c_str(), strerror(errno));
    return false;
  }

  uint8_t num_axes = 0, num_buttons = 0;
  ioctl(fd_, JSIOCGAXES,    &num_axes);
  ioctl(fd_, JSIOCGBUTTONS, &num_buttons);
  axes_.resize(num_axes, 0.0f);
  buttons_.resize(num_buttons, 0);

  char name[128] = "Unknown";
  ioctl(fd_, JSIOCGNAME(sizeof(name)), name);
  ROS_INFO("Joystick: %s  (%d axes, %d buttons)", name, num_axes, num_buttons);
  return true;
}

void Extreme3DProDriver::close_device()
{
  if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

void Extreme3DProDriver::read_loop()
{
  struct js_event event {};
  while (running_) {
    ssize_t bytes = read(fd_, &event, sizeof(event));
    if (bytes == static_cast<ssize_t>(sizeof(event))) {
      event.type &= ~JS_EVENT_INIT;
      if (event.type == JS_EVENT_AXIS &&
          event.number < static_cast<uint8_t>(axes_.size())) {
        axes_[event.number] = normalise_axis(event.value, -32767, 32767);
        fresh_ = true;
      } else if (event.type == JS_EVENT_BUTTON &&
                 event.number < static_cast<uint8_t>(buttons_.size())) {
        buttons_[event.number] = event.value;
        fresh_ = true;
      }
    } else if (bytes < 0 && errno != EAGAIN) {
      ROS_ERROR("Read error: %s", strerror(errno));
      running_ = false;
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

void Extreme3DProDriver::timer_callback(const ros::TimerEvent &)
{
  if (!fresh_ && !autorepeat_) return;
  publish_joy();
  if (publish_twist_) publish_twist();

  std_msgs::Bool dm;
  dm.data = (deadman_btn_ >= 0 &&
             deadman_btn_ < static_cast<int>(buttons_.size()) &&
             buttons_[deadman_btn_] != 0);
  deadman_pub_.publish(dm);
  fresh_ = false;
}

void Extreme3DProDriver::publish_joy()
{
  sensor_msgs::Joy msg;
  msg.header.stamp    = ros::Time::now();
  msg.header.frame_id = "extreme3dpro";
  msg.axes.resize(axes_.size());
  for (size_t i = 0; i < axes_.size(); ++i)
    msg.axes[i] = apply_deadzone(axes_[i]);
  msg.buttons.resize(buttons_.size());
  for (size_t i = 0; i < buttons_.size(); ++i)
    msg.buttons[i] = buttons_[i];
  joy_pub_.publish(msg);
}

void Extreme3DProDriver::publish_twist()
{
  bool active = (deadman_btn_ >= 0 &&
                 deadman_btn_ < static_cast<int>(buttons_.size()) &&
                 buttons_[deadman_btn_] != 0);
  geometry_msgs::Twist twist;
  if (active) {
    auto sa = [&](int i) -> float {
      return (i >= 0 && i < static_cast<int>(axes_.size()))
             ? apply_deadzone(axes_[i]) : 0.0f;
    };
    twist.linear.x  = -sa(axis_y_)        * linear_scale_;
    twist.linear.y  =  sa(axis_x_)         * linear_scale_;
    twist.linear.z  =  sa(axis_throttle_)  * linear_scale_;
    twist.angular.z = -sa(axis_yaw_)       * angular_scale_;
  }
  twist_pub_.publish(twist);
}

float Extreme3DProDriver::apply_deadzone(float v) const
{
  if (std::fabs(v) < deadzone_) return 0.0f;
  float s = (v > 0.0f) ? 1.0f : -1.0f;
  return s * (std::fabs(v) - static_cast<float>(deadzone_)) /
         (1.0f - static_cast<float>(deadzone_));
}

float Extreme3DProDriver::normalise_axis(int16_t raw, int16_t mn, int16_t mx)
{
  float c = (mx + mn) / 2.0f, r = (mx - mn) / 2.0f;
  if (r == 0.0f) return 0.0f;
  return std::max(-1.0f, std::min(1.0f, (static_cast<float>(raw) - c) / r));
}

}  // namespace extreme3dpro_driver
