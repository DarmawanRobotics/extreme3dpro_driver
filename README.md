<div align="center">

# extreme3dpro_driver

**ROS 1 and ROS 2 driver for the Logitech Extreme 3D Pro joystick — `Joy`, `cmd_vel` with a
deadman trigger and throttle scaling, and it reconnects after an unplug.**

[![CI](https://github.com/DarmawanRobotics/extreme3dpro_driver/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/DarmawanRobotics/extreme3dpro_driver/actions/workflows/ci.yml)
[![ROS 1](https://img.shields.io/badge/ROS-Noetic-22314E?logo=ros&logoColor=white)](https://wiki.ros.org/noetic)
[![ROS 2](https://img.shields.io/badge/ROS%202-Foxy%20%7C%20Humble%20%7C%20Jazzy-22314E?logo=ros&logoColor=white)](https://docs.ros.org)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
[![License](https://img.shields.io/badge/license-Apache%202.0-blue)](LICENSE)

</div>

---

One package for both ROS versions: `catkin_make` or `colcon build` picks the right one from the
sourced environment. The joystick reader (Linux `/dev/input/js*`) and the stick-to-velocity
mapping have no ROS dependency; the ROS 1 and ROS 2 nodes are thin wrappers around them.

## Quick start

```bash
# ROS 2 (Foxy, Humble, Jazzy)
cd ~/ros2_ws/src && git clone https://github.com/DarmawanRobotics/extreme3dpro_driver.git
cd ~/ros2_ws && rosdep install --from-paths src --ignore-src -y
colcon build --packages-select extreme3dpro_driver && source install/setup.bash
ros2 launch extreme3dpro_driver extreme3dpro.launch.py

# ROS 1 (Noetic)
cd ~/catkin_ws/src && git clone https://github.com/DarmawanRobotics/extreme3dpro_driver.git
cd ~/catkin_ws && rosdep install --from-paths src --ignore-src -y
catkin_make && source devel/setup.bash
roslaunch extreme3dpro_driver extreme3dpro.launch
```

Another device: `device:=/dev/input/js1` (both launch files), or the fixed name from the udev
rule below.

## Permissions

```bash
sudo usermod -aG input $USER          # then log out and in
# or the udev rule: everyone can read it, and it appears as /dev/input/extreme3dpro
sudo cp udev/99-extreme3dpro.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger
```

## Interface

| Topic | Type | |
|---|---|---|
| `joy` | `sensor_msgs/Joy` | 6 axes (−1..1, deadzone applied) and 12 buttons, while connected |
| `cmd_vel` | `geometry_msgs/Twist` | zero unless the trigger is held; zero when the joystick is unplugged |
| `deadman` | `std_msgs/Bool` | trigger held |

| Joystick | `cmd_vel` |
|---|---|
| stick forward / back | `linear.x` |
| stick left / right | `linear.y` (left +) |
| stick twist | `angular.z` (left +) |
| slider | scales everything, back 0 → forward 1 (`throttle_mode: scale`) |
| trigger (button 0) | deadman |

```
Axes: 0 X · 1 Y · 2 twist · 3 slider · 4 hat X · 5 hat Y
Buttons: 0 trigger · 1 thumb · 2-5 stick top (3-6) · 6-11 base (7-12)
```

## Parameters

All in [extreme3dpro.yaml](config/extreme3dpro.yaml), the same file for ROS 1 and ROS 2.

| Parameter | Default | |
|---|---|---|
| `device` | `/dev/input/js0` | |
| `publish_rate` | `50.0` | Hz |
| `reconnect_sec` | `1.0` | retry period while unplugged |
| `autorepeat` | `true` | publish every period, not only on change |
| `deadzone` | `0.05` | 0..1 |
| `publish_twist` | `true` | |
| `linear_scale`, `angular_scale` | `1.0`, `1.0` | m/s, rad/s at full deflection |
| `throttle_mode` | `scale` | `scale`, `linear_z`, `none` |
| `axis_x`, `axis_y`, `axis_yaw`, `axis_throttle` | `0`, `1`, `2`, `3` | |
| `deadman_btn` | `0` | `-1`: no deadman (careful) |

## Check

```bash
jstest /dev/input/js0                  # joystick package: raw values
ros2 topic echo /joy                   # rostopic echo /joy on ROS 1
ros2 topic echo /cmd_vel               # hold the trigger, slider forward
```

Unplug the joystick while holding the trigger: `cmd_vel` drops to zero, the node logs
`lost, waiting for it to come back` and continues when it is plugged in again.

## Develop

```bash
colcon test --packages-select extreme3dpro_driver && colcon test-result --verbose
catkin_make run_tests && catkin_test_results build/test_results     # ROS 1
```

The tests cover the mapping and the reader against a FIFO standing in for the device (events,
unplug, reconnect). CI builds and tests on Noetic, Foxy, Humble and Jazzy.

Released under the [Apache License 2.0](LICENSE).
