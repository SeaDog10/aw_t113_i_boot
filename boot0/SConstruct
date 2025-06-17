import os
import json
from building import *

# 校验配置完整性
def validate_config(config):
    required_keys = ['biuld_flags', 'environment', 'pre_defines']
    for key in required_keys:
        if key not in config:
            raise KeyError(f"Missing key: {key}")

# 执行自定义后处理命令
def execute_post_build(*args, **kwargs):
    if not post_build:
        print("No post_build commands to execute.")
        return
    for cmd in post_build:
        command = cmd.get('command')
        args = cmd.get('args', [])
        if not command:
            print("Error: Missing 'command' in post_build")
            continue

        # 拼接完整命令路径
        full_command = os.path.join(EXEC_PATH, command)
        full_args = ' '.join(args)
        full_cmd_line = f"{full_command} {full_args}"
        if os.system(full_cmd_line) != 0:
            print(f"Error: Command failed -> {full_cmd_line}")
            exit(1)

# 配置文件路径
json_path = os.path.join('.', 'build.json')

if not os.path.exists(json_path):
    print(f"{json_path} is not found in {os.getcwd()}, please ensure the file exists")
    exit(0)

with open(json_path, 'r') as f:
    config = json.load(f)

# 校验配置完整性
validate_config(config)

# 提取配置项
TARGET    = config['biuld_flags']['target']
EXEC_PATH = config['biuld_flags']['exec_path']
BUILD_DIR = config['biuld_flags']['biuld_dir']
environment = config['environment']
pre_defines = config['pre_defines']
post_build = config.get('post_build', [])

env = Environment(
    tools     = [environment['tools']],
    AS        = environment['AS'],
    CC        = environment['CC'],
    CXX       = environment['CXX'],
    AR        = environment['AR'],
    LINK      = environment['LINK'],
    ARFLAGS   = ' '.join(environment['ARFLAGS']),
    LINKFLAGS = ' '.join(environment['LINKFLAGS']),
    ASFLAGS   = ' '.join(environment['ASFLAGS']),
    CFLAGS    = ' '.join(environment['CFLAGS']),
    CXXFLAGS  = ' '.join(environment['CXXFLAGS'])
)

# 添加工具链路径
if EXEC_PATH not in os.environ['PATH']:
    env.PrependENVPath('PATH', EXEC_PATH)
env['ASCOM'] = env['ASPPCOM']

VariantDir(BUILD_DIR, '.', duplicate=0)

# rtconfig_path = os.path.join('./', 'rtconfig.h')
# parse_rtconfig_header(rtconfig_path)

# 导出环境和宏定义
Export("env")
Export("pre_defines")

# 加载目标对象和路径
objs, path = SConscript(os.path.join(BUILD_DIR, 'SConscript'))

# 添加头文件路径和宏定义
env.Append(CPPPATH=path)
if pre_defines and pre_defines[0]:
    env.Append(CPPDEFINES=pre_defines)

if env['PLATFORM'] == 'win32':
    win32_spawn = my_spawn()
    win32_spawn.env = env
    env['SPAWN'] = win32_spawn.spawn

# 创建主目标
target = env.Program(os.path.join(BUILD_DIR, TARGET), objs)

# 添加后处理别名
env.Alias('post_build', target, Action(execute_post_build))

# 默认构建目标和后处理
Default([target, 'post_build'])
