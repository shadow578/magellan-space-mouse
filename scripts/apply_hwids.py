"""
Apply custom USB Hardware IDs and Product String to the board configuration.
see https://community.platformio.org/t/changing-usb-device-vid-pid/3986/24
"""

VID = "0x256f"
PID = "0xc631"
PRODUCT_STRING = "Magellan USB"
VENDOR_STRING = "Logitech"

Import("env")

board_config = env.BoardConfig()
board_config.update("build.hwids", [[VID, PID]])
board_config.update("build.usb_product", PRODUCT_STRING)
board_config.update("vendor", VENDOR_STRING)

print(f"Applied custom USB IDs: VID={VID}, PID={PID}, Product={PRODUCT_STRING}, Vendor={VENDOR_STRING}")
