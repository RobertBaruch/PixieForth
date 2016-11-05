################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/parse.c 

CPP_SRCS += \
../src/DataStack.cpp \
../src/PicturedNumericArea.cpp \
../src/System.cpp \
../src/main.cpp 

S_UPPER_SRCS += \
../src/forth_system.S 

OBJS += \
./src/DataStack.o \
./src/PicturedNumericArea.o \
./src/System.o \
./src/forth_system.o \
./src/main.o \
./src/parse.o 

S_UPPER_DEPS += \
./src/forth_system.d 

C_DEPS += \
./src/parse.d 

CPP_DEPS += \
./src/DataStack.d \
./src/PicturedNumericArea.d \
./src/System.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DF_CPU=120000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -D__FRDM_K64F__ -I"F:\kalkulyator-workspace-v2\PixieForth\src" -I"F:\kalkulyator-workspace-v2\TeensyCore3\src" -std=gnu++11 -fabi-version=0 -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -x assembler-with-cpp -DF_CPU=120000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -D__FRDM_K64F__ -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DF_CPU=120000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -D__FRDM_K64F__ -I"F:\kalkulyator-workspace-v2\PixieForth\src" -I"F:\kalkulyator-workspace-v2\TeensyCore3\src" -std=gnu11 -Wa,-adhlns="$@.lst" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


