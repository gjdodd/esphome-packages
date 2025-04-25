#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#include <vector>

namespace esphome {
namespace spd2010 {

using namespace touchscreen;

class Spd2010Touchscreen : public Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;

  void dump_config() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }

 protected:
  void update_touches() override;

  InternalGPIOPin *interrupt_pin_;
  
 private:
  esp_err_t write_tp_point_mode_cmd();
  esp_err_t write_tp_start_cmd();
  esp_err_t write_tp_cpu_start_cmd();
  esp_err_t write_tp_clear_int_cmd();
  esp_err_t read_tp_status_length(tp_status_t *tp_status);
  esp_err_t read_tp_hdp(tp_status_t *tp_status, SPD2010_Touch *touch);
  esp_err_t read_tp_hdp_status(tp_hdp_status_t *tp_hdp_status);
  esp_err_t Read_HDP_REMAIN_DATA(tp_hdp_status_t *tp_hdp_status);  
  esp_err_t tp_read_data(SPD2010_Touch *touch); 
};

}  // namespace spd2010
}  // namespace esphome