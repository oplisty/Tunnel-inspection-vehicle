import sensor, image, time, pyb

# 初始化摄像头
sensor.reset()  # 重置摄像头
sensor.set_pixformat(sensor.RGB565)  # 设置彩色模式
sensor.set_framesize(sensor.QVGA)  # 设置分辨率 (320x240)
sensor.skip_frames(time=2000)  # 等待设置生效
sensor.set_auto_gain(False)  # 关闭自动增益
sensor.set_auto_whitebal(False)  # 关闭自动白平衡

# 定义黑色在LAB颜色空间中的阈值范围
# L: 亮度 (0-100)
# A: 绿红分量 (-128 to 127)
# B: 蓝黄分量 (-128 to 127)
# 黑色通常具有低亮度(L)和中间A/B值
BLACK_THRESHOLD = (0, 40, -10, 10, -10, 10)  # (L_min, L_max, A_min, A_max, B_min, B_max)

# 创建LED对象用于视觉反馈
red_led = pyb.LED(1)
green_led = pyb.LED(2)
blue_led = pyb.LED(3)

# 设置检测区域为整个画面
ROI = (0, 0, sensor.width(), sensor.height())

# 黑色物体检测的最小像素面积
MIN_BLACK_AREA = 1000

clock = time.clock()  # 创建时钟对象用于FPS计算

def leds_off():
    red_led.off()
    green_led.off()
    blue_led.off()

def blink_led(color, duration=50):
    leds_off()
    if color == 'red':
        red_led.on()
    elif color == 'green':
        green_led.on()
    elif color == 'blue':
        blue_led.on()
    pyb.delay(duration)
    leds_off()

# 初始LED测试
for _ in range(3):
    blink_led('red', 100)
    blink_led('green', 100)
    blink_led('blue', 100)

print("Starting black object detection...")

while(True):
    clock.tick()  # 更新FPS时钟
    img = sensor.snapshot()  # 捕获图像

    # 在LAB颜色空间中查找黑色区域
    blobs = img.find_blobs([BLACK_THRESHOLD],
                           roi=ROI,
                           pixels_threshold=MIN_BLACK_AREA,
                           merge=True)

    black_detected = False

    if blobs:
        # 找到最大的黑色区域
        largest_blob = max(blobs, key=lambda b: b.pixels())

        # 在检测到的黑色物体周围绘制矩形
        img.draw_rectangle(largest_blob.rect(), color=(255, 0, 0))

        # 在物体中心绘制十字标记
        img.draw_cross(largest_blob.cx(), largest_blob.cy(), color=(0, 255, 0))

        # 显示物体中心坐标
        img.draw_string(10, 10, "X: %d, Y: %d" % (largest_blob.cx(), largest_blob.cy()),
                        color=(255, 255, 255))

        # 显示物体面积
        img.draw_string(10, 30, "Area: %d" % largest_blob.pixels(),
                        color=(255, 255, 255))

        black_detected = True

    # 显示FPS
    img.draw_string(10, sensor.height() - 20, "FPS: %.1f" % clock.fps(),
                    color=(255, 255, 255))

    # 显示检测状态
    status = "Black: DETECTED" if black_detected else "Black: NOT FOUND"
    img.draw_string(sensor.width() - 150, 10, status,
                    color=(0, 255, 0) if black_detected else (255, 0, 0))

    # LED视觉反馈
    if black_detected:
        green_led.on()
        red_led.off()
    else:
        green_led.off()
        red_led.on()

    # 每10帧闪烁蓝色LED表示程序运行中
    if pyb.millis() % 500 < 50:
        blue_led.on()
    else:
        blue_led.off()
