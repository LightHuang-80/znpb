################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/io/CO_fifo.c \
../Core/Src/io/canport.c \
../Core/Src/io/process.c \
../Core/Src/io/usartport.c 

OBJS += \
./Core/Src/io/CO_fifo.o \
./Core/Src/io/canport.o \
./Core/Src/io/process.o \
./Core/Src/io/usartport.o 

C_DEPS += \
./Core/Src/io/CO_fifo.d \
./Core/Src/io/canport.d \
./Core/Src/io/process.d \
./Core/Src/io/usartport.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/io/%.o Core/Src/io/%.su Core/Src/io/%.cyclo: ../Core/Src/io/%.c Core/Src/io/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/projects/AmsDriver/SBSensor/Core/Src/conf" -I"D:/projects/AmsDriver/SBSensor/Core/Src/eeprom" -I"D:/projects/AmsDriver/SBSensor/Core/Src/io" -I"D:/projects/AmsDriver/SBSensor/Core/Src/sensors" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-io

clean-Core-2f-Src-2f-io:
	-$(RM) ./Core/Src/io/CO_fifo.cyclo ./Core/Src/io/CO_fifo.d ./Core/Src/io/CO_fifo.o ./Core/Src/io/CO_fifo.su ./Core/Src/io/canport.cyclo ./Core/Src/io/canport.d ./Core/Src/io/canport.o ./Core/Src/io/canport.su ./Core/Src/io/process.cyclo ./Core/Src/io/process.d ./Core/Src/io/process.o ./Core/Src/io/process.su ./Core/Src/io/usartport.cyclo ./Core/Src/io/usartport.d ./Core/Src/io/usartport.o ./Core/Src/io/usartport.su

.PHONY: clean-Core-2f-Src-2f-io

