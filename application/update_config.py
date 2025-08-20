# update_config.py
# -*- coding: utf-8 -*-
import os
import re
import shutil
import sys

def is_pkg_special_config(config_str):
    ''' judge if it's CONFIG_PKG_XX_PATH or CONFIG_PKG_XX_VER'''

    if type(config_str) == type('a'):
        if config_str.startswith("PKG_") and (config_str.endswith('_PATH') or config_str.endswith('_VER')):
            return True
    return False

def mk_rtconfig(filename):
    try:
        config = open(filename, 'r')
    except:
        print('open config:%s failed' % filename)
        return

    rtconfig = open('rtconfig.h', 'w')
    rtconfig.write('#ifndef RT_CONFIG_H__\n')
    rtconfig.write('#define RT_CONFIG_H__\n\n')

    empty_line = 1

    for line in config:
        line = line.lstrip(' ').replace('\n', '').replace('\r', '')

        if len(line) == 0:
            continue

        if line[0] == '#':
            if len(line) == 1:
                if empty_line:
                    continue

                rtconfig.write('\n')
                empty_line = 1
                continue

            if line.startswith('# CONFIG_'):
                line = ' ' + line[9:]
            else:
                line = line[1:]
                rtconfig.write('/*%s */\n' % line)

            empty_line = 0
        else:
            empty_line = 0
            setting = line.split('=')
            if len(setting) >= 2:
                if setting[0].startswith('CONFIG_'):
                    setting[0] = setting[0][7:]

                # remove CONFIG_PKG_XX_PATH or CONFIG_PKG_XX_VER
                if is_pkg_special_config(setting[0]):
                    continue

                if setting[1] == 'y':
                    rtconfig.write('#define %s\n' % setting[0])
                else:
                    rtconfig.write('#define %s %s\n' % (setting[0], re.findall(r"^.*?=(.*)$",line)[0]))

    if os.path.isfile('rtconfig_project.h'):
        rtconfig.write('#include "rtconfig_project.h"\n')

    rtconfig.write('\n')
    rtconfig.write('#endif\n')
    rtconfig.close()

def update_config(target_amp):
    config_file = ".config"
    config_m = "config_m"
    config_s = "config_s"

    if not os.path.exists(config_file):
        print("[ERROR] 未找到 .config 文件，请先生成配置。")
        sys.exit(1)

    # 读取当前 .config，判断是 master 还是 slave
    with open(config_file, "r") as f:
        config_text = f.read()

    is_master = "RT_AMP_MASTER=y" in config_text
    is_slave = "RT_AMP_SLAVE=y" in config_text

    if target_amp == "master":
        if is_slave:
            print("[INFO] 当前是 slave 配置，切换到 master")
            shutil.copy(config_file, config_s)
            shutil.copy(config_m, config_file)
        else:
            print("[INFO] 已经是 master 配置，无需切换")
            shutil.copy(config_file, config_m)
    elif target_amp == "slave":
        if is_master:
            print("[INFO] 当前是 master 配置，切换到 slave")
            shutil.copy(config_file, config_m)
            shutil.copy(config_s, config_file)
        else:
            print("[INFO] 已经是 slave 配置，无需切换")
            shutil.copy(config_file, config_s)
    else:
        print("[ERROR] amp 参数错误，仅支持 master 或 slave")
        sys.exit(1)
    mk_rtconfig(config_file)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python tools/update_amp_config.py [master|slave]")
        sys.exit(1)
    target = sys.argv[1]
    update_config(target)
