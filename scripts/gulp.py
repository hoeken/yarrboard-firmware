import os
Import("env")

#
# Build actions
#

# def before_build(source, target, env):
#     print("Compressing web app into header")
#     env.Execute("gulp")

#env.AddPreAction("buildprog", before_build)
#env.AddPreAction("$BUILD_DIR/src/server.cpp.o", before_build)

#print("Packaging yarrboard-client from NPM")
#env.Execute("browserify html/stub.js > html/yarrboard-client.js")

# Define the folder path
# Change "gulp" to the full path if it's located somewhere specific
gulp_folder = "src/gulp"

# Check if the folder exists, if not, create it
if not os.path.exists(gulp_folder):
    print(f"Creating missing directory: {gulp_folder}")
    os.makedirs(gulp_folder)

print("Compressing web app into header")
env.Execute("GULP_BASE_PATH=.. gulp --gulpfile scripts/gulpfile.mjs")