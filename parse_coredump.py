#!/usr/bin/env python3

import argparse, os

if __name__ == '__main__':

	parser = argparse.ArgumentParser()
	parser.add_argument("--version", help="Version of the firmware, eg. 1.2.3", default="dev")
	parser.add_argument("board", help="Hardware board revision, eg. RGB_INPUT_REV_A")

	args = parser.parse_args()

	#we need a coredump file
	if not os.path.isfile("coredump.txt"):
		print("Error: coredump.txt not found")
	else:
		#convert to binary format
		print("Converting coredump to binary format.")
		cmd = f'xxd -r -p coredump.txt coredump.bin'
		os.system(cmd)
		
		#keep our ELF file for debugging later on....
		print("Analyzing coredump")
		if args.version == 'dev':
			cmd = f'. ~/esp/esp-idf/export.sh && espcoredump.py info_corefile -c coredump.bin -t raw .pio/build/{args.board}/firmware.elf'
		else:
			cmd = f'. ~/esp/esp-idf/export.sh && espcoredump.py info_corefile -c coredump.bin -t raw releases/{args.board}-{args.version}.elf'
		os.system(cmd)
		#print(cmd)