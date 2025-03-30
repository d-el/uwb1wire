/*!****************************************************************************
 * @file		oneWireUart.c
 * @author		d_el
 * @version		V1.3
 * @date		30.03.2025
 * @copyright	The MIT License (MIT). Copyright (c) 2025 Storozhenko Roman
 */

/*!****************************************************************************
* Include
*/
#include <assert.h>
#include <string.h>
#include <crc.h>
#include "oneWireUart.h"

#ifndef OW_resetBaud
#define OW_RESETBAUD 9600
#endif
#ifndef OW_rwbitBaud
#define OW_RWBITBAUD 115200
#endif

// ROM COMMANDS Definition
#define SEARCH_ROM			0xF0
#define MATCH_ROM			0x55
#define READ_ROM			0x33
#define SKIP_ROM			0xCC
#define ALARM_SEARCH		0xEC

owuart_init_t owuart_init;
owuart_setBaud_t owuart_setBaud;
owuart_write_t owuart_write;
owuart_readEnable_t owuart_readEnable;
owuart_read_t owuart_read;
owuart_strongPullup_t owuart_strongPullup;

#ifndef OW_TX_BFF
#define OW_TX_BFF 128
#endif
uint8_t txBff[OW_TX_BFF];

#ifndef OW_RX_BFF
#define OW_RX_BFF 128
#endif
uint8_t rxBff[OW_RX_BFF];

/*!****************************************************************************
* @brief	Initialization one wire interface
*/
bool ow_init(	owuart_init_t init,
				owuart_setBaud_t setBaud,
				owuart_write_t write,
				owuart_readEnable_t readEnable,
				owuart_read_t read,
				owuart_strongPullup_t strongPullup){
	owuart_init = init;
	owuart_setBaud = setBaud;
	owuart_write = write;
	owuart_readEnable = readEnable;
	owuart_read = read;
	owuart_strongPullup = strongPullup;
	return owuart_init();
}

/*!****************************************************************************
* @brief	Set strong pullup
* @param	true - pullup enable, false - pullup disable
* @retval	owSt_type
*/
owSt_type ow_strongPullup(bool v){
	if(owuart_strongPullup){
		owuart_strongPullup(v);
		return owOk;
	}
	return owOther;
}

/*!****************************************************************************
* @brief	Reset pulse and presence detect
* @param	None
* @retval	owSt_type
*/
owSt_type ow_reset(void){
	owuart_setBaud(OW_RESETBAUD);
	owuart_readEnable();
	txBff[0] = 0xF0;
	owuart_write(txBff, 1);
	size_t len = owuart_read(rxBff, sizeof(rxBff));
	if(len < 1){
		return owUartTimeout;
	}
	if((rxBff[0] >= 0x90)&&(rxBff[0] <= 0xF0)){
		return owOk;
	}else{
		return owNotFound;
	}
}

/*!***************************************************************************
* @brief	Write one bit
* @param	src - bit value
* @retval	None
*/
owSt_type ow_writebit(uint8_t src){
	uint8_t *pBff = txBff;
	uint8_t byteTrans = 1;
	if(src != 0){
		*pBff = 0xFF;
	}else{
		*pBff = 0x00;
	}
	owuart_setBaud(OW_RWBITBAUD);
	owuart_readEnable();
	owuart_write(txBff, byteTrans);
	size_t len = owuart_read(rxBff, byteTrans);
	if(len < 1){
		return owUartTimeout;
	}
	return owOk;
}

/*!***************************************************************************
* @brief	Write data
* @param	src		pointer to source buffer
* @param	len		number bytes for transmit
* @retval	None
*/
owSt_type ow_write(const void *src, uint8_t len){
	uint8_t *pSrc		= (uint8_t*)src;
	uint8_t *pSrcEnd	= pSrc + len;
	uint8_t *pBff		= txBff;
	uint8_t mask, byteTrans = len << 3;

	while(pSrc < pSrcEnd){
		for(mask = 1; mask != 0; mask <<= 1){
			if((*pSrc & mask) != 0){
				*pBff++ = 0xFF;
			}else{
				*pBff++ = 0x00;
			}
		}
		pSrc++;
	}

	owuart_setBaud(OW_RWBITBAUD);
	owuart_readEnable();
	owuart_write(txBff, byteTrans);
	size_t rxlen = owuart_read(rxBff, byteTrans);
	if(rxlen < 1){
		return owUartTimeout;
	}
	return owOk;
}

/*!***************************************************************************
* @brief	Read one bit
* @param	dst - pointer to destination bt value
* @retval	Status operation
*/
owSt_type ow_readbit(uint8_t *dst){
	uint8_t	 byteTrans = 1;
	memset(txBff, 0xFF, byteTrans);
	owuart_setBaud(OW_RWBITBAUD);
	owuart_readEnable();
	owuart_write(txBff, byteTrans);
	size_t len = owuart_read(rxBff, byteTrans);
	if(len > 0){
		if(rxBff[0] == 0xFF){
			*dst = 1; //Read '1'
		}else{
			*dst = 0;
		}
	}
	else{
		return owUartTimeout;
	}

	return owOk;
}

/*!***************************************************************************
* @brief	Read data
* @param	dst - pointer to destination buffer
* @param	len - number bytes for receive
* @retval	Status operation
*/
owSt_type ow_read(void *dst, uint8_t len){
	uint8_t		*pDst		= dst;
	uint8_t		*pDstEnd	= pDst + len;
	uint8_t		*pBff		= rxBff;
	uint8_t		mask, byteTrans = len << 3;

	memset(txBff, 0xFF, byteTrans);

	owuart_setBaud(OW_RWBITBAUD);
	owuart_readEnable();
	owuart_write(txBff, byteTrans);
	size_t rxlen = owuart_read(rxBff, byteTrans);
	if(rxlen == byteTrans){
		while(pDst < pDstEnd){
			*pDst = 0;
			for(mask = 1; mask != 0; mask <<= 1){
				if(*pBff++ == 0xFF){
					*pDst |= mask; //Read '1'
				}
			}
			pDst++;
		}
	}
	else{
		return owUartTimeout;
	}

	return owOk;
}

/*!***************************************************************************
 * @brief	Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing search state
 * @param	Searching context
 * @retval	Status operation
 */
owSt_type ow_searchRom(ow_searchRomContext_t* context){
	owSt_type searchResult = owOk;
	uint8_t lastZero = 0, idBitNumber = 1, romByteNumber = 0;
	uint8_t idBit, cmpIdBit;
	uint8_t romByteMask = 1, searchDirection;

	// If the last call was not the last one
	if(!context->lastDeviceFlag){
		if(ow_reset() != owOk){
			// Reset the search
			context->lastDiscrepancy = 0;
			context->lastDeviceFlag = 0;
			context->lastFamilyDiscrepancy = 0;
			return owNotFound;
		}

		// Issue the search command
		const uint8_t search = SEARCH_ROM;
		ow_write(&search, 1);
		// loop to do the search
		do{
			// Read a bit and its complement
			ow_readbit(&idBit);
			ow_readbit(&cmpIdBit);

			// Check for no devices on 1-wire
			if((idBit == 1) && (cmpIdBit == 1)){
				break;
			}else{
				// All devices coupled have 0 or 1
				if(idBit != cmpIdBit){
					searchDirection = idBit; // bit write value for search
				}else{
					// If this discrepancy if before the Last Discrepancy
					// On a previous next then pick the same as last time
					if(idBitNumber < context->lastDiscrepancy){
						searchDirection = ((context->rom[romByteNumber] & romByteMask) > 0);
					}else{
						// If equal to last pick 1, if not then pick 0
						searchDirection = (idBitNumber == context->lastDiscrepancy);
					}
					// If 0 was picked then record its position in LastZero
					if(searchDirection == 0){
						lastZero = idBitNumber;
						// Check for Last discrepancy in family
						if(lastZero < 9)
							context->lastFamilyDiscrepancy = lastZero;
					}
				}
				// Set or clear the bit in the ROM byte rom_byte_number
				// With mask rom_byte_mask
				if(searchDirection == 1){
					context->rom[romByteNumber] |= romByteMask;
				}else{
					context->rom[romByteNumber] &= ~romByteMask;
				}
				// Serial number search direction write bit
				ow_writebit(searchDirection);
				// Increment the byte counter id_bit_number
				// And shift the mask rom_byte_mask
				idBitNumber++;
				romByteMask <<= 1;
				// If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if(romByteMask == 0){
					romByteNumber++;
					romByteMask = 1;
				}
			}
		}while(romByteNumber < 8); // Loop until through all ROM bytes 0-7
		// If the search was successful then
		if(!((idBitNumber < 65) || (crc8Calc(&crc1Wire, context->rom, 8) != 0))){
			// Search successful so set lastDiscrepancy, lastDeviceFlag, search_result
			context->lastDiscrepancy = lastZero;
			// Check for last device
			if(context->lastDiscrepancy == 0){
				context->lastDeviceFlag = 1;
				searchResult = owSearchLast;
			}else{
				searchResult = owSearchOk;
			}
		}
	}
	// If no device found then reset counters so next 'search' will be like a first
	if(!searchResult || context->rom[0] == 0){
		context->lastDiscrepancy = 0;
		context->lastDeviceFlag = 0;
		context->lastFamilyDiscrepancy = 0;
		searchResult = owSearchError;
	}
	return searchResult;
}

/*!***************************************************************************
 * @brief	Read the slave’s 64-bit ROM code
 * @param	rom		destination
 * @retval	Status operation
 */
owSt_type ow_readRom(uint8_t rom[8]){
	uint8_t buff[8];
	buff[0] = READ_ROM;
	owSt_type result = ow_write(buff, 1);
	if(result != owOk){
		return result;
	}

	result = ow_read(rom, 8);
	if(result != owOk){
		return result;
	}

	uint8_t crc = crc8Calc(&crc1Wire, rom, 8);
	if(crc != 0){
		return owCrcError;
	}
	return owOk;
}

/*!***************************************************************************
 * @brief	Address a specific slave device
 * @param	rom - slave ID or NULL for skip ROM
 * @retval	Status operation
 */
owSt_type ow_selectRom(const uint8_t rom[8]){
	uint8_t buff[9];
	uint8_t size = 0;

	if(rom != NULL){
		buff[size++] = MATCH_ROM;
		for(size_t i = 0; i < 8; i++){
			buff[size++] = rom[i];
		}
	}else{
		buff[size++] = SKIP_ROM;
	}
	owSt_type result = ow_write(buff, size);
	return result;
}

/******************************** END OF FILE ********************************/
