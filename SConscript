import os
from building import *

Import('env', 'pre_defines')

def GetCurrentDir():
    conscript = File('SConscript')
    fn = conscript.rfile()
    name = fn.name
    path = os.path.dirname(fn.abspath)
    return path

def source_remove(src_list, name):
    src_list.remove(Glob(name)[0])

cwd           = GetCurrentDir()
list          = os.listdir(cwd)
objs          = []
source        = []
include_path  = []
user_defines  = []

# User Define

# User Define

pre_defines += user_defines
objs = [env.Object(src) for src in source]

for d in list:
    path = os.path.join(cwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        sub_objs, sub_path = SConscript(os.path.join(d, 'SConscript'))
        objs.extend(sub_objs)
        include_path.extend(sub_path)

Return('objs', 'include_path')
