#!/usr/bin/env python3

'''
This file is part of AVRUnderspeedRelay.

(c) Artyom Protaskin <a.protaskin@gmail.com>

For the full copyright and license information, please view the LICENSE
file that was distributed with this source code.
'''

import math

MIN_TARGET_TICKS = 1000
MID_TARGET_TICKS = 100
MAX_TARGET_TICKS = 10

ref_points = [0, 6, 12, 20, 28, 39, 51, 67, 89, 128]
REF_POINTS_SIZE = 9 # The last element of the ref_points list is not counted

def calc_target_ticks(adc_value):
    if adc_value >= 0x80:
        adc_value -= 0x80
        rval = MID_TARGET_TICKS
        step = (MID_TARGET_TICKS - MAX_TARGET_TICKS) / REF_POINTS_SIZE
    else:
        rval = MIN_TARGET_TICKS
        step = (MIN_TARGET_TICKS - MID_TARGET_TICKS) / REF_POINTS_SIZE

    for i in reversed(range(REF_POINTS_SIZE)): # 8..0
        if adc_value >= ref_points[i]:
            adc_value -= ref_points[i]
            rval -= i * step
            if adc_value:
                rval -= int((step * adc_value) /
                    (ref_points[i + 1] - ref_points[i])) # The last element of the ref_points list is used only here
            break

    return rval

def accy_calc_target_ticks(adc_value):
    return pow(10, (3 - float(adc_value) / 128))

for adc_value in range(256):
    ticks = calc_target_ticks(adc_value)
    accy_ticks = accy_calc_target_ticks(adc_value)
    error = 100 * math.fabs(accy_ticks - ticks) / accy_ticks
    print('adc_value: %d; ticks: %d (approx), %.3f (accy); error: %.3f%%'
        % (adc_value, ticks, accy_ticks, error))
    if error > 15:
        print('WARNING: The error is greater than 15%')
    if adc_value and calc_target_ticks(adc_value - 1) < ticks:
        print('WARNING: The value is greater than the previous')
