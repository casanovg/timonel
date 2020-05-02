import os

Import("env")

PR_LIB_DIR = "nb-libs"
GITHUB_USR = "casanovg"
GITHUB_REP = "nb-libs"
NBLIBS_DIR = env.subst("$PROJECT_DIR/" + PR_LIB_DIR)

LIB_DEPS =  "twi/arduino/* "
LIB_DEPS += "twi/nb-twi-cmd/* "
LIB_DEPS += "timonel-ms/arduino/* "

if not os.path.exists(NBLIBS_DIR):
    print ("Getting " + GITHUB_REP + " repository libraries into this project ... ")
    env.Execute("git init " + PR_LIB_DIR)
    os.chdir(PR_LIB_DIR)
    env.Execute("git remote add -f origin https://github.com/" + GITHUB_USR + "/" + GITHUB_REP + ".git")
    env.Execute("git sparse-checkout init")
    env.Execute("git sparse-checkout set " + LIB_DEPS)    
    env.Execute("git pull origin master")
else:
    print ("Checking " + GITHUB_REP + " repository updates ... ")
    env.Execute("git --work-tree=$PROJECT_DIR/" + PR_LIB_DIR + " --git-dir=$PROJECT_DIR/" + PR_LIB_DIR + "/.git pull origin master --depth 100")
