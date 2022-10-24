from .imcx.imcx import main as imcx_test
from .common.cli import SuiteVars, StatusUpdate
from .common.threading import Observer


class StatusUpdateObserver(Observer):
    def update(self):
        status = self.data
        if status.report:
            print(status.data)
        else:
            print(" " * 64, end="\r")
            print(status.data, end="\r")


def main():
    print('Extensions Concurrency Test')
    print('(C) ketteQ Inc.')
    suite_vars = SuiteVars()
    suite_vars.status_observable = StatusUpdate()
    suite_vars.status_observable.status_notifier.add_observer(StatusUpdateObserver())
    imcx_test(suite_vars)
    pass


if __name__ == '__main__':
    main()
