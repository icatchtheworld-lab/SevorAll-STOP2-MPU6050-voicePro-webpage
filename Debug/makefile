################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include ra_gen/subdir.mk
-include ra/fsp/src/r_sci_uart/subdir.mk
-include ra/fsp/src/r_ioport/subdir.mk
-include ra/fsp/src/r_iic_master/subdir.mk
-include ra/fsp/src/r_gpt/subdir.mk
-include ra/fsp/src/r_flash_hp/subdir.mk
-include ra/fsp/src/bsp/mcu/ra6m5/subdir.mk
-include ra/fsp/src/bsp/mcu/all/subdir.mk
-include ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/subdir.mk
-include dshanmcu_ra6m5/drivers/subdir.mk
-include dshanmcu_ra6m5/applications/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(JMP_DEPS)),)
-include $(JMP_DEPS)
endif
ifneq ($(strip $(FSY_DEPS)),)
-include $(FSY_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_UPPER_DEPS)),)
-include $(SRC_UPPER_DEPS)
endif
ifneq ($(strip $(P_UPPER_DEPS)),)
-include $(P_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(PP_UPPER_DEPS)),)
-include $(PP_UPPER_DEPS)
endif
ifneq ($(strip $(P_DEPS)),)
-include $(P_DEPS)
endif
ifneq ($(strip $(FSY_UPPER_DEPS)),)
-include $(FSY_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(SRC_DEPS)),)
-include $(SRC_DEPS)
endif
ifneq ($(strip $(JMP_UPPER_DEPS)),)
-include $(JMP_UPPER_DEPS)
endif
ifneq ($(strip $(PP_DEPS)),)
-include $(PP_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(SX_UPPER_DEPS)),)
-include $(SX_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.srec \

SECONDARY_SIZE += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.siz \


# All Target
all: SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.elf secondary-outputs

# Tool invocations
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.elf: $(OBJS) $(USER_OBJS) $(LINKER_SCRIPT)
	$(file > SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -gdwarf-4 -T "fsp.ld" -Xlinker --gc-sections -L "C:\\Users\\Lin\\Downloads\\e2_studio\\workspace2\\SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/script" -Wl,-Map,"SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.map" --specs=nano.specs -o "SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf" -Wl,--start-group $(OBJS) $(USER_OBJS) $(LIBS) -Wl,--end-group)
	@echo Building target: $@ && arm-none-eabi-gcc @"SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf.in"
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.srec: SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.elf
	arm-none-eabi-objcopy -O srec "SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf"  "SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.srec"
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.siz: SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.elf
	arm-none-eabi-size --format=berkeley "SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf"
# Other Targets
clean:
	-$(file > clean_file.tmp,  $(JMP_DEPS)  $(FSY_DEPS)  $(C_UPPER_DEPS)  $(SECONDARY_SIZE)  $(SRC_UPPER_DEPS)  $(P_UPPER_DEPS)  $(S_DEPS)  $(PP_UPPER_DEPS)  $(P_DEPS)  $(FSY_UPPER_DEPS)  $(C_DEPS)  $(SRC_DEPS)  $(JMP_UPPER_DEPS)  $(PP_DEPS)  $(SX_DEPS)  $(ASM_UPPER_DEPS)  $(SX_UPPER_DEPS)  $(OBJS)  $(SECONDARY_FLASH)  $(ASM_DEPS)  $(SREC)  $(S_UPPER_DEPS)  $(MAP) )
	-@ xargs -P 2 -s 32000 -t -a clean_file.tmp $(RM)
	-@$(RM) clean_file.tmp
	-$(RM)  "SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter.elf"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets
