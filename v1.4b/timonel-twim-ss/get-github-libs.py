import os

Import("env")

NBLIBS_DIR = env.subst("$PROJECT_DIR/nb-libs")

if not os.path.exists(NBLIBS_DIR):
    print ("Cloning nb-libs into this project ... ")
    env.Execute("git clone --depth 100 https://github.com/casanovg/nb-libs $PROJECT_DIR/nb-libs")
else:
    print ("Checking nb-libs repository updates ... ")
    env.Execute("git --work-tree=$PROJECT_DIR\\nb-libs --git-dir=$PROJECT_DIR\\nb-libs\.git pull origin master --depth 100")
