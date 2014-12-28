################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CommHelper.cpp \
../src/NetworkConfig.cpp \
../src/RevSSIDPWDaemon.cpp \
../src/TCPClient.cpp \
../src/json_reader.cpp \
../src/json_value.cpp \
../src/json_writer.cpp \
../src/main.cpp \
../src/unit.cpp 

C_SRCS += \
../src/inifile.c 

OBJS += \
./src/CommHelper.o \
./src/NetworkConfig.o \
./src/RevSSIDPWDaemon.o \
./src/TCPClient.o \
./src/inifile.o \
./src/json_reader.o \
./src/json_value.o \
./src/json_writer.o \
./src/main.o \
./src/unit.o 

C_DEPS += \
./src/inifile.d 

CPP_DEPS += \
./src/CommHelper.d \
./src/NetworkConfig.d \
./src/RevSSIDPWDaemon.d \
./src/TCPClient.d \
./src/json_reader.d \
./src/json_value.d \
./src/json_writer.d \
./src/main.d \
./src/unit.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	mips-openwrt-linux-g++ -I/opt/OpenWrt-SDK/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	mips-openwrt-linux-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


