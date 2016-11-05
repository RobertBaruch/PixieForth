################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main.cpp \
../src/unit_tests.cpp 

S_UPPER_SRCS += \
../src/forth_system.S 

OBJS += \
./src/forth_system.o \
./src/main.o \
./src/unit_tests.o 

S_UPPER_DEPS += \
./src/forth_system.d 

CPP_DEPS += \
./src/main.d \
./src/unit_tests.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -x assembler-with-cpp -DF_CPU=120000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -D__FRDM_K64F__ -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DF_CPU=120000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -D__FRDM_K64F__ -I"F:\kalkulyator-workspace-v2\PixieForth\src" -I"F:\kalkulyator-workspace-v2\TeensyCore3\src" -std=gnu++11 -fabi-version=0 -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


