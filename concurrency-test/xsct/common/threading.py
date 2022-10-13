from threading import Condition, RLock
from multiprocessing.pool import Pool


class CountdownLatch:
    def __init__(self, count):
        self.__count = count
        self.__condition = Condition()

    def countdown(self):
        with self.__condition:
            if self.__count == 0:
                return
            self.__count -= 1
            if self.__count == 0:
                self.__condition.notify_all()

    def wait(self):
        with self.__condition:
            if self.__count == 0:
                return
            self.__condition.wait()


# def synchronized(method):
#     def f(*args):
#         self = args[0]
#         self.mutex.acquire()
#         # print(method.__name__, 'acquired')
#         try:
#             return apply(method, args)
#         finally:
#             self.mutex.release()
#             # print(method.__name__, 'released')
#
#     return f


# def synchronize(klass, names=None):
#     """Synchronize methods in the given class.
#     Only synchronize the methods whose names are
#     given, or all methods if names=None."""
#     if isinstance(names, type('')):
#         names = names.split()
#     for (name, val) in klass.__dict__.items():
#         if callable(val) and name != '__init__' and \
#                 (names is None or name in names):
#             # print("synchronizing", name)
#             setattr(klass, name, synchronized(val))


class Synchronization:
    def __init__(self):
        self.mutex = RLock()


class Observer:
    def __init__(self):
        self.data = None

    def update(self):
        pass


class Observable(Synchronization):
    def __init__(self):
        self.__obs = []
        self.__changed = 0
        self.data = None
        Synchronization.__init__(self)

    def add_observer(self, observer: Observer):
        if observer not in self.__obs:
            self.__obs.append(observer)

    def delete_observer(self, observer: Observer):
        self.__obs.remove(observer)

    def notify_observers(self):
        """If 'changed' indicates that this object
        has changed, notify all its observers, then
        call clearChanged(). Each observer has its
        update() called with two arguments: this
        observable object and the generic 'arg'."""
        self.mutex.acquire()
        try:
            if not self.__changed:
                return
            # Make a local copy in case of synchronous
            # additions of observers:
            localArray = self.__obs[:]
            self.clear_changed()
        finally:
            self.mutex.release()
        # Updating is not required to be synchronized:
        for observer in localArray:
            observer.data = self.data
            observer.update()

    def delete_observers(self): self.__obs = []
    def set_changed(self): self.__changed = 1
    def clear_changed(self): self.__changed = 0
    def has_changed(self): return self.__changed
    def count_observers(self): return len(self.__obs)
#
#
# synchronize(Observable, "add_observer delete_observer delete_observers "
#                         "set_changed clear_changed has_changed "
#                         "count_observers")
