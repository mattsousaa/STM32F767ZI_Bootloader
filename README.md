# Embedded Bootloader for STM32F767ZI

**First of all, what is Bootloader?**

A bootloader is a special program which allows user application program to be updated. The new user application can be obtained using alternative download channels, 
such as a USB stick or a network port. The bootloader and the user application should be written and built as two separate projects resulting in two separate executables.
The main tasks of the bootloader are to reprogram/replace the user application, if necessary, and to jump to the user application to execute it. The user application doesn’t 
necessarily need to know the existence of the bootloader.
