
Import("env")

# Uncomment the below code to use the project folder name as
# output file name base:

#import os
#env.Replace(PROGNAME="%s" % os.path.basename(os.getcwd()))

# or ...

# Uncomment the below code to use the platformio.ini
# PROJECT_NAME build option as output file name base:

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
env.Replace(PROGNAME="%s" % defines.get("PROJECT_NAME"))

# If none of these options are used, the default output
# file name "firmware.bin" will be produced. Please do 
# NOT use both solutions at the same time.
