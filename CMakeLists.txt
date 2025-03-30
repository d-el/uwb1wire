cmake_minimum_required(VERSION 3.6)

PROJECT(usb1wire
	VERSION 0.0.1
	LANGUAGES CXX C
	)
	
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

add_compile_options(
	-g3 -O2
	-fno-exceptions
	-fmessage-length=0
	-ffunction-sections
	-fdata-sections
	-fsigned-char
	-fsingle-precision-constant
	-Wfloat-equal
	-Wuninitialized
	-Wextra
	-Wall
	)

include_directories(PUBLIC
	./
	)

add_executable(${PROJECT_NAME} main.cpp ds18b20.c oneWireUart.c crc.c)
