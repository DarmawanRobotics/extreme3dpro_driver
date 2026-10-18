// ROS 1 node.
#include <geometry_msgs/Twist.h>
#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <std_msgs/Bool.h>

#include <stdexcept>
#include <string>

#include "extreme3dpro_driver/joystick.hpp"
#include "extreme3dpro_driver/mapping.hpp"

using extreme3dpro::Joystick;
using extreme3dpro::State;
using extreme3dpro::TwistConfig;

int main(int argc, char ** argv)
{
  ros::init(argc, argv, "extreme3dpro_driver");
  ros::NodeHandle nh, pnh("~");

  std::string device = pnh.param<std::string>("device", "/dev/input/js0");
  std::string frame_id = pnh.param<std::string>("frame_id", "extreme3dpro");
  double rate = pnh.param("publish_rate", 50.0);
  double reconnect = pnh.param("reconnect_sec", 1.0);
  bool autorepeat = pnh.param("autorepeat", true);
  bool publish_twist = pnh.param("publish_twist", true);

  TwistConfig config;
  pnh.param("axis_x", config.axis_x, config.axis_x);
  pnh.param("axis_y", config.axis_y, config.axis_y);
  pnh.param("axis_yaw", config.axis_yaw, config.axis_yaw);
  pnh.param("axis_throttle", config.axis_throttle, config.axis_throttle);
  pnh.param("deadman_btn", config.deadman_button, config.deadman_button);
  pnh.param("deadzone", config.deadzone, config.deadzone);
  pnh.param("linear_scale", config.linear_scale, config.linear_scale);
  pnh.param("angular_scale", config.angular_scale, config.angular_scale);
  try {
    config.throttle = extreme3dpro::parse_throttle_mode(
      pnh.param<std::string>("throttle_mode", "scale"));
  } catch (const std::invalid_argument & e) {
    ROS_FATAL("%s", e.what());
    return 1;
  }
  if (rate <= 0.0) {
    ROS_FATAL("publish_rate must be > 0");
    return 1;
  }

  auto joy_pub = nh.advertise<sensor_msgs::Joy>("joy", 10);
  auto deadman_pub = nh.advertise<std_msgs::Bool>("deadman", 10);
  ros::Publisher twist_pub;
  if (publish_twist) {
    twist_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 10);
  }

  Joystick joystick(device, [](bool error, const std::string & text) {
      if (error) {
        ROS_WARN("%s", text.c_str());
      } else {
        ROS_INFO("%s", text.c_str());
      }
    });
  joystick.start(reconnect);

  ros::Timer timer = nh.createTimer(ros::Duration(1.0 / rate), [&](const ros::TimerEvent &) {
      bool fresh = joystick.take_fresh();
      if (!fresh && !autorepeat) {
        return;
      }
      State s = joystick.state();
      if (s.connected) {
        sensor_msgs::Joy joy;
        joy.header.stamp = ros::Time::now();
        joy.header.frame_id = frame_id;
        for (size_t i = 0; i < s.axes.size(); ++i) {
          joy.axes.push_back(extreme3dpro::axis(s, static_cast<int>(i), config.deadzone));
        }
        joy.buttons = s.buttons;
        joy_pub.publish(joy);
      }
      std_msgs::Bool deadman;
      deadman.data = extreme3dpro::deadman_held(s, config);
      deadman_pub.publish(deadman);
      if (publish_twist) {
        auto t = extreme3dpro::compute_twist(s, config);
        geometry_msgs::Twist msg;
        msg.linear.x = t.linear_x;
        msg.linear.y = t.linear_y;
        msg.linear.z = t.linear_z;
        msg.angular.z = t.angular_z;
        twist_pub.publish(msg);
      }
    });

  ROS_INFO("extreme3dpro_driver: %s at %.0f Hz", device.c_str(), rate);
  ros::spin();
  joystick.stop();
  return 0;
}
