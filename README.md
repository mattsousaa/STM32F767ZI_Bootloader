# Embedded Bootloader for STM32F767ZI

##First of all, what is Bootloader?

A bootloader is a special program which allows user application program to be updated. The new user application can be obtained using alternative download channels, 
such as a USB stick or a network port. The bootloader and the user application should be written and built as two separate projects resulting in two separate executables. The main tasks of the bootloader are to reprogram/replace the user application, if necessary, and to jump to the user application to execute it. The user application doesn’t necessarily need to know the existence of the bootloader. The bootloader is usually placed at the chips flash base address, so that it will be executed by the CPU after reset. 

##STM32F767ZI Memory Organization
* Internal Flash memory also called as Embedded Flash memory of 2MB
* Internal SRAM1 of 368KB
* Internal SRAM2 of 16KB
* System Memory (ROM) of 60KB
* OTP memory of 1024 bytes
* Option bytes memory of 32 bytes
* Backup RAM of 4KB
