import threading
import serial
import time


class Frame(object):
    """  Read/Write of serial port, framing and de-framing of data  """

    # some useful constants
    SOF = int(b'f7', 16)
    EOF = int(b'7f', 16)
    ESC = int(b'f6', 16)
    ESC_XOR = int(b'20', 16)

    def __init__(self):
        """ Initializes the serial port and creates object threads """
        # initialize the serial port
        try:
            self.port = serial.Serial("COM9", baudrate=57600, timeout=0.1)
            self.port.flushInput()
            self.raw = []
            self.rx_messages = []

            thread = threading.Thread(target=self.run, args=())
            thread.daemon = True
            thread.start()

        except serial.SerialException:
            self.port = None
            self.raw = []
            print("Error accessing the serial port")

    def __del__(self):
        """ Closes the serial port """
        if self.port is not None:
            self.port.close()

    def rx_is_available(self):
        """ Determines if rx data is available and returns the value

        Returns:
            True if de-framed data is available to be read
            False if otherwise
        """
        if len(self.rx_messages) > 0:
            return True
        else:
            return False

    def pull_rx_message(self):
        """ Accesses the rx message and returns it as a list

        Returns:
            the received, de-framed message as a list
        """
        msg = self.rx_messages.pop(0)
        return msg

    def push_tx_message(self, msg):
        """ Sends a message, performing all framing transparently

        Args:
            msg: a list containing the message to be sent
        """
        # start the frame
        frame = [self.SOF]

        # calc the checksum before the framing bits are added
        checksum = self.fletcher16_checksum(msg)
        msg.append(checksum & 0x00ff)
        msg.append((checksum & 0xff00) >> 8)

        for element in msg:
            if element == self.SOF or element == self.EOF or element == self.ESC:
                frame.append(self.ESC)
                frame.append(self.ESC_XOR ^ element)
            else:
                frame.append(element)

        frame.append(self.EOF)
        self.port.write(bytearray(frame))

        return

    def fletcher16_checksum(self, data):
        """ Calculates the fletcher16 checksum on a list of data

        Args:
            data: a list of 8-bit data on which to calculate the checksum

        Returns:
            the fletcher16 checksum
        """
        sum1 = 0xff
        sum2 = 0xff

        check_summed_data = data[:]

        for b in check_summed_data:
            sum1 += b
            sum1 &= 0xffff  # Results wrapped at 16 bits
            sum2 += sum1
            sum2 &= 0xffff

        sum1 = (sum1 & 0x00ff) + (sum1 >> 8)
        sum2 = (sum2 & 0x00ff) + (sum2 >> 8)

        checksum = ((sum2 * 256) & 0xffff) | sum1

        return checksum

    def run(self):
        """ Represents the continually executed thread of the object

        This function continually monitors the serial port and de-frames
        the data.  When a complete frame has been received and verified,
        it attaches the de-framed data to the outgoing que to be read.

        """
        while True:
            for element in self.port.read(1000):
                self.raw.append(element)

            # pull out the frames
            while (self.SOF in self.raw) and (self.EOF in self.raw):

                # find the SOF by removing bytes in front of it
                while self.raw[0] != self.SOF:
                    self.raw.pop(0)

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

                # remove the fletcher16 checksum
                f16_check = message.pop(-1) * 256
                f16_check += message.pop(-1)

                # calculate the checksum
                if self.fletcher16_checksum(message) == f16_check:
                    self.rx_messages.append(message)

            time.sleep(0.1)


if __name__ == "__main__":
    f = Frame()
    while True:
        msg = [0, 1, 2, 3, 4, 5, 6, 7]
        f.push_tx_message(msg)
        time.sleep(5.0)

