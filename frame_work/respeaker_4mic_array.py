"""
Control pixel ring on ReSpeaker 4 Mic Array

pip install pixel_ring gpiozero
"""

import time

from pixel_ring import pixel_ring
from gpiozero import LED

power = LED(5)
power.on()
pixel_ring.set_brightness(10)

def Led_lighton(dir):
	pixel_ring.wakeup(dir)
  
def Led_lightoff():
	pixel_ring.off()
