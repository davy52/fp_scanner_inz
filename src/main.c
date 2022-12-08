#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "driver/uart.h"

#include <string.h>

#include "r503_uart.h"

const uart_port_t uart_num = UART_NUM_1;
uart_config_t uart_config = {
	.baud_rate = 9600,
	.data_bits = UART_DATA_8_BITS,
	.parity = UART_PARITY_DISABLE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	.rx_flow_ctrl_thresh = 0,
};
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;

void uart_init()
{
	static const char* TAG = "[UART_INIT]";
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, 10, 9, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
	ESP_LOGI(TAG, "Init Complete");
}

int __r503_write(uint8_t* data, uint16_t len)
{
	return uart_write_bytes(uart_num, data, len);
}

int __r503_read(uint8_t* data, uint16_t len)
{
	return uart_read_bytes(uart_num, data, len, 100/portTICK_PERIOD_MS);
}

void app_main() {
	static const char* TAG = "[APP_MAIN]";
	uart_init();
	
	ESP_LOGI(TAG, "WAITING 1s");
	vTaskDelay(500/portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "WRITING");
	r503_control(0x12322332, 0x22);

}
