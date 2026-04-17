################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/conf/ndsconf.c 

OBJS += \
./Core/Src/conf/ndsconf.o 

C_DEPS += \
./Core/Src/conf/ndsconf.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/conf/%.o Core/Src/conf/%.su Core/Src/conf/%.cyclo: ../Core/Src/conf/%.c Core/Src/conf/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/conf" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/eeprom" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/io" -I"C:/Users/xiaob/Desktop/SBSensor - led/led/Core/Src/sensors" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-conf

clean-Core-2f-Src-2f-conf:
	-$(RM) ./Core/Src/conf/ndsconf.cyclo ./Core/Src/conf/ndsconf.d ./Core/Src/conf/ndsconf.o ./Core/Src/conf/ndsconf.su

.PHONY: clean-Core-2f-Src-2f-conf

