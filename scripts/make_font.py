#!/usr/bin/env python3
import sys
import os
import math
import struct
import zlib


def main(argv):
	if len(argv) < 2:
		print("Usage: %s input.bdf" % argv[0])
		return

	input_filename = argv[1]
	output_base_filename = os.path.splitext(input_filename)[0]
	output_image_filename = "%s.png" % output_base_filename
	output_map_filename = "%s.png.txt" % output_base_filename
	
	size_x = None
	size_y = None
	default_char = None
	col_count = None
	char_code = None
	char_map = []
	image = []
	is_inside_bitmap = False
	char_index = 0
	x0 = None
	y0 = None
	image_stripe = None
	
	with open(input_filename, "r") as f:
		for line in f:
			line = line.strip()
			parts = line.split(" ")
			keyword = parts[0]
			if keyword == "FONTBOUNDINGBOX" and len(parts) >= 3:
				size_x = int(parts[1])
				size_y = int(parts[2])
			elif keyword == "DEFAULT_CHAR" and len(parts) >= 2:
				default_char = int(parts[1])
			elif keyword == "CHARS" and len(parts) >= 2:
				count = int(parts[1])
				col_count = int(math.ceil(math.sqrt(count * size_y / size_x)))
				image_width = col_count * size_x
				image_height = int(math.ceil(count / col_count)) * size_y
				image_stripe = math.ceil(image_width / 8) + 1
				image = [0 for i in range(image_stripe * image_height)]
			elif keyword == "ENCODING" and len(parts) >= 2:
				char_code = int(parts[1])
				char_map.append(char_code)
			elif keyword == "BITMAP":
				is_inside_bitmap = True
				x0 = (char_index % col_count) * size_x
				y0 = (char_index // col_count) * size_y
			elif keyword == "ENDCHAR":
				is_inside_bitmap = False
				char_index += 1
				x0 = None
				y0 = None
			elif is_inside_bitmap:
				bitmap = int(keyword, 16)
				for x in range(size_x):
					if bitmap & (1 << (size_x - x - 1)) != 0:
						image[y0 * image_stripe + 1 + (x0 + x) // 8] |= 1 << (8 - ((x0 + x) % 8) - 1)
				y0 += 1
	
	with open(output_image_filename, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		
		def png_pack(png_tag, data):
			chunk_head = png_tag + data
			f.write(struct.pack("!I", len(data)))
			f.write(chunk_head)
			f.write(struct.pack("!I", 0xFFFFFFFF & zlib.crc32(chunk_head)))
		
		png_pack(b'IHDR', struct.pack("!2I5B", image_width, image_height, 1, 0, 0, 0, 0)),
		png_pack(b'IDAT', zlib.compress(bytes(image), 9)),
		png_pack(b'IEND', b'')
	
	with open(output_map_filename, "w") as f:
		f.write("%s %s %s\n" % (size_x, size_y, default_char))
		for i in char_map:
			f.write("%s\n" % i)
		

if __name__ == "__main__":
	main(sys.argv)
