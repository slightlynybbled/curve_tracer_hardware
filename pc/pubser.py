from frame import Frame
import threading
import time
import copy


class PubSerial(object):
    format_specifiers = {0: 'NONE', 1: 'STRING', 2: 'U8', 3: 'S8', 4: 'U16', 5: 'S16', 6: 'U32', 7: 'S32', 8: 'FLOAT'}
    topical_data = {}
    subscribers = {}

    def __init__(self):
        self.frame = Frame()
        self.data = {}

        thread = threading.Thread(target=self.run, args=())
        thread.daemon = True
        thread.start()

    def subscribe(self, topic, callback):
        self.subscribers[topic] = callback

    def run(self):
        while True:
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

                print('________________________')
                print(topic, dim, length, format_specifiers, msg)

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
                print(self.topical_data)

                temp_dict = copy.deepcopy(self.topical_data)
                keys = temp_dict.keys()
                for key in keys:
                    print(key)
                    if key not in self.subscribers:
                        self.topical_data.pop(key)

            time.sleep(0.1)


if __name__ == "__main__":
    ps = PubSerial()
    while True:
        pass
