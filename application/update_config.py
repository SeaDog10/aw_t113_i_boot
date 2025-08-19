# update_config.py
# -*- coding: utf-8 -*-
import os
import sys
import shutil

def detect_config_type(config_file):
    """检测当前.config是master还是slave"""
    if not os.path.exists(config_file):
        return None
    with open(config_file, "r") as f:
        content = f.read()
        if "RT_AMP_MASTER=y" in content:
            return "master"
        elif "RT_AMP_SLAVE=y" in content:
            return "slave"
    return None

def switch_config(target, config_file=".config"):
    """根据目标切换.config"""
    backup = ".config.bak"
    if os.path.exists(config_file):
        shutil.copy(config_file, backup)  # 备份当前配置

    src = "config_m" if target == "master" else "config_s"
    if not os.path.exists(src):
        print("[ERROR] 找不到 %s，请先保存好对应的配置文件！" % src)
        sys.exit(1)

    shutil.copy(src, config_file)
    print("[INFO] 已自动切换到 %s 配置" % target)

def main():
    amp = None
    for arg in sys.argv:
        if arg.startswith("amp="):
            amp = arg.split("=")[1]

    if not amp:
        return  # 没传amp参数就不处理

    target = "master" if amp == "master" else "slave"
    current = detect_config_type(".config")

    if current is None:
        print("[WARN] 未检测到有效的.config，自动切换到 %s" % target)
        switch_config(target)
    elif current != target:
        print("[WARN] 当前 .config 是 %s，和目标 %s 不一致" % (current, target))
        switch_config(target)
    else:
        print("[INFO] .config 已是 %s 配置，无需切换" % target)

if __name__ == "__main__":
    main()
