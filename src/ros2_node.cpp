// ROS 2 node.
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/bool.hpp>

#include "extreme3dpro_driver/joystick.hpp"
#include "extreme3dpro_driver/mapping.hpp"

namespace extreme3dpro
{

class Extreme3DProNode : public rclcpp::Node
{
public:
  explicit Extreme3DProNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("extreme3dpro_driver", options)
  {
    device_ = declare_parameter<std::string>("device", "/dev/input/js0");
    frame_id_ = declare_parameter<std::string>("frame_id", "extreme3dpro");
    double rate = declare_parameter<double>("publish_rate", 50.0);
    double reconnect = declare_parameter<double>("reconnect_sec", 1.0);
    autorepeat_ = declare_parameter<bool>("autorepeat", true);
    publish_twist_ = declare_parameter<bool>("publish_twist", true);

    config_.axis_x = declare_parameter<int>("axis_x", config_.axis_x);
    config_.axis_y = declare_parameter<int>("axis_y", config_.axis_y);
    config_.axis_yaw = declare_parameter<int>("axis_yaw", config_.axis_yaw);
    config_.axis_throttle = declare_parameter<int>("axis_throttle", config_.axis_throttle);
    config_.deadman_button = declare_parameter<int>("deadman_btn", config_.deadman_button);
    config_.deadzone = declare_parameter<double>("deadzone", config_.deadzone);
    config_.linear_scale = declare_parameter<double>("linear_scale", config_.linear_scale);
    config_.angular_scale = declare_parameter<double>("angular_scale", config_.angular_scale);
    config_.throttle = parse_throttle_mode(declare_parameter<std::string>("throttle_mode", "scale"));
    if (rate <= 0.0) {
      throw std::invalid_argument("publish_rate must be > 0");
    }

    joy_pub_ = create_publisher<sensor_msgs::msg::Joy>("joy", 10);
    deadman_pub_ = create_publisher<std_msgs::msg::Bool>("deadman", 10);
    if (publish_twist_) {
      twist_pub_ = create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    }

    joystick_ = std::make_unique<Joystick>(device_, [this](bool error, const std::string & text) {
        if (error) {
          RCLCPP_WARN(get_logger(), "%s", text.c_str());
        } else {
          RCLCPP_INFO(get_logger(), "%s", text.c_str());
        }
      });
    joystick_->start(reconnect);

    timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / rate)),
      [this] {tick();});
    RCLCPP_INFO(get_logger(), "%s at %.0f Hz", device_.c_str(), rate);
  }

  ~Extreme3DProNode() override
  {
    joystick_->stop();
  }

private:
  void tick()
  {
    bool fresh = joystick_->take_fresh();
    if (!fresh && !autorepeat_) {
      return;
    }
    State s = joystick_->state();
    if (s.connected) {
      sensor_msgs::msg::Joy joy;
      joy.header.stamp = now();
      joy.header.frame_id = frame_id_;
      for (size_t i = 0; i < s.axes.size(); ++i) {
        joy.axes.push_back(axis(s, static_cast<int>(i), config_.deadzone));
      }
      joy.buttons = s.buttons;
      joy_pub_->publish(joy);
    }
    std_msgs::msg::Bool deadman;
    deadman.data = deadman_held(s, config_);
    deadman_pub_->publish(deadman);
    if (publish_twist_) {
      auto t = compute_twist(s, config_);
      geometry_msgs::msg::Twist msg;
      msg.linear.x = t.linear_x;
      msg.linear.y = t.linear_y;
      msg.linear.z = t.linear_z;
      msg.angular.z = t.angular_z;
      twist_pub_->publish(msg);
    }
  }

  std::string device_, frame_id_;
  bool autorepeat_ = true, publish_twist_ = true;
  TwistConfig config_;
  std::unique_ptr<Joystick> joystick_;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr deadman_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr twist_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace extreme3dpro

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  int code = 0;
  try {
    rclcpp::spin(std::make_shared<extreme3dpro::Extreme3DProNode>());
  } catch (const std::exception & e) {
    RCLCPP_FATAL(rclcpp::get_logger("extreme3dpro_driver"), "%s", e.what());
    code = 1;
  }
  rclcpp::shutdown();
  return code;
}
