#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"
#include "esp_int_wdt.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/queue.h"

#include "driver/uart.h"

#include <string.h>

#include "r503_uart.h"



//#define BIT(x) 1<<x

//	r503 pins
#define R503_POWER 13
#define TX 16
#define RX 17
#define WAKEUP_PIN 18
#define BUTTON_PIN 14

#define READ_TIMEOUT 5000

int button_flag = 0;

// r503 setup
uint32_t r503_adder = 0xFAFAFAFA;

r503_system_settings r503_sys_params = {
	.baud_rate = 6,			// 57600
	.security_level = 5,	// hgihest
	.data_size = 3
};

xQueueHandle intrQ;

const uart_port_t uart_num = UART_NUM_1;
uart_config_t uart_config = {
	.baud_rate = 57600,
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
	ESP_ERROR_CHECK(uart_set_pin(uart_num, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
	ESP_LOGI(TAG, "Init Complete");
}


int __r503_write(uint8_t* data, uint16_t len)
{
	//ESP_LOGE("WIRE", "code %u", data[9]);
	int ret = uart_write_bytes(uart_num, data, len);
	//vTaskDelay(10/portTICK_PERIOD_MS);
	//uart_flush_input(uart_num);
	return ret;
}

 int __r503_read(uint8_t* data, uint16_t len)
 {
	static const char* TAG= "[READ]";

	int ret = uart_read_bytes(uart_num, data, len, READ_TIMEOUT/portTICK_PERIOD_MS);
	return ret;
 }
// {
// 	static const char* TAG = "[READ]";
// 	//ESP_LOGI(TAG, "READ FRAME");
// 	//ESP_LOGI(TAG, "data*: %p \tlen: %d", data, len);
// 	int ret = 0;
// 	do
// 	{
// 		ret = uart_read_bytes(uart_num, data, 1, READ_TIMEOUT/portTICK_PERIOD_MS);
// 		ESP_LOGW(TAG, "data: %X", data[0]);
// 	} while (data[0] == 0xFF);	//random 0xFF
	
// 	if(len > 1){
// 		ESP_LOGI(TAG, "data longer");
// 		ret = uart_read_bytes(uart_num, data + 1, len - 1, READ_TIMEOUT/portTICK_PERIOD_MS);
// 		ret++;
// 	}
// 	ESP_LOGW(TAG, "READ WHOLE MESSAGE");
// 	// ESP_LOGW(TAG, "DATA:");
// 	// for(int i = 0; i < len; i++){
// 	// 	ESP_LOGW(TAG, "DATA%d: %X", i, data[i]);
// 	// }
// 	return ret;
// }

// mask bit pos: 1-baudrate 2-data_len 3-sec_level
int r503_check_params(r503_system_settings params1, r503_system_settings params2, uint8_t check_mask)
{
	int ret = 0;

	if(check_mask & 1<<1)
		if(params1.baud_rate != params2.baud_rate)
			ret |= 1<<1;
	
	if(check_mask & 1<<2)
		if(params1.data_size != params2.data_size)
			ret |= 1<<2;

	if(check_mask & 1<<3)
		if(params1.security_level != params2.security_level)
			ret |= 1<<3;
	
	return ret;
}

int r503_init(uint32_t adder)
{
	static const char* TAG = "[503 INIT]";
	ESP_LOGI(TAG, "START");

	__r503_confirm_code ret_code = 0;

	vTaskDelay(500/portTICK_PERIOD_MS);
	//uart_flush_input(uart_num);
	ESP_ERROR_CHECK(gpio_set_level(R503_POWER, 1));
	//vTaskDelay(15/portTICK_PERIOD_MS);
	//uart_flush_input(uart_num);

	ESP_LOGW(TAG, "Reading boot handshake");
	ret_code = r503_read_handshake(R503_DEF_ADDER);
	if(ret_code != HANDSHAKE_BOOT_GOOD){
		ESP_LOGE(TAG, "bad boot handshake \tcode: %X", ret_code);
		//return -1;
	}

	vTaskDelay(500/portTICK_PERIOD_MS);
	uart_flush_input(uart_num);	

	ESP_LOGW(TAG, "Veryfying def passwd");
	ret_code = r503_verify_pwd(R503_DEF_ADDER, R503_DEF_PWD);
	if(ret_code){
		ESP_LOGE(TAG, "Veryfying passwd \t|code: %X", ret_code);
		return -1;
	}else{
		ESP_LOGI(TAG, "PASSWD verified");
	}

	ESP_LOGW(TAG, "setting adder");
	ret_code = r503_set_adder(R503_DEF_ADDER, adder);
	if(ret_code){
		ESP_LOGE(TAG, "Error sendig ADDER \t|code: %X", ret_code);
		return -1;
	}else{
		ESP_LOGI(TAG, "Address changed to: %X", adder);
	}

	ret_code = r503_set_sys_param(adder, PARAM_BAUD_RATE, r503_sys_params.baud_rate);
	if(ret_code){
		ESP_LOGE(TAG, "Error setting baudrate \t|code: %X", ret_code);
		return -1;
	}

	ret_code = r503_set_sys_param(adder, PARAM_DATA_MAX_LEN, r503_sys_params.data_size);
	if(ret_code){
		ESP_LOGE(TAG, "Error setting data len \t|code: %X", ret_code);
		return -1;
	}

	ret_code = r503_set_sys_param(adder, PARAM_SECURITY_LEVEL, r503_sys_params.security_level);
	if(ret_code){
		ESP_LOGE(TAG, "Error setting security lever \t|code: %X", ret_code);
		return -1;
	}
	
	r503_system_settings temp;
	ret_code = r503_read_sys_param(adder, &temp);
	if(ret_code){
		ESP_LOGE(TAG, "Error reading sys params \t|code: %X", ret_code);
		return -1;
	}

	if(r503_check_params(r503_sys_params, temp, 0b111)){
		ESP_LOGE(TAG, "Error checking params");
		return -1;
	}
	r503_sys_params = temp;
	ESP_LOGI(TAG, "status: %X", temp.status);
	ESP_LOGI(TAG, "indent: %X", temp.indent_code);
	ESP_LOGI(TAG, "libsize: %d", temp.f_lib_size);
	ESP_LOGI(TAG, "sec level: %d", temp.security_level);
	ESP_LOGI(TAG, "adder: %X", temp.adder);
	ESP_LOGI(TAG, "data size: %d", temp.data_size);
	ESP_LOGI(TAG, "baudrate: %d", temp.baud_rate);

	ret_code = __r503_check_sensor(adder);
	if(ret_code){
		ESP_LOGE(TAG, "Error sensor check \t|code: %X", ret_code);
		return -1;
	}

	// ESP_ERROR_CHECK(gpio_set_intr_type(WAKEUP_PIN, GPIO_INTR_NEGEDGE));
	// ESP_ERROR_CHECK(gpio_intr_enable(WAKEUP_PIN));

	// gpio_set_level(R503_POWER, 0);
	
	return 0;
}

void r503_pin_init(){
	static const char*  TAG = "R503 PIN INIT";

	gpio_config_t conf = {
		.pin_bit_mask = BIT(R503_POWER),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&conf);

	// gpio_reset_pin(R503_POWER);
	// gpio_set_intr_type(R503_POWER, GPIO_INTR_DISABLE);
	// gpio_intr_disable(R503_POWER);
	// gpio_set_direction(R503_POWER, GPIO_MODE_OUTPUT);
	// gpio_set_pull_mode(R503_POWER, GPIO_FLOATING);
	// gpio_set_level(R503_POWER, 0);


	

	
	conf.pin_bit_mask = BIT(WAKEUP_PIN);
	conf.mode = GPIO_MODE_INPUT;
	conf.pull_up_en = GPIO_PULLUP_DISABLE;
	conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
	gpio_config(&conf);

	// gpio_reset_pin(WAKEUP_PIN);
	// gpio_set_intr_type(WAKEUP_PIN, GPIO_INTR_NEGEDGE);
	// gpio_intr_enable(WAKEUP_PIN);
	// gpio_set_direction(WAKEUP_PIN, GPIO_MODE_INPUT);
	// gpio_set_pull_mode(WAKEUP_PIN, GPIO_FLOATING);
	

	conf.pin_bit_mask = BIT(BUTTON_PIN);
	conf.mode = GPIO_MODE_INPUT;
	conf.pull_up_en = GPIO_PULLUP_ENABLE;
	conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
	gpio_config(&conf);
}

static void IRAM_ATTR wakeup_isr(void *args)
{
	int pin = (int)args;
	if(pin == BUTTON_PIN){
		if(button_flag == 0){
			button_flag = 1;
		}else{
			return;
		}
	}
	xQueueSendFromISR(intrQ, &pin, NULL);
}

void main_task(void *params)
{
	static const char* TAG = "MAIN TASK";
	esp_task_wdt_delete(NULL);
	vTaskDelay(500/portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "START");
	
	int flag;
	__r503_set_aura_led_config(r503_adder, LIGHT_ON, 0, AURA_YELLOW, 0);
	vTaskDelay(10/portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "while START");
	__r503_aura_color_index color = AURA_RED;
	while(1){
		ESP_LOGW(TAG, "LOOP");
		if (xQueueReceive(intrQ, &flag, portMAX_DELAY)){
			switch (flag)
			{
			case WAKEUP_PIN:
				ESP_LOGI(TAG, "FINGER!!!");
				
				// ESP_LOGE(TAG, "aura");
				__r503_set_aura_led_config(r503_adder, BREATHING_LIGHT, 0x0f, AURA_GREEN, 5);
				vTaskDelay(300*5/portTICK_PERIOD_MS);
				// ESP_LOGE(TAG, "aura2");
				__r503_set_aura_led_config(r503_adder, LIGHT_ON, 0, color, 0);
				ESP_LOGE(TAG, "continue \t|color: %X", color);
				color++;
				if(color > 0x07)
					color = 0x01;
				xQueueReset(intrQ);
				break;

			case BUTTON_PIN:
				ESP_LOGI(TAG, "ENROLL START");
				vTaskDelay(100/portTICK_PERIOD_MS);
				__r503_set_aura_led_config(r503_adder, BREATHING_LIGHT, 0x0f, AURA_CYAN, 0);
				xQueueReset(intrQ);
				button_flag = 0;
				if(!xQueueReceive(intrQ, &flag, 5000/portTICK_PERIOD_MS)){	//wait 5s for finger
					ESP_LOGE(TAG, "Failed to find finger");
					break;	// break if no item in queue
				}else{
					if(flag != WAKEUP_PIN){
						ESP_LOGE(TAG, "bad flag");
						break;	// break out of routine if button pressed again
					}
				}
				ESP_LOGE(TAG, "after Q");
				// finger detected - scan and save to finger library
				__r503_confirm_code ret = __r503_auto_enroll(r503_adder, 0x01, true, true, true, true);
				if(ret == 0x00){
					ESP_LOGI(TAG, "Registration Complete! \t|code: %X", ret);
				}else{
					ESP_LOGE(TAG, "Enroll fail \t|code: %X", ret);
				}
				__r503_set_aura_led_config(r503_adder, LIGHT_ON, 0, color, 0);
				xQueueReset(intrQ);		// clear queue - multple interrupts during enroll
				ESP_LOGW(TAG, "END OF ENROLL");
				break;
				
			default:
				ESP_LOGE(TAG, "Queue ERROR - bad value");
				break;
			}
		}
	}
}


void app_main() 
{
	static const char* TAG = "[APP_MAIN - SETTUP]";

	esp_task_wdt_delete(NULL);
	ESP_LOGI(TAG, "INIT START");

	//vTaskDelay(5000/portTICK_PERIOD_MS);
	r503_pin_init();
	uart_init();

	// gpio_set_level(R503_POWER, 1);
	// vTaskDelay(500/portTICK_PERIOD_MS);
	// uart_flush_input(uart_num);
	// r503_search();

	r503_init(r503_adder);	


	ESP_LOGI(TAG, "INIT FINISHED");
	ESP_LOGI(TAG, "Setting up MAIN TASK and ISR");

	intrQ = xQueueCreate(10, sizeof(int));
	xTaskCreatePinnedToCore(main_task, "MAIN TASK", 4096, NULL, 10, NULL, tskNO_AFFINITY);

	ESP_LOGI(TAG, "SETTING INTERRUPTS");
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	ESP_ERROR_CHECK(gpio_isr_handler_add(WAKEUP_PIN, wakeup_isr, (void*)WAKEUP_PIN));
	ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_PIN, wakeup_isr, (void*)BUTTON_PIN));
	ESP_LOGW(TAG, "FINISHED");
}
