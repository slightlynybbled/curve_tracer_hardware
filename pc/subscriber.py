from pubser import PubSerial
import tkinter as tk


class Plot():

    def __init__(self, master, x_pixels=200, y_pixels=200, xrange=1.0, yrange=1.0, grid=False):
        self.width_px = x_pixels
        self.height_px = y_pixels

        self.x_per_pixel = xrange/x_pixels
        self.y_per_pixel = yrange/y_pixels
        self.grid = grid

        self.plot = tk.Canvas(master, width=x_pixels, height=y_pixels)
        self.plot.pack()

        self.draw_axes()
        self.draw_grid()

    def remove_points(self):
        self.plot.delete('data_point')

    def draw_axes(self):
        # draw the primary axes
        x0, y0 = self.to_screen_coords(-self.width_px / 2, 0)
        x1, y1 = self.to_screen_coords(self.width_px / 2, 0)
        x_axis = self.plot.create_line(x0, y0, x1, y1)

        x0, y0 = self.to_screen_coords(0, self.height_px / 2)
        x1, y1 = self.to_screen_coords(0, -self.height_px / 2)
        y_axis = self.plot.create_line(x0, y0, x1, y1)

    def draw_grid(self):
        if self.grid:
            # create the grid
            x_grid_interval_px = self.width_px / 10
            y_grid_interval_px = self.height_px / 10
            dash_tuple = (1, 1)

            for i in range(4):
                # top to bottom lines, right quadrants
                grid_x = (i + 1) * x_grid_interval_px
                grid_y = self.height_px / 2
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
                grid_x = self.width_px / 2
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

    def plot_line(self, first_point, second_point, point_format=None):
        if point_format == 'px':
            x0, y0 = first_point
            x1, y1 = second_point
            x0_screen, y0_screen = self.to_screen_coords(x0, y0)
            x1_screen, y1_screen = self.to_screen_coords(x1, y1)
            self.plot.create_line(x0_screen, y0_screen, x1_screen, y1_screen, fill='blue', width=3.0)
        else:
            # scale coordinates based on xrange and yrange
            x0, y0 = first_point
            x1, y1 = second_point

            x0 /= self.x_per_pixel
            x1 /= self.x_per_pixel
            y0 /= self.y_per_pixel
            y1 /= self.y_per_pixel

            x0_screen, y0_screen = self.to_screen_coords(x0, y0)
            x1_screen, y1_screen = self.to_screen_coords(x1, y1)
            self.plot.create_line(x0_screen, y0_screen, x1_screen, y1_screen, fill='blue', width=3.0)

    def plot_point(self, point, point_format=None):
        if point_format == 'px':
            point_width = 2
            x, y = point
            x_screen, y_screen = self.to_screen_coords(x, y)
            point = self.plot.create_oval(x_screen-point_width,
                                          y_screen-point_width,
                                          x_screen+point_width,
                                          y_screen+point_width,
                                          outline='blue',
                                          fill='blue',
                                          tag='data_point')

        else:
            point_width = 2
            x, y = point

            x /= self.x_per_pixel
            y /= self.y_per_pixel

            x_screen, y_screen = self.to_screen_coords(x, y)
            point = self.plot.create_oval(x_screen - point_width,
                                          y_screen - point_width,
                                          x_screen + point_width,
                                          y_screen + point_width,
                                          outline='blue',
                                          fill='blue',
                                          tag='data_point')

    def to_screen_coords(self, x, y):
        new_x = x + self.width_px/2
        new_y = self.height_px/2 - y

        return new_x, new_y

if __name__ == "__main__":
    ps = PubSerial()

    # initialize tk items for display
    root = tk.Tk()
    plot = Plot(master=root, xrange=65536, yrange=65536, grid=True)


def vi_subscriber():
    print("____________________________")
    print('data from the first callback', ps.get_data('vi'))
    plot.remove_points()
    for i, element in enumerate(ps.get_data('vi')[0]):
        plot.plot_point((ps.get_data('vi')[0][i], ps.get_data('vi')[1][i]))


if __name__ == "__main__":
    ps.subscribe('vi', vi_subscriber)

    root.mainloop()
