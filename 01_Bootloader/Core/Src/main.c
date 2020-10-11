/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

//enable this line to get debug messages over debug uart
#define BL_DEBUG_MSG_EN

CRC_HandleTypeDef hcrc;
ETH_HandleTypeDef heth;
PCD_HandleTypeDef hpcd_USB_OTG_FS;

UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

uint8_t supported_commands[] = {
								BL_GET_VER ,
                                BL_GET_HELP,
                                BL_GET_CID,
                                BL_GET_RDP_STATUS,
                                BL_GO_TO_ADDR,
                                BL_FLASH_ERASE,
                                BL_MEM_WRITE,
                                BL_READ_SECTOR_P_STATUS} ;

#define D_UART   &huart6
#define C_UART   &huart3

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_CRC_Init(void);
static void MX_USART6_UART_Init(void);
static void printmsg(char *format,...);

char somedata[] = "Hello from Bootloader\r\n";

#define BL_RX_LEN  200
uint8_t bl_rx_buffer[BL_RX_LEN];

int main(void){

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_ETH_Init();
	MX_USART3_UART_Init();
	MX_USB_OTG_FS_PCD_Init();
	MX_CRC_Init();
	MX_USART6_UART_Init();

//	while(1){
//		/* USER CODE END WHILE */
//		uint32_t current_tick = HAL_GetTick();
//		printmsg("current_tick = %d\r\n", current_tick);
//		while (HAL_GetTick() <= (current_tick + 500));
//
//	}
	if(HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET){
		printmsg("BL_DEBUG_MSG:Button is pressed .. going to BL mode\n\r");

		//we should continue in bootloader mode
		bootloader_uart_read_data();
	} else{
		printmsg("BL_DEBUG_MSG:Button is not pressed .. executing user app\n");

		//jump to user application
		bootloader_jump_to_user_app();
	}
}

void bootloader_uart_read_data(void){

	uint8_t rcv_len = 0;

	while(1){
		memset(bl_rx_buffer, 0, 200);
		//here we will read and decode the commands coming from host
		//first read only one byte from the host, which is the "length" field of the command packet
		HAL_UART_Receive(C_UART, bl_rx_buffer, 1, HAL_MAX_DELAY);
		rcv_len = bl_rx_buffer[0];
		HAL_UART_Receive(C_UART, &bl_rx_buffer[1], rcv_len, HAL_MAX_DELAY);

		switch(bl_rx_buffer[1]){
			case BL_GET_VER:
				bootloader_handle_getver_cmd(bl_rx_buffer);
				break;
			case BL_GET_HELP:
				bootloader_handle_gethelp_cmd(bl_rx_buffer);
				break;
			case BL_GET_CID:
				bootloader_handle_getcid_cmd(bl_rx_buffer);
				break;
			case BL_GET_RDP_STATUS:
				bootloader_handle_getrdp_cmd(bl_rx_buffer);
				break;
			case BL_GO_TO_ADDR:
				//bootloader_handle_go_cmd(bl_rx_buffer);
				break;
			case BL_FLASH_ERASE:
				//bootloader_handle_flash_erase_cmd(bl_rx_buffer);
				break;
			case BL_MEM_WRITE:
				//bootloader_handle_mem_write_cmd(bl_rx_buffer);
				break;
			case BL_EN_RW_PROTECT:
				//bootloader_handle_en_rw_protect(bl_rx_buffer);
				break;
			case BL_MEM_READ:
				//bootloader_handle_mem_read(bl_rx_buffer);
				break;
			case BL_READ_SECTOR_P_STATUS:
				//bootloader_handle_read_sector_protection_status(bl_rx_buffer);
				break;
			case BL_OTP_READ:
				//bootloader_handle_read_otp(bl_rx_buffer);
				break;
			case BL_DIS_R_W_PROTECT:
				//bootloader_handle_dis_rw_protect(bl_rx_buffer);
				break;
			default:
				printmsg("BL_DEBUG_MSG:Invalid command code received from host \n");
				break;
		}
	}
}

/* Code to jump to user application
 * Here we are assuming FLASH_SECTOR2_BASE_ADDRESS
 * is where the user application is stored
 */
void bootloader_jump_to_user_app(void){

	//just a function pointer to hold the address of the reset handler of the user app.
	void (*app_reset_handler)(void);

	printmsg("BL_DEBUG_MSG:bootloader_jump_to_user_app\n");

	// 1. configure the MSP by reading the value from the base address of the sector 1
	uint32_t msp_value = *(volatile uint32_t *)FLASH_SECTOR1_BASE_ADDRESS;
	printmsg("BL_DEBUG_MSG:MSP value : %#x\n", msp_value);

	//This function comes from CMSIS.
	__set_MSP(msp_value);

	//SCB->VTOR = FLASH_SECTOR1_BASE_ADDRESS;

	/* 2. Now fetch the reset handler address of the user application
	 * from the location FLASH_SECTOR2_BASE_ADDRESS+4
	 */
	uint32_t resethandler_address = *(volatile uint32_t*)(FLASH_SECTOR1_BASE_ADDRESS + 4);

	app_reset_handler = (void*) resethandler_address;

	printmsg("BL_DEBUG_MSG: app reset handler addr : %#x\n", app_reset_handler);

	//3. jump to reset handler of the user application
	app_reset_handler();
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void){
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 96;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
		Error_Handler();
	}
	/** Activate the Over-Drive mode
	 */
	if(HAL_PWREx_EnableOverDrive() != HAL_OK){
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
								| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK){
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3
			| RCC_PERIPHCLK_USART6 | RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
	PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void){

	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
	if (HAL_CRC_Init(&hcrc) != HAL_OK){
		Error_Handler();
	}
}

/**
 * @brief ETH Initialization Function
 * @param None
 * @retval None
 */
static void MX_ETH_Init(void){

	heth.Instance = ETH;
	heth.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
	heth.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
	heth.Init.MACAddr[0] = 0x00;
	heth.Init.MACAddr[1] = 0x80;
	heth.Init.MACAddr[2] = 0xE1;
	heth.Init.MACAddr[3] = 0x00;
	heth.Init.MACAddr[4] = 0x00;
	heth.Init.MACAddr[5] = 0x00;
	heth.Init.RxMode = ETH_RXPOLLING_MODE;
	heth.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
	heth.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

	if(HAL_ETH_Init(&heth) != HAL_OK){
		Error_Handler();
	}
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void){

	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if(HAL_UART_Init(&huart3) != HAL_OK){
		Error_Handler();
	}
}

/**
 * @brief USART6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART6_UART_Init(void){

	huart6.Instance = USART6;
	huart6.Init.BaudRate = 115200;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Parity = UART_PARITY_NONE;
	huart6.Init.Mode = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
	huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart6) != HAL_OK){
		Error_Handler();
	}
}

/**
 * @brief USB_OTG_FS Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_OTG_FS_PCD_Init(void){

	hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
	hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
	hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
	hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
	hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
	hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
	hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
	if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void){
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LD1_Pin | LD3_Pin | LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : USER_Btn_Pin */
	GPIO_InitStruct.Pin = USER_Btn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
	GPIO_InitStruct.Pin = LD1_Pin | LD3_Pin | LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : USB_PowerSwitchOn_Pin */
	GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : USB_OverCurrent_Pin */
	GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void){
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

/*Prints formatted string to console over UART */
void printmsg(char *format, ...){

#ifdef BL_DEBUG_MSG_EN
	char str[80];
	/*Extract the the argument list using VA apis */
	va_list args;
	va_start(args, format);
	vsprintf(str, format, args);
	HAL_UART_Transmit(D_UART, (uint8_t*) str, strlen(str), HAL_MAX_DELAY);
	va_end(args);
#endif

}

/**************Implementation of Boot-loader Command Handle functions *********/

/*Helper function to handle BL_GET_VER command */
void bootloader_handle_getver_cmd(uint8_t *bl_rx_buffer){

	uint8_t bl_version;

	// 1) verify the checksum
	printmsg("BL_DEBUG_MSG:bootloader_handle_getver_cmd\n");

	// Total length of the command packet
	uint32_t command_packet_len = bl_rx_buffer[0]+1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *)(bl_rx_buffer+command_packet_len - 4));

	if(!bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc)){

		printmsg("BL_DEBUG_MSG:checksum success !!\n");
		// checksum is correct..
		bootloader_send_ack(bl_rx_buffer[0], 1); // send 'ACK'
		bl_version = get_bootloader_version();	 // Obtain reply
		printmsg("BL_DEBUG_MSG:BL_VER : %d %#x\n", bl_version, bl_version);
		bootloader_uart_write_data(&bl_version, 1); // Send reply

	} else{
		printmsg("BL_DEBUG_MSG:checksum fail !!\n");
		//checksum is wrong send nack
		bootloader_send_nack();
	}
}

/* Helper function to handle BL_GET_HELP command
 * Bootloader sends out All supported Command codes
 */
void bootloader_handle_gethelp_cmd(uint8_t *pBuffer){
	//Total length of the command packet
	uint32_t command_packet_len = bl_rx_buffer[0] + 1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *)(bl_rx_buffer + command_packet_len - 4));

	if(!bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc)){

		printmsg("BL_DEBUG_MSG:checksum success !!\n");
		bootloader_send_ack(pBuffer[0], sizeof(supported_commands)); // send 'ACK'
		bootloader_uart_write_data(supported_commands, sizeof(supported_commands)); // Send reply

	} else{
		printmsg("BL_DEBUG_MSG:checksum fail !!\n");
		//checksum is wrong send nack
		bootloader_send_nack();
	}
}

/*Helper function to handle BL_GET_CID command */
void bootloader_handle_getcid_cmd(uint8_t *pBuffer){

	uint16_t bl_cid_num = 0;
	printmsg("BL_DEBUG_MSG:bootloader_handle_getcid_cmd\n");

	//Total length of the command packet
	uint32_t command_packet_len = bl_rx_buffer[0]+1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t*) (bl_rx_buffer + command_packet_len - 4));

	if(!bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc)){
		printmsg("BL_DEBUG_MSG:checksum success !!\n");
		bootloader_send_ack(pBuffer[0], 2);	// send 'ACK'
		bl_cid_num = get_mcu_chip_id(); // Obtain reply
		printmsg("BL_DEBUG_MSG:MCU id : %d %#x !!\n", bl_cid_num, bl_cid_num);
		bootloader_uart_write_data((uint8_t*) &bl_cid_num, 2); // Send reply

	} else{
		printmsg("BL_DEBUG_MSG:checksum fail !!\n");
		bootloader_send_nack();
	}
}

/*Helper function to handle BL_GET_RDP_STATUS command */
void bootloader_handle_getrdp_cmd(uint8_t *pBuffer){

	uint8_t rdp_level = 0x00;
	printmsg("BL_DEBUG_MSG:bootloader_handle_getrdp_cmd\n");

	//Total length of the command packet
	uint32_t command_packet_len = bl_rx_buffer[0] + 1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t*) (bl_rx_buffer + command_packet_len - 4));

	if(!bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc)){
		printmsg("BL_DEBUG_MSG:checksum success !!\n");
		bootloader_send_ack(pBuffer[0], 1);
		rdp_level = get_flash_rdp_level();
		printmsg("BL_DEBUG_MSG:RDP level: %d %#x\n", rdp_level, rdp_level);
		bootloader_uart_write_data(&rdp_level, 1);

	} else{
		printmsg("BL_DEBUG_MSG:checksum fail !!\n");
		bootloader_send_nack();
	}
}

/*This function sends ACK if CRC matches along with "len to follow"*/
void bootloader_send_ack(uint8_t command_code, uint8_t follow_len){
	//here we send 2 bytes... first byte is ack and the second byte is len value
	uint8_t ack_buf[2];
	ack_buf[0] = BL_ACK;
	ack_buf[1] = follow_len;
	HAL_UART_Transmit(C_UART, ack_buf, 2, HAL_MAX_DELAY);
}

/*This function sends NACK */
void bootloader_send_nack(void){
	uint8_t nack = BL_NACK;
	HAL_UART_Transmit(C_UART, &nack, 1, HAL_MAX_DELAY);
}

//This verifies the CRC of the given buffer in pData
uint8_t bootloader_verify_crc(uint8_t *pData, uint32_t len, uint32_t crc_host){
	uint32_t uwCRCValue = 0xFF;

	for(uint32_t i = 0; i < len; i++){
		uint32_t i_data = pData[i];
		uwCRCValue = HAL_CRC_Accumulate(&hcrc, &i_data, 1);
	}

	/* Reset CRC Calculation Unit */
	__HAL_CRC_DR_RESET(&hcrc);

	if(uwCRCValue == crc_host){
		return VERIFY_CRC_SUCCESS;
	}

	return VERIFY_CRC_FAIL;

}

/* This function writes data in to C_UART */
void bootloader_uart_write_data(uint8_t *pBuffer, uint32_t len){
    /*you can replace the below ST's USART driver API call with your MCUs driver API call */
	HAL_UART_Transmit(C_UART, pBuffer, len, HAL_MAX_DELAY);

}

//Just returns the macro value
uint8_t get_bootloader_version(void){
  return (uint8_t)BL_VERSION;
}

//Read the chip identifier or device Identifier
uint16_t get_mcu_chip_id(void){
/*
	The STM32F76xxx and STM32F77xxx MCUs integrate an MCU ID code. This ID identifies the ST MCU part-number and the die
	revision. It is part of the DBG_MCU component and is mapped on the external PPB bus (see Section 44.16 on page 1925).
	This code is accessible using the JTAG debug port (4 to 5 pins) or the SW debug port (two pins) or by the user
	software. It is even accessible while the MCU is under system reset. */

	uint16_t cid;
	cid = (uint16_t)(DBGMCU->IDCODE) & 0x0FFF; // convert to a 16-bit unsigned int
	return cid;

}

/*This function reads the RDP (Read protection option byte) value
 *For more info refer 3.5.1 section in stm32f767xxx
 */
uint8_t get_flash_rdp_level(void){

/* Memory read protection Level 2 is an irreversible operation. When Level 2 is activated,
 * the level of protection cannot be decreased to Level 0 or Level 1. Be careful with this.
 * */

	uint8_t rdp_status = 0;
#if 0
	FLASH_OBProgramInitTypeDef  ob_handle;
	HAL_FLASHEx_OBGetConfig(&ob_handle);
	rdp_status = (uint8_t)ob_handle.RDPLevel;
#else

	volatile uint32_t *pOB_addr = (uint32_t*) 0x1FFF0000;
	rdp_status = (uint8_t)(*pOB_addr >> 8); // get only 1 byte
#endif

	return rdp_status;

}
