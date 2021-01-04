"""
LED pattern 
"""

import time


class my_pattern(object):
    brightness = 24 * 8

    def __init__(self, show, number=12):
        self.pixels_number = number
        self.pixels = [0] * 4 * number

        if not callable(show):
            raise ValueError('show parameter is not callable')

        self.show = show
        self.stop = False

    def wakeup(self, direction=0):
        position = int((direction + 15) / (360 / self.pixels_number)) % self.pixels_number

        pixels = [0, 0, 0, self.brightness] * self.pixels_number
        pixels[position * 4 + 1] = self.brightness
        pixels[position * 4 + 3] = 0
        self.show(pixels)

    def off(self):
        self.show([0] * 4 * 12)
