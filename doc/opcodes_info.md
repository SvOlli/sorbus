Infos For The Opcodes CSV Tables
================================

Format is CSV with semicolor (`;`) as separator.

Columns
-------

- 1st (empty title): opcode byte preceeded by "$" ($00 for BRK)
- Name: name of instruction (e.g. "LDA")
- Mode: addressing mode (e.g. "REL" for branch)
- Reserved: is it a resevered / undocumented (aka illegal) opcode
- Bytes: bytes used for command (8 bit mode for 65816)
- Cycles: cycles taken (without extra for e.g. page crossing)
- ExtraCycles: extra cycles taken when crossing a page(1) and/or taking a branch(2)
- MX: will this instruction add an extra byte for 16 bit A(1) or X,Y(2)?

Those columns will be mapped onto a 32 bit config bit array, see
`src/rp2040/disassemble.c` for details.

First column allows for more "useful" order of opcodes.