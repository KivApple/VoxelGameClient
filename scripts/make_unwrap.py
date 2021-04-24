#!/usr/bin/env python3
import sys


def main(argv):
	if len(argv) < 3:
		print("Usage: %s input.obj output.svg" % argv[0])
		return
	input_filename = argv[1]
	output_filename = argv[2]

	tex_coords = []
	with open(input_filename, "r", encoding="utf-8") as inp:
		with open(output_filename, "w") as out:
			w = 1024
			h = 1024
			
			out.write("<svg height=\"%s\" width=\"%s\" xmlns=\"http://www.w3.org/2000/svg\">\n" % (w, h))
			for line in inp:
				parts = line.split("#")[0].strip().split(" ")
				if parts[0] == "vt":
					tex_coords.append((float(parts[1]), float(parts[2])))
				elif parts[0] == "f":
					points = []
					for part in parts[1:]:
						parts2 = part.split("/")
						tex_coord = tex_coords[int(parts2[1]) - 1]
						x = tex_coord[0] * w
						y = h - tex_coord[1] * h - 1
						points.append((x, y))
					out.write(
						"\t<polygon points=\"%s\" style=\"fill: rgb(200, 200, 200); stroke: black; stroke-width: 1\" />\n" %
						" ".join(map(lambda point: ",".join(map(str, point)), points))
					)
			
			out.write("</svg>\n")


if __name__ == "__main__":
	main(sys.argv)
