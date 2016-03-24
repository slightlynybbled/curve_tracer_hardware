import threading
import serial
import time


class Frame(object):
    # some useful constants
    SOF = int(b'f7', 16)
    EOF = int(b'7f', 16)
    ESC = int(b'f6', 16)
    ESC_XOR = int(b'20', 16)

    def __init__(self):
        # initialize the serial port
        try:
            self.port = serial.Serial("COM9", baudrate=57600, timeout=0.1)
            self.port.flushInput()
            self.raw = []
            self.messages = []

            thread = threading.Thread(target=self.run, args=())
            thread.daemon = True
            thread.start()

        except serial.SerialException:
            self.port = None
            self.raw = []
            print("Error accessing the serial port")

    def __del__(self):
        if self.port is not None:
            self.port.close()

    def fletcher16_checksum(self, data, check_value):
        sum1 = 0xff
        sum2 = 0xff

        check_summed_data = data[:]

        for b in check_summed_data:
            sum1 += b
            sum1 &= 0xffff  # Results wrapped at 16 bits
            sum2 += sum1
            sum2 &= 0xffff

            print(sum1, sum2)

        sum1 = (sum1 & 0x00ff) + (sum1 >> 8)
        sum2 = (sum2 & 0x00ff) + (sum2 >> 8)

        print(sum1, sum2)

        checksum = ((sum2 * 256) & 0xffff) | sum1

        print('check_value: ', check_value)
        print('checksum: ', checksum)

        if checksum == check_value:
            return True
        else:
            return False

    def run(self):
        while True:
            for element in self.port.read(1000):
                self.raw.append(element)

            # pull out the frames
            while (self.SOF in self.raw) and (self.EOF in self.raw):
                print('\nraw: ', self.raw)

                # find the SOF by removing bytes in front of it
                while self.raw[0] != self.SOF:
                    removed = self.raw.pop(0)
                    print('removed: ', removed)

                # find the EOF
                for i, element in enumerate(self.raw):
                    if element == self.EOF:
                        end_char_index = i
                        break
                frame = self.raw[:end_char_index]
                self.raw = self.raw[end_char_index:]

                # remove the SOF and EOF from the frame
                frame.pop(0)

                message = []
                escape_flag = False
                for element in frame:
                    if escape_flag is False:
                        if element == self.ESC:
                            escape_flag = True
                        else:
                            message.append(element)
                    else:
                        message.append(element ^ self.ESC_XOR)
                        escape_flag = False

                print('message w/ checksum: ', message)
                # remove the fletcher16 checksum
                f16_check = message.pop(-1) * 256
                f16_check += message.pop(-1)

                # TODO: calculate the checksum
                if self.fletcher16_checksum(message, f16_check):
                    message = []
                else:
                    self.messages.append(message)

                print('message: ', message)
                print('messages: ', self.messages)

            time.sleep(0.1)


if __name__ == "__main__":
    f = Frame()
    while True:
        pass

