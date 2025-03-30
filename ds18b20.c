/*!****************************************************************************
 * @file		ds18b20.c
 * @author		Storozhenko Roman - D_EL
 * @version		V2.3
 * @date		30.03.2025
 * @copyright	The MIT License (MIT). Copyright (c) 2025 Storozhenko Roman
 */

/*!****************************************************************************
* Include
*/
#include <stddef.h>
#include "crc.h"
#include "ds18b20.h"

#define DS18B20_FAMILY_CODE	0x28
#define READ_SCRATCHPAD		0xBE
#define CONVERT_T			0x44
#define WRITE_SCRATCHPAD	0x4E
#define COPY_SCRATCHPAD		0x48
#define RECALL_E2			0xB8
#define READ_POWER_SUPPLY	0xB4

/*!****************************************************************************
* @brief	Init ds18b20
* @param	rom - slave ID or NULL for skip ROM
* @param	bits - resolution bits
*/
ds18b20_state_type ds18b20_init(const uint8_t rom[8], uint8_t bits){
	owSt_type result = ow_reset();
	if(result != 0){
		return (ds18b20_state_type)result;
	}

	if(rom == NULL){
		uint8_t readRom[8];
		result = ow_readRom(readRom);
		if(result != 0){
			return (ds18b20_state_type)result;
		}
		if(readRom[0] != DS18B20_FAMILY_CODE){
			return ds18b20st_notDs18b20;
		}
	}

	result = ow_selectRom(rom);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	uint8_t buff[4];
	buff[0] = WRITE_SCRATCHPAD;
	buff[1] = 127;				//TH
	buff[2] = 0;				//TL
	switch(bits){
		case 9:  buff[3] = 0x1F; break;
		case 10: buff[3] = 0x3F; break;
		case 11: buff[3] = 0x5F; break;
		case 12:
		default: buff[3] = 0x7F;;
	}
	buff[3] = 0x7F;				//12bit 750ms	0.0625
	result = ow_write(buff, sizeof(buff));
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	return ds18b20st_ok;
}

/*!****************************************************************************
* @brief	Read the contents of the scratchpad
* @param	rom - slave ID or NULL for skip ROM
* @param	scratchpad - save to
*/
ds18b20_state_type ds18b20_readScratchpad(const uint8_t rom[8], uint8_t scratchpad[9]){
	owSt_type result = ow_reset();
	if(result != 0){
		return (ds18b20_state_type)result;
	}
	result = ow_selectRom(rom);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	const uint8_t functionCommand = READ_SCRATCHPAD;
	result = ow_write(&functionCommand, 1);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	result = ow_read(scratchpad, 9);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	uint8_t crc = crc8Calc(&crc1Wire, scratchpad, 9);
	if(crc != 0){
		return ds18b20st_errorCrc;
	}
	return ds18b20st_ok;
}

/*!****************************************************************************
* @brief	Initiates a single temperature conversion
* @param	rom - slave ID or NULL for skip ROM
*/
ds18b20_state_type ds18b20_convertTemp(const uint8_t rom[8]){
	owSt_type result = ow_reset();
	if(result != 0){
		return (ds18b20_state_type)result;
	}
	result = ow_selectRom(rom);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}

	const uint8_t functionCommand = CONVERT_T;
	result = ow_write(&functionCommand, 1);
	if(result != owOk){
		return (ds18b20_state_type)result;
	}
	return ds18b20st_ok;
}

/*!****************************************************************************
* @param	rl - low temperature register
* @param	rh - hight temperature register
* @retval	temperature X_XX
*/
int16_t ds18b20_reg2tmpr(const uint8_t scratchpad[2]){
	union{
		struct{
			uint8_t	rl;
			uint8_t	rh;
		}byte;
		int16_t	word;
	}tregs;

	tregs.byte.rl = scratchpad[0];
	tregs.byte.rh = scratchpad[1];

	return (tregs.word * 10U + (16/2)) / 16;	// Div with round
}

/*!****************************************************************************
* @param	bits - resolution bits
* @retval	conversion time in ms
*/
uint16_t ds18b20_getTconv(uint8_t bits){
	switch(bits){
		case 9:  return 94;
		case 10: return 188;
		case 11: return 375;
		case 12:
		default: return 750;
	}
}

/******************************** END OF FILE ********************************/
