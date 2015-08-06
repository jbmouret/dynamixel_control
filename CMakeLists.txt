cmake_minimum_required(VERSION 2.8.3)
project(dynamixel_control)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  message_generation
)

add_service_files(
  FILES
  Hold.srv
  GetIDs.srv
  GetActuatorLoad.srv
  GetActuatorsLoads.srv
  GetActuatorVoltage.srv
  GetActuatorsVoltages.srv
  GetActuatorPosition.srv
  GetActuatorsPositions.srv
  SetActuatorPosition.srv
  SetActuatorSpeed.srv
  SetPower.srv
  SetActualActuatorsPositions.srv
)

add_message_files(
  FILES
  PositionCtrl.msg
  SpeedCtrl.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
)

catkin_package(
 CATKIN_DEPENDS roscpp rospy std_msgs message_runtime
)

include_directories(
  ${catkin_INCLUDE_DIRS}
  ~/Code/libdynamixel/src/dynamixel/
)

link_directories(~/Code/libdynamixel/build/default/src/dynamixel)

add_executable(dynamixel_control src/dynamixel_control.cpp)
target_link_libraries(dynamixel_control ${catkin_LIBRARIES} dynamixel)
add_dependencies(dynamixel_control dynamixel_control_gencpp dynamixel_control_generate_messages_cpp)
