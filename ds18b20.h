/*!****************************************************************************
 * @file		ds18b20.h
 * @author		Storozhenko Roman - D_EL
 * @version		V2.3
 * @date		30.03.2025
 * @copyright	The MIT License (MIT). Copyright (c) 2025 Storozhenko Roman
 */
#ifndef ds18b20_H
#define ds18b20_H

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************************
* Include
*/
#include <stdint.h>
#include "oneWireUart.h"

/*!****************************************************************************
* User typedef
*/
typedef struct{
	uint8_t		integer;
	uint8_t		frac;
	uint8_t		result;
	uint8_t		sign	:1;
	uint8_t		update	:1;
}tmpr_type;

typedef enum{
	ds18b20st_ok,
	ds18b20st_NotFound = owNotFound,
	ds18b20st_ShortCircle = owShortCircle,
	ds18b20st_wTimeOut = owTimeOut,
	ds18b20st_errorCrc = owCrcError,
	ds18b20st_SearchLast = owSearchLast,
	ds18b20st_SearchError = owSearchError,
	ds18b20st_UartTimeout = owUartTimeout,
	ds18b20st_other = owOther,
	ds18b20st_notDs18b20
}ds18b20_state_type;

/*!****************************************************************************
* Prototypes for the functions
*/
ds18b20_state_type ds18b20_init(const uint8_t rom[8], uint8_t bits);
ds18b20_state_type ds18b20_readScratchpad(const uint8_t rom[8], uint8_t scratchpad[9]);
ds18b20_state_type ds18b20_convertTemp(const uint8_t rom[8]);
int16_t ds18b20_reg2tmpr(const uint8_t scratchpad[2]);
uint16_t ds18b20_getTconv(uint8_t bits);

#ifdef __cplusplus
}
#endif

#endif //ds18b20_H
/******************************** END OF FILE ********************************/
