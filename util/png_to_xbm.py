#!/usr/bin/env python3
# Convert PNG images to XBM format for use in C/C++ projects (e.g., Arduino, U8g2).
# Requires Pillow (install with: pip install Pillow).
# Usage: python png_to_xbm.py input.png [-o output.xbm]
# If no output is specified, saves as inputname.xbm in the same directory.
# The output is a monochrome bitmap C header with 'im_' prefixes replaced to avoid conflicts.
# Suitable for embedded systems with monochrome displays.

from PIL import Image
import io
import os
import argparse

def png_to_xbm(input_filename: str, output_filename: str = None):

    if not os.path.isfile(input_filename):
        print(f"Error: File '{input_filename}' does not exist.")
        exit(1)

    # Open the image and convert to 1-bit pixels (monochrome)
    image = Image.open(input_filename).convert('1')  # 1 = 1-bit pixels, black and white

    # Save to an in-memory buffer
    buffer = io.BytesIO()
    image.save(buffer, format="XBM")
    xbm_data = buffer.getvalue().decode('ascii')

    input_file_basename = os.path.splitext(os.path.basename(input_filename))[0]

    # Determine output file name if provided
    if not output_filename:
        output_filename = f"{input_file_basename}.xbm"

    output_file_basename = os.path.splitext(os.path.basename(output_filename))[0]

    # use modern style for xbm bits
    xbm_data = xbm_data.replace("static char", "static const unsigned char")

    # Replace default 'im_' prefixes with existing 
    xbm_data = xbm_data.replace("im_width", f"{output_file_basename}_width")
    xbm_data = xbm_data.replace("im_height", f"{output_file_basename}_height")
    xbm_data = xbm_data.replace("im_bits", f"{output_file_basename}_bits")

    with open(output_filename, "w") as f:
        f.write(xbm_data)
    print(f"Saved: {output_filename}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert PNG to XBM monochrome format.')
    parser.add_argument('input_filename', help='Input PNG image')
    parser.add_argument('-o', '--output', help='Output XBM filename (optional)')
    args = parser.parse_args()

    png_to_xbm(args.input_filename, args.output)
