/*!****************************************************************************
 * @file		oneWireUart.h
 * @author		d_el
 * @version		V1.3
 * @date		30.03.2025
 * @copyright	The MIT License (MIT). Copyright (c) 2025 Storozhenko Roman
 */
#ifndef oneWireUart_H
#define oneWireUart_H

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************************
* Include
*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*!****************************************************************************
* User typedef
*/
typedef enum{
	owOk,
	owNotFound,
	owShortCircle,
	owTimeOut,
	owCrcError,
	owSearchOk,
	owSearchLast,
	owSearchError,
	owUartTimeout,
	owOther
}owSt_type;

typedef struct{
	uint8_t rom[8];
	uint8_t lastDiscrepancy;
	uint8_t lastFamilyDiscrepancy;
	uint8_t lastDeviceFlag;
}ow_searchRomContext_t;

typedef bool (*owuart_init_t)(void);
typedef bool (*owuart_strongPullup_t)(bool hi);
typedef bool (*owuart_setBaud_t)(uint32_t baud);
typedef bool (*owuart_write_t)(const void* src, size_t len);
typedef size_t (*owuart_readEnable_t)(void* dst, size_t len);
typedef size_t (*owuart_read_t)(void* dst, size_t len);

/*!****************************************************************************
* Prototypes for the functions
*/
bool ow_init(	owuart_init_t init,
				owuart_setBaud_t setBaud,
				owuart_write_t write,
				owuart_readEnable_t readEnable,
				owuart_read_t read,
				owuart_strongPullup_t setOut);
owSt_type ow_strongPullup(bool v);
owSt_type ow_reset(void);
owSt_type ow_write(const void *src, uint8_t len);
owSt_type ow_read(void *dst, uint8_t len);
owSt_type ow_searchRom(ow_searchRomContext_t* context);
owSt_type ow_readRom(uint8_t rom[8]);
owSt_type ow_selectRom(const uint8_t rom[8]);

#ifdef __cplusplus
}
#endif

#endif //oneWireUart_H
/******************************** END OF FILE ********************************/
