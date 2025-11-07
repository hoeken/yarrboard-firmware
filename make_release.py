#!/usr/bin/env python3

import argparse, os, json, re, sys, subprocess

#what boards / build targets to include in the release
boards = [
	"FROTHFET_REV_E",
	"BRINEOMATIC_REV_B"
]

if __name__ == '__main__':

	# --- parse arguments ---
	parser = argparse.ArgumentParser(description="Build Yarrboard firmware release")
	parser.add_argument("--test", action="store_true",
											help="Run in test mode (do not actually do anything, but print the results)")
	args = parser.parse_args()

	test_mode = args.test  # boolean True/False

	#look up our version #
	version = False
	dev_mode = False

	#check our config file for info.
	file = open("src/config.h", "r")
	for line in file:
		#version lookup
		result = re.search(r'YB_FIRMWARE_VERSION "(.*)"', line)
		if result:
			version = result.group(1)
			break

		# Development mode lookup
		m = re.search(r'#define\s+YB_IS_DEVELOPMENT\s+([A-Za-z0-9_]+)', line)
		if m:
			val = m.group(1).lower()
			dev_mode = val in ("true", "1")

 	#only proceed if we found the version
	if not version:
		print("🔴 YB_FIRMWARE_VERSION not #defined in src/config.h 🔴")
		sys.exit(1)   # bail out

 	#turn off dev mode
	if dev_mode:
		print("🔴 YB_IS_DEVELOPMENT set to true in src/config.h 🔴")
		sys.exit(1)   # bail out

	#check that we are on the "main" branch
	branch = subprocess.check_output(
		["git", "rev-parse", "--abbrev-ref", "HEAD"],
		text=True
	).strip()
	if branch != "main":
		print(f"🔴 You are on '{branch}', not 'main' 🔴")
		sys.exit(1)   # bail out

	config = []

	# get only the latest version section from CHANGELOG
	with open("CHANGELOG", "r") as f:
		content = f.read()

	# Extract the *first* version block
	# This grabs:
	#   ## Version <version>
	#   ...lines...
	# up until the next `## Version` OR end of file
	pattern = rf"(## Version {re.escape(version)}(?:\n(?!## Version).*)*)"
	m = re.search(pattern, content, re.MULTILINE)
	if not m:
		print("🔴 Could not extract latest version block from CHANGELOG.  Needs this format: ## Version x.y.z 🔴")
		sys.exit(1)

	changelog = m.group(1).strip()

	print (f'Making firmware release for version {version}')

	for board in boards:
		print (f'Building {board} firmware')

		#make our firmware.json entry
		bdata = {}
		bdata['type'] = board
		bdata['version'] = version
		bdata['url'] = f'https://raw.githubusercontent.com/hoeken/yarrboard-firmware/main/firmware/releases/{board}-{version}.bin'
		bdata['changelog'] = changelog
		config.append(bdata)
		
		#build the firmware
		cmd = f'pio run -e "{board}" -s'
		if test_mode:
			print (cmd)
		else:
			os.system(cmd)

		#sign the firmware
		cmd = f'openssl dgst -sign ~/Dropbox/misc/yarrboard/priv_key.pem -keyform PEM -sha256 -out .pio/build/{board}/firmware.sign -binary .pio/build/{board}/firmware.bin'
		if test_mode:
			print (cmd)
		else:
			os.system(cmd)

		#combine the signatures
		cmd = f'cat .pio/build/{board}/firmware.sign .pio/build/{board}/firmware.bin > .pio/build/{board}/signed.bin'
		if test_mode:
			print (cmd)
		else:
			os.system(cmd)

		#copy our fimrware to releases directory
		cmd = f'cp .pio/build/{board}/signed.bin releases/{board}-{version}.bin'
		if test_mode:
			print (cmd)
		else:
			os.system(cmd)

		#keep our ELF file for debugging later on....
		cmd = f'cp .pio/build/{board}/firmware.elf releases/{board}-{version}.elf'
		if test_mode:
			print (cmd)
		else:
			os.system(cmd)

	#write our config json file
	config_str = json.dumps(config, indent=4)
	if test_mode:
		print (config_str)
	else:
		with open("firmware.json", "w") as text_file:
				text_file.write(config_str)

	#some info to the user to finish the release
	print("Build complete.\n")
	print("Next steps:")
	print(f'1. Add the new firmware files: git add releases')
	print(f'2. Commit the new version: git commit -am "Firmware release v{version}"')
	print(f'3. Push changes to github: git push')
	print(f'4. Create a new tag: git tag -a v{version} -m "Firmware release v{version}"')
	print(f'5. Push your tags: git push origin v{version}')
	print(f'\nOr just run this:\n')
	print(f'git add releases && git commit -am "Firmware release v{version}" && git push && git tag -a v{version} -m "Firmware release v{version}" && git push origin v{version}')