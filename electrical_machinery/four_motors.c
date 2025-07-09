#include "four_motors.h"

// 全局状态变量
system_state_t g_system_state = {
    .current_move = MOVE_STOP,
    .extend_running = RT_FALSE,
    .stop_flag = RT_FALSE
};

// 状态互斥锁
rt_mutex_t state_mutex = RT_NULL;

// 伸缩电机控制信号量和方向
rt_sem_t extend_sem = RT_NULL;
rt_bool_t extend_direction = RT_FALSE;

/**
 * @brief      四电机系统初始化
 * @param      无
 * @retval     无
 */
void four_motors_init(void)
{
    // 创建状态互斥锁
    state_mutex = rt_mutex_create("state_mtx", RT_IPC_FLAG_FIFO);
    if (state_mutex == RT_NULL)
    {
        rt_kprintf("创建状态互斥锁失败!\n");
    }

    // 创建伸缩信号量
    extend_sem = rt_sem_create("extend_sem", 0, RT_IPC_FLAG_FIFO);
    if (extend_sem == RT_NULL)
    {
        rt_kprintf("创建伸缩信号量失败!\n");
    }

    // 初始化系统状态
    g_system_state.current_move = MOVE_STOP;
    g_system_state.extend_running = RT_FALSE;
    g_system_state.stop_flag = RT_FALSE;

    rt_kprintf("四电机控制系统初始化完成\n");
}

/**
 * @brief      设置移动电机方向
 * @param      rt_bool_t forward: RT_TRUE-前进, RT_FALSE-后退
 * @retval     无
 */
void set_move_direction(rt_bool_t forward)
{
    if (forward) {
        // 前进方向
        rt_pin_write(MOTOR1_DIR_PIN, PIN_HIGH);
        rt_pin_write(MOTOR2_DIR_PIN, PIN_HIGH);
        rt_pin_write(MOTOR3_DIR_PIN, PIN_HIGH);
    } else {
        // 后退方向
        rt_pin_write(MOTOR1_DIR_PIN, PIN_LOW);
        rt_pin_write(MOTOR2_DIR_PIN, PIN_LOW);
        rt_pin_write(MOTOR3_DIR_PIN, PIN_LOW);
    }
}

/**
 * @brief      执行移动指令（在线程中循环调用）
 * @param      无
 * @retval     无
 */
void execute_move_command(void)
{
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    
    if ((g_system_state.current_move == MOVE_FORWARD || 
         g_system_state.current_move == MOVE_BACKWARD) && 
         !g_system_state.extend_running && 
         !g_system_state.stop_flag)
    {
        // 同时控制三个移动电机步进
        rt_pin_write(MOTOR1_STP_PIN, PIN_HIGH);
        rt_pin_write(MOTOR2_STP_PIN, PIN_HIGH);
        rt_pin_write(MOTOR3_STP_PIN, PIN_HIGH);
        
        rt_mutex_release(state_mutex);
        
        rt_hw_us_delay(800);  // 控制移动速度
        
        rt_pin_write(MOTOR1_STP_PIN, PIN_LOW);
        rt_pin_write(MOTOR2_STP_PIN, PIN_LOW);
        rt_pin_write(MOTOR3_STP_PIN, PIN_LOW);
        
        rt_hw_us_delay(800);
    }
    else
    {
        rt_mutex_release(state_mutex);
    }
}

/**
 * @brief      伸缩电机转动函数（一圈定位）
 * @param      rt_bool_t reverse: RT_TRUE-缩回, RT_FALSE-伸出
 * @retval     无
 */
void rotate_extend_motor(rt_bool_t reverse)
{
    rt_uint32_t i;
    
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    g_system_state.extend_running = RT_TRUE;
    rt_mutex_release(state_mutex);
    
    // 设置伸缩电机转动方向
    if (reverse) {
        rt_pin_write(EXTEND_DIR_PIN, PIN_HIGH);  // 缩回方向
    } else {
        rt_pin_write(EXTEND_DIR_PIN, PIN_LOW);   // 伸出方向
    }
    
    // 发送3200个脉冲控制电机转动一圈
    for(i = 0; i < 3200; i++) {
        // 检查紧急停止标志
        rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
        if (g_system_state.stop_flag) {
            rt_kprintf("伸缩电机被紧急停止!\n");
            g_system_state.extend_running = RT_FALSE;
            rt_mutex_release(state_mutex);
            return;
        }
        rt_mutex_release(state_mutex);
        
        rt_pin_write(EXTEND_STP_PIN, PIN_HIGH);
        rt_hw_us_delay(100);  // 控制伸缩速度
        rt_pin_write(EXTEND_STP_PIN, PIN_LOW);
        rt_hw_us_delay(100);
    }
    
    // 伸缩完成
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    g_system_state.extend_running = RT_FALSE;
    rt_mutex_release(state_mutex);
    
    if (reverse) {
        rt_kprintf("缩回完成\n");
    } else {
        rt_kprintf("伸出完成\n");
    }
}

/**
 * @brief      紧急停止所有电机
 * @param      无
 * @retval     无
 */
void emergency_stop(void)
{
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    g_system_state.stop_flag = RT_TRUE;
    g_system_state.extend_running = RT_FALSE;
    g_system_state.current_move = MOVE_STOP;
    rt_mutex_release(state_mutex);
    
    rt_pin_write(LED_PIN, PIN_LOW);  // 熄灭LED
    rt_kprintf("紧急停止！所有电机已停止\n");
}

/**
 * @brief      处理指令函数
 * @param      char cmd: 指令字符
 * @retval     无
 */
void process_command(char cmd)
{
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    rt_bool_t extend_busy = g_system_state.extend_running;
    move_command_t current_move = g_system_state.current_move;
    rt_mutex_release(state_mutex);
    
    switch (cmd) {
        case 'F':
        case 'f':
            if (!extend_busy) {
                rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
                g_system_state.current_move = MOVE_FORWARD;
                g_system_state.stop_flag = RT_FALSE;
                rt_mutex_release(state_mutex);
                
                rt_kprintf("开始前进...\n");
                set_move_direction(RT_TRUE);  // 正转方向
                rt_pin_write(LED_PIN, PIN_HIGH);  // 点亮LED表示前进
            } else {
                rt_kprintf("伸缩电机运行中，请等待...\n");
            }
            break;
            
        case 'B':
        case 'b':
            if (!extend_busy) {
                rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
                g_system_state.current_move = MOVE_BACKWARD;
                g_system_state.stop_flag = RT_FALSE;
                rt_mutex_release(state_mutex);
                
                rt_kprintf("开始后退...\n");
                set_move_direction(RT_FALSE);  // 反转方向
                rt_pin_write(LED_PIN, PIN_HIGH);  // 点亮LED表示后退
            } else {
                rt_kprintf("伸缩电机运行中，请等待...\n");
            }
            break;
            
        case 'S':
        case 's':
            rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
            g_system_state.current_move = MOVE_STOP;
            rt_mutex_release(state_mutex);
            
            rt_kprintf("停止移动\n");
            rt_pin_write(LED_PIN, PIN_LOW);  // 熄灭LED
            break;
            
        case 'U':
        case 'u':
            if (!extend_busy && current_move == MOVE_STOP) {
                rt_kprintf("开始伸出...\n");
                start_extend_motor(RT_FALSE);  // 正转伸出
            } else if (extend_busy) {
                rt_kprintf("伸缩电机正在运行中...\n");
            } else {
                rt_kprintf("请先停止移动后再操作伸缩\n");
            }
            break;
            
        case 'D':
        case 'd':
            if (!extend_busy && current_move == MOVE_STOP) {
                rt_kprintf("开始缩回...\n");
                start_extend_motor(RT_TRUE);   // 反转缩回
            } else if (extend_busy) {
                rt_kprintf("伸缩电机正在运行中...\n");
            } else {
                rt_kprintf("请先停止移动后再操作伸缩\n");
            }
            break;
            
        case 'E':
        case 'e':
            emergency_stop();
            break;
            
        case 'H':
        case 'h':
            print_help_info();
            break;
            
        default:
            rt_kprintf("无效指令！请输入正确的指令\n");
            rt_kprintf("F(前进) B(后退) S(停止) U(伸出) D(缩回) E(紧急停止) H(帮助)\n");
            break;
    }
}

/**
 * @brief      打印帮助信息
 * @param      无
 * @retval     无
 */
void print_help_info(void)
{
    rt_kprintf("四电机控制系统\n");
    rt_kprintf("===================\n");
    rt_kprintf("移动控制指令：\n");
    rt_kprintf("F - 前进（三个电机正转）\n");
    rt_kprintf("B - 后退（三个电机反转）\n");
    rt_kprintf("S - 停止移动\n");
    rt_kprintf("\n");
    rt_kprintf("伸缩控制指令：\n");
    rt_kprintf("U - 伸出（伸缩电机正转一圈）\n");
    rt_kprintf("D - 缩回（伸缩电机反转一圈）\n");
    rt_kprintf("\n");
    rt_kprintf("其他指令：\n");
    rt_kprintf("E - 紧急停止所有电机\n");
    rt_kprintf("H - 显示帮助信息\n");
    rt_kprintf("===================\n");
}

/**
 * @brief      启动伸缩电机（通过信号量）
 * @param      reverse: RT_TRUE-缩回, RT_FALSE-伸出
 * @retval     无
 */
void start_extend_motor(rt_bool_t reverse)
{
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    if (!g_system_state.extend_running && g_system_state.current_move == MOVE_STOP)
    {
        extend_direction = reverse;
        rt_sem_release(extend_sem);  // 发送信号量启动伸缩电机
    }
    rt_mutex_release(state_mutex);
}