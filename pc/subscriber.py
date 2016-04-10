from pubser import PubSerial
import tkinter as tk


class Plot(tk.Canvas):

    def __init__(self, master, x_pixels=200, y_pixels=200):
        tk.Canvas.__init__(self, master)
        self.width_px = x_pixels
        self.height_px = y_pixels

        self.plot = tk.Canvas(master, width=x_pixels, height=y_pixels)
        self.plot.pack()

        # draw the primary axes
        x0, y0 = self.to_screen_coords(-x_pixels/2, 0)
        x1, y1 = self.to_screen_coords(x_pixels/2, 0)
        x_axis = self.plot.create_line(x0, y0, x1, y1)

        x0, y0 = self.to_screen_coords(0, y_pixels/2)
        x1, y1 = self.to_screen_coords(0, -y_pixels/2)
        y_axis = self.plot.create_line(x0, y0, x1, y1)

        # create the grid
        x_grid_interval_px = x_pixels/10
        y_grid_interval_px = y_pixels/10
        dash_tuple = (1, 1)
        for i in range(4):
            # top to bottom lines, right quadrants
            grid_x = (i + 1) * x_grid_interval_px
            grid_y = self.height_px/2
            x1, y1 = self.to_screen_coords(grid_x, grid_y)
            x1, y2 = self.to_screen_coords(grid_x, -grid_y)
            self.plot.create_line(x1, y1, x1, y2, dash=dash_tuple)

            # top to bottom lines, left quadrants
            grid_x = -(i + 1) * x_grid_interval_px
            grid_y = self.height_px / 2
            x1, y1 = self.to_screen_coords(grid_x, grid_y)
            x1, y2 = self.to_screen_coords(grid_x, -grid_y)
            self.plot.create_line(x1, y1, x1, y2, dash=dash_tuple)

            # left-to-right lines, upper quadrants
            grid_x = self.width_px/2
            grid_y = (i + 1) * y_grid_interval_px
            x1, y1 = self.to_screen_coords(grid_x, grid_y)
            x2, y1 = self.to_screen_coords(-grid_x, grid_y)
            self.plot.create_line(x1, y1, x2, y1, dash=dash_tuple)

            # left-to-right lines, lower quadrants
            grid_x = self.width_px / 2
            grid_y = -(i + 1) * y_grid_interval_px
            x1, y1 = self.to_screen_coords(grid_x, grid_y)
            x2, y1 = self.to_screen_coords(-grid_x, grid_y)
            self.plot.create_line(x1, y1, x2, y1, dash=dash_tuple)

    def plot_line(self, first_point, second_point):
        x0, y0 = first_point
        x1, y1 = second_point
        x0_screen, y0_screen = self.to_screen_coords(x0, y0)
        x1_screen, y1_screen = self.to_screen_coords(x1, y1)
        self.plot.create_line(x0_screen, y0_screen, x1_screen, y1_screen, fill='blue', width=3.0)

    def plot_point(self, point):
        point_width = 2
        x, y = point
        x_screen, y_screen = self.to_screen_coords(x, y)
        self.plot.create_oval(x_screen-point_width,
                              y_screen-point_width,
                              x_screen+point_width,
                              y_screen+point_width,
                              outline='blue',
                              fill='blue')

    def to_screen_coords(self, x, y, type='px'):
        new_x = 0
        new_y = 0

        if type == 'px':
            new_x = x + self.width_px/2
            new_y = self.height_px/2 - y
        elif type == 'standard':
            new_x = x + self.x_range/2
            new_y = self.y_range/2 - y

        return new_x, new_y

if __name__ == "__main__":
    ps = PubSerial()

    # initialize tk items for display
    root = tk.Tk()
    plot = Plot(master=root)


def vi_subscriber():
    print("____________________________")
    print('data from the first callback', ps.get_data('vi'))


if __name__ == "__main__":
    ps.subscribe('vi', vi_subscriber)

    plot.plot_line((0, 0), (10, 10))
    plot.plot_point((30, 30))

    root.mainloop()
