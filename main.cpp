#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <string>
#include <inttypes.h>
#include <array>

#include "ds18b20.h"

/******************************************************************************
 */
int main(int argc, char** argv){
	(void)argc;
	static int fd;
	static const char* adapter = argv[1];

	static auto uartInit = []() -> bool {
		printf("Open: %s\n", adapter);
		fd = open(adapter, O_RDWR| O_NOCTTY);
		if(fd < 0){
			printf("Error %i from open: %s\n", errno, strerror(errno));
			return false;
		}

		struct termios tty;
		if(tcgetattr(fd, &tty) != 0){
			printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		}

		tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
		tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
		tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
		tty.c_cflag |= CS8; // 8 bits per byte (most common)
		tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
		tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

		tty.c_lflag &= ~ICANON;
		tty.c_lflag &= ~ECHO; // Disable echo
		tty.c_lflag &= ~ECHOE; // Disable erasure
		tty.c_lflag &= ~ECHONL; // Disable new-line echo
		tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
		tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
		tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

		tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
		tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
		// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
		// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

		tty.c_cc[VTIME] = 1; // Wait for up to 0.1s (1 deciseconds), returning as soon as any data is received.
		tty.c_cc[VMIN] = 0;

		// Save tty settings, also checking for error
		if(tcsetattr(fd, TCSANOW, &tty) != 0) {
			printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
			return false;
		}

		return true;
	};

	auto uartSetBaud = [](uint32_t baud) -> bool {
		#include <asm/termios.h>
		struct termios2 tio;
		ioctl(fd, TCGETS2, &tio);
		tio.c_cflag &= ~CBAUD;
		tio.c_cflag |= BOTHER;
		tio.c_ispeed = baud;
		tio.c_ospeed = baud;
		/* do other miscellaneous setup options with the flags here */
		return ioctl(fd, TCSETS2, &tio) > 0;
	};

	auto uartWrite = [](const void* src, size_t len) -> bool {
		return write(fd, src, len) == (ssize_t)len;
	};

	auto uartReadEnable = []() -> size_t {
		tcflush(fd, TCIOFLUSH);
		return true;
	};

	auto uartRead = [](void* dst, size_t len) -> size_t {
		size_t rxlen = read(fd, dst, len);
		return rxlen;
	};

	// Init one wire stack
	ow_init(uartInit,
			uartSetBaud,
			uartWrite,
			uartReadEnable,
			uartRead,
			nullptr);

	owSt_type result = ow_reset();
	if(result != owOk){
		printf("Error ow_reset, %u\n", result);
		return 1;
	}

	// Search ROM
	constexpr uint8_t addrSize = 8;
	using rom_t = std::array<uint8_t, addrSize>;
	constexpr uint8_t maxAddrs = 127;
	std::array<rom_t, maxAddrs> roms;
	uint8_t addrNum;
	ow_searchRomContext_t searchRomContext = {};
	for(addrNum = 0; addrNum < maxAddrs;){
		result = ow_searchRom(&searchRomContext);
		if(result == owSearchOk || result == owSearchLast){
			memcpy(&roms[addrNum], &searchRomContext.rom[0], addrSize);
			printf("Found ROM[%u]: ", addrNum);
			for(uint8_t a : roms[addrNum]){
				printf("%02X ", a);
			}
			printf("\n");
			addrNum++;
		}

		if(result == owSearchLast){
			break;
		}

		if(result != owSearchOk && result != owSearchLast){
			printf("Error search ROM, %u\n", result);
			return 1;
		}
	}

	// Init sensors
	uint8_t bits = 12;
	for(uint8_t dev = 0; dev < addrNum; dev++){
		ds18b20_state_type dss = ds18b20_init(roms[dev].data(), bits);
		if(dss == ds18b20st_ok){
			printf("Init ds18b20[%u] to %uBit resolution\n", dev, bits);
		}else{
			printf("Error init ds18b20, %u\n", dss);
		}
	}

	// Meas loop
	while(1){
		ds18b20_convertTemp(nullptr); // Command to all sensors
		usleep(ds18b20_getTconv(bits) * 1000);

		for(uint8_t dev = 0; dev < addrNum; dev++){
			uint8_t scratchpad[9];
			ds18b20_state_type dss = ds18b20_readScratchpad(roms[dev].data(), scratchpad);
			if(dss == ds18b20st_ok){
				int16_t temperature = ds18b20_reg2tmpr(scratchpad);
				int16_t abst = abs(temperature);
				printf("%3" PRIi16 ".%" PRIu16 "°C ", temperature / 10, abst % 10);
			}else{
				printf("Error_ReadScratchpad, %u", dss);
			}
		}
		printf("\n");
	}

	return 0;
}
