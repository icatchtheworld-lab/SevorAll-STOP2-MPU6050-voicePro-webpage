################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ra_gen/common_data.c \
../ra_gen/hal_data.c \
../ra_gen/main.c \
../ra_gen/pin_data.c \
../ra_gen/vector_data.c 

C_DEPS += \
./ra_gen/common_data.d \
./ra_gen/hal_data.d \
./ra_gen/main.d \
./ra_gen/pin_data.d \
./ra_gen/vector_data.d 

OBJS += \
./ra_gen/common_data.o \
./ra_gen/hal_data.o \
./ra_gen/main.o \
./ra_gen/pin_data.o \
./ra_gen/vector_data.o 

SREC += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.srec 

MAP += \
SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary\ filter.map 


# Each subdirectory must supply rules for building sources it contributes
ra_gen/%.o: ../ra_gen/%.c
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -gdwarf-4 -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/src" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/api" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/fsp/inc/instances" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_gen" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg/bsp" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra_cfg/fsp_cfg" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/applications" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/drivers" -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/dshanmcu_ra6m5/Middlewares" -I"." -I"C:/Users/Lin/Downloads/e2_studio/workspace2/SevorAll+STOP2+MPU6050+voicePro+webpage+Complementary filter/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

