#include <stdint.h>
#include <avr/interrupt.h>
#include "i2cmaster.h"

static unsigned char twi_buffer[TWI_BUFFER_SIZE];
static uint8_t twi_packet_size;
static uint8_t twi_state;
static uint8_t twi_status_flags;

void i2c_master_init(void)
{
	twi_status_flags &= (0 << TWI_STATUS_LAST_TRANS_OK);
	TWBR = TWI_BIT_RATE;
	TWDR = 0xFF; /* not sure why we do this but ref implementation does it */
	TWCR = (1 << TWEN)  | /* enable TWI interface */
		   (0 << TWIE)  | /* clear TWI interrupt */
		   (0 << TWINT) | /* clearwarning: control reaches end of non-void function interrupt flag */
		   (0 << TWEA)  | /* clear ACK bit*/
		   (0 << TWSTA) | /* request bus master */
		   (0 << TWSTO) | /* clear STOP condition */
		   (0 << TWWC);	  /* clear attempt to write bit */
}

uint8_t i2c_get_state(void)
{
	return twi_state;
}

uint8_t i2c_transceiver_busy(void)
{
	/* while TWI interrupt bit is set */
	return (TWCR & (1 << TWIE));
}

void i2c_start_transceiver_with_data(unsigned char *d, uint8_t size)
{
	while(i2c_transceiver_busy());

	twi_packet_size = size;
	twi_buffer[0] = d[0]; /* slave addr and r/w flag */

	/* if read bit is not set, copy data to write */
	if(!(d[0] & 1)) {
		uint8_t x;
		for(x = 1; x < size; x++) {
			twi_buffer[x] = d[x];
		}
	}

	twi_state = TWI_STATE_NO_STATE;
	TWCR = (1 << TWEN)  | /* enable TWI, should be enabled already though? */
		   (1 << TWIE)  | /* enable TWI interrupt request */
		   (1 << TWINT) | /* clear interrupt flag */
		   (0 << TWEA)  | /* disable ACK */
		   (1 << TWSTA) | /* generate START condition */
		   (0 << TWSTO) | /* clear STOP flag */
		   (0 << TWWC);   /* clear write collision flag */
}

void i2c_start_transceiver(void)
{
	while(i2c_transceiver_busy());

	twi_state = TWI_STATE_NO_STATE;
	TWCR = (1 << TWEN)  | /* enable TWI, should be enabled already though? */
		   (1 << TWIE)  | /* enable TWI interrupt request */
		   (1 << TWINT) | /* clear interrupt flag */
		   (0 << TWEA)  | /* disable ACK */
		   (1 << TWSTA) | /* generate START condition */
		   (0 << TWSTO) | /* clear STOP flag */
		   (0 << TWWC);   /* clear write collision flag */
}

uint8_t i2c_get_data_from_transceiver(unsigned char *d, uint8_t size)
{
	while(i2c_transceiver_busy());
	if(twi_status_flags & (1 << TWI_STATUS_LAST_TRANS_OK)) {
		int x;
		for(x = 0; x < size; x++) {
			d[x] = twi_buffer[x];
		}
	}

	return twi_status_flags & (1 << TWI_STATUS_LAST_TRANS_OK);
}

ISR(TWI_vect)
{
	static uint8_t twi_buffer_ptr;

	switch(TWSR) {
		case TW_START:			/* START condition transmitted */
		case TW_REP_START:  	/* START condition repeat transmitted */
			twi_buffer_ptr = 0;
			/* no break here... */
		case TW_MT_SLA_ACK:		/* SLA+W transmitted, ACK received */
		case TW_MT_DATA_ACK:	/* data byte transmitted, ACK received */
			if(twi_buffer_ptr < twi_packet_size) {
				TWDR = twi_buffer[twi_buffer_ptr++]; /* set data register to next byte to transmit */
				TWCR = (1 << TWEN)  | /* enable twi */
					   (1 << TWIE)  | /* enable twi interrupt */
					   (1 << TWINT) | /* clear last int flag to send another */
					   (0 << TWEA)  | /* clear ACK */
					   (0 << TWSTA) | /* clear START */
					   (0 << TWSTO) | /* clear STOP */
					   (0 << TWWC);   /* clear write collision */
			} else { /* we've sent the entire packet, so send STOP*/
				twi_status_flags |= (1 << TWI_STATUS_LAST_TRANS_OK);
				TWCR = (1 << TWEN)  | /* enable twi */
					   (0 << TWIE)  | /* interrupt off */
					   (1 << TWINT) | /* clear for next send  */
					   (0 << TWEA)  | /* clear ACK */
					   (0 << TWSTA) | /* clear START */
					   (1 << TWSTO) | /* send STOP */
					   (0 << TWWC);   /* clear collision */
			}

			break;

		case TW_MR_DATA_ACK: /* data byte recieved, ACK transmitted */
			twi_buffer[twi_buffer_ptr++] = TWDR;
			/* no break here... */

		case TW_MR_SLA_ACK: /* SLA+R transmitted, ACK received */
			if(twi_buffer_ptr < (twi_packet_size - 1)) { /* if not last byte ACK, else we NACK */
				TWCR = (1 << TWEN)  | /* enable twi */
					   (1 << TWIE)  | /* enable twi interrupt */
					   (1 << TWINT) | /* clear for next send */
					   (1 << TWEA)  | /* send ACK */
					   (0 << TWSTA) | /* clear START */
					   (0 << TWSTO) | /* clear STOP */
					   (0 << TWWC);   /* clear collision */
			} else { /* we shall NACK */
				TWCR = (1 << TWEN)  | /* enable twi */
					   (1 << TWIE)  | /* enable twi interrupt */
					   (1 << TWINT) | /* clear for next send */
					   (0 << TWEA)  | /* clear ACK */
					   (0 << TWSTA) | /* clear START */
					   (0 << TWSTO) | /* clear STOP */
					   (0 << TWWC);   /* clear collision */
			}

			break;

		case TW_MR_DATA_NACK: /* byte recwarning: control reaches end of non-void functioneived and NACK transmitted */
			twi_buffer[twi_buffer_ptr] = TWDR; /* copy last byte into our buffer */
			twi_status_flags |= (1 << TWI_STATUS_LAST_TRANS_OK); /* mark successful transaction */

			TWCR = (1 << TWEN)  | /* enable twi */
				   (0 << TWIE)  | /* disable twi interrupt */
				   (1 << TWINT) | /* clear flag for last send */
				   (0 << TWEA)  | /* clear ACK */
				   (0 << TWSTA) | /* clear START */
				   (1 << TWSTO) | /* send STOP */
				   (0 << TWWC);   /* clear collision */

			break;

		case TW_MT_ARB_LOST: /* lost arbitration, so we re-START */
			TWCR = (1 << TWEN)  | /* enable twi */
				   (1 << TWIE)  | /* enable twi interrupt */
				   (1 << TWINT) | /* clear flag for last send */
				   (0 << TWEA)  | /* clear ACK */
				   (1 << TWSTA) | /* send START */
				   (0 << TWSTO) | /* clear STOP */
				   (0 << TWWC); /* clear collision */

			break;

		case TW_MT_SLA_NACK:
		case TW_MR_SLA_NACK:
		case TW_MT_DATA_NACK:
		case TW_BUS_ERROR:
		default:
			twi_state = TWSR;
			/* reset twi interface */
			TWCR = (1 << TWEN)  | /* enable twi */
				   (0 << TWIE)  | /* no interrupt */
				   (0 << TWINT) | /* no cleared interrupt flag */
				   (0 << TWEA)  | /* clear ACK */
				   (0 << TWSTA) | /* no START */
				   (0 << TWSTO) | /* no SOP */
				   (0 << TWWC);   /* clear collision */


	}
}
