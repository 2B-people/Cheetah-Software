import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import os

# Plot configuration
PLOT_VERTICALLY = 0
PLOT_HORIZONTALLY = 1

# number of figures in this plot
num_figures = 5

def create_figures(subfigure_width=480, subfigure_height=600, starting_figure_no=1, starting_col_index = 0, starting_row_index=0, plot_configuration=PLOT_HORIZONTALLY):
    figure_number = starting_figure_no
    col_index = starting_col_index
    row_index = starting_row_index

    file_path = os.getcwd() + "/../test_data/"

    ## read files
    spline = np.genfromtxt(file_path+'spline.txt', delimiter=None, dtype=(float))
    vel = np.genfromtxt(file_path+'spline_vel.txt', delimiter=None, dtype=(float))
    t = np.genfromtxt(file_path+'time.txt', delimiter='\n', dtype=(float))

    numeric_vel = np.copy(spline)
    length = len(t)
    for i in range(length-1):
        numeric_vel[i,:] = (spline[i+1,:] - spline[i,:])/(t[i+1]-t[i])

    ## plot pos
    fig = plt.figure(figure_number)
    plt.get_current_fig_manager().window.wm_geometry(str(subfigure_width) + "x" + str(subfigure_height) +  "+" + str(subfigure_width*col_index) + "+" + str(subfigure_height*row_index))
    fig.canvas.set_window_title('spline')
    for i in range(3):
        ax1 = plt.subplot(3, 1, i+1)
        plt.plot(t, spline[:,i], "b-")
        plt.grid(True)
    plt.xlabel('time (sec)')
    ## increment figure number and index
    figure_number += 1
    if plot_configuration == PLOT_HORIZONTALLY:
        col_index += 1
    elif plot_configuration == PLOT_VERTICALLY:
        row_index +=1

    ## plot vel
    fig = plt.figure(figure_number)
    plt.get_current_fig_manager().window.wm_geometry(str(subfigure_width) + "x" + str(subfigure_height) +  "+" + str(subfigure_width*col_index) + "+" + str(subfigure_height*row_index))
    fig.canvas.set_window_title('spline vel')
    for i in range(3):
        ax1 = plt.subplot(3, 1, i+1)
        plt.plot(t, vel[:,i], "b-")
        plt.plot(t[:-1], numeric_vel[:-1,i], "r-")
        plt.grid(True)
    plt.xlabel('time (sec)')
    ## increment figure number and index
    figure_number += 1
    if plot_configuration == PLOT_HORIZONTALLY:
        col_index += 1
    elif plot_configuration == PLOT_VERTICALLY:
        row_index +=1
      
if __name__ == "__main__":
    create_figures()
    plt.show()
