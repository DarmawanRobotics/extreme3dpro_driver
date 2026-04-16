# extreme3dpro_driver (ROS 1 Noetic)

[![Noetic Build](https://github.com/DarmawanRobotics/extreme3dpro_driver/actions/workflows/noetic-build-ci.yaml/badge.svg?branch=ros1)](https://github.com/DarmawanRobotics/extreme3dpro_driver/actions/workflows/noetic-build-ci.yaml)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

> **This is the `ros1` branch.** For ROS 2 (Foxy–Rolling), switch to [`main`](https://github.com/DarmawanRobotics/extreme3dpro_driver/tree/main).

ROS 1 Noetic driver for the **Logitech Extreme 3D Pro** joystick.

## Install

```bash
# From source
cd ~/catkin_ws/src
git clone -b ros1 https://github.com/DarmawanRobotics/extreme3dpro_driver.git
cd ~/catkin_ws
rosdep install --from-paths src --ignore-src -r -y
catkin_make
source devel/setup.bash

# From apt (after bloom release)
sudo apt install ros-noetic-extreme3dpro-driver
```

## Usage

```bash
roslaunch extreme3dpro_driver extreme3dpro.launch

# Verify
rostopic echo /joy
rostopic echo /cmd_vel   # hold trigger!
```

## Topics

| Topic      | Type                   | Description                                |
|------------|------------------------|--------------------------------------------|
| `/joy`     | `sensor_msgs/Joy`      | All axes (−1…+1) and buttons               |
| `/cmd_vel` | `geometry_msgs/Twist`  | Velocity command (deadman trigger required) |
| `/deadman` | `std_msgs/Bool`        | Deadman switch state                       |

## Parameters

| Parameter        | Default          | Description                    |
|------------------|------------------|--------------------------------|
| `device`         | `/dev/input/js0` | Joystick device path           |
| `publish_rate`   | `50.0`           | Hz                             |
| `deadzone`       | `0.05`           | 0.0–1.0                        |
| `autorepeat`     | `true`           | Publish without new events     |
| `publish_twist`  | `true`           | Enable cmd_vel output          |
| `linear_scale`   | `1.0`            | m/s at full deflection         |
| `angular_scale`  | `1.0`            | rad/s at full yaw              |
| `deadman_btn`    | `0`              | Trigger button (−1 to disable) |

