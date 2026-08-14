import struct
import capstone

CODE = b"\x00\x02\x1F\xD6\x70\x8B\xD8\x39\x71\x00\x00\x00\xFD\x03\x00\x91\xF3\x53\x01\xA9"

md = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_ARM)
for i in md.disasm(CODE, 0x1000):
    print("0x%x:\t%s\t%s" %(i.address, i.mnemonic, i.op_str))
