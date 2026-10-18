#include "extreme3dpro_driver/joystick.hpp"

#include <fcntl.h>
#include <linux/joystick.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>

namespace extreme3dpro
{

float normalize(int16_t raw)
{
  return std::clamp(static_cast<float>(raw) / 32767.0f, -1.0f, 1.0f);
}

Joystick::Joystick(std::string device, Logger logger)
: device_(std::move(device)), logger_(std::move(logger))
{
}

Joystick::~Joystick()
{
  stop();
  close();
}

void Joystick::log(bool error, const std::string & text) const
{
  if (logger_) {
    logger_(error, text);
  }
}

bool Joystick::open()
{
  close();
  int fd = ::open(device_.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return false;
  }
  uint8_t axes = 0, buttons = 0;
  char name[128] = "";
  bool is_joystick = ioctl(fd, JSIOCGAXES, &axes) == 0;
  ioctl(fd, JSIOCGBUTTONS, &buttons);
  ioctl(fd, JSIOCGNAME(sizeof(name)), name);

  std::lock_guard<std::mutex> lock(mutex_);
  fd_ = fd;
  name_ = name[0] ? name : (is_joystick ? "unknown joystick" : device_);
  state_.connected = true;
  state_.axes.assign(axes, 0.0f);
  state_.buttons.assign(buttons, 0);
  fresh_ = true;
  return true;
}

void Joystick::close()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  // Everything released: a lost joystick never keeps a button or an axis held.
  bool was_connected = state_.connected;
  state_.connected = false;
  std::fill(state_.axes.begin(), state_.axes.end(), 0.0f);
  std::fill(state_.buttons.begin(), state_.buttons.end(), 0);
  fresh_ = fresh_ || was_connected;
}

void Joystick::handle(uint8_t type, uint8_t number, int16_t value)
{
  type &= static_cast<uint8_t>(~JS_EVENT_INIT);
  std::lock_guard<std::mutex> lock(mutex_);
  if (type == JS_EVENT_AXIS) {
    if (number >= state_.axes.size()) {
      state_.axes.resize(number + 1, 0.0f);
    }
    state_.axes[number] = normalize(value);
    fresh_ = true;
  } else if (type == JS_EVENT_BUTTON) {
    if (number >= state_.buttons.size()) {
      state_.buttons.resize(number + 1, 0);
    }
    state_.buttons[number] = value ? 1 : 0;
    fresh_ = true;
  }
}

bool Joystick::poll(int timeout_ms)
{
  if (fd_ < 0) {
    return false;
  }
  pollfd pfd{fd_, POLLIN, 0};
  int n = ::poll(&pfd, 1, timeout_ms);
  if (n < 0) {
    return errno == EINTR;
  }
  if (n == 0) {
    return true;
  }
  if (pfd.revents & (POLLERR | POLLNVAL)) {
    close();
    return false;
  }
  js_event events[32];
  while (true) {
    ssize_t bytes = ::read(fd_, events, sizeof(events));
    if (bytes > 0) {
      for (ssize_t i = 0; i < bytes / static_cast<ssize_t>(sizeof(js_event)); ++i) {
        handle(events[i].type, events[i].number, events[i].value);
      }
      continue;
    }
    if (bytes < 0 && (errno == EAGAIN || errno == EINTR)) {
      return true;
    }
    // ENODEV after an unplug, or end of file.
    close();
    return false;
  }
}

void Joystick::start(double reconnect_sec)
{
  stop();
  running_ = true;
  thread_ = std::thread(&Joystick::run, this, reconnect_sec);
}

void Joystick::stop()
{
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Joystick::run(double reconnect_sec)
{
  using clock = std::chrono::steady_clock;
  bool warned = false;
  auto next_try = clock::now();
  while (running_) {
    if (!is_open()) {
      if (clock::now() < next_try) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }
      if (open()) {
        auto s = state();
        log(false, "opened " + device_ + ": " + name() + " (" + std::to_string(s.axes.size()) +
          " axes, " + std::to_string(s.buttons.size()) + " buttons)");
        warned = false;
      } else {
        if (!warned) {
          log(true, "cannot open " + device_ + ": " + std::strerror(errno) +
            " (retrying every " + std::to_string(reconnect_sec).substr(0, 3) + " s)");
          warned = true;
        }
        next_try = clock::now() + std::chrono::duration_cast<clock::duration>(
          std::chrono::duration<double>(reconnect_sec));
        continue;
      }
    }
    if (!poll(100)) {
      log(true, device_ + " lost, waiting for it to come back");
      warned = true;
      next_try = clock::now() + std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(reconnect_sec));
    }
  }
}

State Joystick::state() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

bool Joystick::take_fresh()
{
  std::lock_guard<std::mutex> lock(mutex_);
  bool f = fresh_;
  fresh_ = false;
  return f;
}

std::string Joystick::name() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return name_;
}

}  // namespace extreme3dpro
