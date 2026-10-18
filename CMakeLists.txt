cmake_minimum_required(VERSION 3.5)
project(extreme3dpro_driver)

# One package for ROS 1 (catkin) and ROS 2 (ament), picked by the sourced ROS environment.

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

# Joystick reader and mapping, no ROS.
add_library(extreme3dpro_core STATIC src/joystick.cpp src/mapping.cpp)
set_target_properties(extreme3dpro_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(extreme3dpro_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(extreme3dpro_core Threads::Threads)

if("$ENV{ROS_VERSION}" STREQUAL "1")

  find_package(catkin REQUIRED COMPONENTS roscpp sensor_msgs geometry_msgs std_msgs)
  catkin_package()

  add_executable(extreme3dpro_node src/ros1_node.cpp)
  target_include_directories(extreme3dpro_node PRIVATE ${catkin_INCLUDE_DIRS})
  target_link_libraries(extreme3dpro_node extreme3dpro_core ${catkin_LIBRARIES})

  install(TARGETS extreme3dpro_node RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  install(DIRECTORY launch config udev DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

  if(CATKIN_ENABLE_TESTING)
    catkin_add_gtest(test_extreme3dpro test/test_core.cpp)
    target_link_libraries(test_extreme3dpro extreme3dpro_core gtest_main)
  endif()

else()

  find_package(ament_cmake REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(sensor_msgs REQUIRED)
  find_package(geometry_msgs REQUIRED)
  find_package(std_msgs REQUIRED)

  add_executable(extreme3dpro_node src/ros2_node.cpp)
  target_link_libraries(extreme3dpro_node extreme3dpro_core)
  ament_target_dependencies(extreme3dpro_node rclcpp sensor_msgs geometry_msgs std_msgs)

  install(TARGETS extreme3dpro_node DESTINATION lib/${PROJECT_NAME})
  install(DIRECTORY launch config udev DESTINATION share/${PROJECT_NAME})

  if(BUILD_TESTING)
    find_package(ament_cmake_gtest REQUIRED)
    ament_add_gtest(test_extreme3dpro test/test_core.cpp)
    target_link_libraries(test_extreme3dpro extreme3dpro_core)
  endif()

  ament_package()

endif()
