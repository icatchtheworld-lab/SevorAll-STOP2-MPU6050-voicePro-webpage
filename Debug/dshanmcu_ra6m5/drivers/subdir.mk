################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../dshanmcu_ra6m5/drivers/Flash.c \
../dshanmcu_ra6m5/drivers/SensorMPU6050.c \
../dshanmcu_ra6m5/drivers/Servor.c \
../dshanmcu_ra6m5/drivers/VoiceControl.c \
../dshanmcu_ra6m5/drivers/Wifi.c \
../dshanmcu_ra6m5/drivers/drv_uart.c 

C_DEPS += \
./dshanmcu_ra6m5/drivers/Flash.d \
./dshanmcu_ra6m5/drivers/SensorMPU6050.d \
./dshanmcu_ra6m5/drivers/Servor.d \
./dshanmcu_ra6m5/drivers/VoiceControl.d \
./dshanmcu_ra6m5/drivers/Wifi.d \
./dshanmcu_ra6m5/drivers/drv_uart.d 

OBJS += \
./dshanmcu_ra6m5/drivers/Flash.o \
./dshanmcu_ra6m5/drivers/SensorMPU6050.o \
./dshanmcu_ra6m5/drivers/Servor.o \
./dshanmcu_ra6m5/drivers/VoiceControl.o \
./dshanmcu_ra6m5/drivers/Wifi.o \
./dshanmcu_ra6m5/drivers/drv_uart.o 

SREC += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.srec 

MAP += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.map 


# Each subdirectory must supply rules for building sources it contributes
dshanmcu_ra6m5/drivers/%.o: ../dshanmcu_ra6m5/drivers/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/src" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/api" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/instances" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_gen" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg/bsp" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/applications" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/drivers" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/Middlewares" -I"." -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

