import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import struct
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import binascii

class SerialGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Serial Communication GUI")
        self.root.geometry("900x700")
        
        self.serial_port = None
        self.receiving = False
        self.sample_count = 0
        self.x_data = []
        self.y_data = []
        
        self.create_widgets()
        self.update_ports()
        
    def create_widgets(self):
        # Serial Port Configuration Frame
        port_frame = ttk.LabelFrame(self.root, text="Serial Port Configuration", padding=10)
        port_frame.grid(row=0, column=0, columnspan=2, padx=10, pady=5, sticky="ew")
        
        ttk.Label(port_frame, text="Port:").grid(row=0, column=0, padx=5, pady=5)
        self.port_combo = ttk.Combobox(port_frame, width=15)
        self.port_combo.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(port_frame, text="Baud Rate:").grid(row=0, column=2, padx=5, pady=5)
        self.baud_combo = ttk.Combobox(port_frame, values=[9600, 19200, 38400, 57600, 115200, 460800], width=10)
        self.baud_combo.set(115200)
        self.baud_combo.grid(row=0, column=3, padx=5, pady=5)
        
        self.refresh_btn = ttk.Button(port_frame, text="Refresh Ports", command=self.update_ports)
        self.refresh_btn.grid(row=0, column=4, padx=5, pady=5)
        
        self.connect_btn = ttk.Button(port_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=5, padx=5, pady=5)
        
        self.status_label = ttk.Label(port_frame, text="Disconnected", foreground="red")
        self.status_label.grid(row=0, column=6, padx=5, pady=5)
        
        # Command Frame
        cmd_frame = ttk.LabelFrame(self.root, text="Command Configuration", padding=10)
        cmd_frame.grid(row=1, column=0, padx=10, pady=5, sticky="ew")
        
        ttk.Label(cmd_frame, text="Command ID:").grid(row=0, column=0, padx=5, pady=5, sticky="e")
        self.cmd_id_entry = ttk.Entry(cmd_frame, width=10)
        self.cmd_id_entry.insert(0, "1")
        self.cmd_id_entry.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(cmd_frame, text="Target Address:").grid(row=1, column=0, padx=5, pady=5, sticky="e")
        self.address_entry = ttk.Entry(cmd_frame, width=10)
        self.address_entry.insert(0, "0")
        self.address_entry.grid(row=1, column=1, padx=5, pady=5)
        
        ttk.Label(cmd_frame, text="Payload 1:").grid(row=2, column=0, padx=5, pady=5, sticky="e")
        self.payload1_entry = ttk.Entry(cmd_frame, width=10)
        self.payload1_entry.insert(0, "0")
        self.payload1_entry.grid(row=2, column=1, padx=5, pady=5)
        
        ttk.Label(cmd_frame, text="Payload 2:").grid(row=3, column=0, padx=5, pady=5, sticky="e")
        self.payload2_entry = ttk.Entry(cmd_frame, width=10)
        self.payload2_entry.insert(0, "0")
        self.payload2_entry.grid(row=3, column=1, padx=5, pady=5)
        
        ttk.Label(cmd_frame, text="Payload 3:").grid(row=4, column=0, padx=5, pady=5, sticky="e")
        self.payload3_entry = ttk.Entry(cmd_frame, width=10)
        self.payload3_entry.insert(0, "0")
        self.payload3_entry.grid(row=4, column=1, padx=5, pady=5)
        
        ttk.Label(cmd_frame, text="Payload 4:").grid(row=5, column=0, padx=5, pady=5, sticky="e")
        self.payload4_entry = ttk.Entry(cmd_frame, width=10)
        self.payload4_entry.insert(0, "0")
        self.payload4_entry.grid(row=5, column=1, padx=5, pady=5)
        
        # Command Buttons Frame
        btn_frame = ttk.LabelFrame(self.root, text="Send Commands", padding=10)
        btn_frame.grid(row=2, column=0, padx=10, pady=5, sticky="ew")
        
        self.cmd1_btn = ttk.Button(btn_frame, text="Command 1", command=lambda: self.send_command(1))
        self.cmd1_btn.grid(row=0, column=0, padx=5, pady=5)
        
        self.cmd2_btn = ttk.Button(btn_frame, text="Command 2", command=lambda: self.send_command(2))
        self.cmd2_btn.grid(row=0, column=1, padx=5, pady=5)
        
        self.cmd3_btn = ttk.Button(btn_frame, text="Command 3", command=lambda: self.send_command(3))
        self.cmd3_btn.grid(row=0, column=2, padx=5, pady=5)
        
        self.cmd4_btn = ttk.Button(btn_frame, text="Command 4", command=lambda: self.send_command(4))
        self.cmd4_btn.grid(row=0, column=3, padx=5, pady=5)
        
        self.cmd5_btn = ttk.Button(btn_frame, text="Command 5", command=lambda: self.send_command(5))
        self.cmd5_btn.grid(row=0, column=4, padx=5, pady=5)
        
        # Plot Frame
        plot_frame = ttk.LabelFrame(self.root, text="Data Plot", padding=10)
        plot_frame.grid(row=1, column=1, rowspan=2, padx=10, pady=5, sticky="nsew")
        
        self.fig = Figure(figsize=(5, 4), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_xlabel("Sample Number")
        self.ax.set_ylabel("Payload 1 Value")
        self.ax.set_title("Real-time Data Plot")
        self.ax.grid(True)
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        clear_plot_btn = ttk.Button(plot_frame, text="Clear Plot", command=self.clear_plot)
        clear_plot_btn.pack(pady=5)
        
        # Log Frame
        log_frame = ttk.LabelFrame(self.root, text="Communication Log", padding=10)
        log_frame.grid(row=3, column=0, columnspan=2, padx=10, pady=5, sticky="ew")
        
        self.log_text = tk.Text(log_frame, height=8, width=100)
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        scrollbar = ttk.Scrollbar(log_frame, command=self.log_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.config(yscrollcommand=scrollbar.set)
        
        # Configure grid weights
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(1, weight=1)
        
    def update_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.current(0)
    
    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        else:
            self.connect()
    
    def connect(self):
        try:
            port = self.port_combo.get()
            baud = int(self.baud_combo.get())
            self.serial_port = serial.Serial(port, baud, timeout=0.1)
            self.status_label.config(text="Connected", foreground="green")
            self.connect_btn.config(text="Disconnect")
            self.log("Connected to " + port)
            
            # Start receiving thread
            self.receiving = True
            self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
            self.receive_thread.start()
        except Exception as e:
            messagebox.showerror("Connection Error", str(e))
            self.log("Connection error: " + str(e))
    
    def disconnect(self):
        self.receiving = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.status_label.config(text="Disconnected", foreground="red")
        self.connect_btn.config(text="Connect")
        self.log("Disconnected")
    
    def calculate_crc16(self, data):
        crc = 0xFFFF
        for byte in data:
            crc ^= (byte << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc <<= 1
                crc &= 0xFFFF
        return crc
    
    def send_command(self, cmd_id):
        if not self.serial_port or not self.serial_port.is_open:
            messagebox.showwarning("Not Connected", "Please connect to a serial port first")
            return
        
        try:
            # Get values from entries
            address = int(self.address_entry.get()) & 0xFF
            payload1 = int(self.payload1_entry.get())
            payload2 = int(self.payload2_entry.get())
            payload3 = int(self.payload3_entry.get())
            payload4 = int(self.payload4_entry.get())
            
            # Build packet: cmd_id (1 byte) + address (1 byte) + 4 floats (16 bytes)
            packet = struct.pack('<BB4i', cmd_id, address, payload1, payload2, payload3, payload4)
            
            # Calculate CRC
            crc = self.calculate_crc16(packet)
            
            # Append CRC (2 bytes)
            packet += struct.pack('<H', crc)
            
            # Send packet
            self.serial_port.write(packet)
            
            self.log(f"Sent Command {cmd_id}: Addr={address}, Payloads=[{payload1:.2f}, {payload2:.2f}, {payload3:.2f}, {payload4:.2f}], CRC={crc:04X}")
        except ValueError as e:
            messagebox.showerror("Input Error", "Please enter valid numeric values")
            self.log("Input error: " + str(e))
        except Exception as e:
            messagebox.showerror("Send Error", str(e))
            self.log("Send error: " + str(e))
    
    def receive_data(self):
        buffer = b''
        packet_size = 20  # 1 + 1 + 16 + 2 bytes
        
        while self.receiving:
            try:
                if self.serial_port.in_waiting:
                    buffer += self.serial_port.read(self.serial_port.in_waiting)
                    
                    # Process complete packets
                    while len(buffer) >= packet_size:
                        packet = buffer[:packet_size]
                        buffer = buffer[packet_size:]
                        
                        # Parse packet
                        try:
                            cmd_id, address, p1, p2, p3, p4, crc_received = struct.unpack('<BB4iH', packet)
                            
                            # Verify CRC
                            crc_calculated = self.calculate_crc16(packet[:18])
                            
                            #if crc_calculated == crc_received:
                            self.log(f"Received: CMD={cmd_id}, Addr={address}, P1={p1:.2f}, P2={p2:.2f}, P3={p3:.2f}, P4={p4:.2f}")
                                
                                # Update plot with first payload
                            self.update_plot(p1)
                            #else:
                            #    self.log(f"CRC Error: Expected {crc_calculated:04X}, Got {crc_received:04X}")
                        except struct.error:
                            self.log("Packet parsing error")
            except Exception as e:
                if self.receiving:
                    self.log(f"Receive error: {str(e)}")
    
    def update_plot(self, value):
        self.sample_count += 1
        self.x_data.append(self.sample_count)
        self.y_data.append(value)
        
        # Keep only last 100 samples
        if len(self.x_data) > 500:
            self.x_data.pop(0)
            self.y_data.pop(0)
        
        # Update plot
        self.ax.clear()
        self.ax.plot(self.x_data, self.y_data, 'b-')
        self.ax.set_xlabel("Sample Number")
        self.ax.set_ylabel("Payload 1 Value")
        self.ax.set_title("Real-time Data Plot")
        self.ax.grid(True)
        self.canvas.draw()
    
    def clear_plot(self):
        self.x_data.clear()
        self.y_data.clear()
        self.sample_count = 0
        self.ax.clear()
        self.ax.set_xlabel("Sample Number")
        self.ax.set_ylabel("Payload 1 Value")
        self.ax.set_title("Real-time Data Plot")
        self.ax.grid(True)
        self.canvas.draw()
    
    def log(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)

if __name__ == "__main__":
    root = tk.Tk()
    app = SerialGUI(root)
    root.mainloop()