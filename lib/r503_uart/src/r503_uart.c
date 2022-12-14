#include "r503_uart.h"
#include <string.h>

//test
#include "esp_log.h"
//test


// functions

int __r503_write_frame(__r503_uart_frame frame)
{
	uint8_t data[48] = {
		frame.header >> 8,
		frame.header,
		frame.adder >> 24,
		frame.adder >> 16,
		frame.adder >> 8,
		frame.adder,
		frame.pid,
		frame.len >> 8,
		frame.len,
		0,
	};
	for(int i = 0; i < frame.len - 2; i++){
		data[i+9] = frame.data[i];
	}
	data[8 + frame.len - 1] = frame.sum >> 8;
	data[8 + frame.len] = frame.sum;

	return __r503_write(data, 9 + frame.len);
}

int __r503_read_frame(__r503_uart_frame* frame)
{

	uint8_t data_rcv[256] = {0};
	int ret = __r503_read(data_rcv, 9 + frame->len);
	
	frame->header = data_rcv[0] << 8 | data_rcv[0];
	frame->adder = data_rcv[2] << 24 | data_rcv[3] << 16 | data_rcv[4] << 8 | data_rcv[5];
	frame->pid = data_rcv[6];
	frame->len = data_rcv[7] << 8 | data_rcv[8];
	for(int i = 0, j = 9; i < frame->len - 2; i++, j++){
		frame->data[i] = data_rcv[j];
	}
	frame->sum = data_rcv[8+frame->len-1] << 8 | data_rcv[8+frame->len];
	// memcpy(frame, data_rcv, 9);
	// memcpy(frame->data, data_rcv, frame->len - 2);
	// memcpy(&frame->sum, data_rcv + (9 + frame->len -2), 2);

	return ret;
}

// helper functions 
uint16_t __r503_gen_sum(__r503_uart_frame *frame)
{
	uint32_t sum = 0;
	sum += frame->pid;
	sum += frame->len;
	
	for (int i = 0; i < frame->len - 2; i++){
		sum += frame->data[i];
	}

	frame->sum = (uint16_t)sum;
	return 0;
}

int __r503_check_sum(__r503_uart_frame frame)
{
	__r503_uart_frame newFrame = frame;
	if(__r503_gen_sum(&newFrame)){
		return -1;
	} 

	if(newFrame.sum == frame.sum){
		return 0;
	}

	return 1;

}

__r503_uart_frame r503_gen_frame(uint32_t adder, uint8_t pid, uint16_t len, uint8_t* data)
{
	__r503_uart_frame frame = {0};
	frame.header = R503_HEADER;
	frame.adder = adder;
	frame.pid = pid;
	frame.len = len;
	frame.data = data;


	__r503_gen_sum(&frame);	//possible problems if sum should eq 0 	TODO: check if checksum can ever be 0
	if(frame.sum != 0){
		return frame;
	}

	__r503_uart_frame frame_error = {0};
	return frame_error;
}

__r503_confirm_code r503_verify_pwd(uint32_t adder, uint32_t pwd)
{
	uint8_t data[5] = {
		0x13,
		(uint8_t)(pwd >> 24),
		(uint8_t)(pwd >> 16),
		(uint8_t)(pwd >> 8),
		(uint8_t)(pwd >> 0)
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x07, data);


	if(__r503_write_frame(frame) != 16){
		return FAIL_TO_WRITE;
	}

	__r503_uart_frame rcvFrame;
	uint8_t rcvData[256];
	rcvFrame.data = &rcvData;
	rcvFrame.len = 0x03;

	if(__r503_read_frame(&rcvFrame) != 12){
		return FAIL_TO_READ;
	}
	return rcvFrame.data[0];
}

__r503_confirm_code r503_set_pass(uint32_t adder, uint32_t pwd)
{
		uint8_t data[5] = {
		0x12,
		(uint8_t)(pwd >> 24),
		(uint8_t)(pwd >> 16),
		(uint8_t)(pwd >> 8),
		(uint8_t)(pwd >> 0)
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x07, data);

	if(__r503_write_frame(frame)){
		return FAIL_TO_WRITE;
	}

	frame.len = 0x03;

	if(__r503_read_frame(&frame)){
		return FAIL_TO_READ;
	}

	return *(frame.data);
}

__r503_confirm_code r503_set_adder(uint32_t adder, uint32_t new_adder)
{
	uint8_t data[5] = {
		0x15,
		(uint8_t)(new_adder >> 24),
		(uint8_t)(new_adder >> 16),
		(uint8_t)(new_adder >> 8),
		(uint8_t)(new_adder >> 0)
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x07, data);
	if(__r503_write_frame(frame) < 0){
		return FAIL_TO_WRITE;
	}

	uint8_t rcvData = 0;
	frame.data = &rcvData;
	frame.len = 0x03;
	
	if(__r503_read_frame(&frame) < 0){
		return FAIL_TO_READ;
	}

	return frame.data[0];
}

__r503_confirm_code r503_set_sys_param(uint32_t adder, r503_sys_param param, uint8_t paramSetting)
{
	uint8_t data[3] = {
		0x0E,
		param,
		paramSetting
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x05, data);

	if(__r503_write_frame(frame) != 14){
		return FAIL_TO_WRITE;
	}

	uint8_t rcvData = 0;
	frame.data = &rcvData;
	frame.len = 0x03;
	
	if(__r503_read_frame(&frame) != 12){
		return FAIL_TO_READ;
	}

	return *(frame.data);
}

__r503_confirm_code r503_control(uint32_t adder, uint8_t state)
{
	uint8_t data[2] = {
		0x17,
		state
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x04, data);

	if(__r503_write_frame(frame)){
		return FAIL_TO_WRITE;
	}

	uint8_t rcvData = 0;
	frame.data = &rcvData;
	frame.len = 0x03;

	if(__r503_read_frame(&frame)){
		return FAIL_TO_READ;
	}

	return *(frame.data);
}

__r503_confirm_code r503_read_sys_param(uint32_t adder, r503_system_settings* settings)
{
	uint8_t data = 0x0F;

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x03, &data);

	if(__r503_write_frame(frame) < 0){
		return FAIL_TO_WRITE;
	}

	uint8_t rcvData[17] = {0};
	frame.data = rcvData;
	frame.len = 19;
	// TODO: <0 on all reads and writes
	if(__r503_read_frame(&frame) < 0){
		return FAIL_TO_READ;
	}

	settings->status = frame.data[1] << 8 | frame.data[2];
	settings->indent_code = frame.data[3] << 8 | frame.data[4];
	settings->f_lib_size = frame.data[5] << 8 | frame.data[6];
	settings->security_level = frame.data[7] << 8 | frame.data[8];
	settings->adder = frame.data[9] << 24 | frame.data[10] << 16 | frame.data[11] << 8 | frame.data[12];
	settings->data_size = frame.data[13] << 8 | frame.data[14];
	settings->baud_rate = frame.data[15] << 8 | frame.data[16];

	return frame.data[0];
}

/*
__r503_confirm_code r503_read_template_num(uint32_t adder, uint16_t templN);
__r503_confirm_code __r503_collect_finger_image(uint32_t adder);
__r503_confirm_code __r503_upload_image(uint32_t adder, uint8_t* data, uint16_t* len);
__r503_confirm_code r503_read_finger_image(uint32_t adder, uint8_t* data, uint16_t* len);
__r503_confirm_code r503_write_finger_image(uint32_t adder, uint8_t* image, uint16_t len);

__r503_confirm_code __r503_gen_char(uint32_t adder, uint8_t bufferID);
__r503_confirm_code __r503_gen_template(uint32_t adder);
__r503_confirm_code __r503_read_char(uint32_t adder, uint8_t bufferID, uint8_t* data, uint16_t* len);
__r503_confirm_code __r503_write_char(uint32_t adder, uint8_t bufferID, uint8_t* data, uint8_t len);
__r503_confirm_code __r503_store_template(uint32_t adder, uint8_t bufferID, uint16_t pageID);
__r503_confirm_code __r503_load_char(uint32_t adder, uint8_t bufferID, uint16_t pageID);

__r503_confirm_code __r503_delete_char(uint32_t adder, uint16_t pageID, uint16_t len);
__r503_confirm_code __r503_empty_lib(uint32_t adder);
__r503_confirm_code __r503_match(uint32_t adder);
__r503_confirm_code __r503_search(uint32_t adder, uint8_t bufferID, uint16_t startPage, uint16_t pageNum, uint16_t* pageID, uint16_t* matchScore);

__r503_confirm_code __r503_cancel(uint32_t adder);*/

__r503_confirm_code r503_read_handshake(uint32_t adder)
{
	uint8_t rcv_data;
	

	if(__r503_read(&rcv_data, 1) < 0)
		return FAIL_TO_READ;

	return rcv_data;
}
/*
__r503_confirm_code r503_handshake(uint32_t adder);*/
__r503_confirm_code __r503_check_sensor(uint32_t adder){
	uint8_t data = 0x36;
	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x3, &data);

	if(__r503_write_frame(frame) < 0)
		return FAIL_TO_WRITE;
	
	if(__r503_read_frame(&frame) < 0)
		return FAIL_TO_READ;

	return data;
}

/*
__r503_confirm_code __r503_get_alg_ver(uint32_t adder, __r503_alg_ver* alg_ver);
__r503_confirm_code __r503_get_fw_ver(uint32_t adder, __r503_fw_ver* fw_ver);
__r503_confirm_code __r503_get_prod_info(uint32_t adder, __r503_prod_info* prod_info);

__r503_confirm_code __r503_soft_rst(uint32_t adder);*/

__r503_confirm_code __r503_set_aura_led_config(uint32_t adder, __r503_aura_control_code controlCode, uint8_t speed, __r503_aura_color_index colorIndex, uint8_t nCycles){
	uint8_t data[5] = {
		0X35,
		controlCode,
		speed,
		colorIndex,
		nCycles
	};
	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x07, data);

	if(__r503_write_frame(frame) < 0)
		return FAIL_TO_WRITE;
	
	uint8_t data_rcv;
	frame.data = &data_rcv;
	if(__r503_read_frame(&frame) < 0)
		return FAIL_TO_READ;

	if(__r503_check_sum(frame))
		return INCORRECT_RCV_SUM;
	
	return data_rcv;
}

__r503_confirm_code __r503_auto_enroll(uint32_t adder, uint8_t location, uint8_t dup_id, uint8_t dup_fp, uint8_t ret_crit_step, uint8_t finger_leave)
{
	uint8_t data[6] = {
		0x31,
		location, 
		dup_id, 
		dup_fp,
		ret_crit_step,
		finger_leave
	};

	__r503_uart_frame frame = r503_gen_frame(adder, PID_CMD, 0x0008, data);

	int ret = __r503_write_frame(frame);
	if(ret <= 0)
		return FAIL_TO_WRITE;

	frame.len = 0x5;
	for(int i = 0; i <= 0x0F; i++){
		ret = __r503_read_frame(&frame);
		if(ret != 14)
		 	ESP_LOGE("[ENROLL]", "FAIL TO READ \t|read: %d", ret);
			//return FAIL_TO_READ;

		if(data[0] != 0){
			ESP_LOGE("[ENROLL]", "other fail \t|code: %X", data[0]);
			//return data[0];
		}		

		if(data[1] != i){
			ESP_LOGE("[ENROLL]", "step misshap \t|code: %X", data[1]);
			//return FAIL_TO_READ;
		}
	}

	ESP_LOGI("[ENROLL]", "COMPLETE");
	return data[0];
}

/*
__r503_confirm_code __r503_get_random_number(uint32_t adder, uint32_t randNumber);
__r503_confirm_code __r503_read_info_page(uint32_t adder, uint8_t* infoPage, uint16_t* len);

__r503_confirm_code __r503_write_notepad(uint32_t adder, uint8_t pageID, __r503_notepad_content notepadContent);
__r503_confirm_code __r503_read_notepad(uint32_t adder, uint8_t pageID, __r503_notepad_content* notepadContent);


*/
