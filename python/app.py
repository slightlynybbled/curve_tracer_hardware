import tkinter as tk
from tkinter import filedialog
import csv

import threading
import time
import datetime
import serial
import serial.tools.list_ports
import serialdispatch
from PIL import Image, ImageTk

from shortcut_bar import ShortcutBar
from plot4q import Plot4Q
from status_bar import StatusBar
from about import About


class CurveTracer(tk.Frame):
    widget_padding = 5

    serial_baud_rate = 57600
    serial_update_period = 0.05
    port_str = ''
    port = serial.Serial(None)

    instructions_per_second = 12000000.0
    samples_per_waveform = 128
    max_period = 65250
    min_period = 1000

    max_gate_voltage = 5.0
    min_gate_voltage = 0.0

    max_vp = 5.0
    min_vp = 0.0

    max_offset = 5.0
    min_offset = -5.0

    volts_per_bit = 5.0/32768
    milli_amps_per_bit = 25.0/16384

    def __init__(self, parent):
        self.parent = parent

        # configure the menus
        self.menu_bar = tk.Menu(self.parent)

        # create the file menu
        self.file_menu = tk.Menu(self.menu_bar, tearoff=0)
        self.file_menu.add_command(label='Test')

        self.edit_menu = tk.Menu(self.menu_bar, tearoff=0)
        self.edit_menu.add_command(label='Select Serial Port...', command=self.select_port_window)
        self.edit_menu.add_command(label='Edit Waveform...', command=self.setup_waveform_window)

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
        self.plot = Plot4Q(self.plot_frame, x_pixels=600, y_pixels=600, xrange=65536, yrange=32768, grid=True, x_axis_label_str='1.0V/div', y_axis_label_str='5mA/div')
        self.status_bar = StatusBar(self.status_frame)

        self.ps = None

        # ----------------------------
        # add the shortcut bar buttons and commands
        self.shortcut_bar.add_btn(image_path='images/btn-save.png', command=self.save_waveform)
        self.shortcut_bar.add_btn(image_path='images/btn-load.png', command=self.load_waveform)
        self.shortcut_bar.add_btn(image_path='images/btn-erase.png', command=self.clear_waveforms)
        self.shortcut_bar.add_btn(image_path='images/connections.png', command=self.select_port_window)
        self.shortcut_bar.add_btn(image_path='images/btn-pause.png', command=self.pause_plot)
        self.shortcut_bar.add_btn(image_path='images/btn-run.png', command=self.run_plot)
        self.shortcut_bar.add_btn(image_path='images/cal.png', command=self.send_cal_command)
        self.shortcut_bar.add_btn(image_path='images/select-output.png', command=self.select_output_mode)
        self.shortcut_bar.add_btn(image_path='images/gate-voltage.png', command=self.select_gate_voltage_window)
        self.shortcut_bar.add_btn(image_path='images/btn-waveform.png', command=self.setup_waveform_window)

        # ----------------------------
        # initialize the canvas interactive objects
        self.live_points = []
        self.loaded_points = []
        self.plot_data_changed = False
        self.parent.after(50, self.update_plots)

        # ----------------------------
        # create the thread that will monitor the comm channel and display the status
        self.last_comm_time = 0
        dispatch_monitor_thread = threading.Thread(target=self.monitor_dispatch, args=())
        dispatch_monitor_thread.daemon = True
        dispatch_monitor_thread.start()

    def vi_subscriber(self):
        # create a list of points as (x, y) tuples in preparation for plotting
        list_of_points = []
        for i, element in enumerate(self.ps.get_data('vi')[0]):

            x1 = self.ps.get_data('vi')[0][i]
            y1 = self.ps.get_data('vi')[1][i]

            list_of_points.append((x1, y1))

        self.live_points = list_of_points
        self.plot_data_changed = True

        self.samples_per_waveform = len(self.ps.get_data('vi')[0])
        self.last_comm_time = time.time()

    def period_subscriber(self):
        period_in_cycles = self.ps.get_data('period')[0][0]
        frequency = int(self.instructions_per_second/(self.samples_per_waveform * period_in_cycles) + 0.5)

        self.status_bar.set_freq(frequency)

        self.last_comm_time = time.time()

    def gate_voltage_subscriber(self):
        gate_voltage = 5.0 * self.ps.get_data('gate voltage')[0][0]/32768
        self.status_bar.set_gate_voltage(gate_voltage)

    def peak_voltage_subscriber(self):
        peak_voltage = 5.0 * self.ps.get_data('peak voltage')[0][0]/32768
        self.status_bar.set_peak_voltage(peak_voltage)

    def offset_voltage_subscriber(self):
        offset_voltage = 5.0 * self.ps.get_data('offset voltage')[0][0]/32768
        self.status_bar.set_offset_voltage(offset_voltage)

    def mode_subscriber(self):
        mode = self.ps.get_data('mode')[0][0]
        self.status_bar.set_mode(mode)

    def monitor_dispatch(self):

        while True:
            if (time.time() - self.last_comm_time) <= 2.0:
                self.status_bar.set_comm_status(True)
                self.status_bar.set_comm_text('active')
            else:
                self.status_bar.set_comm_status(False)
                self.status_bar.set_comm_text('idle')

            time.sleep(0.1)

    def update_plots(self):
        if self.plot_data_changed:
            # all interactions with the canvas should be in one place
            # plot the live points points
            if self.live_points:
                self.plot.scatter(self.live_points, color='green', tag='live')

            if self.loaded_points:
                self.plot.scatter(self.loaded_points, color='blue', tag='loaded')
            else:
                self.plot.remove_scatter(tag='loaded')

            self.plot_data_changed = False

        # re-register this function
        self.parent.after(200, self.update_plots)

    def save_waveform(self):
        points = self.plot.get_scatter(tag='live')

        if points:
            f = filedialog.asksaveasfile(mode='w', defaultextension='.csv', filetypes=[('comma-separated values', '.csv')])
            print('saving waveform to ', f)

            if f:
                f.write('Curve Tracer Export\n')
                f.write('{:%Y-%m-%d %H:%M:%S}\n'.format(datetime.datetime.now()))
                f.write('type: raw-export\n\n')

                f.write('voltage (int),current(int),voltage(Volts),current(mA)\n')

                for point in points:
                    x, y = point
                    f.write(str(x) + ',' + str(y) + ',')
                    f.write(str(x * self.volts_per_bit) + ',' + str(y * self.milli_amps_per_bit) + '\n')

                f.close()

    def load_waveform(self):

        f = filedialog.askopenfile(mode='r', defaultextension='.csv',  filetypes=[('comma-separated values', '.csv')])

        if f:
            csv_reader = csv.reader(f, delimiter=',')
            for row in csv_reader:
                try:
                    x = int(row[0])
                    y = int(row[1])
                    self.loaded_points.append((x, y))

                    self.plot_data_changed = True

                except:
                    pass

            f.close()

    def clear_waveforms(self):

        self.loaded_points = []
        self.plot_data_changed = True

    def select_port_window(self):
        port_selector_window = tk.Toplevel(padx=self.widget_padding, pady=self.widget_padding)
        port_selector_window.grab_set()
        port_selector_window.title('for(embed) - Serial Port Selector')
        port_selector_window.iconbitmap('images/forembed.ico')

        txt = "Select a port from the list: "
        port_label = tk.Label(port_selector_window, text=txt)
        port_label.pack(fill=tk.BOTH, expand=1, padx=self.widget_padding, pady=self.widget_padding)

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
                self.ps.subscribe('gate voltage', self.gate_voltage_subscriber)
                self.ps.subscribe('peak voltage', self.peak_voltage_subscriber)
                self.ps.subscribe('offset voltage', self.offset_voltage_subscriber)
                self.ps.subscribe('mode', self.mode_subscriber)
                self.status_bar.set_port_status(True)

            except serial.SerialException:
                self.port = serial.Serial(None)
                self.status_bar.set_port_status(False)

            port_selector_window.destroy()

        btn_sel = tk.Button(port_selector_window, text='Select', command=select_port)
        btn_sel.pack(fill=tk.BOTH, expand=1, padx=self.widget_padding, pady=self.widget_padding)

    def pause_plot(self):
        pass

    def run_plot(self):
        pass

    def select_gate_voltage_window(self):
        gate_voltage_selector_window = tk.Toplevel(padx=self.widget_padding, pady=self.widget_padding)
        gate_voltage_selector_window.grab_set()
        gate_voltage_selector_window.title('for(embed) - Gate Voltage Setting Window')
        gate_voltage_selector_window.iconbitmap('images/forembed.ico')

        e = tk.Entry(gate_voltage_selector_window)
        e.pack(side=tk.LEFT)

        volts_label = tk.Label(gate_voltage_selector_window, text='Volts')
        volts_label.pack(side=tk.LEFT)

        def set_gate_voltage():
            gate_voltage_str = e.get()

            # validate freq_str
            valid = True
            if not gate_voltage_str:
                valid = False

            for c in gate_voltage_str:
                if c not in '0123456789.':
                    valid = False

            if valid:
                gate_voltage = float(gate_voltage_str)
                if gate_voltage > self.max_gate_voltage:
                    gate_voltage = self.max_gate_voltage
                elif gate_voltage < self.min_gate_voltage:
                    gate_voltage = self.min_gate_voltage

                voltage = int(gate_voltage * 32768/5.0) - 1
                if voltage < 0:
                    voltage = 0

                self.ps.publish('gate voltage', [[voltage]], ['S16'])
                print('gate voltage set to {:.1f}V ({})'.format(gate_voltage, voltage))

                gate_voltage_selector_window.destroy()

        def return_function(event):
            set_gate_voltage()

        # bind the 'ENTER' key to the function
        e.focus()
        e.bind('<Return>', return_function)

        btn = tk.Button(gate_voltage_selector_window, text='Set Gate Voltage', command=set_gate_voltage)
        btn.pack(side=tk.BOTTOM)

    def calc_period(self, freq_str):
        valid = True

        for c in freq_str:
            if c not in '0123456789.':
                valid = False

        if valid:
            frequency = float(freq_str)
            period = int(self.instructions_per_second / (self.samples_per_waveform * frequency))

            if period > self.max_period:
                period = self.max_period
            elif period < self.min_period:
                period = self.min_period

            return period

        else:
            raise ValueError('Could not convert the string to a float')

    def scale_voltage(self, voltage_str, allow_negative=None, max_value=None, min_value=None, scaler=5.0):
        valid = True

        for c in voltage_str:
            if allow_negative:
                if c not in '-0123456789.':
                    valid = False
            else:
                if c not in '0123456789.':
                    valid = False

        if valid:
            voltage = float(voltage_str)

            if max_value:
                if voltage > max_value:
                    voltage = max_value

            if min_value:
                if voltage < min_value:
                    voltage = min_value

            voltage = int(voltage * 32768 / scaler) - 1

            if allow_negative:
                if voltage < -32768:
                    voltage = -32768
            else:
                if voltage < 0:
                    voltage = 0

            return voltage
        else:
            raise ValueError('Could not convert the string to a float')

    def setup_waveform_window(self):
        waveform_window = tk.Toplevel(padx=self.widget_padding, pady=self.widget_padding)
        waveform_window.grab_set()
        waveform_window.title('Waveform Setup')
        waveform_window.iconbitmap('images/forembed.ico')

        # create the widgets
        img = ImageTk.PhotoImage(Image.open("images/waveform.png"))
        c = tk.Canvas(waveform_window, width=img.width(), height=img.height())
        c.image = img   # keep a reference so the image doesn't get garbage-collected
        ci = c.create_image((0, 0), image=img, anchor="nw")

        label_f = tk.Label(waveform_window, text="Frequency (f): ")
        entry_f = tk.Entry(waveform_window)
        label_unit_f = tk.Label(waveform_window, text="Hz")

        label_a = tk.Label(waveform_window, text="Peak Voltage (a): ")
        entry_a = tk.Entry(waveform_window)
        label_unit_a = tk.Label(waveform_window, text="Volts")

        label_o = tk.Label(waveform_window, text="Voltage Offset (o): ")
        entry_o = tk.Entry(waveform_window)
        label_unit_o = tk.Label(waveform_window, text="Volts")

        # define helper functions locally
        def set_freq():
            try:
                f_str = entry_f.get()

                if f_str:
                    period = self.calc_period(f_str)
                    self.ps.publish('period', [[period]], ['U16'])
                    print('set frequency to {}Hz ({} period)'.format(f_str, period))

            except ValueError:
                pass

        def set_peak_voltage():
            try:
                v_str = entry_a.get()

                if v_str:
                    peak_v = self.scale_voltage(v_str, allow_negative=False, min_value=0.0, max_value=5.0)
                    self.ps.publish('peak voltage', [[peak_v]], ['S16'])
                    print('peak voltage set to {}V ({})'.format(v_str, peak_v))
            except ValueError:
                pass

        def set_offset_voltage():
            try:
                v_str = entry_o.get()

                if v_str:
                    offset_v = self.scale_voltage(v_str, allow_negative=True, min_value=-5.0, max_value=5.0)
                    self.ps.publish('offset voltage', [[offset_v]], ['S16'])
                    print('offset voltage set to {}V ({})'.format(v_str, offset_v))
            except ValueError:
                pass

        def setup_waveform():
            set_freq()
            set_peak_voltage()
            set_offset_voltage()
            waveform_window.destroy()

        btn = tk.Button(waveform_window, text="Setup Waveform", command=setup_waveform)

        def return_function(event):
            setup_waveform()

        # bind the 'ENTER' key to the function
        entry_f.focus()
        entry_f.bind('<Return>', return_function)
        entry_a.bind('<Return>', return_function)
        entry_o.bind('<Return>', return_function)

        # Grid all widgets
        c.grid(row=0, column=0, columnspan=3)

        label_f.grid(row=1, column=0, sticky=tk.E, padx=self.widget_padding, pady=self.widget_padding)
        entry_f.grid(row=1, column=1, padx=self.widget_padding, pady=self.widget_padding)
        label_unit_f.grid(row=1, column=2, sticky=tk.W, padx=self.widget_padding, pady=self.widget_padding)

        label_a.grid(row=2, column=0, sticky=tk.E, padx=self.widget_padding, pady=self.widget_padding)
        entry_a.grid(row=2, column=1, padx=self.widget_padding, pady=self.widget_padding)
        label_unit_a.grid(row=2, column=2, sticky=tk.W, padx=self.widget_padding, pady=self.widget_padding)

        label_o.grid(row=3, column=0, sticky=tk.E, padx=self.widget_padding, pady=self.widget_padding)
        entry_o.grid(row=3, column=1, padx=self.widget_padding, pady=self.widget_padding)
        label_unit_o.grid(row=3, column=2, sticky=tk.W, padx=self.widget_padding, pady=self.widget_padding)

        btn.grid(row=4, column=0, columnspan=3, padx=self.widget_padding, pady=self.widget_padding)

    def send_cal_command(self):
        self.ps.publish('cal', [['']], ['STRING'])

    def select_output_mode(self):
        self.ps.publish('mode', [['']], ['STRING'])


if __name__ == '__main__':
    root = tk.Tk()
    root.resizable(0, 0)
    root.title("for(embed) - Curve Tracer")
    root.iconbitmap('images/forembed.ico')
    app = CurveTracer(root)
    root.mainloop()
