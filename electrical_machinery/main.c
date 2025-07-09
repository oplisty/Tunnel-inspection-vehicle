#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "four_motors.h"

// 线程句柄
static rt_thread_t motor_control_thread = RT_NULL;
static rt_thread_t serial_thread = RT_NULL;
static rt_thread_t extend_thread = RT_NULL;

// 串口设备
static rt_device_t serial_dev = RT_NULL;

/**
 * @brief      电机控制线程入口函数
 * @param      parameter: 线程参数
 * @retval     无
 */
static void motor_control_thread_entry(void *parameter)
{
    while(1)
    {
        execute_move_command();
        rt_thread_mdelay(2);  // 控制循环频率
    }
}

/**
 * @brief      伸缩电机控制线程入口函数
 * @param      parameter: 线程参数
 * @retval     无
 */
static void extend_thread_entry(void *parameter)
{
    while(1)
    {
        // 等待伸缩信号量
        if (rt_sem_take(extend_sem, RT_WAITING_FOREVER) == RT_EOK)
        {
            rotate_extend_motor(extend_direction);
        }
    }
}

/**
 * @brief      串口数据接收回调函数
 * @param      dev: 设备句柄
 * @param      size: 数据大小
 * @retval     接收结果
 */
static rt_err_t uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    /* 唤醒串口接收线程 */
    if (serial_thread != RT_NULL)
    {
        rt_thread_resume(serial_thread);
    }
    return RT_EOK;
}

/**
 * @brief      串口处理线程入口函数
 * @param      parameter: 线程参数
 * @retval     无
 */
static void serial_thread_entry(void *parameter)
{
    char ch;
    
    while (1)
    {
        /* 从串口读取一个字节的数据 */
        while (rt_device_read(serial_dev, -1, &ch, 1) != 1)
        {
            /* 挂起线程等待接收数据 */
            rt_thread_suspend(rt_thread_self());
            rt_schedule();
        }
        
        /* 处理接收到的指令 */
        process_command(ch);
    }
}

int main(void)
{
    /* 初始化板载外设 */
    board_init();

    /* 初始化四电机系统 */
    four_motors_init();

    /* 查找串口设备 */
    serial_dev = rt_device_find("uart1");
    if (!serial_dev)
    {
        rt_kprintf("找不到串口设备uart1!\n");
        return -1;
    }

    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial_dev, RT_DEVICE_FLAG_INT_RX);

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial_dev, uart_rx_ind);

    /* 上电延时2秒等待电机初始化完毕 */
    rt_thread_mdelay(2000);

    /* 创建电机控制线程 */
    motor_control_thread = rt_thread_create("motor_ctrl",
                                           motor_control_thread_entry,
                                           RT_NULL,
                                           1024,
                                           10,
                                           20);

    /* 创建串口处理线程 */
    serial_thread = rt_thread_create("serial",
                                    serial_thread_entry,
                                    RT_NULL,
                                    1024,
                                    8,
                                    20);

    /* 创建伸缩电机线程 */
    extend_thread = rt_thread_create("extend",
                                    extend_thread_entry,
                                    RT_NULL,
                                    1024,
                                    12,
                                    20);

    /* 启动线程 */
    if (motor_control_thread != RT_NULL)
        rt_thread_startup(motor_control_thread);

    if (serial_thread != RT_NULL)
        rt_thread_startup(serial_thread);

    if (extend_thread != RT_NULL)
        rt_thread_startup(extend_thread);

    /* 打印启动信息 */
    rt_thread_mdelay(100);
    print_help_info();
    rt_kprintf("系统初始化完成，请输入指令：\n");

    return 0;
}