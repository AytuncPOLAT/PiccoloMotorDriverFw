import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit(
        "PySerial is required. Install it with: "
        "py -3 -m pip install -r scripts\\requirements.txt"
    ) from exc

from weight_torque_protocol import (
    build_torque_command_frame,
    build_torque_mode_frame,
    grams_to_torque_command,
    parse_weight_grams,
)


class WeightTorqueGui:
    MAX_WEIGHT_AGE_SECONDS = 2.0

    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Piccolo Weight-to-Torque")
        self.root.minsize(680, 500)
        self.root.protocol("WM_DELETE_WINDOW", self.close)

        self.motor_serial = None
        self.weight_serial = None
        self.weight_reader = None
        self.reader_stop = threading.Event()
        self.events = queue.Queue()
        self.output_enabled = False
        self.active_device_address = None
        self.last_weight = None
        self.last_weight_time = None
        self.last_invalid_line = None
        self.last_invalid_log_time = 0.0
        self.closed = False

        self.motor_port_var = tk.StringVar()
        self.motor_baud_var = tk.StringVar(value="115200")
        self.device_address_var = tk.StringVar(value="1")
        self.weight_port_var = tk.StringVar(value="COM36")
        self.weight_baud_var = tk.StringVar(value="115200")
        self.factor_var = tk.DoubleVar(value=1.0)
        self.motor_status_var = tk.StringVar(value="Disconnected")
        self.weight_status_var = tk.StringVar(value="Disconnected")
        self.weight_reading_var = tk.StringVar(value="Weight: waiting for data")
        self.torque_command_var = tk.StringVar(value="Torque command: --")
        self.output_status_var = tk.StringVar(value="Output stopped")

        self._create_widgets()
        self.refresh_ports()
        self.root.after(50, self._process_events)

    def _create_widgets(self) -> None:
        self.root.columnconfigure(0, weight=1)

        motor_frame = ttk.LabelFrame(self.root, text="Piccolo motor driver", padding=10)
        motor_frame.grid(row=0, column=0, padx=10, pady=6, sticky="ew")
        motor_frame.columnconfigure(1, weight=1)

        ttk.Label(motor_frame, text="Serial port").grid(row=0, column=0, padx=4, pady=4)
        self.motor_port_combo = ttk.Combobox(
            motor_frame, textvariable=self.motor_port_var, state="normal", width=16
        )
        self.motor_port_combo.grid(row=0, column=1, padx=4, pady=4, sticky="ew")
        ttk.Label(motor_frame, text="Baud").grid(row=0, column=2, padx=4, pady=4)
        ttk.Entry(motor_frame, textvariable=self.motor_baud_var, width=9).grid(
            row=0, column=3, padx=4, pady=4
        )
        ttk.Label(motor_frame, text="Device address").grid(
            row=0, column=4, padx=4, pady=4
        )
        ttk.Entry(motor_frame, textvariable=self.device_address_var, width=5).grid(
            row=0, column=5, padx=4, pady=4
        )
        self.motor_connect_button = ttk.Button(
            motor_frame, text="Connect", command=self.toggle_motor_connection
        )
        self.motor_connect_button.grid(row=0, column=6, padx=4, pady=4)
        ttk.Label(motor_frame, textvariable=self.motor_status_var).grid(
            row=1, column=0, columnspan=7, sticky="w", padx=4, pady=3
        )

        weight_frame = ttk.LabelFrame(self.root, text="Weight sensor", padding=10)
        weight_frame.grid(row=1, column=0, padx=10, pady=6, sticky="ew")
        weight_frame.columnconfigure(1, weight=1)

        ttk.Label(weight_frame, text="Serial port").grid(
            row=0, column=0, padx=4, pady=4
        )
        self.weight_port_combo = ttk.Combobox(
            weight_frame, textvariable=self.weight_port_var, state="normal", width=16
        )
        self.weight_port_combo.grid(row=0, column=1, padx=4, pady=4, sticky="ew")
        ttk.Label(weight_frame, text="Baud").grid(row=0, column=2, padx=4, pady=4)
        ttk.Entry(weight_frame, textvariable=self.weight_baud_var, width=9).grid(
            row=0, column=3, padx=4, pady=4
        )
        self.weight_connect_button = ttk.Button(
            weight_frame, text="Connect", command=self.toggle_weight_connection
        )
        self.weight_connect_button.grid(row=0, column=4, padx=4, pady=4)
        ttk.Button(weight_frame, text="Refresh ports", command=self.refresh_ports).grid(
            row=0, column=5, padx=4, pady=4
        )
        ttk.Label(weight_frame, textvariable=self.weight_status_var).grid(
            row=1, column=0, columnspan=6, sticky="w", padx=4, pady=3
        )
        ttk.Label(weight_frame, textvariable=self.weight_reading_var).grid(
            row=2, column=0, columnspan=6, sticky="w", padx=4, pady=3
        )

        factor_frame = ttk.LabelFrame(
            self.root, text="Grams-to-torque multiplier", padding=10
        )
        factor_frame.grid(row=2, column=0, padx=10, pady=6, sticky="ew")
        factor_frame.columnconfigure(0, weight=1)
        self.factor_slider = tk.Scale(
            factor_frame,
            from_=0.1,
            to=10.0,
            resolution=0.1,
            orient=tk.HORIZONTAL,
            variable=self.factor_var,
            command=self._on_factor_changed,
            label="Torque command units per gram",
        )
        self.factor_slider.grid(row=0, column=0, sticky="ew")
        ttk.Label(factor_frame, textvariable=self.torque_command_var).grid(
            row=1, column=0, sticky="w", padx=4, pady=3
        )

        output_frame = ttk.LabelFrame(self.root, text="Torque output", padding=10)
        output_frame.grid(row=3, column=0, padx=10, pady=6, sticky="ew")
        self.output_button = ttk.Button(
            output_frame, text="Start torque output", command=self.toggle_output
        )
        self.output_button.grid(row=0, column=0, padx=4, pady=4)
        ttk.Label(output_frame, textvariable=self.output_status_var).grid(
            row=0, column=1, sticky="w", padx=8, pady=4
        )

        log_frame = ttk.LabelFrame(self.root, text="Log", padding=8)
        log_frame.grid(row=4, column=0, padx=10, pady=6, sticky="nsew")
        self.root.rowconfigure(4, weight=1)
        self.log_text = tk.Text(log_frame, height=8, wrap=tk.WORD, state=tk.DISABLED)
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar = ttk.Scrollbar(log_frame, command=self.log_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.configure(yscrollcommand=scrollbar.set)

    def refresh_ports(self) -> None:
        ports = sorted(port.device for port in list_ports.comports())
        self.motor_port_combo.configure(values=ports)
        self.weight_port_combo.configure(values=ports)

        if not self.motor_port_var.get():
            motor_candidates = [
                port for port in ports if port.upper() != self.weight_port_var.get().upper()
            ]
            if motor_candidates:
                self.motor_port_var.set(motor_candidates[0])

    @staticmethod
    def _read_baud(value: str) -> int:
        baud = int(value)
        if baud <= 0:
            raise ValueError("Baud rate must be a positive integer")
        return baud

    @staticmethod
    def _same_port(first: str, second: str) -> bool:
        return first.strip().upper() == second.strip().upper()

    def toggle_motor_connection(self) -> None:
        if self.motor_serial is not None and self.motor_serial.is_open:
            self.disconnect_motor()
        else:
            self.connect_motor()

    def connect_motor(self) -> None:
        port_name = self.motor_port_var.get().strip()
        if not port_name:
            messagebox.showerror("Motor connection", "Select the motor driver's COM port.")
            return
        if (
            self.weight_serial is not None
            and self.weight_serial.is_open
            and self._same_port(port_name, self.weight_port_var.get())
        ):
            messagebox.showerror(
                "Motor connection", "The motor and weight sensor must use different ports."
            )
            return

        try:
            baud = self._read_baud(self.motor_baud_var.get())
            self.motor_serial = serial.Serial(
                port_name, baudrate=baud, timeout=0.1, write_timeout=1.0
            )
        except (ValueError, serial.SerialException, OSError) as exc:
            self.motor_serial = None
            self.motor_status_var.set("Disconnected")
            messagebox.showerror("Motor connection", str(exc))
            return

        self.motor_connect_button.configure(text="Disconnect")
        self.motor_status_var.set(f"Connected to {port_name} at {baud} baud")
        self._log(f"Connected motor driver: {port_name} @ {baud}")

    def disconnect_motor(self) -> None:
        self._stop_output("Motor disconnected", send_zero=True)
        port = self.motor_serial
        self.motor_serial = None
        if port is not None and port.is_open:
            port.close()
        self.motor_connect_button.configure(text="Connect")
        self.motor_status_var.set("Disconnected")
        self._log("Motor driver disconnected")

    def toggle_weight_connection(self) -> None:
        if self.weight_serial is not None and self.weight_serial.is_open:
            self.disconnect_weight()
        else:
            self.connect_weight()

    def connect_weight(self) -> None:
        port_name = self.weight_port_var.get().strip()
        if not port_name:
            messagebox.showerror("Weight sensor", "Select the weight sensor's COM port.")
            return
        if (
            self.motor_serial is not None
            and self.motor_serial.is_open
            and self._same_port(port_name, self.motor_port_var.get())
        ):
            messagebox.showerror(
                "Weight sensor", "The motor and weight sensor must use different ports."
            )
            return

        try:
            baud = self._read_baud(self.weight_baud_var.get())
            weight_port = serial.Serial(
                port_name, baudrate=baud, timeout=0.2, write_timeout=1.0
            )
        except (ValueError, serial.SerialException, OSError) as exc:
            self.weight_serial = None
            self.weight_status_var.set("Disconnected")
            messagebox.showerror("Weight sensor", str(exc))
            return

        self.last_weight = None
        self.last_weight_time = None
        self.weight_serial = weight_port
        self.reader_stop = threading.Event()
        self.weight_reader = threading.Thread(
            target=self._read_weight, args=(weight_port, self.reader_stop), daemon=True
        )
        self.weight_reader.start()
        self.weight_connect_button.configure(text="Disconnect")
        self.weight_status_var.set(f"Connected to {port_name} at {baud} baud")
        self.weight_reading_var.set("Weight: waiting for data")
        self._update_torque_preview()
        self._log(f"Connected weight sensor: {port_name} @ {baud}")

    def disconnect_weight(self) -> None:
        self._stop_output("Weight sensor disconnected", send_zero=True)
        self.reader_stop.set()
        port = self.weight_serial
        self.weight_serial = None
        if port is not None and port.is_open:
            port.close()
        if self.weight_reader is not None and self.weight_reader.is_alive():
            self.weight_reader.join(timeout=0.5)
        self.weight_reader = None
        self.last_weight = None
        self.last_weight_time = None
        self.weight_connect_button.configure(text="Connect")
        self.weight_status_var.set("Disconnected")
        self.weight_reading_var.set("Weight: waiting for data")
        self._update_torque_preview()
        self._log("Weight sensor disconnected")

    def _read_weight(self, port: serial.Serial, stop_event: threading.Event) -> None:
        while not stop_event.is_set():
            try:
                raw_line = port.readline()
            except (serial.SerialException, OSError) as exc:
                if not stop_event.is_set():
                    self.events.put(("sensor_error", str(exc)))
                return

            if not raw_line:
                continue
            line = raw_line.decode("ascii", errors="replace").strip()
            if not line:
                continue

            weight = parse_weight_grams(line)
            if weight is None:
                self.events.put(("invalid_line", line))
            else:
                self.events.put(("weight", weight, line))

    def toggle_output(self) -> None:
        if self.output_enabled:
            self._stop_output("Torque output stopped by user", send_zero=True)
        else:
            self.start_output()

    def start_output(self) -> None:
        if self.motor_serial is None or not self.motor_serial.is_open:
            messagebox.showwarning("Torque output", "Connect the motor driver first.")
            return
        if self.weight_serial is None or not self.weight_serial.is_open:
            messagebox.showwarning("Torque output", "Connect the weight sensor first.")
            return
        if (
            self.last_weight_time is None
            or time.monotonic() - self.last_weight_time > self.MAX_WEIGHT_AGE_SECONDS
        ):
            messagebox.showwarning(
                "Torque output", "Waiting for a fresh weight reading before starting."
            )
            return

        try:
            address = int(self.device_address_var.get())
            if not 0 <= address <= 0xFF:
                raise ValueError("Device address must be between 0 and 255")
            self._write_motor_frame(build_torque_mode_frame(address))
            self.active_device_address = address
            self.output_enabled = True
            self.output_button.configure(text="Stop torque output")
            self.output_status_var.set("Torque output active")
            self._send_current_torque()
            if self.output_enabled:
                self._log(f"Torque output started for device address {address}")
        except (ValueError, serial.SerialException, OSError) as exc:
            self._stop_output(f"Could not start torque output: {exc}", send_zero=True)
            messagebox.showerror("Torque output", str(exc))

    def _write_motor_frame(self, frame: bytes) -> None:
        if self.motor_serial is None or not self.motor_serial.is_open:
            raise serial.SerialException("Motor driver is not connected")
        written = self.motor_serial.write(frame)
        if written != len(frame):
            raise serial.SerialTimeoutException("Incomplete serial frame write")

    def _send_current_torque(self) -> None:
        if (
            not self.output_enabled
            or self.last_weight is None
            or self.active_device_address is None
        ):
            return

        try:
            torque_command = grams_to_torque_command(
                self.last_weight, self.factor_var.get()
            )
            frame = build_torque_command_frame(
                self.active_device_address, torque_command
            )
            self._write_motor_frame(frame)
        except (ValueError, serial.SerialException, OSError) as exc:
            self._stop_output(f"Torque send failed: {exc}", send_zero=True)
            return
        self._update_torque_preview()

    def _stop_output(self, reason: str, send_zero: bool) -> None:
        was_enabled = self.output_enabled
        self.output_enabled = False
        self.output_button.configure(text="Start torque output")

        address = self.active_device_address
        if (
            send_zero
            and was_enabled
            and address is not None
            and self.motor_serial is not None
            and self.motor_serial.is_open
        ):
            try:
                self._write_motor_frame(build_torque_command_frame(address, 0))
            except (ValueError, serial.SerialException, OSError) as exc:
                self._log(f"Failed to send zero torque: {exc}")

        self.active_device_address = None
        self.output_status_var.set("Output stopped")
        if was_enabled:
            self._log(reason)

    def _on_factor_changed(self, _value: str) -> None:
        self._update_torque_preview()
        if self.output_enabled:
            self._send_current_torque()

    def _update_torque_preview(self) -> None:
        if self.last_weight is None:
            self.torque_command_var.set("Torque command: --")
            return
        try:
            command = grams_to_torque_command(
                self.last_weight, self.factor_var.get()
            )
        except ValueError:
            self.torque_command_var.set("Torque command: out of range")
            return
        self.torque_command_var.set(
            f"Torque command: {command} "
            f"(weight {self.last_weight:g} g x factor {self.factor_var.get():.1f})"
        )

    def _process_events(self) -> None:
        if self.closed:
            return
        while True:
            try:
                event = self.events.get_nowait()
            except queue.Empty:
                break

            if event[0] == "weight":
                _, weight, line = event
                self.last_weight = weight
                self.last_weight_time = time.monotonic()
                self.weight_reading_var.set(f"Weight: {weight:g} g  |  {line}")
                self._update_torque_preview()
                if self.output_enabled:
                    self._send_current_torque()
            elif event[0] == "invalid_line":
                line = event[1]
                now = time.monotonic()
                if (
                    line != self.last_invalid_line
                    or now - self.last_invalid_log_time >= 2.0
                ):
                    self._log(f"Ignored sensor line: {line}")
                    self.last_invalid_line = line
                    self.last_invalid_log_time = now
            elif event[0] == "sensor_error":
                error = event[1]
                self._log(f"Weight sensor read failed: {error}")
                self.disconnect_weight()

        if (
            self.output_enabled
            and self.last_weight_time is not None
            and time.monotonic() - self.last_weight_time > self.MAX_WEIGHT_AGE_SECONDS
        ):
            self._stop_output("Weight reading stale; sent zero torque", send_zero=True)
            self.weight_status_var.set("No recent weight data")

        self.root.after(50, self._process_events)

    def _log(self, message: str) -> None:
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        line_count = int(self.log_text.index("end-1c").split(".")[0])
        if line_count > 500:
            self.log_text.delete("1.0", f"{line_count - 500}.0")
        self.log_text.see(tk.END)
        self.log_text.configure(state=tk.DISABLED)

    def close(self) -> None:
        self._stop_output("Application closing", send_zero=True)
        self.closed = True
        self.reader_stop.set()
        for port in (self.weight_serial, self.motor_serial):
            if port is not None and port.is_open:
                port.close()
        if self.weight_reader is not None and self.weight_reader.is_alive():
            self.weight_reader.join(timeout=0.5)
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    WeightTorqueGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
