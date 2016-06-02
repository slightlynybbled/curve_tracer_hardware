import tkinter as tk
import threading
import time
import serial
import serial.tools.list_ports
import serialdispatch

from shortcut_bar import ShortcutBar
from plot4q import Plot4Q
from status_bar import StatusBar
from about import About


class CurveTracer(tk.Frame):
    widget_padding = 5

    serial_baud_rate = 57600
    serial_update_period = 0.1
    port_str = ''
    port = serial.Serial(None)

    instructions_per_second = 12000000.0
    samples_per_waveform = 64
    min_freq = 3
    max_freq = 188

    def __init__(self, parent):
        self.parent = parent

        # configure the menus
        self.menu_bar = tk.Menu(self.parent)

        # create the file menu
        self.file_menu = tk.Menu(self.menu_bar, tearoff=0)
        self.file_menu.add_command(label='Test')

        self.edit_menu = tk.Menu(self.menu_bar, tearoff=0)
        self.edit_menu.add_command(label='Select Serial Port...', command=self.select_port_window)
        self.edit_menu.add_command(label='Change frequency...', command=self.select_freq)

        self.help_menu = tk.Menu(self.menu_bar, tearoff=0)
        self.help_menu.add_command(label='About', command=About)

        # add the menus to the window
        self.menu_bar.add_cascade(label='File', menu=self.file_menu)
        self.menu_bar.add_cascade(label='Edit', menu=self.edit_menu)
        self.menu_bar.add_cascade(label='Help', menu=self.help_menu)
        self.parent.config(menu=self.menu_bar)

        # ----------------------------
        # configure the high-level frames and add them to the root window
        self.shortcut_frame = tk.Frame(self.parent)
        self.shortcut_frame.pack(fill=tk.X)

        self.plot_frame = tk.Frame(self.parent)
        self.plot_frame.pack(fill=tk.X)

        self.status_frame = tk.Frame(self.parent)
        self.status_frame.pack(fill=tk.X)

        # ----------------------------
        # create the widgets within their frames
        self.shortcut_bar = ShortcutBar(self.shortcut_frame)
        self.plot = Plot4Q(self.plot_frame, x_pixels=600, y_pixels=600, xrange=256, yrange=128, grid=True, x_axis_label_str='1.0V/div', y_axis_label_str='5mA/div')
        self.status_bar = StatusBar(self.status_frame)

        self.ps = None

        # ----------------------------
        # add the shortcut bar buttons and commands
        self.shortcut_bar.add_btn(image_path='images/connections.png', command=self.select_port_window)
        self.shortcut_bar.add_btn(image_path='images/freq.png', command=self.select_freq)
        self.shortcut_bar.add_btn(image_path='images/cal.png', command=self.send_cal_command)

        # ----------------------------
        # create the thread that will monitor the comm channel and display the status
        self.last_comm_time = 0
        dispatch_monitor_thread = threading.Thread(target=self.monitor_dispatch, args=())
        dispatch_monitor_thread.daemon = True
        dispatch_monitor_thread.start()

    def vi_subscriber(self):
        self.plot.remove_lines()

        data_length = len(self.ps.get_data('vi')[0])
        color_increment = int(256 / data_length)
        red = 0 << 16
        blue = 0
        base_color = red + blue

        for i, element in enumerate(self.ps.get_data('vi')[0]):
            if i < len(self.ps.get_data('vi')[0]) - 1:
                color = base_color + (int(i * color_increment) << 8)
                color_str = '#' + hex(color)[2:].zfill(6)

                x1 = self.ps.get_data('vi')[0][i]
                x2 = self.ps.get_data('vi')[0][i + 1]
                y1 = self.ps.get_data('vi')[1][i]
                y2 = self.ps.get_data('vi')[1][i + 1]
                self.plot.plot_line((x1, y1), (x2, y2), fill=color_str)

        self.last_comm_time = time.time()

    def period_subscriber(self):
        period_in_cycles = self.ps.get_data('period')[0][0]
        frequency = int(self.instructions_per_second/(self.samples_per_waveform * period_in_cycles) + 0.5)

        self.status_bar.set_freq(frequency)

        self.last_comm_time = time.time()

    def monitor_dispatch(self):

        while True:
            if (time.time() - self.last_comm_time) <= 1.0:
                self.status_bar.set_comm_status(True)
                self.status_bar.set_comm_text('active')
            else:
                self.status_bar.set_comm_status(False)
                self.status_bar.set_comm_text('idle')

            time.sleep(0.5)

    def select_port_window(self):
        port_selector_window = tk.Toplevel(padx=self.widget_padding, pady=self.widget_padding)
        port_selector_window.grab_set()
        port_selector_window.title('for(embed) - Serial Port Selector')
        port_selector_window.iconbitmap('images/forembed.ico')

        message = "Select a port from the list: "
        msg = tk.Message(port_selector_window, text=message)
        msg.pack(fill=tk.BOTH, expand=1, padx=self.widget_padding, pady=self.widget_padding)

        ports = serial.tools.list_ports.comports()
        port_list = []
        for port in ports:
            port_list.append(port.device)
        port_list.sort()
        option_list = tuple(port_list)

        var = tk.StringVar(port_selector_window)
        var.set(port_list[0])

        dd = tk.OptionMenu(port_selector_window, var, *option_list)
        dd.pack(fill=tk.BOTH, expand=1, padx=self.widget_padding, pady=self.widget_padding)

        def select_port():
            if self.ps:
                self.ps.close()

            if self.port:
                self.port.close()

            self.port_str = var.get()
            print('serial port set to ', self.port_str)
            self.status_bar.set_port_text(self.port_str)

            try:
                self.port = serial.Serial(self.port_str, baudrate=self.serial_baud_rate, timeout=self.serial_update_period)
                self.ps = serialdispatch.SerialDispatch(self.port)
                self.ps.subscribe('vi', self.vi_subscriber)
                self.ps.subscribe('period', self.period_subscriber)
                self.status_bar.set_port_status(True)

            except serial.SerialException:
                self.port = serial.Serial(None)
                self.status_bar.set_port_status(False)

            port_selector_window.destroy()

        btn_sel = tk.Button(port_selector_window, text='Select', command=select_port)
        btn_sel.pack(fill=tk.BOTH, expand=1, padx=self.widget_padding, pady=self.widget_padding)

    def select_freq(self):
        freq_selector_window= tk.Toplevel(padx=self.widget_padding, pady=self.widget_padding)
        freq_selector_window.grab_set()
        freq_selector_window.title('for(embed) - Serial Port Selector')
        freq_selector_window.iconbitmap('images/forembed.ico')

        e = tk.Entry(freq_selector_window)
        e.pack(side=tk.LEFT)

        hz = tk.Message(freq_selector_window, text='Hz')
        hz.pack(side=tk.LEFT)

        def set_freq():
            freq_str = e.get()

            # validate freq_str
            valid = True
            if not freq_str:
                valid = False

            for c in freq_str:
                if c not in '0123456789.':
                    valid = False

            if valid:
                frequency = float(freq_str)
                if frequency > self.max_freq:
                    frequency = self.max_freq
                elif frequency < self.min_freq:
                    frequency = self.min_freq

                period = int(self.instructions_per_second/(self.samples_per_waveform * frequency))

                self.ps.publish('period', [[period]], ['U16'])
                print('set frequency to {:.1f}Hz'.format(frequency))
                print('period set to ', period)

                freq_selector_window.destroy()

        def return_function(event):
            set_freq()

        # bind the 'ENTER' key to the function
        e.focus()
        e.bind('<Return>', return_function)

        btn = tk.Button(freq_selector_window, text='Set Frequency', command=set_freq)
        btn.pack(side=tk.BOTTOM)

    def send_cal_command(self):
        self.ps.publish('cal', [['']], ['STRING'])

if __name__ == '__main__':
    root = tk.Tk()
    root.resizable(0, 0)
    root.title("for(embed) - Curve Tracer")
    root.iconbitmap('images/forembed.ico')
    app = CurveTracer(root)
    root.mainloop()
