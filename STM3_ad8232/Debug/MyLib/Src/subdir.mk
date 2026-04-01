################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MyLib/Src/HM10.c \
../MyLib/Src/JDY23.c \
../MyLib/Src/SDCard.c 

OBJS += \
./MyLib/Src/HM10.o \
./MyLib/Src/JDY23.o \
./MyLib/Src/SDCard.o 

C_DEPS += \
./MyLib/Src/HM10.d \
./MyLib/Src/JDY23.d \
./MyLib/Src/SDCard.d 


# Each subdirectory must supply rules for building sources it contributes
MyLib/Src/%.o MyLib/Src/%.su MyLib/Src/%.cyclo: ../MyLib/Src/%.c MyLib/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/DSP/Include -I../Drivers/CMSIS/NN/Include -I../Drivers/CMSIS/RTOS2/Include -I"D:/MonHoc_HK6/ECG_wireless/STM3_ad8232/MyLib/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-MyLib-2f-Src

clean-MyLib-2f-Src:
	-$(RM) ./MyLib/Src/HM10.cyclo ./MyLib/Src/HM10.d ./MyLib/Src/HM10.o ./MyLib/Src/HM10.su ./MyLib/Src/JDY23.cyclo ./MyLib/Src/JDY23.d ./MyLib/Src/JDY23.o ./MyLib/Src/JDY23.su ./MyLib/Src/SDCard.cyclo ./MyLib/Src/SDCard.d ./MyLib/Src/SDCard.o ./MyLib/Src/SDCard.su

.PHONY: clean-MyLib-2f-Src

