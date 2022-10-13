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


class IMCXThread:
    __thread = None

    def __init__(self, thread_number, latch):
        print(f'Creating thread {thread_number}...')
        self.__thread = Thread(
            target=self.__thread_concrete, args=(thread_number,latch)
        )

    def start(self):
        self.__thread.start()

    @staticmethod
    def __thread_concrete(thread_number, latch: CountdownLatch):
        time.sleep(1)
        print(f'Thread {thread_number} completed.')
        latch.countdown()


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
