/*
 * This file is part of AVRUnderspeedRelay.
 *
 * (c) Artyom Protaskin <a.protaskin@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#define MIN_DELAY_TICKS 100 // 1s
#define MAX_DELAY_TICKS 1000 // 10s
#define MIN_TARGET_TICKS 1000 // 10s
#define MID_TARGET_TICKS 100 // 1s
#define MAX_TARGET_TICKS 10 // 10ms

#define PULSE_LOW_LEVEL 0x19 // 5V/1023*(0x19<<2)=0.49V
#define PULSE_HIGH_LEVEL 0xE6 // 5V/1023*(0xE6<<2)=4.5V

#define UNDERSPEED_ERROR 0 // Without blinking

#ifdef DEBUG
#	define DEBUG_DELAY_MISCALC_ERROR 5
#	define DEBUG_TARGET_MISCALC_ERROR 6
#	define DEBUG_MISSED_COMPARISON_ERROR 7
#	define DEBUG_MISSED_TICK_ERROR 8
#endif

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "iodef.h"

static const uint8_t ref_points[] = {0, 6, 12, 20, 28, 39, 51, 67, 89, 128};
#define REF_POINTS_SIZE 9 // The last element of the ref_points array is not counted

static uint16_t delay_ticks;
static uint16_t target_ticks;

static void error_loop(uint8_t error_code)
{
	RELAY_PORT &= ~_BV(RELAY_BIT);
	STATUS_PORT &= ~_BV(STATUS_BIT);

	while (true) {
		for (uint16_t i = 0; i < error_code * 2; i++) {
			STATUS_PORT ^= _BV(STATUS_BIT);
			_delay_ms(500);
		}

		_delay_ms(1500);
	}
}

/**
 * Calculates time (in ticks) to delay for the given ADC value.
 *
 * Uses bit shifting in order to increase the accuracy of the calculation
 * without using 32-bit integers or floats.
 *
 * delay_ticks = MIN + (adc_value / UINT8_MAX) * (MAX - MIN) =
 *             = MIN + adc_value * ((MAX - MIN) / UINT8_MAX) =
 *             = MIN + adc_value / (UINT8_MAX / (MAX - MIN)) =
 *             = MIN + (k * adc_value) / ((k * UINT8_MAX) / (MAX - MIN))
 */
static uint16_t calc_delay_ticks(uint8_t adc_value)
{
	return MIN_DELAY_TICKS + ((uint16_t) adc_value << 8) /
		(((uint16_t) UINT8_MAX << 8) / (MAX_DELAY_TICKS - MIN_DELAY_TICKS));
}

/**
 * Calculates time (in ticks) to wait for a pulse for the given ADC value.
 */
static uint16_t calc_target_ticks(uint8_t adc_value)
{
	uint8_t i;
	uint8_t step;
	uint16_t ticks;

	if (adc_value >= 0x80) {
		adc_value -= 0x80;
		ticks = MID_TARGET_TICKS;
		step = (MID_TARGET_TICKS - MAX_TARGET_TICKS) / REF_POINTS_SIZE;
	} else {
		ticks = MIN_TARGET_TICKS;
		step = (MIN_TARGET_TICKS - MID_TARGET_TICKS) / REF_POINTS_SIZE;
	}

	i = REF_POINTS_SIZE;
	while (i--) {
		if (adc_value >= ref_points[i]) {
			adc_value -= ref_points[i];
			ticks -= i * step;
			if (adc_value) {
				ticks -= (step * adc_value) / (ref_points[i + 1] - ref_points[i]);
			}
			break;
		}
	}

	return ticks;
}

ISR(TIM0_COMPA_vect) // T=10.0266ms
{
#ifdef DEBUG
	TIFR0 |= _BV(OCF0A); // Proteus 8.4 SP1 does not clear the compare flag
#endif

	static uint16_t tick_count = 0;
	static bool pulse = true;

	if (delay_ticks) {
		delay_ticks--;
		return;
	}

	tick_count++;

#ifdef DEBUG
if (tick_count > target_ticks) {
	error_loop(DEBUG_MISSED_COMPARISON_ERROR);
}
#endif

	ADMUX &= ~ADC_CHANNEL_MASK;
	ADMUX |= TARGET_ADC_CHANNEL;
	ADCSRA |= _BV(ADSC);
	while (ADCSRA & _BV(ADSC));
	target_ticks = calc_target_ticks(ADCH);

#ifdef DEBUG
	if (target_ticks > MIN_TARGET_TICKS || target_ticks < MAX_TARGET_TICKS) {
		error_loop(DEBUG_TARGET_MISCALC_ERROR);
	}
#endif

	ADMUX &= ~ADC_CHANNEL_MASK;
	ADMUX |= PULSE_ADC_CHANNEL;
	ADCSRA |= _BV(ADSC);
	while (ADCSRA & _BV(ADSC));

	if (pulse) {
		pulse = (ADCH > PULSE_LOW_LEVEL);
		if (!pulse) {
			tick_count = 0; // Drop the counter on a falling edge
		}
	} else {
		pulse = (ADCH > PULSE_HIGH_LEVEL);
	}

 	if (tick_count >= target_ticks) {
		error_loop(UNDERSPEED_ERROR);
	}

#ifdef DEBUG
	if (TIFR0 & _BV(OCF0A)) {
		error_loop(DEBUG_MISSED_TICK_ERROR);
	}
#endif
}

int main(void)
{
	// Configure the system clock prescaler
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS2); // Select the /16 prescaler

	// Configure the i/o
	RELAY_DDR |= _BV(RELAY_BIT);
	STATUS_DDR |= _BV(STATUS_BIT);

	// Configure the timer
	OCR0A = 93; // 600kHz/64/(93+1)=100Hz
	TCCR0A |= _BV(WGM01); // Select CTC mode
	TCCR0B |= _BV(CS01) | _BV(CS00); // Select the i/o clock as source with the /64 prescaler

	// Configure the ADC
	ADMUX |= _BV(ADLAR); // Left adjust the result
	ADCSRA |= _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0); // Enable the ADC and select the /8 prescaler (75kHz)

	ADMUX |= DELAY_ADC_CHANNEL; // Select an ADC channel
	ADCSRA |= _BV(ADSC); // Start a conversion
	while (ADCSRA & _BV(ADSC)); // Wait until the conversion is complete
	delay_ticks = calc_delay_ticks(ADCH);

#ifdef DEBUG
	if (delay_ticks < MIN_DELAY_TICKS || delay_ticks > MAX_DELAY_TICKS + 10) { // Acceptable error
		error_loop(DEBUG_DELAY_MISCALC_ERROR);
	}

	target_ticks = 1; // The initial value is used to pass the first check for missed comparison
#endif

	RELAY_PORT |= _BV(RELAY_BIT);
	STATUS_PORT |= _BV(STATUS_BIT);

	TIMSK0 |= _BV(OCIE0A); // Enable Timer0 interrupt

	sei();
	while (true);
}
