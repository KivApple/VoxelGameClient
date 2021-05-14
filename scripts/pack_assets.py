#!/usr/bin/env python3
import sys
import os
import glob
import argparse


ESCAPE_CHARS = {
	"\"": "\"",
	"\\": "\\",
	"\n": "n",
	"\r": "r",
	"\t": "t"
}


def filename_to_cname(filename):
	return "__".join(filename).replace("-", "_").replace(".", "_")


def add_assets(assets, pattern, binary):
	max_mtime = 0
	for filename in glob.glob(pattern, recursive=True):
		path = []
		dirname = filename
		while True:
			dirname, filename = os.path.split(dirname)
			if filename != "":
				path.append(filename)
			elif dirname != "":
				path.append(dirname)
			else:
				break
		path.reverse()
		assets.append((path, binary))
		max_mtime = max(max_mtime, os.path.getmtime(os.path.join(*path)))
	return max_mtime


def main():
	parser = argparse.ArgumentParser(description="Pack assets into C++ source")
	parser.add_argument("output", type=str)
	parser.add_argument("--name", type=str, default="packed_assets")
	parser.add_argument("--binary", metavar="ASSET", type=str, action="append")
	parser.add_argument("--text", metavar="ASSET", type=str, action="append")
	args = parser.parse_args()
	assets = []
	max_mtime = os.path.getmtime(sys.argv[0])
	if args.binary is not None:
		for asset in args.binary:
			max_mtime = max(max_mtime, add_assets(assets, asset, binary=True))
	if args.text is not None:
		for asset in args.text:
			max_mtime = max(max_mtime, add_assets(assets, asset, binary=False))
	output_mtime = 0
	if os.path.exists(args.output):
		output_mtime = os.path.getmtime(args.output)
	if output_mtime > max_mtime:
		print("All assets seems up to date")
		return
	with open(args.output, "w") as source:
		source.write("#include <cstddef>\n")
		source.write("#include <string>\n")
		source.write("#include <tuple>\n")
		source.write("#include <unordered_map>\n\n")
		for asset in assets:
			source.write("static const %schar %s[] = " % (
				"unsigned " if asset[1] else "",
				filename_to_cname(asset[0])
			))
			if asset[1]:
				source.write("{")
			else:
				source.write("\"\"")
			with open(
					os.path.join(*asset[0]),
					"rb" if asset[1] else "r",
					encoding="utf-8" if not asset[1] else None
			) as f:
				line_length = 0
				if not asset[1]:
					source.write("\n\t\"")
				while True:
					buffer = f.read(1024)
					if len(buffer) == 0:
						break
					prev_c = ""
					prev_is_hex = False
					for b in buffer:
						c = chr(b) if isinstance(b, int) else b
						cur_is_hex = False
						escape = ESCAPE_CHARS.get(c)
						if not asset[1] and escape is not None:
							s = "\\" + escape
						elif not asset[1] and c == "?" and prev_c == "?":
							s = "\\?"
						elif not asset[1] and " " <= c <= "~" and not prev_is_hex:
							s = c
						elif not asset[1] and (" " <= c < "0" or "9" < c < "A" or "Z" < c < "a" or "z" < c <= "~"):
							s = c
						else:
							if isinstance(b, int):
								s = "%s," % b
							else:
								s = "".join(["\\x{0:02X}".format(b2).lower() for b2 in b.encode("utf-8")])
							cur_is_hex = True
						source.write(s)
						line_length += len(s)
						if (not asset[1] and c == "\n") or line_length >= 80:
							if asset[1]:
								source.write("\n\t")
							else:
								source.write("\"\n\t\"")
							line_length = 0
						prev_c = c
						prev_is_hex = cur_is_hex
				if asset[1]:
					source.write("}")
				else:
					source.write("\"")
			source.write(";\n\n")
		source.write("extern \"C\" const std::unordered_map<std::string, std::tuple<const void*, size_t>> %s {\n" % args.name)
		for asset in assets:
			cname = filename_to_cname(asset[0])
			source.write(
				"\t{\n\t\t\"%s\",\n\t\t{\n\t\t\t%s,\n\t\t\t%s\n\t\t}\n\t},\n" % (
					"/".join(asset[0]), cname,
					os.path.getsize(os.path.join(*asset[0])) if asset[1] else "sizeof(%s) - 1" % cname
				)
			)
		source.write("};\n\n")
		for asset in assets:
			if not asset[1]:
				continue
			source.write("static_assert(sizeof(%s) == %s);\n" % (
				filename_to_cname(asset[0]), os.path.getsize(os.path.join(*asset[0]))
			))
	print("Packed %s asset(s)" % len(assets))


if __name__ == "__main__":
	main()
