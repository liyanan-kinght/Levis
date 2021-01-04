"""
Control pixel ring on ReSpeaker 4 Mic Array

pip install pixel_ring gpiozero
"""

import time

from pixel_ring import pixel_ring
from gpiozero import LED

class cled_test(object):

    def __init__(self):
        #self.power = LED(5)
        #pixel_ring.set_brightness(10)
        pass
        
    def power_on_test(self):
        self.power = LED(5)
        self.power.on()
        pixel_ring.set_brightness(10)
    
    def power_off_test(self):
        self.power.off()

    def Led_lighton(self,dir):
        pixel_ring.wakeup(dir)
  
    def Led_lightoff(self):
	    pixel_ring.off()

if __name__ == '__main__':
    cled_obj = cled_test()
    cled_obj.power_on_test()

    while True:
        try:
            cled_obj.Led_lighton(0)
            time.sleep(3)
            cled_obj.Led_lighton(30)
            time.sleep(3)
            cled_obj.Led_lighton(60)
            time.sleep(3)
            cled_obj.Led_lighton(90)
            time.sleep(3)
            cled_obj.Led_lighton(120)
            time.sleep(3)
            cled_obj.Led_lighton(150)
            time.sleep(3)
            cled_obj.Led_lighton(180)
            time.sleep(3)
            cled_obj.Led_lighton(210)
            time.sleep(3)
            cled_obj.Led_lighton(240)
            time.sleep(3)
            cled_obj.Led_lighton(270)
            time.sleep(3)
            cled_obj.Led_lighton(300)
            time.sleep(3)
            cled_obj.Led_lighton(330)
            time.sleep(3)
            cled_obj.Led_lighton(360)
            time.sleep(3)
            cled_obj.Led_lightoff()
            time.sleep(3)
        except KeyboardInterrupt:
            break
    cled_obj.power_off_test()
    time.sleep(1)
