import os

Import("env")

PR_LIB_DIR = "."
GITHUB_USR = "casanovg"
GITHUB_REP = "timonel"
GITHUB_BRN = "master"
GITHUB_TAG = "v1.4"
NBLIBS_DIR = env.subst("$PROJECT_DIR/" + PR_LIB_DIR)

LIB_DEPS =  "nb-libs/cmd "
LIB_DEPS += "nb-libs/twim "

if not os.path.exists(NBLIBS_DIR + "/.git"):
    print ("Getting " + GITHUB_REP + " repository libraries into this project ... ")
    env.Execute("git init " + PR_LIB_DIR)
    os.chdir(PR_LIB_DIR)
    env.Execute("git remote add -t " + GITHUB_BRN + " -f origin https://github.com/" + GITHUB_USR + "/" + GITHUB_REP + ".git")
    env.Execute("git sparse-checkout init")
    env.Execute("git sparse-checkout set " + LIB_DEPS)    
    env.Execute("git checkout -f tags/" + GITHUB_TAG)
else:
    print ("Checking " + GITHUB_REP + " repository updates ... ")
    env.Execute("git --work-tree=$PROJECT_DIR/" + PR_LIB_DIR + " --git-dir=$PROJECT_DIR/" + PR_LIB_DIR + "/.git checkout -f tags/" + GITHUB_TAG)
