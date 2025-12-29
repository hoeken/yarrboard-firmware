import os
import glob
Import("env")


#print("Packaging yarrboard-client from NPM")
#env.Execute("browserify html/stub.js > html/yarrboard-client.js")

# Ensure the output directory exists
gulp_folder = "src/gulp"
if not os.path.exists(gulp_folder):
    print(f"Creating missing directory: {gulp_folder}")
    os.makedirs(gulp_folder)

# Find the YarrboardFramework library path
# PlatformIO installs dependencies in .pio/libdeps/[environment_name]/
framework_path = None

# Look for YarrboardFramework in the libdeps directory
libdeps_pattern = os.path.join(".pio", "libdeps", "*", "YarrboardFramework")
matches = glob.glob(libdeps_pattern)

if matches:
    # Use the first match (they should all be the same version)
    framework_path = matches[0]
    print(f"Found YarrboardFramework at: {framework_path}")
else:
    print("ERROR: Could not find YarrboardFramework library!")
    print("Make sure the library is installed via platformio.ini")
    env.Exit(1)

# Set the environment variable for the gulpfile
os.environ["YARRBOARD_FRAMEWORK_PATH"] = framework_path

print("Compressing web app into header")
env.Execute("gulp --gulpfile scripts/gulpfile.mjs --cwd .")