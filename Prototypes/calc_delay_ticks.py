#!/usr/bin/env python3

'''
This file is part of AVRUnderspeedRelay.

(c) Artyom Protaskin <a.protaskin@gmail.com>

For the full copyright and license information, please view the LICENSE
file that was distributed with this source code.
'''

import math

UINT8_MAX = 255

MIN_DELAY_TICKS = 100
MAX_DELAY_TICKS = 1000

def calc_delay_ticks(adc_value):
	return (MIN_DELAY_TICKS + (adc_value << 8) /
		((UINT8_MAX << 8) / (MAX_DELAY_TICKS - MIN_DELAY_TICKS)))

def accy_calc_delay_ticks(adc_value):
	return (MIN_DELAY_TICKS +
		(float(adc_value) / 255) * (MAX_DELAY_TICKS - MIN_DELAY_TICKS))

for adc_value in range(256):
	ticks = calc_delay_ticks(adc_value)
	accy_ticks = accy_calc_delay_ticks(adc_value)
	error = 100 * math.fabs(accy_ticks - ticks) / accy_ticks
	print('adc_value: %d; ticks: %d (approx), %.3f (accy); error: %.3f%%'
		% (adc_value, ticks, accy_ticks, error))
	if error > 1:
		print('WARNING: The error is greater than 1%')
	if adc_value and calc_delay_ticks(adc_value - 1) > ticks:
		print('WARNING: The value is less than the previous')
