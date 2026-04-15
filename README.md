# extreme3dpro_driver

ROS 2 Foxy C++ driver for the **Logitech Extreme 3D Pro** joystick.

Reads raw Linux joystick events (`/dev/input/jsX`) and publishes:

| Topic      | Type                        | Description                              |
|------------|-----------------------------|------------------------------------------|
| `/joy`     | `sensor_msgs/msg/Joy`       | All axes (normalised −1…+1) and buttons  |
| `/cmd_vel` | `geometry_msgs/msg/Twist`   | Velocity command (requires deadman held) |
| `/deadman` | `std_msgs/msg/Bool`         | Deadman switch state                     |

---

## Axis & Button Map (Extreme 3D Pro)

```
Axes:
  0  X       (roll  — left / right)
  1  Y       (pitch — forward / back)
  2  Rz      (twist — yaw)
  3  Throttle (slider at base)
  4  Hat X   (−1, 0, +1)
  5  Hat Y   (−1, 0, +1)

Buttons:
  0  Trigger        (index finger)
  1  Thumb button   (top of stick)
  2  Button 3
  3  Button 4
  4  Button 5
  5  Button 6
  6–11  Base buttons (7‑12)
```

---

## Build

```bash
# In your ROS 2 workspace
cd ~/ros2_ws/src
cp -r /path/to/extreme3dpro_driver .
cd ~/ros2_ws
colcon build --packages-select extreme3dpro_driver
source install/setup.bash
```

## Run

```bash
# With launch file (loads config/extreme3dpro.yaml)
ros2 launch extreme3dpro_driver extreme3dpro.launch.py

# Or directly
ros2 run extreme3dpro_driver extreme3dpro_node \
  --ros-args --params-file config/extreme3dpro.yaml
```

## Verify

```bash
# Check joy topic
ros2 topic echo /joy

# Check twist output (hold trigger to enable)
ros2 topic echo /cmd_vel

# Monitor at 10 Hz
ros2 topic hz /joy
```

---

## Parameters

| Parameter        | Type   | Default            | Description                                    |
|------------------|--------|--------------------|------------------------------------------------|
| `device`         | string | `/dev/input/js0`   | Linux joystick device path                     |
| `publish_rate`   | double | `50.0`             | Publish frequency (Hz)                         |
| `deadzone`       | double | `0.05`             | Axis deadzone (0.0–1.0)                        |
| `autorepeat`     | bool   | `true`             | Publish even when no new events                |
| `publish_twist`  | bool   | `true`             | Also publish `cmd_vel`                         |
| `linear_scale`   | double | `1.0`              | m/s at full axis deflection                    |
| `angular_scale`  | double | `1.0`              | rad/s at full yaw deflection                   |
| `axis_x`         | int    | `0`                | Axis index for roll (strafe)                   |
| `axis_y`         | int    | `1`                | Axis index for pitch (forward/back)            |
| `axis_yaw`       | int    | `2`                | Axis index for twist (yaw)                     |
| `axis_throttle`  | int    | `3`                | Axis index for throttle                        |
| `deadman_btn`    | int    | `0`                | Button for deadman switch (−1 to disable)      |

---

## Permissions

Make sure your user can read the joystick device:

```bash
sudo usermod -aG input $USER
# Log out and back in, then verify:
ls -la /dev/input/js0
```