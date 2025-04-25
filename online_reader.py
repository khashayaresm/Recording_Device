import tkinter as tk
from tkinter import ttk, filedialog
import serial
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import csv
import serial.tools.list_ports

# Initialize the main window
root = tk.Tk()
root.title("Serial Monitor with Plotter")
root.configure(background="Gray")
root.geometry('1366x768')
root.resizable(True, True)  # Make window resizable

# Serial communication setup
ser = None
data_buffer = []  # List to store data for plotting
all_data = []  # List to store all data from start

# Baud rate selection
B_label = ttk.Label(root, text="Baudrate:")
B_label.grid(row=0, column=0, padx=10, pady=10, sticky="w")
number = tk.StringVar()
numberChosen = ttk.Combobox(root, width=12, textvariable=number)
numberChosen['values'] = (1000000,)
numberChosen.grid(row=0, column=1, padx=10, pady=10, sticky="ew")
numberChosen.current(0)

# Serial port selection
port_label = ttk.Label(root, text="Select Port:")
port_label.grid(row=1, column=0, padx=10, pady=10, sticky="w")

def get_ports():
    ports = [port.device for port in serial.tools.list_ports.comports()]
    return ports if ports else ['COM1']  # Default to COM1 if no ports found

ports = get_ports()
port_var = tk.StringVar()
port_select = ttk.Combobox(root, width=12, textvariable=port_var)
port_select['values'] = ports
port_select.grid(row=1, column=1, padx=10, pady=10, sticky="ew")
port_select.current(0)

# Serial output display
output_text = tk.Text(root, width=10, height=20)
output_text.grid(row=2, column=0, columnspan=3, padx=10, pady=10, sticky="nsew")

# Figure for plotting
fig, ax = plt.subplots(figsize=(8, 4))
line, = ax.plot([], [], label="Data", color='blue')
ax.set_xlim(0, 200)
ax.set_ylim(0, 100)
ax.set_xlabel("Time")
ax.set_ylabel("Value")
ax.legend()

# Embed plot in Tkinter
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().grid(row=0, column=2, rowspan=3, padx=10, pady=10, sticky="nsew")

root.grid_rowconfigure(0, weight=1)
root.grid_rowconfigure(1, weight=0)
root.grid_rowconfigure(2, weight=1)
root.grid_columnconfigure(0, weight=1)
root.grid_columnconfigure(1, weight=1)
root.grid_columnconfigure(2, weight=2)

# Optimized update function
def update_plot():
    """Efficiently updates the plot without overloading Matplotlib."""
    if len(data_buffer) == 0:
        return

    line.set_xdata(range(len(data_buffer)))
    line.set_ydata(data_buffer)

    ax.set_xlim(max(0, len(data_buffer) - 200), len(data_buffer))  # Keep last 200 points
    ax.set_ylim(min(data_buffer) - 10, max(data_buffer) + 10)  # Dynamic Y range

    canvas.draw_idle()  # Faster updates

# Periodic plot update (every 100ms)
def periodic_update():
    update_plot()
    root.after(500, periodic_update)

root.after(500, periodic_update)

# Read serial data
def read_serial_data():
    global ser
    while True:
        if ser and ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip()
            try:
                data_value = float(data)
                data_buffer.append(data_value)
                all_data.append(data_value)

                if len(data_buffer) > 200:
                    data_buffer.pop(0)

                output_text.insert(tk.END, f"{data}\n")
                output_text.yview(tk.END)
            except ValueError:
                pass

# Start serial monitor
def start_serial_monitor():
    global ser
    baud_rate = int(numberChosen.get())
    selected_port = port_var.get()
    try:
        ser = serial.Serial(selected_port, baud_rate)
        threading.Thread(target=read_serial_data, daemon=True).start()
    except serial.SerialException:
        output_text.insert(tk.END, f"Error: Could not open port {selected_port}.\n")
        output_text.yview(tk.END)

# Stop serial monitor
def stop_serial_monitor():
    global ser
    if ser:
        ser.close()

# Save to CSV
def save_to_csv():
    if all_data:
        file_path = filedialog.asksaveasfilename(defaultextension=".csv", filetypes=[("CSV Files", "*.csv")])
        if file_path:
            with open(file_path, mode='w', newline='') as file:
                writer = csv.writer(file)
                # writer.writerow(["Value"])
                for i, value in enumerate(all_data):
                    writer.writerow([value])

# Send data to serial port
def send_data():
    if ser:
        data_to_send = send_entry.get()
        ser.write(data_to_send.encode())
        output_text.insert(tk.END, f"Sent: {data_to_send}\n")
        output_text.yview(tk.END)
        send_entry.delete(0, tk.END)

        ax.set_ylim(0, 2000)
        update_plot()

# UI buttons
send_label = ttk.Label(root, text="Send Data:")
send_label.grid(row=4, column=0, padx=10, pady=10, sticky="w")

send_entry = ttk.Entry(root, width=50)
send_entry.grid(row=4, column=1, padx=10, pady=10, sticky="ew")

send_button = tk.Button(root, text="Send", height=2, width=10, command=send_data)
send_button.grid(row=4, column=2, padx=10, pady=10, sticky="ew")

start_B = tk.Button(root, text="Start", height=2, width=10, command=start_serial_monitor)
start_B.grid(row=3, column=0, padx=10, pady=10, sticky="ew")

stop_B = tk.Button(root, text="Stop", height=2, width=10, command=stop_serial_monitor)
stop_B.grid(row=3, column=1, padx=10, pady=10, sticky="ew")

save_B = tk.Button(root, text="Save to CSV", height=2, width=15, command=save_to_csv)
save_B.grid(row=3, column=2, padx=10, pady=10, sticky="ew")

root.mainloop()
