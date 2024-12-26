import os
import re

config_list = []

class my_spawn:
    def spawn(self, sh, escape, cmd, args, env):
        # deal with the cmd build-in commands which cannot be used in
        # subprocess.Popen
        if cmd == 'del':
            for f in args[1:]:
                try:
                    os.remove(f)
                except Exception as e:
                    print('Error removing file: ' + e)
                    return -1
            return 0

        import subprocess

        newargs = ' '.join(args[1:])
        cmdline = cmd + " " + newargs

        # Make sure the env is constructed by strings
        _e = dict([(k, str(v)) for k, v in env.items()])

        # Windows(tm) CreateProcess does not use the env passed to it to find
        # the executables. So we have to modify our own PATH to make Popen
        # work.
        old_path = os.environ['PATH']
        os.environ['PATH'] = _e['PATH']

        try:
            proc = subprocess.Popen(cmdline, env=_e, shell=False)
        except Exception as e:
            print('Error in calling command:' + cmdline.split(' ')[0])
            print('Exception: ' + os.strerror(e.errno))
            if (os.strerror(e.errno) == "No such file or directory"):
                print ("\nPlease check Toolchains PATH setting.\n")

            return e.errno
        finally:
            os.environ['PATH'] = old_path

        return proc.wait()

def parse_rtconfig_header(header_file):
    try:
        with open(header_file, 'r') as f:
            for line in f:
                match = re.match(r'^\s*#define\s+(\w+)\s*', line)
                if match:
                    macro = match.group(1)
                    config_list.append(macro)
    except FileNotFoundError:
        print(f"Error: {header_file} not found.")

def GetDepend(depend):
    if depend in config_list:
        return True
    else:
        return False
