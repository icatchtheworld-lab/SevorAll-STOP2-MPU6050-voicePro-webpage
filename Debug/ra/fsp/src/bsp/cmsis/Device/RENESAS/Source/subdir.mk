################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/startup.c \
../ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/system.c 

C_DEPS += \
./ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/startup.d \
./ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/system.d 

OBJS += \
./ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/startup.o \
./ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/system.o 

SREC += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.srec 

MAP += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.map 


# Each subdirectory must supply rules for building sources it contributes
ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/%.o: ../ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/src" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/api" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/instances" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_gen" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg/bsp" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/applications" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/drivers" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/Middlewares" -I"." -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

