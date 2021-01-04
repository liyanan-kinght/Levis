import time
from .apa102 import APA102
from .my_pattern import my_pattern

class MYPixelRing(object):
    PIXELS_N = 12

    def __init__(self):
        self.pattern = my_pattern(show=self.show)
        self.dev = APA102(num_led=self.PIXELS_N)
        self.off()

    def set_brightness(self, brightness):
        if brightness > 100:
            brightness = 100

        if brightness > 0:
            self.dev.global_brightness = int(0b11111 * brightness / 100)

    def wakeup(self, direction=0):
        self.pattern.wakeup(direction)

    def off(self):
        self.pattern.off

    def show(self, data):
        for i in range(self.PIXELS_N):
            self.dev.set_pixel(i, int(data[4*i + 1]), int(data[4*i + 2]), int(data[4*i + 3]))
        self.dev.show()

if __name__ == '__main__':
    pixel_ring = MYPixelRing()
    while True:
        try:
            pixel_ring.wakeup(0)
            time.sleep(3)
            pixel_ring.wakeup(30)
            time.sleep(3)
            pixel_ring.off()
            time.sleep(3)
        except KeyboardInterrupt:
            break


    pixel_ring.off()
    time.sleep(1)