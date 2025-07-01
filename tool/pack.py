import struct
import zlib
import argparse
import sys
import os

MAGIC = 0x46574D47  # 'FWMG' 固件魔数

def parse_elf_addresses(elf_path):
    try:
        from elftools.elf.elffile import ELFFile
    except ImportError:
        print("[ERROR] pyelftools not installed. Run 'pip install pyelftools'")
        sys.exit(1)

    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        entry = elf.header['e_entry']

        # 找第一个PT_LOAD段的起始虚拟地址作为加载地址（通常是第一个）
        load_addr = None
        for seg in elf.iter_segments():
            if seg['p_type'] == 'PT_LOAD':
                load_addr = seg['p_vaddr']
                break

        if load_addr is None:
            print("[ERROR] No PT_LOAD segment found in ELF")
            sys.exit(1)

        return load_addr, entry


def parse_hex_addresses(hex_path):
    try:
        from intelhex import IntelHex
    except ImportError:
        print("[ERROR] intelhex not installed. Run 'pip install intelhex'")
        sys.exit(1)

    ih = IntelHex()
    try:
        ih.loadhex(hex_path)
    except Exception as e:
        print(f"[ERROR] Failed to load HEX file: {e}")
        sys.exit(1)

    addresses = ih.addresses()
    if not addresses:
        print("[ERROR] HEX file contains no addresses")
        sys.exit(1)

    load_addr = min(addresses)
    entry_point = None

    # IntelHex库start_addr属性中可能有EIP(Entry Instruction Pointer)
    if ih.start_addr and 'EIP' in ih.start_addr:
        entry_point = ih.start_addr['EIP']
    else:
        # 如果没有，默认入口设为load_addr
        entry_point = load_addr

    return load_addr, entry_point


def pack_firmware(input_path, output_path, version, load_addr=None, start_addr=None):
    ext = os.path.splitext(input_path)[1].lower()
    firmware = None

    # 解析地址（加载地址和启动地址）
    if ext == '.elf':
        if load_addr is not None or start_addr is not None:
            print("[WARN] For ELF input, load/start addresses are ignored, parsed from ELF file.")
        load_addr, start_addr = parse_elf_addresses(input_path)

        # 使用 pyelftools 重新导出固件bin（strip ELF头）
        try:
            from elftools.elf.elffile import ELFFile
        except ImportError:
            print("[ERROR] pyelftools not installed. Run 'pip install pyelftools'")
            sys.exit(1)
        with open(input_path, 'rb') as f:
            elf = ELFFile(f)
            # 拼接所有可加载段数据（PT_LOAD）
            firmware_chunks = []
            for seg in elf.iter_segments():
                if seg['p_type'] == 'PT_LOAD':
                    data = seg.data()
                    firmware_chunks.append(data)
            firmware = b''.join(firmware_chunks)
    elif ext == '.hex':
        if load_addr is not None or start_addr is not None:
            print("[WARN] For HEX input, load/start addresses are ignored, parsed from HEX file.")
        load_addr, start_addr = parse_hex_addresses(input_path)
        # 用intelhex导出bin数据
        from intelhex import IntelHex
        ih = IntelHex(input_path)
        firmware = ih.tobinarray()
    elif ext == '.bin':
        # 纯bin，必须输入地址
        if load_addr is None or start_addr is None:
            print("[ERROR] For .bin input, load and start addresses must be provided.")
            sys.exit(1)
        with open(input_path, 'rb') as f:
            firmware = f.read()
    else:
        print("[ERROR] Unsupported file format. Only .bin, .elf, .hex supported.")
        sys.exit(1)

    size = len(firmware)
    crc32 = zlib.crc32(firmware) & 0xFFFFFFFF
    version_int = int(version, 16) if version.startswith("0x") else int(version)
    load_addr_int = int(load_addr, 16) if isinstance(load_addr, str) and str(load_addr).startswith("0x") else int(load_addr)
    start_addr_int = int(start_addr, 16) if isinstance(start_addr, str) and str(start_addr).startswith("0x") else int(start_addr)

    print(f"[INFO] Magic      = 0x{MAGIC:08X} ('FWMG')")
    print(f"[INFO] Size       = {size} bytes")
    print(f"[INFO] CRC32      = 0x{crc32:08X}")
    print(f"[INFO] Version    = 0x{version_int:08X}")
    print(f"[INFO] Load Addr  = 0x{load_addr_int:08X}")
    print(f"[INFO] Start Addr = 0x{start_addr_int:08X}")

    # 32字节头部结构（8字节保留字段填充）
    reserved = b'\x00' * 8

    with open(output_path, 'wb') as f:
        f.write(struct.pack('<I', MAGIC))            # 4字节魔数
        f.write(struct.pack('<I', size))             # 4字节固件大小
        f.write(struct.pack('<I', crc32))            # 4字节CRC32
        f.write(struct.pack('<I', version_int))      # 4字节版本号
        f.write(struct.pack('<I', load_addr_int))    # 4字节加载地址
        f.write(struct.pack('<I', start_addr_int))   # 4字节启动地址
        f.write(reserved)                             # 8字节保留
        f.write(firmware)

    print(f"[INFO] Packed firmware saved to: {output_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Pack firmware with 32-byte header (magic,size,crc,ver,load,start,reserved).")
    parser.add_argument("input", help="Input firmware file (.bin/.elf/.hex)")
    parser.add_argument("-o", "--output", help="Output packed file name (default: input_packed.bin)")
    parser.add_argument("-v", "--version", default="0x00010001", help="Firmware version (hex or int), default 0x00010001")
    parser.add_argument("-l", "--load", help="Load address (hex or int), required for .bin only")
    parser.add_argument("-s", "--start", help="Start address (hex or int), required for .bin only")

    args = parser.parse_args()

    output_file = args.output if args.output else args.input.rsplit('.', 1)[0] + "_packed.bin"

    pack_firmware(args.input, output_file, args.version, args.load, args.start)
