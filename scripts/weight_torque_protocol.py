import math
import re
import struct
from typing import Optional


# Values mirror Common::CMD_TYPE, PROPERTY, and CONTROLLER_TYPE.
CMD_WRITE_TO_DEVICE = 3
CMD_MOTION_TORQUE_COMMAND = 13
PROPERTY_DEV_CONTROL_MODE = 4
CONTROLLER_TYPE_TORQUE = 1
SOURCE_ID = 1

WEIGHT_FIELD_INDEX = 2
_NUMBER_PATTERN = re.compile(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?")


def parse_weight_grams(line: str) -> Optional[float]:
    fields = line.split(",")
    if len(fields) <= WEIGHT_FIELD_INDEX:
        return None

    values = _NUMBER_PATTERN.findall(fields[WEIGHT_FIELD_INDEX])
    if len(values) != 1:
        return None

    weight = float(values[0])
    return weight if math.isfinite(weight) else None


def grams_to_torque_command(weight_grams: float, factor: float) -> int:
    if not math.isfinite(weight_grams) or not math.isfinite(factor):
        raise ValueError("Weight and factor must be finite numbers")
    if not 0.1 <= factor <= 10.0:
        raise ValueError("Factor must be between 0.1 and 10.0")

    scaled_weight = weight_grams * factor
    if scaled_weight < -(1 << 31) or scaled_weight >= (1 << 31):
        raise ValueError("Scaled torque command is outside the signed 32-bit range")
    return int(scaled_weight)


def crc16_ccitt(data: bytes) -> int:
    checksum = 0
    for byte in data:
        checksum ^= byte << 8
        for _ in range(8):
            if checksum & 0x8000:
                checksum = (checksum << 1) ^ 0x1021
            else:
                checksum <<= 1
            checksum &= 0xFFFF
    return checksum


def build_serial_frame(
    device_address: int, command: int, register_address: int, payload: bytes
) -> bytes:
    for value in (device_address, command, register_address):
        if not 0 <= value <= 0xFF:
            raise ValueError("Frame fields must fit in one byte")
    if len(payload) != 4:
        raise ValueError("Serial frame payload must be exactly four bytes")

    # Matches the firmware's packed 9-byte CANBusFrame and 2-byte checksum.
    frame = struct.pack(
        "<BBBBB", device_address, SOURCE_ID, register_address, command, 0
    ) + payload
    return frame + struct.pack("<H", crc16_ccitt(frame))


def build_torque_mode_frame(device_address: int) -> bytes:
    return build_serial_frame(
        device_address,
        CMD_WRITE_TO_DEVICE,
        PROPERTY_DEV_CONTROL_MODE,
        struct.pack("<I", CONTROLLER_TYPE_TORQUE),
    )


def build_torque_command_frame(device_address: int, torque_command: int) -> bytes:
    if not -(1 << 31) <= torque_command < (1 << 31):
        raise ValueError("Torque command must fit in a signed 32-bit integer")
    return build_serial_frame(
        device_address,
        CMD_MOTION_TORQUE_COMMAND,
        0,
        struct.pack("<i", torque_command),
    )
