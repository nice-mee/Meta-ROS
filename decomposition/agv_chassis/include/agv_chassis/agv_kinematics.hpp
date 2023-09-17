#ifndef AGV_KINEMATICS_HPP
#define AGV_KINEMATICS_HPP

#include <cmath>
#include <motor_interface/msg/detail/motor_goal__struct.hpp>
#include <vector>

#include "movement_interface/msg/natural_move.hpp"
#include "movement_interface/msg/absolute_move.hpp"
#include "motor_interface/msg/motor_goal.hpp"

#define PI 3.14159265358979323846
#define RADIUS 0.1 // meter
#define LF0 0
#define RF0 1
#define LB0 2
#define RB0 3
#define LF1 4
#define RF1 5
#define LB1 6
#define RB1 7


#define ROT_COMP (msg->omega * RADIUS / std::sqrt(2)) // component velocity caused by rotation

namespace AgvKinematics
{
    motor_interface::msg::MotorGoal natural_decompo(const movement_interface::msg::NaturalMove::SharedPtr msg);
    motor_interface::msg::MotorGoal absolute_decompo(const movement_interface::msg::AbsoluteMove::SharedPtr msg);
    void add_goal(motor_interface::msg::MotorGoal &motor_goals, const int motor_id, const float goal_vel, const float goal_pos);
    float rss(float x, float y); // root sum square
};

#endif // AGV_KINEMATICS_HPP