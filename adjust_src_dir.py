
Import("env")
Import("projenv")

# access to global build environment
print(env)

# access to project build environment (is used source files in "src" folder)
print(projenv)

# Dump construction environments (for debug purpose)
# print(env.Dump())
# print(projenv.Dump())


# "env.GetProjectOption" shortcut for the active environment
VALUE_1 = env.GetProjectOption("custom_option1")

env['PROJECT_SRC_DIR'] = env['PROJECT_DIR'] + VALUE_1 + env["PIOENV"]

print("Setting the project directory to: {}".format(env['PROJECT_SRC_DIR']))
