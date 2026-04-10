#include "extreme3dpro_driver/extreme3dpro_driver.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cmath>
#include <chrono>

using namespace std::chrono_literals;

namespace extreme3dpro_driver
{

// ══════════════════════════════════════════════════════════════
//  Construction / Destruction
// ══════════════════════════════════════════════════════════════

Extreme3DProDriver::Extreme3DProDriver(const rclcpp::NodeOptions & options)
: Node("extreme3dpro_driver", options),
  fd_(-1),
  fresh_(false),
  running_(false)
{
  declare_parameters();
  load_parameters();

  // ── Publishers ──────────────────────────────────────────────
  joy_pub_    = this->create_publisher<sensor_msgs::msg::Joy>("joy", 10);
  deadman_pub_= this->create_publisher<std_msgs::msg::Bool>("deadman", 10);

  if (publish_twist_) {
    twist_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
  }

  // ── Open device ────────────────────────────────────────────
  if (!open_device()) {
    RCLCPP_FATAL(get_logger(),
      "Cannot open joystick device '%s'. Is the Extreme 3D Pro connected?",
      device_path_.c_str());
    rclcpp::shutdown();
    return;
  }

  // ── Background read thread ─────────────────────────────────
  running_ = true;
  read_thread_ = std::thread(&Extreme3DProDriver::read_loop, this);

  // ── Timer for publishing ───────────────────────────────────
  auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  timer_ = this->create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&Extreme3DProDriver::timer_callback, this));

  RCLCPP_INFO(get_logger(),
    "Extreme 3D Pro driver started — device: %s  rate: %.0f Hz",
    device_path_.c_str(), publish_rate_);
}

Extreme3DProDriver::~Extreme3DProDriver()
{
  running_ = false;
  if (read_thread_.joinable()) {
    read_thread_.join();
  }
  close_device();
}

// ══════════════════════════════════════════════════════════════
//  Parameters
// ══════════════════════════════════════════════════════════════

void Extreme3DProDriver::declare_parameters()
{
  this->declare_parameter<std::string>("device",          "/dev/input/js0");
  this->declare_parameter<double>     ("publish_rate",    50.0);
  this->declare_parameter<double>     ("deadzone",        0.05);
  this->declare_parameter<bool>       ("autorepeat",      true);
  this->declare_parameter<bool>       ("publish_twist",   true);
  this->declare_parameter<double>     ("linear_scale",    1.0);
  this->declare_parameter<double>     ("angular_scale",   1.0);

  // Axis mapping (Logitech Extreme 3D Pro defaults)
  this->declare_parameter<int>("axis_x",        0);   // roll
  this->declare_parameter<int>("axis_y",        1);   // pitch
  this->declare_parameter<int>("axis_yaw",      2);   // twist
  this->declare_parameter<int>("axis_throttle", 3);   // slider
  this->declare_parameter<int>("deadman_btn",   0);   // trigger
}

void Extreme3DProDriver::load_parameters()
{
  device_path_    = this->get_parameter("device").as_string();
  publish_rate_   = this->get_parameter("publish_rate").as_double();
  deadzone_       = this->get_parameter("deadzone").as_double();
  autorepeat_     = this->get_parameter("autorepeat").as_bool();
  publish_twist_  = this->get_parameter("publish_twist").as_bool();
  linear_scale_   = this->get_parameter("linear_scale").as_double();
  angular_scale_  = this->get_parameter("angular_scale").as_double();

  axis_x_         = this->get_parameter("axis_x").as_int();
  axis_y_         = this->get_parameter("axis_y").as_int();
  axis_yaw_       = this->get_parameter("axis_yaw").as_int();
  axis_throttle_  = this->get_parameter("axis_throttle").as_int();
  deadman_btn_    = this->get_parameter("deadman_btn").as_int();
}

// ══════════════════════════════════════════════════════════════
//  Device I/O
// ══════════════════════════════════════════════════════════════

bool Extreme3DProDriver::open_device()
{
  fd_ = open(device_path_.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd_ < 0) {
    RCLCPP_ERROR(get_logger(), "open(%s): %s", device_path_.c_str(), strerror(errno));
    return false;
  }

  // Query number of axes & buttons
  uint8_t num_axes = 0, num_buttons = 0;
  ioctl(fd_, JSIOCGAXES,    &num_axes);
  ioctl(fd_, JSIOCGBUTTONS, &num_buttons);

  axes_.resize(num_axes, 0.0f);
  buttons_.resize(num_buttons, 0);

  // Read device name for logging
  char name[128] = "Unknown";
  ioctl(fd_, JSIOCGNAME(sizeof(name)), name);
  RCLCPP_INFO(get_logger(), "Joystick: %s  (%d axes, %d buttons)", name, num_axes, num_buttons);

  return true;
}

void Extreme3DProDriver::close_device()
{
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

// ══════════════════════════════════════════════════════════════
//  Read loop (background thread)
// ══════════════════════════════════════════════════════════════

void Extreme3DProDriver::read_loop()
{
  struct js_event event {};

  while (running_) {
    ssize_t bytes = read(fd_, &event, sizeof(event));

    if (bytes == sizeof(event)) {
      // Strip init flag
      event.type &= ~JS_EVENT_INIT;

      if (event.type == JS_EVENT_AXIS) {
        if (event.number < axes_.size()) {
          // Extreme 3D Pro axes: X/Y ±1023, Rz ±127, Throttle 0‑255, Hat ±1
          int16_t min_val = -32767;
          int16_t max_val =  32767;
          axes_[event.number] = normalise_axis(event.value, min_val, max_val);
          fresh_ = true;
        }
      } else if (event.type == JS_EVENT_BUTTON) {
        if (event.number < buttons_.size()) {
          buttons_[event.number] = event.value;
          fresh_ = true;
        }
      }
    } else if (bytes < 0 && errno != EAGAIN) {
      RCLCPP_ERROR(get_logger(), "Read error: %s", strerror(errno));
      running_ = false;
      break;
    } else {
      // No data available — sleep briefly to avoid spinning
      std::this_thread::sleep_for(1ms);
    }
  }
}

// ══════════════════════════════════════════════════════════════
//  Timer callback — publish at fixed rate
// ══════════════════════════════════════════════════════════════

void Extreme3DProDriver::timer_callback()
{
  if (!fresh_ && !autorepeat_) {
    return;
  }

  publish_joy();

  if (publish_twist_) {
    publish_twist();
  }

  // Publish deadman switch state
  std_msgs::msg::Bool deadman_msg;
  deadman_msg.data = (deadman_btn_ >= 0 &&
                      deadman_btn_ < static_cast<int>(buttons_.size()) &&
                      buttons_[deadman_btn_] != 0);
  deadman_pub_->publish(deadman_msg);

  fresh_ = false;
}

// ══════════════════════════════════════════════════════════════
//  Publishers
// ══════════════════════════════════════════════════════════════

void Extreme3DProDriver::publish_joy()
{
  sensor_msgs::msg::Joy msg;
  msg.header.stamp    = this->now();
  msg.header.frame_id = "extreme3dpro";

  // Copy axes with deadzone applied
  msg.axes.resize(axes_.size());
  for (size_t i = 0; i < axes_.size(); ++i) {
    msg.axes[i] = apply_deadzone(axes_[i]);
  }

  // Copy buttons
  msg.buttons.resize(buttons_.size());
  for (size_t i = 0; i < buttons_.size(); ++i) {
    msg.buttons[i] = buttons_[i];
  }

  joy_pub_->publish(msg);
}

void Extreme3DProDriver::publish_twist()
{
  // Only publish twist when deadman (trigger) is held
  bool deadman_active = (deadman_btn_ >= 0 &&
                         deadman_btn_ < static_cast<int>(buttons_.size()) &&
                         buttons_[deadman_btn_] != 0);

  geometry_msgs::msg::Twist twist;

  if (deadman_active) {
    // axis_y → forward/backward (linear.x)
    // axis_x → strafe left/right (linear.y)
    // axis_throttle → up/down    (linear.z)
    // axis_yaw → rotation        (angular.z)

    auto safe_axis = [&](int idx) -> float {
      if (idx >= 0 && idx < static_cast<int>(axes_.size())) {
        return apply_deadzone(axes_[idx]);
      }
      return 0.0f;
    };

    twist.linear.x  = -safe_axis(axis_y_)        * linear_scale_;   // push fwd = +x
    twist.linear.y  =  safe_axis(axis_x_)         * linear_scale_;   // left = +y
    twist.linear.z  =  safe_axis(axis_throttle_)  * linear_scale_;   // throttle
    twist.angular.z = -safe_axis(axis_yaw_)       * angular_scale_;  // CCW = +z
  }
  // else: all zeros (stop)

  twist_pub_->publish(twist);
}

// ══════════════════════════════════════════════════════════════
//  Helpers
// ══════════════════════════════════════════════════════════════

float Extreme3DProDriver::apply_deadzone(float value) const
{
  if (std::fabs(value) < deadzone_) {
    return 0.0f;
  }
  // Re‑scale so that the output starts at 0 just outside the deadzone
  float sign = (value > 0.0f) ? 1.0f : -1.0f;
  return sign * (std::fabs(value) - static_cast<float>(deadzone_)) /
         (1.0f - static_cast<float>(deadzone_));
}

float Extreme3DProDriver::normalise_axis(int16_t raw, int16_t min_val, int16_t max_val)
{
  float centre = (max_val + min_val) / 2.0f;
  float range  = (max_val - min_val) / 2.0f;
  if (range == 0.0f) return 0.0f;
  float normed = (static_cast<float>(raw) - centre) / range;
  return std::clamp(normed, -1.0f, 1.0f);
}

}  // namespace extreme3dpro_driver
