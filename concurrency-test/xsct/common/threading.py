from threading import Condition


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
