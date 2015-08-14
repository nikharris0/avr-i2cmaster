/* I2C Master using ATMega TWI
 * Adapted from AVR315 TWI Master implementation by Atmel
 */

#ifndef _I2CMASTER_H_
#define _I2CMASTER_H_

#include <util/twi.h>

#define TWI_BUFFER_SIZE	8

/* 400KHz i2c bus at 8MHz CPU *
 * doc2564.pdf page 5 table 1
 * If a different bus speed or CPU speed is used, this value needs to be
 * changed. refer to above doc table.
 */
#define TWI_BIT_RATE	2

#define TWI_READ_BIT	0

#define TWI_GENERAL_CALL 0x00 /* general call code SLA+W addressing all slaves */


/* TWI master state codes */
#define TWI_STATE_START			0x08 /* START condition transmitted */
#define TWI_STATE_REP_START 	0x10 /* repeated START condition transmitted */
#define TWI_STATE_ARB_LOST 		0x38 /* arbitration lost */

#define TWI_STATE_NO_STATE 		0xF8 /* no state info available / TWINT == 0 */
#define TWI_STATE_BUS_ERR		0x00 /* illegal START or STOP condition */

/* TWI status flags */
#define TWI_STATUS_LAST_TRANS_OK	0


void i2c_master_init(void);
uint8_t i2c_get_state(void);
uint8_t i2c_transceiver_busy(void);
void i2c_start_transceiver_with_data(unsigned char *, uint8_t);
void i2c_start_transceiver(void);
uint8_t i2c_get_data_from_transceiver(unsigned char *, uint8_t);


#endif
