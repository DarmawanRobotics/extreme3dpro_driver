#include <fcntl.h>
#include <gtest/gtest.h>
#include <linux/joystick.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "extreme3dpro_driver/joystick.hpp"
#include "extreme3dpro_driver/mapping.hpp"

using namespace extreme3dpro;

namespace
{

State make_state(std::vector<float> axes, std::vector<int32_t> buttons)
{
  State s;
  s.connected = true;
  s.axes = std::move(axes);
  s.buttons = std::move(buttons);
  return s;
}

// A FIFO stands in for /dev/input/js0.
class FakeDevice
{
public:
  FakeDevice()
  {
    path_ = "/tmp/extreme3dpro_test_" + std::to_string(getpid()) + "_" + std::to_string(count_++);
    unlink(path_.c_str());
    mkfifo(path_.c_str(), 0600);
  }
  ~FakeDevice()
  {
    disconnect();
    unlink(path_.c_str());
  }
  void connect() {fd_ = open(path_.c_str(), O_WRONLY | O_NONBLOCK);}
  void disconnect()
  {
    if (fd_ >= 0) {
      close(fd_);
      fd_ = -1;
    }
  }
  void send(uint8_t type, uint8_t number, int16_t value)
  {
    js_event e{};
    e.type = type;
    e.number = number;
    e.value = value;
    ASSERT_EQ(write(fd_, &e, sizeof(e)), static_cast<ssize_t>(sizeof(e)));
  }
  const std::string & path() const {return path_;}

private:
  static int count_;
  std::string path_;
  int fd_ = -1;
};
int FakeDevice::count_ = 0;

}  // namespace

TEST(Mapping, Normalize)
{
  EXPECT_FLOAT_EQ(normalize(0), 0.0f);
  EXPECT_FLOAT_EQ(normalize(32767), 1.0f);
  EXPECT_FLOAT_EQ(normalize(-32767), -1.0f);
  EXPECT_FLOAT_EQ(normalize(-32768), -1.0f);
}

TEST(Mapping, Deadzone)
{
  EXPECT_FLOAT_EQ(apply_deadzone(0.04f, 0.05), 0.0f);
  EXPECT_FLOAT_EQ(apply_deadzone(-0.05f, 0.05), 0.0f);
  EXPECT_NEAR(apply_deadzone(0.525f, 0.05), 0.5f, 1e-6);
  EXPECT_FLOAT_EQ(apply_deadzone(1.0f, 0.05), 1.0f);
  EXPECT_FLOAT_EQ(apply_deadzone(-1.0f, 0.05), -1.0f);
  EXPECT_FLOAT_EQ(apply_deadzone(0.3f, 0.0), 0.3f);
  EXPECT_FLOAT_EQ(apply_deadzone(0.9f, 1.0), 0.0f);
}

TEST(Mapping, ThrottleMode)
{
  EXPECT_EQ(parse_throttle_mode("none"), ThrottleMode::None);
  EXPECT_EQ(parse_throttle_mode("scale"), ThrottleMode::Scale);
  EXPECT_EQ(parse_throttle_mode("linear_z"), ThrottleMode::LinearZ);
  EXPECT_THROW(parse_throttle_mode("fast"), std::invalid_argument);
}

TEST(Mapping, NoDeadmanNoMotion)
{
  TwistConfig c;
  auto t = compute_twist(make_state({-1, -1, 1, -1}, {0, 0}), c);
  EXPECT_EQ(t.linear_x, 0.0);
  EXPECT_EQ(t.angular_z, 0.0);
}

TEST(Mapping, Directions)
{
  TwistConfig c;
  c.deadzone = 0.0;
  c.throttle = ThrottleMode::None;
  c.linear_scale = 2.0;
  c.angular_scale = 3.0;
  // stick forward (y -1) and left (x -1), twisted left (yaw -1)
  auto t = compute_twist(make_state({-1, -1, -1, 0}, {1}), c);
  EXPECT_DOUBLE_EQ(t.linear_x, 2.0);
  EXPECT_DOUBLE_EQ(t.linear_y, 2.0);
  EXPECT_DOUBLE_EQ(t.angular_z, 3.0);
  EXPECT_DOUBLE_EQ(t.linear_z, 0.0);
}

TEST(Mapping, ThrottleScale)
{
  TwistConfig c;
  c.deadzone = 0.0;
  EXPECT_DOUBLE_EQ(compute_twist(make_state({0, -1, 0, -1}, {1}), c).linear_x, 1.0);  // forward
  EXPECT_DOUBLE_EQ(compute_twist(make_state({0, -1, 0, 0}, {1}), c).linear_x, 0.5);
  EXPECT_DOUBLE_EQ(compute_twist(make_state({0, -1, 0, 1}, {1}), c).linear_x, 0.0);   // back
  c.throttle = ThrottleMode::LinearZ;
  auto t = compute_twist(make_state({0, 0, 0, -1}, {1}), c);
  EXPECT_DOUBLE_EQ(t.linear_z, 1.0);
}

TEST(Mapping, DeadmanOptions)
{
  TwistConfig c;
  c.deadman_button = -1;
  EXPECT_TRUE(deadman_held(make_state({}, {}), c));
  c.deadman_button = 5;
  EXPECT_FALSE(deadman_held(make_state({}, {1, 1}), c));  // out of range
  State s = make_state({}, {1});
  s.connected = false;
  c.deadman_button = 0;
  EXPECT_FALSE(deadman_held(s, c));
  EXPECT_FLOAT_EQ(axis(make_state({0.5f}, {}), 3), 0.0f);
}

TEST(Joystick, MissingDevice)
{
  Joystick j("/nonexistent/js9");
  EXPECT_FALSE(j.open());
  EXPECT_FALSE(j.state().connected);
  EXPECT_FALSE(j.poll(0));
}

TEST(Joystick, ReadsEvents)
{
  FakeDevice dev;
  Joystick j(dev.path());
  ASSERT_TRUE(j.open());
  dev.connect();
  EXPECT_TRUE(j.take_fresh());
  dev.send(JS_EVENT_AXIS | JS_EVENT_INIT, 1, -32767);
  dev.send(JS_EVENT_BUTTON, 0, 1);
  dev.send(JS_EVENT_AXIS, 3, 16384);
  ASSERT_TRUE(j.poll(500));
  State s = j.state();
  EXPECT_TRUE(s.connected);
  ASSERT_EQ(s.axes.size(), 4u);
  EXPECT_FLOAT_EQ(s.axes[1], -1.0f);
  EXPECT_NEAR(s.axes[3], 0.5f, 1e-4);
  ASSERT_EQ(s.buttons.size(), 1u);
  EXPECT_EQ(s.buttons[0], 1);
  EXPECT_TRUE(j.take_fresh());
  EXPECT_FALSE(j.take_fresh());
  ASSERT_TRUE(j.poll(10));  // nothing pending
  EXPECT_FALSE(j.take_fresh());
}

TEST(Joystick, UnplugReleasesEverything)
{
  FakeDevice dev;
  Joystick j(dev.path());
  ASSERT_TRUE(j.open());
  dev.connect();
  dev.send(JS_EVENT_BUTTON, 0, 1);
  dev.send(JS_EVENT_AXIS, 1, -32767);
  ASSERT_TRUE(j.poll(500));
  dev.disconnect();
  EXPECT_FALSE(j.poll(500));
  State s = j.state();
  EXPECT_FALSE(s.connected);
  EXPECT_EQ(s.buttons[0], 0);
  EXPECT_FLOAT_EQ(s.axes[1], 0.0f);
  TwistConfig c;
  EXPECT_EQ(compute_twist(s, c).linear_x, 0.0);
}

TEST(Joystick, BackgroundReconnect)
{
  FakeDevice dev;
  Joystick j(dev.path());
  j.start(0.1);
  auto wait_for = [&](auto cond) {
      for (int i = 0; i < 100 && !cond(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      return cond();
    };
  ASSERT_TRUE(wait_for([&] {return j.state().connected;}));
  dev.connect();
  dev.send(JS_EVENT_BUTTON, 2, 1);
  ASSERT_TRUE(wait_for([&] {auto s = j.state(); return s.buttons.size() > 2 && s.buttons[2];}));
  dev.disconnect();  // unplug
  ASSERT_TRUE(wait_for([&] {return !j.state().connected;}));
  ASSERT_TRUE(wait_for([&] {return j.state().connected;}));  // reopened
  j.stop();
}
