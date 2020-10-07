################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F7xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
bootloader_F767xxx.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
bootloader_F767xxx.list \

OBJCOPY_BIN += \
bootloader_F767xxx.bin \


# All Target
all: bootloader_F767xxx.elf secondary-outputs

# Tool invocations
bootloader_F767xxx.elf: $(OBJS) $(USER_OBJS) C:\Users\mateu\Documents\Bootloader_ST\bootloader_F767xxx\STM32F767ZITX_FLASH.ld
	arm-none-eabi-gcc -o "bootloader_F767xxx.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"C:\Users\mateu\Documents\Bootloader_ST\bootloader_F767xxx\STM32F767ZITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="bootloader_F767xxx.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

bootloader_F767xxx.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "bootloader_F767xxx.list"
	@echo 'Finished building: $@'
	@echo ' '

bootloader_F767xxx.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "bootloader_F767xxx.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
