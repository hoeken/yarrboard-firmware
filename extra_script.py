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

print("Compressing web app into header")
env.Execute("gulp")