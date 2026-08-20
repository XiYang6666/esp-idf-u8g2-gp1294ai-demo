#include <iostream>
#include <iomanip>

#include "esp_log.h"
#include "esp_timer.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"

#include "u8g2.h"
#include "u8g2_idf_adapter.h"

#define VFD_MOSI       GPIO_NUM_17
#define VFD_CLOCK      GPIO_NUM_18
#define VFD_CS         GPIO_NUM_16
#define VFD_RESET      GPIO_NUM_15
#define VFD_FILAMENT   GPIO_NUM_4

extern "C" void app_main() {
    // 初始化 u8g2-idf-adapter
    u8g2_idf_adapter_t context;
    u8g2_idf_adapter_config_t config = U8G2_IDF_ADAPTER_CONFIG_DEFAULT_SPI;
    u8g2_idf_adapter_config_init_spi(&config);
    config.bus.spi.mosi = VFD_MOSI;
    config.bus.spi.clk = VFD_CLOCK;
    config.bus.spi.cs = VFD_CS;
    config.gpio.reset = VFD_RESET;
    // config.frequency = 4167000; // max frequency
    config.frequency = 4100000;
    u8g2_idf_adapter_init(&context, U8G2_IDF_ADAPTER_SPI, &config);

    // 初始化灯丝控制 GPIO
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = BIT(VFD_FILAMENT);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(VFD_FILAMENT, 1);

    // 初始化 u8g2
    u8g2_Setup_gp1294ai_256x48_f(
        &context.u8g2,
        U8G2_R0,
        u8g2_idf_adapter_byte_cb,
        u8g2_idf_adapter_gpio_and_delay_cb
    );

    u8g2_InitDisplay(&context.u8g2);
    u8g2_SetPowerSave(&context.u8g2, 0);
    u8g2_SetFont(&context.u8g2, u8g2_font_7x14_tr);

    // 帧率计算配置
    constexpr int stage_time = 1e6;

    // 帧率计算所需变量
    int frame_count_in_stage = 0;
    int last_stage_number = 0;
    uint64_t last_stage_time = 0;
    double fps = 0;

    // 主循环
    while (true) {
        // 获取运行时长
        const auto startup_time = esp_timer_get_time();
        // 清空缓冲区
        u8g2_ClearBuffer(&context.u8g2);

        // 计算帧率
        if (const int current_stage = startup_time / stage_time; current_stage > last_stage_number) {
            fps = frame_count_in_stage / ((startup_time - last_stage_time) / 1e6);
            last_stage_number = current_stage;
            last_stage_time = startup_time;
            frame_count_in_stage = 0;
        }
        frame_count_in_stage++;
        // 绘制帧率
        char fps_buf[32];
        snprintf(fps_buf, sizeof(fps_buf), "FPS: %.3f", fps);
        u8g2_DrawUTF8(&context.u8g2, 10, 20, fps_buf);
        u8g2_DrawUTF8(&context.u8g2, 10, 20, fps_buf);

        // 绘制开机时间
        char time_buf[32];
        snprintf(time_buf, sizeof(time_buf), "%.3fs", startup_time / 1e6);
        const auto content_width = u8g2_GetUTF8Width(&context.u8g2, time_buf);
        u8g2_DrawUTF8(&context.u8g2, 10, 40, "startup time:");
        u8g2_DrawUTF8(&context.u8g2, 256 - 10 - content_width, 40, time_buf);

        // 发送缓冲区
        u8g2_SendBuffer(&context.u8g2);
        // 等待
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}
