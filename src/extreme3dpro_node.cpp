#include "extreme3dpro_driver/extreme3dpro_driver.hpp"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<extreme3dpro_driver::Extreme3DProDriver>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
