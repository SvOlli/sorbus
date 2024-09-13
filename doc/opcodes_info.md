Infos For The Opcodes CSV Tables
================================

Format is CSV with semicolor (`;`) as separator.

Columns
-------

- 1st (empty title): opcode byte preceeded by "$" ($00 for BRK)
- Name: name of instruction (e.g. "LDA")
- Mode: addressing mode (e.g. "REL" for branch)
- Reserved: is it a resevered / undocumented opcode
- Bytes: bytes used for command (8 bit mode for 65816)
- Cycles: cycles taken (without extra for e.g. page crossing)
- MX: will this instruction add an extra byte for 16 bit A(1) or X,Y(2)?

Those columns will be mapped onto a 32 bit config bit array:
- Mode: bits 0-3 (4?)
- MX: bit 4 (easy to move)
- Bytes: bits 6-7 (0=1 byte, 1=2 bytes, ...)
- Cycles: bits 8-10 (0=1 cycle, 1=2 cycles, ...)

- Possible extra cycles: 0=none, 1=page cross, 2=branch taken

