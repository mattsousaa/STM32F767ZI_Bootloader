# Embedded Bootloader for STM32F767ZI

## What is a Bootloader?

A bootloader is a special program which allows user application program to be updated. The new user application can be obtained using alternative download channels, 
such as a USB stick or a network port. The bootloader and the user application should be written and built as two separate projects resulting in two separate executables. The main tasks of the bootloader are to reprogram/replace the user application, if necessary, and to jump to the user application to execute it. The user application doesn’t necessarily need to know the existence of the bootloader. The bootloader is usually placed at the chips flash base address, so that it will be executed by the CPU after reset. 

## STM32F767ZI Memory Organization
* Internal Flash memory also called as Embedded Flash memory of 2MB
* Internal SRAM1 of 368KB
* Internal SRAM2 of 16KB
* System Memory (ROM) of 60KB
* OTP memory of 1024 bytes
* Option bytes memory of 32 bytes
* Backup RAM of 4KB

## Reset Sequence and Memory Aliasing of the MCU

1. When you reset the MCU, the PC of the processor is loaded with the value *0x0000_0000*.
2. Then processor reads the value @ memory location *0x0000_0000* into MSP (Main stack pointer register). That means, processor first initializes the Stack pointer register.
3. After that, processor reads the value @ memory location *0x0000_0004* in to PC.
4. PC jumps to the reset handler.
5. A reset handler is just a C or Assembly function written by you to carry out any initializations required.
6. From reset handler you call your main function of the application.

## *All ARM Cortex M Based MCUs right after reset does,*
* Load value @ Memory addr. *0x0000_0000* in to MSP.
* Load value @ Memory addr. *0x0000_0004* in to PC (Value = Addr of the reset handler).

## *In STM32 Microcontroller,*
* MSP value stored at *0x0000_0000*.
* Vector table starts from *0x0000_0004*.
* Address of the reset handler found at *0x0000_0004*.

Don't you think we should somehow link *0x0800_0000* to *0x0000_0000*? The answer for this is that both addresses can be linked with the technique called "memory aliasing" and it depends on the MCU. For example, in this case by default the base address of the user flash is mapped onto the base address of the memory map, that is *0x0000_0000*. By default, the user flash is aliased to the very first address of the memory map that is 0. The image below shows this type of configuration.

<p align="center">
	<img src="https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/imagens/flash.png" width="600"/>
</p>

In this project, the STM native bootloader will not be used. We will create our own Bootloader that will be stored in the first sector of the flash memory (Sector 0 - 32KB). **Sector-1** to **Sector-11** will be used for storing user application. For details, refer to the [reference manual](https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/Reference_manual.pdf).  

## Bootloader Code Flow Chart

Whenever we reset the microcontroller the Bootloader code which is stored in the sector 0 will run first. Then the Bootloader code will check the status of the user button. If the user button is pressed during the reset of the microcontroller, then bootloader will execute the function **bootloader_uart_read_data()**. If the user button is not pressed during resetting of the board, then the other path will be executed, that is the bootloader jumps to the user application through function **bootloader_jump_to_user_app()**. The flow chart below shows this behavior.

<p align="center">
	<img src="https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/imagens/flowchart.png" width="700"/>
</p>

The [main.c](https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/01_Bootloader/Core/Src/main.c) file demonstrates the behavior of the Bootloader through the code snippet:

```C
if(HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET){
	printmsg("BL_DEBUG_MSG:Button is pressed .. going to BL mode\n\r");

	//we should continue in bootloader mode
	bootloader_uart_read_data();
} else{
	printmsg("BL_DEBUG_MSG:Button is not pressed .. executing user app\n");

	//jump to user application
	bootloader_jump_to_user_app();
}
```

## Running User Application on 0x0800_8000 Address of the MCU
1. Erase the flash memory of the board with [SMT32Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html).
2. Go to [STM32F767ZITX_FLASH.ld](https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/02_User_app_STM32F7xxx/STM32F767ZITX_FLASH.ld) in **02_User_app_STM32F7xxx** and set:
```C
/* Memories definition */
MEMORY
{
  RAM     (xrw)    : ORIGIN = 0x20000000,   LENGTH = 512K
  FLASH    (rx)    : ORIGIN = 0x08008000,   LENGTH = 2048K
}
```
3. In **02_User_app_STM32F7xxx** go to: \
[startup_stm32f767zitx.s](https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/01_Bootloader/Core/Startup/startup_stm32f767zitx.s) > **SystemInit** > **system_stm32f7xx.c** > **SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET** and relocate the vector table through the Vector Table offset Register (VTOR) editing the line *#define VECT_TAB_OFFSET  0x8000*.
4. After that, save the bootloader application normally at address *0x0800_0000*.

## Supported Bootloader Commands and Host–Bootloader Communication 

The image below demonstrates the communication process between the Host and the MCU Bootloader. First, the Host sends a command to the Bootloader which in turn will respond in two steps. The Bootloader will confirm or not the receipt of the package by sending an ACK or NACK along with the size of the response to the Host. This check is done from the **Cyclic Redundancy Check** peripheral of the board (CRC). Finally, the MCU sends the message with the established size sent previously. The list of commands implemented in this project can be found in this [document](https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/bootloader_commands.pdf).

<p align="center">
	<img src="https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/imagens/host.png" alt="drawing" width="500"/>
</p>

## Bootloader Command Handling Flow-Chart

If the user button is pressed during the board reset, then we will enter the Bootloader to receive messages from the Host. As long as we don't receive any bytes in length, we are in a loop. Otherwise, if we receive a byte in length, then we read that byte and decode the received command. Depending on the type of command, we will handle it using a specific function. After the command is identified, a check is performed on the CRC sent by the Host. If the CRC sent is not the same as the first bytes of information received, then we return to the loop function returning a "NACK". Otherwise, an ACK is sent and a response is obtained. Finally, the MCU sends the message requested by the Host.

<p align="center">
	<img src="https://github.com/mattsousaa/STM32F7xxx_Bootloader/blob/master/00_Documents/imagens/code_flow.png" width="600"/>
</p>

