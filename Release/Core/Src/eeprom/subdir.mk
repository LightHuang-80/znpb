################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/eeprom/eeprom.c 

OBJS += \
./Core/Src/eeprom/eeprom.o 

C_DEPS += \
./Core/Src/eeprom/eeprom.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/eeprom/%.o Core/Src/eeprom/%.su Core/Src/eeprom/%.cyclo: ../Core/Src/eeprom/%.c Core/Src/eeprom/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/conf" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/eeprom" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/io" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/sensors" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-eeprom

clean-Core-2f-Src-2f-eeprom:
	-$(RM) ./Core/Src/eeprom/eeprom.cyclo ./Core/Src/eeprom/eeprom.d ./Core/Src/eeprom/eeprom.o ./Core/Src/eeprom/eeprom.su

.PHONY: clean-Core-2f-Src-2f-eeprom

