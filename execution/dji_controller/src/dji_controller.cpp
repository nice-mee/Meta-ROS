#include "rclcpp/rclcpp.hpp"
#include "dji_controller/dji_driver.h"

#include "motor_interface/msg/motor_goal.hpp"
#include "motor_interface/srv/motor_present.hpp"
#include <cmath>
#include <cstdint>
#include <linux/can.h>
#include <memory>
#include <rclcpp/logging.hpp>
#include <vector>

class DjiController : public rclcpp::Node
{
public:
    DjiController() : Node("DjiController")
    {
        motor_init();
        goal_sub_ = this->create_subscription<motor_interface::msg::MotorGoal>(
            "motor_goal", 10, [this](const motor_interface::msg::MotorGoal::SharedPtr msg){
                goal_callback(msg);
            });
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(CONTROL_R), [this](){
                control_timer_callback();
            });
        feedback_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(FEEDBACK_R), [this](){
                feedback_timer_callback();
            });

        // param_ev_ = std::make_shared<rclcpp::ParameterEventHandler>(this);
        // p2v_kps_cb_ = param_ev_->add_parameter_callback("p2v_kps", std::bind(&DjiController::p2v_kps_callback, this, std::placeholders::_1));
        // p2v_kis_cb_ = param_ev_->add_parameter_callback("p2v_kis", std::bind(&DjiController::p2v_kis_callback, this, std::placeholders::_1));
        // p2v_kds_cb_ = param_ev_->add_parameter_callback("p2v_kds", std::bind(&DjiController::p2v_kds_callback, this, std::placeholders::_1));
        // v2c_kps_cb_ = param_ev_->add_parameter_callback("v2c_kps", std::bind(&DjiController::v2c_kps_callback, this, std::placeholders::_1));
        // v2c_kis_cb_ = param_ev_->add_parameter_callback("v2c_kis", std::bind(&DjiController::v2c_kis_callback, this, std::placeholders::_1));
        // v2c_kds_cb_ = param_ev_->add_parameter_callback("v2c_kds", std::bind(&DjiController::v2c_kds_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "DjiController initialized");
    }

private:
    rclcpp::Subscription<motor_interface::msg::MotorGoal>::SharedPtr goal_sub_;
    rclcpp::TimerBase::SharedPtr control_timer_; // send control frame regularly
    rclcpp::TimerBase::SharedPtr feedback_timer_; // receive feedback frame regularly
    // std::shared_ptr<rclcpp::ParameterEventHandler> param_ev_;
    // std::shared_ptr<rclcpp::ParameterCallbackHandle> p2v_kps_cb_, p2v_kis_cb_, p2v_kds_cb_, v2c_kps_cb_, v2c_kis_cb_, v2c_kds_cb_;
    int dji_motor_count;
    std::vector<std::unique_ptr<DjiDriver>> drivers_; // std::unique_ptr<DjiDriver> drivers_[8];
    std::vector<double> p2v_kps{}, p2v_kis{}, p2v_kds{};
    std::vector<double> v2c_kps{}, v2c_kis{}, v2c_kds{};

    void control_timer_callback()
    {
        for (auto& driver : drivers_)
        {
            driver->write_tx();
            DjiDriver::send_frame();
        }
    }

    void feedback_timer_callback()
    {
        for (auto& driver : drivers_)
        {
            DjiDriver::get_frame();
            driver->process_rx();
            // rclcpp::sleep_for(std::chrono::milliseconds(1)); // sleep for 1 ms
        }
    }
    
    void goal_callback(const motor_interface::msg::MotorGoal::SharedPtr msg)
    {
        int count = msg->motor_id.size();
        for (int i = 0; i < count; i++)
        {
            int rid = msg->motor_id[i];
            float pos = msg->goal_pos[i];
            float vel = msg->goal_vel[i];

            // find corresponding driver
            auto iter = std::find_if(drivers_.begin(), drivers_.end(),
                [rid](const std::unique_ptr<DjiDriver>& driver){
                    return driver->rid == rid;
                });

            // set goal
            if (iter != drivers_.end())
            {
                auto driver = iter->get();
                driver->set_goal(pos, vel);
            }
            else {
                // not found, may be a dm motor
                // RCLCPP_WARN(this->get_logger(), "Motor %d not found", rid);
            }
        }
    }

    void motor_init()
    {
        int motor_count = this->declare_parameter("motor_count", 0);

        std::vector<int64_t> motor_brands {};
        motor_brands = this->declare_parameter("motor_brands", motor_brands);
        std::vector<int64_t> motor_ids {};
        motor_ids = this->declare_parameter("motor_ids", motor_ids);
        std::vector<int64_t> motor_types {};
        motor_types= this->declare_parameter("motor_types", motor_types);
        
        p2v_kps = this->declare_parameter("p2v_kps", p2v_kps);
        p2v_kis = this->declare_parameter("p2v_kis", p2v_kis);
        p2v_kds = this->declare_parameter("p2v_kds", p2v_kds);
        v2c_kps = this->declare_parameter("v2c_kps", v2c_kps);
        v2c_kis = this->declare_parameter("v2c_kis", v2c_kis);
        v2c_kds = this->declare_parameter("v2c_kds", v2c_kds);

        // initialize the drivers
        for (int i = 0; i < motor_count; i++)
        {
            if (motor_brands[i] != 0) continue; // only create drivers for DJI motors
            
            dji_motor_count++;
            MotorType type = static_cast<MotorType>(motor_types[i]);
            int rid = motor_ids[i];
            drivers_.push_back(std::make_unique<DjiDriver>(rid, type));
            drivers_.back()->set_p2v_pid(p2v_kps[i], p2v_kis[i], p2v_kds[i]);
            drivers_.back()->set_v2c_pid(v2c_kps[i], v2c_kis[i], v2c_kds[i]);
        }
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DjiController>());
    rclcpp::shutdown();
    return 0;
}