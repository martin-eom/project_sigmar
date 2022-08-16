### Contains shapes to be drawn.

from numpy import array, dot


class Arrow:

    line_length = 110
    line_width = 4
    triangle_length = 30
    triangle_width = 20
    dl = line_length - triangle_length
    dw = line_width // 2
    dt = triangle_width // 2
    
    def __init__(self, startPos, rot):
        self.points = []
        self.points.append(array(startPos) + dot(rot, [0, Arrow.dw]))
        self.points.append(array(startPos) + dot(rot, [0, -Arrow.dw]))
        self.points.append(array(startPos) + dot(rot, [Arrow.dl, -Arrow.dw]))
        self.points.append(array(startPos) + dot(rot, [Arrow.dl, -Arrow.dt]))
        self.points.append(array(startPos) + dot(rot, [Arrow.line_length, 0]))
        self.points.append(array(startPos) + dot(rot, [Arrow.dl, Arrow.dt]))
        self.points.append(array(startPos) + dot(rot, [Arrow.dl, Arrow.dw]))
