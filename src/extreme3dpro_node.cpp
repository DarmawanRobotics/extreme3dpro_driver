#include "extreme3dpro_driver/extreme3dpro_driver.h"
#include <ros/ros.h>

int main(int argc, char ** argv)
{
  ros::init(argc, argv, "extreme3dpro_driver");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
  extreme3dpro_driver::Extreme3DProDriver driver(nh, pnh);
  ros::spin();
  return 0;
}
