#include "spd2010.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace spd2010 {

static const char *const TAG = "spd2010";
  
#define CONFIG_ESP_LCD_TOUCH_MAX_POINTS     (5)     

struct SPD2010_Touch touch_data = {0};

void Spd2010Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SPD2010 Touchscreen...");
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);  
  }
  
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    //this->enable_();
  }  
  
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->hard_reset_();
  }  
}

void Spd2010Touchscreen::update_touches() {
  ESP_LOGD(TAG, "update_touches");
  uint8_t touch_cnt = 0;
  struct SPD2010_Touch touch = {0};
  this->tp_read_data(&touch);
  
  /* Expect Number of touched points */
  touch_cnt = (touch.touch_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS ? CONFIG_ESP_LCD_TOUCH_MAX_POINTS : touch.touch_num);
  touch_data.touch_num = touch_cnt;
  /* Fill all coordinates */
  
  ESP_LOGD(TAG, "Touches found: %d", touch_cnt);
  
  for (int i = 0; i < touch_cnt; i++) {
	ESP_LOGW(TAG, "Reporting a (%d,%d) touch on %d", touch.rpt[i].x, touch.rpt[i].y, touch.rpt[i].id);
	
    this->add_raw_touch_position_(touch.rpt[i].id, touch.rpt[i].x, touch.rpt[i].y);   
  }

  this->status_clear_warning();
}

void Spd2010Touchscreen::hard_reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->digital_write(false);
    delay(50);
    this->reset_pin_->digital_write(true);
	delay(50);
  }
}

void Spd2010Touchscreen::enable_() {
  if (this->enable_pin_ != nullptr) {
    this->reset_pin_->digital_write(true);
	delay(50);
  }
}

void Spd2010Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "SPD2010 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
}

esp_err_t Spd2010Touchscreen::write_tp_point_mode_cmd()
{
  uint8_t sample_data[4];
  sample_data[0] = 0x50;
  sample_data[1] = 0x00;
  sample_data[2] = 0x00;
  sample_data[3] = 0x00;
  
  this->write_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);  
  esp_rom_delay_us(200);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::write_tp_start_cmd()
{
  uint8_t sample_data[4];
  sample_data[0] = 0x46;
  sample_data[1] = 0x00;
  sample_data[2] = 0x00;
  sample_data[3] = 0x00;
  
  this->write_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);  
  esp_rom_delay_us(200);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::write_tp_cpu_start_cmd()
{
  uint8_t sample_data[4];
  sample_data[0] = 0x04;
  sample_data[1] = 0x00;
  sample_data[2] = 0x01;
  sample_data[3] = 0x00;
  
  this->write_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2);   
  esp_rom_delay_us(200);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::write_tp_clear_int_cmd()
{
  uint8_t sample_data[4];
  sample_data[0] = 0x02;
  sample_data[1] = 0x00;
  sample_data[2] = 0x01;
  sample_data[3] = 0x00;
  
  this->write_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), &sample_data[2], 2); 
  esp_rom_delay_us(200);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::read_tp_status_length(tp_status_t *tp_status)
{
  uint8_t sample_data[4];
  sample_data[0] = 0x20;
  sample_data[1] = 0x00;
  
  this->read_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, 4);  
    
  esp_rom_delay_us(200);
  tp_status->status_low.pt_exist = (sample_data[0] & 0x01);
  tp_status->status_low.gesture = (sample_data[0] & 0x02);
  tp_status->status_low.aux = ((sample_data[0] & 0x08)); //aux, cytang
  tp_status->status_high.tic_busy = ((sample_data[1] & 0x80) >> 7);
  tp_status->status_high.tic_in_bios = ((sample_data[1] & 0x40) >> 6);
  tp_status->status_high.tic_in_cpu = ((sample_data[1] & 0x20) >> 5);
  tp_status->status_high.tint_low = ((sample_data[1] & 0x10) >> 4);
  tp_status->status_high.cpu_run = ((sample_data[1] & 0x08) >> 3);
  tp_status->read_len = (sample_data[3] << 8 | sample_data[2]);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::read_tp_hdp(tp_status_t *tp_status, SPD2010_Touch *touch)
{
  uint8_t sample_data[4+(10*6)]; // 4 Bytes Header + 10 Finger * 6 Bytes
  uint8_t i, offset;
  uint8_t check_id;
  sample_data[0] = 0x00;
  sample_data[1] = 0x03;
  
  this->read_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, tp_status->read_len);   

  check_id = sample_data[4];
  if ((check_id <= 0x0A) && tp_status->status_low.pt_exist) {
    touch->touch_num = ((tp_status->read_len - 4)/6);
    touch->gesture = 0x00;
    for (i = 0; i < touch->touch_num; i++) {
      offset = i*6;
      touch->rpt[i].id = sample_data[4 + offset];
      touch->rpt[i].x = (((sample_data[7 + offset] & 0xF0) << 4)| sample_data[5 + offset]);
      touch->rpt[i].y = (((sample_data[7 + offset] & 0x0F) << 8)| sample_data[6 + offset]);
      touch->rpt[i].weight = sample_data[8 + offset];
    }
    /* For slide gesture recognize */
    if ((touch->rpt[0].weight != 0) && (touch->down != 1)) {
      touch->down = 1;
      touch->up = 0 ;
      touch->down_x = touch->rpt[0].x;
      touch->down_y = touch->rpt[0].y;
    } else if ((touch->rpt[0].weight == 0) && (touch->down == 1)) {
      touch->up = 1;
      touch->down = 0;
      touch->up_x = touch->rpt[0].x;
      touch->up_y = touch->rpt[0].y;
    }    
  } else if ((check_id == 0xF6) && tp_status->status_low.gesture) {
    touch->touch_num = 0x00;
    touch->up = 0;
    touch->down = 0;
    touch->gesture = sample_data[6] & 0x07;
    printf("gesture : 0x%02x\n", touch->gesture);
  } else {
    touch->touch_num = 0x00;
    touch->gesture = 0x00;
  }
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::read_tp_hdp_status(tp_hdp_status_t *tp_hdp_status)
{
  uint8_t sample_data[8];
  sample_data[0] = 0xFC;
  sample_data[1] = 0x02;
  
  this->read_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, 8);  
  
  tp_hdp_status->status = sample_data[5];
  tp_hdp_status->next_packet_len = (sample_data[2] | sample_data[3] << 8);
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::Read_HDP_REMAIN_DATA(tp_hdp_status_t *tp_hdp_status)
{
  uint8_t sample_data[32];
  sample_data[0] = 0x00;
  sample_data[1] = 0x03;
  
  this->read_register((((uint16_t)sample_data[0] << 8) | (sample_data[1])), sample_data, tp_hdp_status->next_packet_len);  
  
  return ESP_OK;
}

esp_err_t Spd2010Touchscreen::tp_read_data(SPD2010_Touch *touch)
{
  tp_status_t tp_status = {0};
  tp_hdp_status_t tp_hdp_status = {0};
  read_tp_status_length(&tp_status);
  if (tp_status.status_high.tic_in_bios) {
    /* Write Clear TINT Command */
    write_tp_clear_int_cmd();
    /* Write CPU Start Command */
    write_tp_cpu_start_cmd();
  } else if (tp_status.status_high.tic_in_cpu) {
    /* Write Touch Change Command */
    write_tp_point_mode_cmd();
    /* Write Touch Start Command */
    write_tp_start_cmd();
    /* Write Clear TINT Command */
    write_tp_clear_int_cmd();
  } else if (tp_status.status_high.cpu_run && tp_status.read_len == 0) {
    write_tp_clear_int_cmd();
  } else if (tp_status.status_low.pt_exist || tp_status.status_low.gesture) {
    /* Read HDP */
    read_tp_hdp(&tp_status, touch);
	hdp_done_check:
    /* Read HDP Status */
    read_tp_hdp_status(&tp_hdp_status);
    if (tp_hdp_status.status == 0x82) {
      /* Clear INT */
      write_tp_clear_int_cmd();
    }
    else if (tp_hdp_status.status == 0x00) {
      /* Read HDP Remain Data */
      Read_HDP_REMAIN_DATA(&tp_hdp_status);
      goto hdp_done_check;
    }
  } else if (tp_status.status_high.cpu_run && tp_status.status_low.aux) {
    write_tp_clear_int_cmd();
  }

  return ESP_OK;
}


}  // namespace spd2010
}  // namespace esphome