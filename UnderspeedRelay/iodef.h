/*
 * This file is part of AVRUnderspeedRelay.
 *
 * (c) Artyom Protaskin <a.protaskin@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef _IODEF_H_
#define _IODEF_H_

#include <avr/io.h>

#define RELAY_DDR  DDRB
#define RELAY_PORT PORTB
#define RELAY_PIN  PINB
#define RELAY_BIT  PB1

#define STATUS_DDR  DDRB
#define STATUS_PORT PORTB
#define STATUS_PIN  PINB
#define STATUS_BIT  PB0

#define PULSE_ADC_CHANNEL  _BV(MUX0) // PB2
#define DELAY_ADC_CHANNEL  _BV(MUX1) // PB4
#define TARGET_ADC_CHANNEL (_BV(MUX0) | _BV(MUX1)) // PB3

#define ADC_CHANNEL_MASK \
	(PULSE_ADC_CHANNEL | DELAY_ADC_CHANNEL | TARGET_ADC_CHANNEL)

#endif /* _IODEF_H_ */
