// Linux joystick reader (no ROS).
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace extreme3dpro
{

struct State
{
  bool connected = false;
  std::vector<float> axes;       // -1..1
  std::vector<int32_t> buttons;  // 0 / 1
};

class Joystick
{
public:
  using Logger = std::function<void(bool error, const std::string & text)>;

  explicit Joystick(std::string device, Logger logger = nullptr);
  ~Joystick();

  Joystick(const Joystick &) = delete;
  Joystick & operator=(const Joystick &) = delete;

  // Single-threaded use.
  bool open();
  void close();
  bool is_open() const { return fd_ >= 0; }
  // Reads every pending event, waiting up to timeout_ms for the first.
  // Returns false when the device is gone (it is closed then).
  bool poll(int timeout_ms);

  // Background reader: opens, reads and reopens every reconnect_sec after an unplug.
  void start(double reconnect_sec = 1.0);
  void stop();

  // Thread-safe.
  State state() const;
  bool take_fresh();  // true once after any change
  std::string name() const;

private:
  void handle(uint8_t type, uint8_t number, int16_t value);
  void log(bool error, const std::string & text) const;
  void run(double reconnect_sec);

  std::string device_;
  Logger logger_;
  int fd_ = -1;
  std::string name_;
  State state_;
  bool fresh_ = false;
  mutable std::mutex mutex_;
  std::atomic<bool> running_{false};
  std::thread thread_;
};

// Raw evdev joystick value (-32767..32767) to -1..1.
float normalize(int16_t raw);

}  // namespace extreme3dpro
