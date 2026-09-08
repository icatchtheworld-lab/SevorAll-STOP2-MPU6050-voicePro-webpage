#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

void servo_init(void);
void servo_set_angleA(uint32_t angle);
void servo_set_angleB(uint32_t angle);
void servo_set_angleC(uint32_t angle);
void servo_set_angleD(uint32_t angle);
void servo_set_angleE(uint32_t angle);
uint32_t servo_get_current_angle(void);
void servo_set_current_angle(uint32_t angle);
uint32_t servo_get_angleA(void);
uint32_t servo_get_angleB(void);
uint32_t servo_get_angleC(void);
uint32_t servo_get_angleD(void);
uint32_t servo_get_angleE(void);
void servo_move_smooth(uint32_t target_angle, uint32_t speed_ms);
void servo_catch_target(void);
void servo_release_target(void);
void servo_emergency_stop(void);
void servo_clear_emergency_stop(void);
uint8_t servo_emergency_stop_is_set(void);
#endif /* SERVO_H_ */
