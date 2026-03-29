################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/sensors/gpioexti.c \
../Core/Src/sensors/hx711.c \
../Core/Src/sensors/modbus.c 

OBJS += \
./Core/Src/sensors/gpioexti.o \
./Core/Src/sensors/hx711.o \
./Core/Src/sensors/modbus.o 

C_DEPS += \
./Core/Src/sensors/gpioexti.d \
./Core/Src/sensors/hx711.d \
./Core/Src/sensors/modbus.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/sensors/%.o Core/Src/sensors/%.su Core/Src/sensors/%.cyclo: ../Core/Src/sensors/%.c Core/Src/sensors/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/projects/AmsDriver/SBSensor/Core/Src/conf" -I"D:/projects/AmsDriver/SBSensor/Core/Src/eeprom" -I"D:/projects/AmsDriver/SBSensor/Core/Src/io" -I"D:/projects/AmsDriver/SBSensor/Core/Src/sensors" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-sensors

clean-Core-2f-Src-2f-sensors:
	-$(RM) ./Core/Src/sensors/gpioexti.cyclo ./Core/Src/sensors/gpioexti.d ./Core/Src/sensors/gpioexti.o ./Core/Src/sensors/gpioexti.su ./Core/Src/sensors/hx711.cyclo ./Core/Src/sensors/hx711.d ./Core/Src/sensors/hx711.o ./Core/Src/sensors/hx711.su ./Core/Src/sensors/modbus.cyclo ./Core/Src/sensors/modbus.d ./Core/Src/sensors/modbus.o ./Core/Src/sensors/modbus.su

.PHONY: clean-Core-2f-Src-2f-sensors

