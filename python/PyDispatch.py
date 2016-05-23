from frame import Frame
import threading
import time
import copy
import serial


class PyDispatch(object):
    """ Sends and receives blocks of data through a byte-aligned interface """
    format_specifiers = {0: 'NONE', 1: 'STRING', 2: 'U8', 3: 'S8', 4: 'U16', 5: 'S16', 6: 'U32', 7: 'S32', 8: 'FLOAT'}

    topical_data = {}
    subscribers = {}

    def __init__(self, port):
        """ Initializes frame and threading """
        if port:
            self.frame = Frame(port)

            self.run_thread = True
            self.thread = threading.Thread(target=self.run, args=())
            self.thread.daemon = True
            self.thread.start()
        else:
            print("Serial port not provided to PyDispatch")

    def close(self):
        self.run_thread = False

    def subscribe(self, topic, callback):
        """ Adds a callback method to a topic

        Args:
            topic: a string representing the topic which is being subscribed to
            callback: a function which is to be called when the topic is received
        """
        try:
            self.subscribers[topic].append(callback)
        except:
            self.subscribers[topic] = [callback]

    def unsubscribe(self, topic, callback):
        """ Removes the callback from the specified topic

        Args:
            topic: a string representing the topic which is being unsubscribed from
            callback: a function which is being removed from the subscribed topic
        """
        if self.subscribers[topic]:
            self.subscribers[topic].remove(callback)

        if len(self.subscribers[topic]) == 0:
            self.subscribers.pop(topic)

    def get_data(self, topic):
        """ Provides a method to receive the data relevant to a topic

        Args:
            topic: the topic string

        Returns:
            The data relating to a particular topic as a list of lists
        """
        return self.topical_data[topic]

    def publish(self, topic, data, format_specifier=['STRING']):
        """ Publishes data to a particular topic

        Args:
            topic: the topic to which to publish
            format_specifier: a list of format specifiers
            data: a list of lists, each internal list
                containing a complete set of data
        """

        # reverse the format specifier dictionary for convenience in this function
        format_specifiers = {}
        for key in self.format_specifiers.keys():
            value = self.format_specifiers[key]
            format_specifiers[value] = key

        length = len(data[0])
        dim = len(data)

        msg = []

        # create the header
        topic_bytes = bytearray(topic, 'utf-8')
        for e in topic_bytes:
            msg.append(e)
        msg.append(0)   # null string terminator

        msg.append(dim)
        msg.append(length & 255)
        msg.append((length & 65280) >> 8)

        for i, e in enumerate(format_specifier):
            fs = format_specifiers[e]
            if (i & 1) == 0:
                msg.append(fs & 15)
            else:
                msg[-1] += ((fs & 15) << 4)

        for i, e in enumerate(format_specifier):
            if e == 'NONE' or e == 'STRING':
                str_array = bytearray(data[0][0], 'utf-8')
                for e in str_array:
                    msg.append(e)
                msg.append(0)

            elif e == 'U8' or e == 'S8':
                unsigned_data8 = [x if x > 0 else x + 256 for x in data[i]]
                msg.extend(unsigned_data8)

            elif e == 'U16' or e == 'S16':
                unsigned_data16 = [x if x > 0 else x + 65536 for x in data[i]]

                for x in unsigned_data16:
                    msg.append(x & 255)
                    msg.append((x & 65280) >> 8)

            elif e == 'U32' or e == 'S32':
                unsigned_data32 = [x if x > 0 else x + 4294967296 for x in data[i]]

                for x in unsigned_data32:
                    msg.append(x & 255)
                    msg.append((x & 65280) >> 8)
                    msg.append((x & 16711680) >> 16)
                    msg.append((x & 4278190080) >> 24)

            else:
                print('width not supported')

        msg_to_send = copy.deepcopy(msg)
        self.frame.push_tx_message(msg_to_send)

        return msg

    def run(self):
        """ Monitors received data and properly formats it """
        while self.run_thread:
            while self.frame.rx_is_available():
                msg = self.frame.pull_rx_message()

                # find the first '0' in the msg
                zero_index = 0
                for i, element in enumerate(msg):
                    if element == 0:
                        zero_index = i
                        break

                topic = ''.join(chr(c) for c in msg[0:zero_index])
                msg = msg[zero_index + 1:]
                dim = msg.pop(0)
                length = msg.pop(0)
                length += msg.pop(0) * 256

                # pull off the format specifiers
                format_specifiers = []
                i = 0
                while True:
                    fs0 = msg.pop(0)
                    format_specifiers.append(self.format_specifiers[fs0 & 15])

                    i += 1
                    if i == dim:
                        break

                    format_specifiers.append(self.format_specifiers[(fs0 & 240) >> 4])

                    i += 1
                    if i == dim:
                        break

                for element in format_specifiers:
                    if element == 'STRING':
                        string_message = ''.join(chr(c) for c in msg)
                        self.topical_data[topic] = string_message

                    elif element == 'U8':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U8 from the msg
                        self.topical_data[topic].append(msg[0: length])
                        msg = msg[length:]

                    elif element == 'S8':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U8 from the msg
                        row = msg[0: length]

                        # apply the sign to each row
                        row = [(e - 256) if e > 127 else e for e in row]

                        self.topical_data[topic].append(row)
                        msg = msg[length:]

                    elif element == 'U16':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U16 from the msg
                        row8 = msg[0: length * 2]
                        row16 = []
                        while row8:
                            num16 = row8.pop(0)
                            num16 += row8.pop(0) * 256
                            row16.append(num16)

                        self.topical_data[topic].append(row16)
                        msg = msg[length * 2:]

                    elif element == 'S16':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U16 from the msg
                        row8 = msg[0: length * 2]
                        row16 = []
                        while row8:
                            num16 = row8.pop(0)
                            num16 += row8.pop(0) * 256
                            row16.append(num16)

                        # apply the sign to each row
                        row16 = [(e - 65536) if e > 32767 else e for e in row16]

                        self.topical_data[topic].append(row16)
                        msg = msg[length * 2:]

                    elif element == 'U32':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U16 from the msg
                        row8 = msg[0: length * 4]
                        row32 = []
                        while row8:
                            num32 = row8.pop(0)
                            num32 += row8.pop(0) * 256
                            num32 += row8.pop(0) * 65536
                            num32 += row8.pop(0) * 16777216
                            row32.append(num32)

                        self.topical_data[topic].append(row32)
                        msg = msg[length * 4:]

                    elif element == 'S32':
                        if not self.topical_data:
                            self.topical_data[topic] = []

                        # pull the next row of U16 from the msg
                        row8 = msg[0: length * 4]
                        row32 = []
                        while row8:
                            num32 = row8.pop(0)
                            num32 += row8.pop(0) * 256
                            num32 += row8.pop(0) * 65536
                            num32 += row8.pop(0) * 16777216
                            row32.append(num32)

                        # apply the sign to each row
                        row32 = [(e - 4294967296) if e > 2147483648 else e for e in row32]

                        self.topical_data[topic].append(row32)
                        msg = msg[length * 4:]

                    elif element == 'FLOAT':
                        pass

                    else:
                        print('unrecognized message type')

                # remove any data that doesn't have subscribers
                temp_dict = copy.deepcopy(self.topical_data)
                keys = temp_dict.keys()
                for key in keys:
                    if key not in self.subscribers:
                        self.topical_data.pop(key)

                    else:
                        # execute callbacks
                        for element in self.subscribers[key]:
                            element()

                        ''' at this point, the data should have
                        been consumed by the function and can
                        now be discarded, which helps keep memory
                        use low'''
                        self.topical_data.pop(key)

            time.sleep(0.1)

''' --------------------------------------------------------------------------
Everything below this line is intended to used in conjunction with the example
Dispatch c file.  It also provides a simple 'getting started' script.
--------------------------------------------------------------------------'''

if __name__ == "__main__":
    # define your serial port or supply it otherwise
    port = serial.Serial("COM9", baudrate=57600, timeout=0.1)

    # create a new instance of PyDispatch
    ps = PyDispatch(port)

    # create your subscribers
    def array_subscriber():
        # retrieve received data for topic 'bar' and print to screen
        print('data: ', ps.get_data('bar'))

    def i_subscriber():
        # retrieve received data for topic 'i' and print to screen
        print('i: ', ps.get_data('i'))

    # use the instance of PyDispatch to associate the subscriber function with the topic
    ps.subscribe('bar', array_subscriber)
    ps.subscribe('i', i_subscriber)

    # publish to topics as desired
    while True:
        ''' publish 'a test message', to subscribers of 'foo', note
            that the message must be in a list of lists '''
        ps.publish('foo', [['a test message']])
        time.sleep(0.2)