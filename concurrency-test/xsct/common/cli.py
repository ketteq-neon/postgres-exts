from .threading import Synchronization, Observable


class SuiteVars:
    def __init__(self):
        self.status_observable = None


class StatusUpdate:
    def __init__(self):
        self.data = ''
        self.report = False
        self.status_notifier = StatusNotifier(self)

    def emit(self, status, report=False):
        self.data = status
        self.report = report
        self.status_notifier.notify_observers()

    def status_observable(self):
        return self.status_notifier


class StatusNotifier(Observable):
    def __init__(self, outer: StatusUpdate):
        Observable.__init__(self)
        self.__outer = outer
        self.__prev_data = outer.data

    def notify_observers(self, arg=None):
        if len(self.__outer.data) > 0 and self.__outer.data != self.__prev_data:
            self.set_changed()
            self.data = self.__outer
            Observable.notify_observers(self)
            self.__prev_data = self.__outer.data

    def close(self):
        self.__prev_data = ''

