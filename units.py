from base import Soldier, Unit
import soldiers

class MonsterUnit(Unit):
    width = 1
    maxSoldiers = 1
    spacing = 0
    wideSpacing = 0
    soldierClass = soldiers.Monster

    def __init__(self):
        super().__init__()

class CavalryUnit(Unit):
    width = 4
    maxSoldiers = 16
    spacing = 18
    wideSpacing = 20
    soldierClass = soldiers.Rider

    def __init__(self):
        super().__init__()

class SimpleUnit(Unit):
    width = 8
    maxSoldiers = 64
    spacing = 12
    wideSpacing = 15
    soldierClass = Soldier
    
    def __init__(self):
        super().__init__()
