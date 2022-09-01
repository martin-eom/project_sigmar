import numpy
from base import Soldier

class Monster(Soldier):
    rad = 15
    mass = 9.
    defaultMaxSpeed = 100.
    accel = 4.
    turn = numpy.pi / 5

    def __init__(self, unit = None):
        super().__init__(unit)

class Rider(Soldier):
    rad = 7
    mass = 3.
    defaultMaxSpeed = 40.
    accel = 3.
    turn = numpy.pi / 3

    def __init__(self, unit = None):
        super().__init__(unit)
                                
