import tkinter as tk
import webbrowser
from PIL import Image, ImageTk


class About:
    DEFAULT_PADDING = 5

    def __init__(self):
        window = tk.Toplevel(padx=5, pady=5)
        window.title('for(embed) - About')
        window.iconbitmap('images/forembed.ico')

        ct_label = tk.Label(window,
                            text='Curve Tracer',
                            padx=self.DEFAULT_PADDING,
                            pady=self.DEFAULT_PADDING,
                           font=('Courier New', 30))
        ct_label.pack()

        fe_label = tk.Label(window,
                            text='for(embed)',
                            padx=self.DEFAULT_PADDING,
                            pady=self.DEFAULT_PADDING,
                            font=('Courier New', 22))
        fe_label.pack()

        logo = Image.open('images/forembed.png')
        logo_pi = ImageTk.PhotoImage(logo)
        logo_label = tk.Label(window, image=logo_pi)
        logo_label.image = logo_pi
        logo_label.pack()

        l1_text = '''This software is provided free of charge to the open-source community in support
        of the curve tracer project.  Details of the project may be found at'''
        l1 = tk.Label(window, text=l1_text, padx=self.DEFAULT_PADDING, pady=self.DEFAULT_PADDING)
        l1.pack()

        forembed_label = tk.Label(window, text=r'http://www.forembed.com', fg='blue')
        forembed_label.pack()
        forembed_label.bind("<Button-1>", self.callback)

    def callback(self, event):
        webbrowser.open_new(event.widget.cget("text"))