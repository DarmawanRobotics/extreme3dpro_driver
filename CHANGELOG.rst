^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package extreme3dpro_driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.2.0
-----
* One package for ROS 1 Noetic and ROS 2 (Foxy, Humble, Jazzy).
* Joystick reader and mapping without ROS, thread-safe, reconnects after an unplug and releases
  every button and axis when the device is lost.
* ``cmd_vel``: left is +y (was inverted), slider scales the output (``throttle_mode``).
* One config file for both ROS versions, udev rule, unit tests, single CI workflow.

0.1.0
-----
* First ROS 2 version.
