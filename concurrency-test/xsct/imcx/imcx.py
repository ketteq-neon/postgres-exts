from ..common.threading import CountdownLatch
from threading import Thread
import time
import psycopg2

# Defaults

db_host = 'localhost'
db_port = '5432'
db_user = 'postgres'
db_password = '123456'
db_name = 'ketteq_7f046426-b221-41e7-9a12-98d967d3becc'

cc_threads = 10

# Queries

q_create_extension = 'CREATE EXTENSION IF NOT EXISTS kq_imcx'
q_drop_extension = 'DROP EXTENSION kq_imcx'
q_invalidate = 'SELECT * FROM kq_invalidate_calendar_cache()'
q_add_days = "SELECT * FROM kq_add_days('2008-01-15', 1, 'quarter')"

class IMCXThread:
    __thread = None
    __connection = None
    __thread_number = 0
    __latch = None

    def __init__(self, thread_number, latch):
        self.__thread_number = thread_number
        self.__latch = latch
        print(f'Creating thread {self.__thread_number}...')
        self.__thread = Thread(
            target=self.__thread_concrete
        )

    def start(self):
        self.__thread.start()

    def __connect(self):
        self.__connection = psycopg2.connect(database=db_name, user=db_user, password=db_password,
                                             host=db_host, port=db_port)

    def __thread_concrete(self):
        self.__connect()
        time.sleep(1)
        print(f'Thread {self.__thread_number} completed.')
        self.__latch.countdown()


def __create_thread(thread_number, latch: CountdownLatch):
    return IMCXThread(thread_number, latch)


def main():
    print('> In-Memory Calendar Extension Concurrency Test <')
    threads = []
    latch = CountdownLatch(cc_threads)
    for c in range(cc_threads):
        threads.append(__create_thread(c, latch))
    for thread in threads:
        thread.start()
    latch.wait()
    print('> DONE <')
