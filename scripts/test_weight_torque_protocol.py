import struct
import unittest

from weight_torque_protocol import (
    build_torque_command_frame,
    build_torque_mode_frame,
    crc16_ccitt,
    grams_to_torque_command,
    parse_weight_grams,
)


class WeightTorqueProtocolTests(unittest.TestCase):
    def test_parse_plain_and_labeled_gram_readings(self) -> None:
        self.assertEqual(parse_weight_grams("1,2,123.4,4"), 123.4)
        self.assertEqual(parse_weight_grams("ST, GS, -2.5 g"), -2.5)
        self.assertEqual(parse_weight_grams("a,b,+1.2e2 grams\r\n"), 120.0)

    def test_reject_empty_or_ambiguous_weight_lines(self) -> None:
        self.assertIsNone(parse_weight_grams(""))
        self.assertIsNone(parse_weight_grams("1,2"))
        self.assertIsNone(parse_weight_grams("1,2,3 4"))
        self.assertIsNone(parse_weight_grams("1,2,NaN g"))

    def test_scale_weight_and_reject_out_of_range_commands(self) -> None:
        self.assertEqual(grams_to_torque_command(120.0, 2.5), 300)
        self.assertEqual(grams_to_torque_command(-1.9, 1.0), -1)
        with self.assertRaises(ValueError):
            grams_to_torque_command(1e308, 10.0)

    def test_crc_matches_ccitt_known_vector(self) -> None:
        self.assertEqual(crc16_ccitt(b"123456789"), 0x31C3)

    def test_torque_frame_matches_firmware_layout(self) -> None:
        frame = build_torque_command_frame(0x23, -1234)

        self.assertEqual(len(frame), 11)
        self.assertEqual(frame[:5], bytes((0x23, 1, 0, 13, 0)))
        self.assertEqual(frame[5:9], struct.pack("<i", -1234))
        self.assertEqual(
            struct.unpack("<H", frame[9:])[0], crc16_ccitt(frame[:9])
        )

    def test_torque_mode_frame_selects_torque_control(self) -> None:
        frame = build_torque_mode_frame(3)

        self.assertEqual(frame[:5], bytes((3, 1, 4, 3, 0)))
        self.assertEqual(frame[5:9], struct.pack("<I", 1))


if __name__ == "__main__":
    unittest.main()
