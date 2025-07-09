#ifndef __BOARD_H
#define __BOARD_H

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stm32f4xx.h>

// 移动电机1引脚定义（前进后退控制）
#define MOTOR1_EN_PIN      GET_PIN(B, 13)    // 使能引脚
#define MOTOR1_STP_PIN     GET_PIN(B, 12)    // 步进引脚
#define MOTOR1_DIR_PIN     GET_PIN(B, 11)    // 方向引脚

// 移动电机2引脚定义
#define MOTOR2_EN_PIN      GET_PIN(B, 10)    // 使能引脚
#define MOTOR2_STP_PIN     GET_PIN(B, 9)     // 步进引脚
#define MOTOR2_DIR_PIN     GET_PIN(B, 8)     // 方向引脚

// 移动电机3引脚定义
#define MOTOR3_EN_PIN      GET_PIN(B, 4)     // 使能引脚
#define MOTOR3_STP_PIN     GET_PIN(B, 3)     // 步进引脚
#define MOTOR3_DIR_PIN     GET_PIN(B, 2)     // 方向引脚

// 伸缩电机引脚定义
#define EXTEND_EN_PIN      GET_PIN(B, 5)     // 使能引脚
#define EXTEND_STP_PIN     GET_PIN(B, 6)     // 步进引脚
#define EXTEND_DIR_PIN     GET_PIN(B, 7)     // 方向引脚

// LED指示灯
#define LED_PIN            GET_PIN(C, 13)    // LED引脚

void board_init(void);

#endif