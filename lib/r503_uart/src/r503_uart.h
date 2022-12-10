#ifndef __R503_uart__

#define __R503_uart__
#include <stdint.h>


#define R503_HEADER 0xEF01
#define R503_DEF_ADDER 0xFFFFFFFF
#define R503_DEF_PWD 0x00000000

/**
 * @brief r503 confirmation code enum 
 * 
 */
typedef enum {
	CMD_EXEC_COMPLETE = 0x00,							// succes!
	FAIL_TO_RCV,										// fail recieving packet
	NO_FINGER,											// no finger on scanner
	FAIL_TO_ENROLL_FINGER,								// or unsyccessful entry?
	FAIL_TO_GEN_FILE_DISORDERLY_FINGER_IMAGE = 0x06,	// fail to gen over-disorderly fingerprint img
	FAIL_TO_GEN_FILE_LACK_OR_SMALL,						// fail to gen lack of char. point or small image
	NO_MATCH,											// no match
	FAIL_TO_FIND_MATCH,									// fail ot match matching finger in library
	PAGE_ID_OVER_LIB_SIZE,								// pageID beyond finger library
	FAIL_TO_READ_TEMPLATE_OR_TEMP_INVALID,				// error reading template from library or template is invalid
	FAIL_TO_UPLOAD_TEMPLATE,							// error uploading template
	CANT_RECIEVE,										// module can't receive the following data packages
	FAIL_TO_UPLOAD_IMAGE,								// error uploading image
	FAIL_TO_DELETE_TEMPLATE,							// fail to delete the template
	FAIL_TO_CLEAR_FINGER_LIBRARY,						// fail to clear finger library
	WRONG_PASS = 0x13,									// wrong password
	FAIL_TO_GEN_IMG_LACK_OF_VALID_PRIMMARY_IMG = 0x15,	// fail to generate the image lack of valid primary image
	FAIL_TO_WRITE_FLASH = 0x18,							// error writing flash
	NO_DEF_ERROR,										// no definition error
	ADDRESS_CODE_INCORRECT = 0x20,						// 
	PASSWD_MUST_BE_VERIFIED = 0x21,						//
	FP_TEMPLATE_EMPTY,									//
	FP_LIB_EMPY = 0x24,									//
	TIMEOUT = 0x26,										//
	FP_ALREADY_EXIST,									//
	SENS_HW_ERROR = 0x29,								//
	REGISTER_NUMBER_INVALID = 0x1A,						// invalid register number
	INCORRECT_REGISTER_CONFIG = 0x1B,					// incorrect configuration of register
	WRONG_NOTEPAD_PAGE_NUMBER = 0x1C,					// wrong notepad page number
	FAIL_TO_OPERATE_COMM_PORT = 0x1D,					// fail to operate the communication port
	FP_LIB_FULL = 0x1F,
	CMD_ERROR = 0xFC,									// unsupported command
	HW_ERROR = 0xFD,									// hardware error
	FAIL_TO_EXEC_CMD = 0xFE,

	HANDSHAKE_BOOT_GOOD = 0x55,							// proper boot occured
	
	//custom
	INCORRECT_RCV_SUM = 0x1FD,							// bad sum on recieved frame
	FAIL_TO_WRITE = 0x1FE,
	FAIL_TO_READ = 0x1FF
} __r503_confirm_code;

typedef enum {
	BREATHING_LIGHT = 0x01,
	FLASHING_LIGHT,
	LIGHT_ON,
	LIGHT_OFF,
	LIGHT_GRAD_ON,
	LIGHT_GRAD_OFF 
} __r503_aura_control_code;

typedef enum {
	AURA_RED = 0x01,
	AURA_BLUE = 0x02, 
	AURA_PURPLE = 0x03,	// still red
	AURA_GREEN = 0x04,	// green
	AURA_YELLOW = 0x05,
	AURA_CYAN = 0x06,		
	AURA_WHITE = 0x07	// yellow
} __r503_aura_color_index;

// Packet indentifier
typedef enum {			
	PID_CMD = 0x01,		//	Command 
	PID_DATA = 0x02,	//	Data
	PID_ACK = 0x07,		//	Acknowledge
	PID_EOD = 0x08		// End Of Data
} PID;

typedef enum {
	PARAM_BAUD_RATE = 4,
	PARAM_SECURITY_LEVEL = 5,
	PARAM_DATA_MAX_LEN = 6
} r503_sys_param;

typedef struct {
	uint16_t header;
	uint32_t adder;
	uint8_t pid;
	uint16_t len;
	uint8_t* data;
	uint16_t sum;
} __attribute__((__packed__)) __r503_uart_frame;

typedef struct {
	uint16_t status;
	uint16_t indent_code;
	uint16_t f_lib_size;
	uint16_t security_level;
	uint32_t adder;
	uint16_t data_size;
	uint16_t baud_rate;
} r503_system_settings;

typedef struct {
	uint8_t bytes[32];
} __r503_data32;

typedef __r503_data32 __r503_index_page;
typedef __r503_data32 __r503_alg_ver;
typedef __r503_data32 __r503_fw_ver;
typedef __r503_data32 __r503_notepad_content;

typedef struct {
	char PARAM_FPM_MODEL[16];
	char PARAM_BN[4];
	char PARAM_SN[8];
	uint8_t PARAM_HW_VER[2];
	char PARAM_FPS_MODEL[8];
	uint16_t PARAM_FPS_WIDTH;
	uint16_t PARAM_FPS_HEIGHT;
	uint16_t PARAM_TMPL_SIZE;
	uint16_t PARAM_TEMPL_TOTAL;
} __r503_prod_info;


// functions
// int r503_init(uint32_t adder);


// base write and receive functions to be implemented by user

/**
 * @brief write uart frames from data array to be implemented by user
 * 
 * @param data data array
 * @param len lenght of data array
 * @return int 0:good -1:error
 */
int __r503_write(uint8_t* data, uint16_t len);

/**
 * @brief reveive uart frames to data array to be implemented by user
 * 
 * @param data	received data array 
 * @param len 	lenght to be read and lenght of data array
 * @return int 	0:good -1:error
 */
int __r503_read(uint8_t* data, uint16_t len);

/**
 * @brief interface to write frame using __r503_uart_frame struct 
 * 
 * @param frame	frame to be written 
 * @return int 	0:good -1:error 
 */
int __r503_write_frame(__r503_uart_frame frame);

/**
 * @brief interface to read frame using __r503_uart_frame struct
 * 
 * @param frame struct to be read into IMPORTANT fame.data pointer has to be 
 * 				prepared	TODO: better word instead of prepared :D
 * @return int 
 */
int __r503_read_frame(__r503_uart_frame* frame);

// helper functions 

/**
 * @brief generate sum from frame and return it
 * 
 * @param frame __r503_uart_frame
 * @return uint16_t sum
 */
uint16_t __r503_gen_sum(__r503_uart_frame *frame);

/**
 * @brief check if sum of frame is good 
 * 
 * @param frame data frame
 * @return int 0:good 0:error
 */
int __r503_check_sum(__r503_uart_frame frame);

/**
 * @brief generate frame structure from frame parts 
 * 
 * @param adder r503 address 
 * @param pid package identifier
 * @param len lenght of data
 * @param data data array pointer
 * @return __r503_uart_frame generated frame
 */
__r503_uart_frame r503_gen_frame(uint32_t adder, uint8_t pid, uint16_t len, uint8_t* data);

/**
 * @brief convert frame to uint8_t array
 * 
 * @param frame frame to be converted
 * @param data data array to be generated memory allocated before fcn execution
 * @return uint16_t 
 */
uint16_t r503_frame_to_data(__r503_uart_frame frame, uint8_t* data);

/**
 * @brief convert uint8_t array to frame structure
 * 
 * @param data	frame array 
 * @param len 	lenght of frame array
 * @return __r503_uart_frame generated frame
 */
__r503_uart_frame r503_data_to_frame(uint8_t* data, uint16_t len);

/**
 * @brief verify password 
 * 
 * @param adder r503 address 
 * @param pwd password to be verified
 * @return __r503_confirm_code confrmation code
 */
__r503_confirm_code r503_verify_pwd(uint32_t adder, uint32_t pwd);

/**
 * @brief set new password (if forgotten module cannot be reset to factory settings)
 * 
 * @param adder module address
 * @param password new password
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_set_pass(uint32_t adder, uint32_t password);

/**
 * @brief set new address for module
 * 
 * @param adder actual address 
 * @param new_adder new address
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_set_adder(uint32_t adder, uint32_t new_adder);

/**
 * @brief set system parameter
 * 
 * @param adder module address
 * @param param parameter
 * @param paramSetting new parameter setting
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_set_sys_param(uint32_t adder, r503_sys_param param, uint8_t paramSetting);

/**
 * @brief read system parameters as settings object 
 * 
 * @param adder module address
 * @param settings settings object 
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_read_sys_param(uint32_t adder, r503_system_settings* settings);

/**
 * @brief read the current valid templat number of module
 * 
 * @param adder module address
 * @param templN template number
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_read_template_num(uint32_t adder, uint16_t templN);

/**
 * @brief control state of usb port  
 * 
 * @param adder module address 
 * @param state new state of usb port (0/1)
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_control(uint32_t adder, uint8_t state);

/**
 * @brief read template pge of module
 * 
 * @param adder module address
 * @param pageID page index number
 * @param page returned page array
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_read_template_index_table(uint32_t adder, uint8_t pageID, __r503_index_page page);

/**
 * @brief detect finger and store image in ImageBuffer
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_collect_finger_image(uint32_t adder);

/**
 * @brief download image from master to ImageBuffer
 * 
 * @param adder module address
 * @param data image data array
 * @param len lenght of image data array
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_read_finger_image(uint32_t adder, uint8_t* data, uint16_t* len);

/**
 * @brief upload image from ImageBuffer to master 
 * 
 * @param adder module address
 * @param data image data array
 * @param len lenght of image data array
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_write_finger_image(uint32_t adder, uint8_t* image, uint16_t len);

/**
 * @brief generate char file from ImageBuffer
 * 
 * @param adder module address
 * @param bufferID buffer number (1/2)
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_gen_char(uint32_t adder, uint8_t bufferID);

/**
 * @brief generate template from CharBuffer1 and 2 then store it in CharBuffer1 and 2
 * 
 * @param adder module address  
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_gen_template(uint32_t adder);

/**
 * @brief read CharBuffer1 or 2 
 * 
 * @param adder module address
 * @param bufferID CharBufferID (1/2)
 * @param data char data array
 * @param len lenght of char data array
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_read_char(uint32_t adder, uint8_t bufferID, uint8_t* data, uint16_t* len);

/**
 * @brief write CharBuffer1 or 2 
 * 
 * @param adder module address
 * @param bufferID buffer index number (1/2)
 * @param data char data array 
 * @param len lenght of char data array
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_write_char(uint32_t adder, uint8_t bufferID, uint8_t* data, uint8_t len);

/**
 * @brief store template form buffer at designated location
 * 
 * @param adder module address
 * @param bufferID buffer index number (1/2)
 * @param pageID template location
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_store_template(uint32_t adder, uint8_t bufferID, uint16_t pageID);

/**
 * @brief load template from fash to specified buffer 
 * 
 * @param adder module address
 * @param bufferID buffer index number (1/2)
 * @param pageID page number
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_load_char(uint32_t adder, uint8_t bufferID, uint16_t pageID);

/**
 * @brief delete a segment of library
 * 
 * @param adder module address
 * @param pageID page index number 
 * @param len lenght of page to be discarded
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_delete_char(uint32_t adder, uint16_t pageID, uint16_t len);

/**
 * @brief clear template library
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_empty_lib(uint32_t adder);

/**
 * @brief match templates from buffer1 to buffer 2 
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_match(uint32_t adder);

/**
 * @brief search for template in library matching to buffer1 or 2 
 * 
 * @param adder module address
 * @param bufferID buffer index (1/2)
 * @param startPage page to start search
 * @param pageNum TODO: dont exacly know what it should do
 * @param pageID matching tempate's location
 * @param matchScore score of match
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_search(uint32_t adder, uint8_t bufferID, uint16_t startPage, uint16_t pageNum, uint16_t* pageID, uint16_t* matchScore);

/**
 * @brief cancel last instruction
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_cancel(uint32_t adder);

/**
 * @brief	read handshake response (used to check if r504 turned on properly) 
 * 
 * @param adder address of r503 
 * @return __r503_configrm_code 
 */
__r503_confirm_code r503_read_handshake(uint32_t adder);

/**
 * @brief send handshake
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code r503_handshake(uint32_t adder);

/**
 * @brief check if sensor is normal
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_check_sensor(uint32_t adder);

/**
 * @brief get algorithm library version
 * 
 * @param adder module address
 * @param alg_ver returned alg version
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_get_alg_ver(uint32_t adder, __r503_alg_ver* alg_ver);

/**
 * @brief get firmware version
 * 
 * @param adder module address 
 * @param fw_ver returned fw version
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_get_fw_ver(uint32_t adder, __r503_fw_ver* fw_ver);

/**
 * @brief get product information
 * 
 * @param adder module address
 * @param prod_info returned production info
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_get_prod_info(uint32_t adder, __r503_prod_info* prod_info);

/**
 * @brief soft reset
 * 
 * @param adder module address
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_soft_rst(uint32_t adder);

/**
 * @brief set aura led config
 * 
 * @param adder module address
 * @param controlCode led control code
 * @param speed speed of flashing or breathing
 * @param colorIndex color index
 * @param nCycles number of cycles for breathing or flashing 0:inf 
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_set_aura_led_config(uint32_t adder, __r503_aura_control_code controlCode, uint8_t speed, __r503_aura_color_index colorIndex, uint8_t nCycles);

/**
 * @brief get random hardware generated number from module
 * 
 * @param adder module addres
 * @param randNumber returned number
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_get_random_number(uint32_t adder, uint32_t randNumber);

/**
 * @brief read information page 521 bytes
 * 
 * @param adder module address
 * @param infoPage returned info page
 * @param len length of info page
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_read_info_page(uint32_t adder, uint8_t* infoPage, uint16_t* len);

/**
 * @brief write data to notepad
 * 
 * @param adder module address
 * @param pageID page number (0:15)
 * @param notepadContent new notepad content (32 bytes)
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_write_notepad(uint32_t adder, uint8_t pageID, __r503_notepad_content notepadContent);

/**
 * @brief read notepad page
 * 
 * @param adder module address
 * @param pageID page number (0:15)
 * @param notepadContent retured content (32bytes)
 * @return __r503_confirm_code 
 */
__r503_confirm_code __r503_read_notepad(uint32_t adder, uint8_t pageID, __r503_notepad_content* notepadContent);




#endif
