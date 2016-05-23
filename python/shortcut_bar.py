import tkinter as tk
from PIL import Image, ImageTk


class ShortcutBar:
    HEIGHT = 50
    WIDTH = 50

    def __init__(self, parent):
        self.parent = parent

        self.btn_images = []
        self.buttons = []

    def add_btn(self, text=None, image_path=None, command=None):
        if image_path:
            # make an image from the item and resize as appropriate
            img = Image.open(image_path)
            img = img.resize((self.HEIGHT, self.WIDTH), Image.ANTIALIAS)
            self.btn_images.append(ImageTk.PhotoImage(img))

            self.buttons.append(tk.Button(self.parent, image=self.btn_images[-1], command=command))

        else:
            self.buttons.append(tk.Button(self.parent, text=text, command=command))

        self.buttons[-1].pack(side=tk.LEFT)


if __name__ == '__main__':
    root = tk.Tk()
    root.title("for(embed) - Curve Tracer")
    root.iconbitmap('images/forembed.ico')
    app = ShortcutBar(root)
    root.mainloop()
