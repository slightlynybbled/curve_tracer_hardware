from pubser import PubSerial
import time


if __name__ == "__main__":
    ps = PubSerial()


def my_first_subscriber():
    print("the first subscriber was called!")
    print('data from the first callback', ps.get_data('bar'))


def my_second_subscriber():
    print("the second subscriber was called!")
    print('data from the second callback', ps.get_data('bar'))

if __name__ == "__main__":
    ps.subscribe('bar', my_first_subscriber)
    ps.subscribe('bar', my_second_subscriber)

    while True:
        ps.publish('baz', ['U16', 'U8'], [[-10, 20, 30], [-3, 4, 5]])
        time.sleep(5.0)
