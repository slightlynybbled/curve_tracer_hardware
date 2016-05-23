import tkinter as tk


class StatusBar:
    def __init__(self, parent):
        """
        Creates the status bar, ready to be framed
        :param parent: partent tk.Frame or element
        """
        self.parent = parent

        # must save a copy using self.* or the image will disappear!
        self.port_label = tk.Label(self.parent, text='Port: none', relief=tk.SUNKEN, bg='red')
        self.port_label.pack(side=tk.LEFT, expand=1, fill=tk.X)

        self.status_label = tk.Label(self.parent, text='Comm Status: idle', relief=tk.SUNKEN, bg='red')
        self.status_label.pack(side=tk.LEFT, expand=1, fill=tk.X)

        self.freq_label = tk.Label(self.parent, text='-Hz', relief=tk.SUNKEN)
        self.freq_label.pack(side=tk.LEFT, expand=1, fill=tk.X)

    def set_port_text(self, port_str):
        """
        Used to set the textual port identifier, ie 'COM6' or '/dev/tty0'
        :param port_str: textual port identifier
        :return: none
        """
        # something is wrong - this part isn't working
        self.port_label.configure(text='Port: '+port_str)

    def set_port_status(self, port_status):
        """
        Sets the bg color based on whether the port_status is True or False
        :param port_status: True or False
        :return: none
        """
        if port_status:
            self.port_label.configure(bg='green')
        else:
            self.port_label.configure(bg='red')

    def set_comm_text(self, comm_str):
        """
        Sets the comm status text
        :param comm_str: the desired string
        :return: none
        """
        self.status_label.configure(text='Comm Status: '+comm_str)

    def set_comm_status(self, comm_status):
        """
        Sets the bg color based on whether the comm_status is True or False
        :param comm_status: True or False
        :return: none
        """
        if comm_status:
            self.status_label.configure(bg='green')
        else:
            self.status_label.configure(bg='red')

    def set_freq(self, frequency):
        """
        Sets the frequency in the status bar
        :param frequency: the curve-tracer frequency
        :return: none
        """
        self.freq_label.configure(text='{}Hz'.format(frequency))

if __name__ == '__main__':
    root = tk.Tk()
    root.title("for(embed) - Curve Tracer")
    root.iconbitmap('images/forembed.ico')
    app = StatusBar(root)
    root.mainloop()
