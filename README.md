# extreme3dpro_driver

[![Foxy Build](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/foxy-build-ci.yaml/badge.svg)](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/foxy-build-ci.yaml)
[![Humble Build](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/humble-build-ci.yaml/badge.svg)](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/humble-build-ci.yaml)
[![Iron Build](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/iron-build-ci.yaml/badge.svg)](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/iron-build-ci.yaml)
[![Jazzy Build](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/jazzy-build-ci.yaml/badge.svg)](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/jazzy-build-ci.yaml)
[![Rolling Build](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/rolling-build-ci.yaml/badge.svg)](https://github.com/YOUR_USERNAME/extreme3dpro_driver/actions/workflows/rolling-build-ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

ROS 2 driver for the **Logitech Extreme 3D Pro** joystick. Reads raw Linux joystick events and publishes standard ROS messages.

> Looking for ROS 1 Noetic? Switch to the [`ros1` branch](https://github.com/YOUR_USERNAME/extreme3dpro_driver/tree/ros1).

## Branch Strategy

| Branch | ROS Distros                            | CI                    |
|--------|----------------------------------------|-----------------------|
| `main` | Foxy · Humble · Iron · Jazzy · Rolling | 5 parallel CI jobs    |
| `ros1` | Noetic                                 | 1 CI job              |

## Topics

| Topic      | Type                        | Description                                |
|------------|-----------------------------|--------------------------------------------|
| `/joy`     | `sensor_msgs/msg/Joy`       | All axes (normalised −1…+1) and buttons    |
| `/cmd_vel` | `geometry_msgs/msg/Twist`   | Velocity command (deadman trigger required) |
| `/deadman` | `std_msgs/msg/Bool`         | Deadman switch state                       |

## Axis & Button Map

```
Axes:                                    Buttons:
  0  X        (roll  — left/right)         0  Trigger    (index finger)
  1  Y        (pitch — fwd/back)           1  Thumb      (top of stick)
  2  Rz       (twist — yaw)              2-5  Top buttons (3-6)
  3  Throttle (slider at base)          6-11  Base buttons (7-12)
  4  Hat X    (−1, 0, +1)
  5  Hat Y    (−1, 0, +1)
```

---

## Install

### Option A — apt (after bloom release)

```bash
sudo apt install ros-${ROS_DISTRO}-extreme3dpro-driver
```

### Option B — From source

```bash
cd ~/ros2_ws/src
git clone https://github.com/YOUR_USERNAME/extreme3dpro_driver.git
cd ~/ros2_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select extreme3dpro_driver
source install/setup.bash
```

## Usage

```bash
# Launch with config
ros2 launch extreme3dpro_driver extreme3dpro.launch.py

# Or run directly
ros2 run extreme3dpro_driver extreme3dpro_node

# Verify
ros2 topic echo /joy
ros2 topic echo /cmd_vel    # hold trigger!
ros2 topic hz /joy
```

## Parameters

| Parameter        | Type   | Default          | Description                    |
|------------------|--------|------------------|--------------------------------|
| `device`         | string | `/dev/input/js0` | Joystick device path           |
| `publish_rate`   | double | `50.0`           | Publish frequency (Hz)         |
| `deadzone`       | double | `0.05`           | Axis deadzone (0.0–1.0)       |
| `autorepeat`     | bool   | `true`           | Publish without new events     |
| `publish_twist`  | bool   | `true`           | Also publish cmd_vel           |
| `linear_scale`   | double | `1.0`            | m/s at full deflection         |
| `angular_scale`  | double | `1.0`            | rad/s at full yaw              |
| `axis_x`         | int    | `0`              | Roll axis index                |
| `axis_y`         | int    | `1`              | Pitch axis index               |
| `axis_yaw`       | int    | `2`              | Twist/yaw axis index           |
| `axis_throttle`  | int    | `3`              | Throttle slider index          |
| `deadman_btn`    | int    | `0`              | Deadman button (−1 to disable) |

---

## Permissions

```bash
# Add user to input group
sudo usermod -aG input $USER
# Log out & back in

# Or use a udev rule:
# /etc/udev/rules.d/99-extreme3dpro.rules
SUBSYSTEM=="input", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c215", MODE="0666"
```

---

## Publishing to ROS Package Repository (bloom)

Supaya package bisa di-install via `apt`, kamu perlu release lewat **bloom** ke ROS build farm.

### 1. Install bloom

```bash
sudo apt install python3-bloom python3-catkin-pkg fakeroot dpkg-dev debhelper
pip3 install -U bloom
```

### 2. Tag release

```bash
git tag 1.0.0
git push origin 1.0.0
```

### 3. Bloom release

```bash
# Ini akan buat release repo di GitHub kamu
bloom-release --rosdistro humble --track humble \
  --repo https://github.com/YOUR_USERNAME/extreme3dpro_driver.git \
  extreme3dpro_driver

# Untuk semua distro sekaligus:
for distro in foxy humble iron jazzy rolling; do
  bloom-release --rosdistro $distro extreme3dpro_driver --track $distro
done
```

### 4. Submit ke rosdistro

1. Fork [ros/rosdistro](https://github.com/ros/rosdistro)
2. Edit `humble/distribution.yaml` (dan distro lainnya):

```yaml
extreme3dpro_driver:
  doc:
    type: git
    url: https://github.com/YOUR_USERNAME/extreme3dpro_driver.git
    version: main
  release:
    packages: [extreme3dpro_driver]
    tags:
      release: release/humble/{package}/{version}
    url: https://github.com/YOUR_USERNAME/extreme3dpro_driver-release.git
    version: 1.0.0-1
  source:
    type: git
    url: https://github.com/YOUR_USERNAME/extreme3dpro_driver.git
    version: main
  status: developed
```

3. Buka **Pull Request** ke `ros/rosdistro`
4. Setelah di-merge, build farm otomatis build `.deb` dan publish ke `packages.ros.org`
5. 1–2 hari kemudian, user bisa `sudo apt install ros-${ROS_DISTRO}-extreme3dpro-driver`

---

## Git Setup (Quick Start)

```bash
# Create repo
mkdir extreme3dpro_driver && cd extreme3dpro_driver
git init

# ── main branch (ROS 2) ──────────────────────────────────────
# Copy isi main_branch/ ke sini
git add -A
git commit -m "feat: extreme3dpro_driver for ROS 2 (Foxy–Rolling)"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/extreme3dpro_driver.git
git push -u origin main

# ── ros1 branch (Noetic) ─────────────────────────────────────
git checkout --orphan ros1
git rm -rf .
# Copy isi ros1_branch/ ke sini
git add -A
git commit -m "feat: extreme3dpro_driver for ROS 1 Noetic"
git push -u origin ros1
```

---

## Repository Structure

```
main branch (ROS 2):               ros1 branch (Noetic):
├── .github/workflows/             ├── .github/workflows/
│   ├── foxy-build-ci.yaml         │   └── noetic-build-ci.yaml
│   ├── humble-build-ci.yaml       ├── CMakeLists.txt          (catkin)
│   ├── iron-build-ci.yaml         ├── package.xml             (format 2)
│   ├── jazzy-build-ci.yaml        ├── include/.../driver.h
│   └── rolling-build-ci.yaml      ├── src/
├── CMakeLists.txt    (ament)      ├── launch/extreme3dpro.launch
├── package.xml       (format 3)   ├── config/extreme3dpro.yaml
├── include/.../driver.hpp         └── README.md
├── src/
├── launch/extreme3dpro.launch.py
├── config/extreme3dpro.yaml
├── LICENSE
└── README.md
```

## License

MIT — see [LICENSE](LICENSE).
