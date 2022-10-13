import traceback

from ..common.threading import CountdownLatch
from ..common.cli import SuiteVars, StatusUpdate
from threading import Thread
import time
import psycopg2

# Defaults

db_host = 'localhost'
db_port = 5432
db_user = 'postgres'
db_password = '123456'
db_name = 'ketteq_7f046426-b221-41e7-9a12-98d967d3becc'

cc_threads = 2
cc_add_days_repetitions = 1

# Queries

q_create_extension = 'CREATE EXTENSION IF NOT EXISTS kq_imcx'
q_drop_extension = 'DROP EXTENSION IF EXISTS kq_imcx'
q_invalidate = 'SELECT * FROM kq_invalidate_calendar_cache()'
q_add_days = "SELECT * FROM kq_add_days('2008-01-15', 1, 'quarter')"
q_slices_names = ['week', 'month', 'quarter', 'year', 'day']

"""
This class defines the thread that will execute the In-Memory Calendar Extension concurrency test,
below there is the main function of the file, in which a certain number of threads will be
spawned at the same time. Each instance will race to execute the extension functions.
"""


class IMCXThread:
    __connection = None

    def __init__(self, thread_number, latch, status_observable: StatusUpdate):
        self.__thread_number = thread_number
        self.__latch = latch
        self.__start = None
        self.__status_observable = status_observable
        self.__status_observable.emit(f'Creating thread {self.__thread_number}...')
        self.__failed_tests = 0
        self.__thread = Thread(
            target=self.__thread_main_target
        )

    def __status(self, status, report=False):
        self.__status_observable.emit(f"Thread [{self.__thread_number}:{self.__thread.native_id}] " + status, report)

    def start(self):
        self.__start = time.time()
        self.__thread.start()

    def __abort(self, error_message):
        self.__status('ERROR: ' + error_message, True)
        self.__latch.countdown()

    def __connect(self):
        self.__status('Connecting...')
        self.__connection = psycopg2.connect(database=db_name, user=db_user, password=db_password,
                                             host=db_host, port=db_port)
        self.__status('Connected')

    def __drop_extension(self):
        self.__status("DROP EXTENSION 'kq_imcx'")
        self.__connection.cursor().execute(q_drop_extension)
        self.__connection.commit()
        self.__status(f'DROP EXECUTED OK')

    def __create_extension(self):
        self.__status("CREATE EXTENSION 'kq_imcx'")
        self.__connection.cursor().execute(q_create_extension)
        self.__connection.commit()
        self.__status(f'CREATE EXECUTED OK')

    def __invalidate_cache(self):
        start = time.time()
        try:
            self.__connection.cursor().execute(q_invalidate)
            self.__status(f'kq_invalidate_calendar_cache() -> OK -> Time: {time.time() - start}', True)
        except Exception:
            traceback.print_exc()
            self.__failed_tests += 1
            self.__status(f'kq_invalidate_calendar_cache() -> FAILED -> Time: {time.time() - start}', True)

    def __add_days(self):
        start = time.time()
        for cc in range(cc_add_days_repetitions):
            try:
                self.__connection.cursor().execute(q_add_days)
                self.__status(f'kq_add_days() -> OK -> Time: {time.time() - start}', True)
            except Exception:
                traceback.print_exc()
                self.__failed_tests += 1
                self.__status(f'kq_invalidate_calendar_cache() -> FAILED -> Time: {time.time() - start}', True)

    def __disconnect(self):
        self.__connection.commit()

    def __thread_main_target(self):
        self.__connect()
        self.__invalidate_cache()
        self.__add_days()
        self.__disconnect()
        self.__status(f'Thread {self.__thread_number} completed. '
                      f'Time: {time.time() - self.__start} '
                      f'Failed Tests: {self.__failed_tests}/{(1 + cc_add_days_repetitions)}', True)
        self.__latch.countdown()


def __create_thread(thread_number, latch: CountdownLatch, suite_vars: SuiteVars):
    return IMCXThread(thread_number, latch, suite_vars.status_observable)


def __init_db(status_observable: StatusUpdate):
    status_observable.emit(f'[INIT] Connecting...')
    try:
        connection = psycopg2.connect(database=db_name, user=db_user, password=db_password,
                                      host=db_host, port=db_port)
        status_observable.emit(f'[INIT] Connected')
        cursor = connection.cursor()
        cursor.execute(q_drop_extension)
        cursor.execute(q_create_extension)
        status_observable.emit(f'[INIT] Successful', True)
        return True
    except:
        status_observable.emit(f'[INIT] Connection Failed', True)
        return False


def main(suite_vars: SuiteVars):
    suite_vars.status_observable.emit(f'> Starting In-Memory Calendar Extension -> Threads: {cc_threads}', True)
    if not __init_db(suite_vars.status_observable):
        suite_vars.status_observable.emit(f'> The database cannot be initialized, check server hostname', True)
        exit(-1)
    threads = []
    latch = CountdownLatch(cc_threads)
    for c in range(cc_threads):
        threads.append(__create_thread(c, latch, suite_vars))
    for thread in threads:
        thread.start()
    latch.wait()
    print('> DONE <')
