#ifndef META_HARDWARE__MOTOR_NETWORK__MI_MOTOR_NETWORK_HPP_
#define META_HARDWARE__MOTOR_NETWORK__MI_MOTOR_NETWORK_HPP_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "meta_hardware/motor_driver/mi_motor_driver.hpp"
#include <CanDriver.hpp>
#include <CanMessage.hpp>

namespace meta_hardware {

class MiMotorNetwork {
  public:
    MiMotorNetwork(
        const std::string &can_network_name, uint32_t host_id,
        const std::vector<std::unordered_map<std::string, std::string>>
            &joint_params);
    ~MiMotorNetwork();

    /**
     * @brief Read the motor feedback
     * @param joint_id The joint ID of the motor
     * @return A tuple of (position, velocity, effort)
     */
    std::tuple<double, double, double> read(uint32_t joint_id) const;

    /**
     * @brief Write the motor command
     * @param joint_id The joint ID of the motor
     * @param position The position to write
     * @param velocity The velocity to write
     * @param effort The effort to write
     */
    void write(uint32_t joint_id, double position, double velocity,
               double effort);

  private:
    [[noreturn]] void rx_loop();
    std::thread rx_thread_;

    void process_mi_frame(const sockcanpp::CanMessage &can_msg);
    void process_mi_info_frame(const sockcanpp::CanMessage &can_msg);
    void process_mi_fb_frame(const sockcanpp::CanMessage &can_msg);

    // CAN driver
    std::unique_ptr<sockcanpp::CanDriver> can_driver_;

    // [motor_id] -> mi_motor
    // This makes it easy to find the motor object in rx_loop
    std::map<uint32_t, std::shared_ptr<MiMotor>> motor_id2motor_;

    // [joint_id] -> mi_motor
    // This makes it easy to find the motor object in read() and write()
    std::vector<std::shared_ptr<MiMotor>> mi_motors_;

    // Host ID
    uint32_t host_id_;
};

} // namespace meta_hardware

#endif // META_HARDWARE__MOTOR_NETWORK__MI_MOTOR_NETWORK_HPP_
