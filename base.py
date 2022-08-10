from events import *
from itertools import chain
from numpy import array


# Event Manager
class EventManager:
    
    def __init__(self):
        from weakref import WeakKeyDictionary
        self.noPrint = [TickEvent, GameStateRequest, GameStateUpdate, PrivateRemoteEvent]
        self.listeners = WeakKeyDictionary()
        self.eventQueue = []
        self.fps = 30
        self.dt = 1 / self.fps

    def RegisterListener(self, listener):
        self.listeners[listener] = True

    def UnregisterListener(self, listener):
        if listener in self.listeners:
            del self.listeners[listener]

    def Post(self, event):
        if not event.__class__ in self.noPrint:
            print(" - Event Posted: " + event.name)
        for listener in self.listeners:
            listener.Notify(event)

    def ConsumeEventQueue(self):
        i = 0
        while i < len(self.eventQueue):
            event = self.eventQueue[i]
            for listener in self.listeners:
                listener.Notify(event)
            i += 1
        self.eventQueue = []


def countNestedListNotNone(soldiers):
    return len([1 for soldier in list(chain(*soldiers)) if not soldier is None])
