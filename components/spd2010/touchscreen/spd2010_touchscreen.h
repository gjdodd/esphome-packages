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

typedef struct {
  uint8_t id;
  uint16_t x;
  uint16_t y;
  uint8_t weight;
} tp_report_t;
struct SPD2010_Touch{
  tp_report_t rpt[10];
  uint8_t touch_num;     // Number of touch points
  uint8_t pack_code;
  uint8_t down;
  uint8_t up;
  uint8_t gesture;
  uint16_t down_x;
  uint16_t down_y;
  uint16_t up_x;
  uint16_t up_y;
};

typedef struct {
  uint8_t none0;
  uint8_t none1;
  uint8_t none2;
  uint8_t cpu_run;
  uint8_t tint_low;
  uint8_t tic_in_cpu;
  uint8_t tic_in_bios;
  uint8_t tic_busy;
} tp_status_high_t;

typedef struct {
  uint8_t pt_exist;
  uint8_t gesture;
  uint8_t key;
  uint8_t aux;
  uint8_t keep;
  uint8_t raw_or_pt;
  uint8_t none6;
  uint8_t none7;
} tp_status_low_t;

typedef struct {
  tp_status_low_t status_low;
  tp_status_high_t status_high;
  uint16_t read_len;
} tp_status_t;
typedef struct {
  uint8_t status;
  uint16_t next_packet_len;
} tp_hdp_status_t;

extern struct SPD2010_Touch touch_data;

class Spd2010Touchscreen : public Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;

  void dump_config() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_enable_pin(GPIOPin *pin) { this->enable_pin_ = pin; }

 protected:
  void hard_reset_();
  void update_touches() override;
  
  InternalGPIOPin *interrupt_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *enable_pin_{nullptr};
  
 private:
  enable_();
  hard_reset_();  
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