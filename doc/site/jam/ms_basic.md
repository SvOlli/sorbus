
# Microsoft OSI BASIC 1.0 Rev 3.2

The BASIC provided is based upon Microsoft BASIC for the [OSI Computer
Ohio Scientific, Inc.](https://www.oldcomputers.net/osi-600.html).

The source code is based upon the single file version created by
[Grant Searle in 2013](https://land-boards.com/blwiki/index.php?title=SIMPLE-6502),
which in itself is based by the [source code reverse engineered by Michael
Steil](https://github.com/mist64/msbasic).

The original configuration is like this:

| Name      | Release | MS Version  | ROM | 9digit | INPUTBUFFER | extensions | .define |
| --------- | ------- | ----------- | --- | ------ | ----------- | ---------- | ------- |
| OSI BASIC |    1977 | 1.0 REV 3.2 |  Y  | N      | ZP          | -          | CONFIG_10A |

Also, the original source code of the slightly newer version 1.1 has been
officially released by [Microsoft on GitHub](https://github.com/microsoft/BASIC-M6502).


## This code contains these additional changes implemented by SvOlli

- zero page addresses totally reshuffled, no POKE works as with OSI BASIC
    - e.g. inputbuffer starts at address 25 ($19) instead of 19 ($13) and is
      79 ($4f) instead of 72 ($48) bytes long
    - vector for USR stays the same (11,12) with 10 being the jmp opcode
- line input is handled by kernel instead of BASIC, allowing line editing
- BASIC keywords are case insensitive (both writings can be used)
- BASIC variables are case sensitive (A <> a, both can be used)
- the token of the useless NULL instrution has been replaced by SYS
    - sys0 calls the machine language monitor
    - addresses 13,14,15 will be used to shadow A,X,Y registers
    - rest of tokens are unchanged, so most of the code for OSI BASIC should
      work on the Sorbus Computer as well
- address 9 can be used to select charset in list output
    - POKE 9,PEEK(9)AND223 switches to upper case output
    - POKE 9,PEEK(9)OR32 switches to lower case output
    - this only changes the tokens, not variable names
- LOAD and SAVE are adapted to use internal drive
    - LOAD"$" shows directory _without_ losing BASIC programm
    - loading a file not available returns an "NF error"
      (reusing "no for" as "not found")
- memory used for BASIC is $0400-$CFFF (=51K)
- FRE(0) fixed
- POKE a, PEEK(b) fixed


## Original Credits

- written and copyright by Microsoft
- main work by Michael Steil
- Grant Searle did the onefile version (credit as mentioned by him)
    - function names and all uppercase comments taken from Bob Sander-Cederlof's
      excellent [AppleSoft II disassembly](http://www.txbobsc.com/scsc/scdocumentor/)
    - [Applesoft lite by Tom Greene](http://cowgod.org/replica1/applesoft/) helped a lot, too.
    - Thanks to Joe Zbicak for help with Intellision Keyboard BASIC
    - This work is dedicated to the memory of my dear hacking pal Michael "acidity" Kollmann.

## Zero Page Addresses

The porting required the addresses in the zero page to be moved. This table
shows what has moved to where.

| Label | Old (hex) | Old (dec) | New (hex) | New (dec) |
| ----- | --------: | --------: | --------: | --------: |
| GORESTART | $00 | (removed) | 0 | (removed) |
| GOSTROUT | $03 | $10 | 3 | 16 |
| GOAYINT | $06 | $13 | 6 | 19 |
| GOGIVEAYF | $08 | $15 | 8 | 21 |
| USR | $0A | $0A | 10 | 10 |
| POSX | $0E | (removed) | 14 | (removed) |
| LINNUM | $11 | $17 | 17 | 23 |
| INPUTBUFFER | $13 | $19 | 19 | 25 |
| CHARAC | $5B | $68 | 91 | 104 |
| ENDCHR | $5C | $69 | 92 | 105 |
| EOLPNTR | $5D | $6A | 93 | 106 |
| DIMFLG | $5E | $6B | 94 | 107 |
| VALTYP | $5F | $6C | 95 | 108 |
| DATAFLG | $60 | $6D | 96 | 109 |
| SUBFLG | $61 | $6E | 97 | 110 |
| INPUTFLG | $62 | $6F | 98 | 111 |
| CPRMASK | $63 | $70 | 99 | 112 |
| TEMPPT | $65 | $72 | 101 | 114 |
| LASTPT | $66 | $73 | 102 | 115 |
| TEMPST | $68 | $75 | 104 | 117 |
| INDEX | $71 | $7E | 113 | 126 |
| DEST | $73 | $80 | 115 | 128 |
| RESULT | $75 | $82 | 117 | 130 |
| TXTTAB | $79 | $86 | 121 | 134 |
| VARTAB | $7B | $88 | 123 | 136 |
| ARYTAB | $7D | $8A | 125 | 138 |
| STREND | $7F | $8C | 127 | 140 |
| FRETOP | $81 | $8E | 129 | 142 |
| FRESPC | $83 | $90 | 131 | 144 |
| MEMSIZ | $85 | $92 | 133 | 146 |
| CURLIN | $87 | $94 | 135 | 148 |
| OLDLIN | $89 | $96 | 137 | 150 |
| OLDTEXT | $8B | $98 | 139 | 152 |
| Z8C | $8D | $9A | 141 | 154 |
| DATPTR | $8F | $9C | 143 | 156 |
| INPTR | $91 | $9E | 145 | 158 |
| VARNAM | $93 | $A0 | 147 | 160 |
| VARPNT | $95 | $A2 | 149 | 162 |
| FORPNT | $97 | $A4 | 151 | 164 |
| LASTOP | $99 | $A6 | 153 | 166 |
| CPRTYP | $9B | $A8 | 155 | 168 |
| FNCNAM | $9C | $A9 | 156 | 169 |
| DSCPTR | $9E | $AB | 158 | 171 |
| DSCLEN | $A0 | $AD | 160 | 173 |
| Z52 | $A2 | $AF | 162 | 175 |
| ARGEXTENSION | $A3 | $B0 | 163 | 176 |
| HIGHDS | $A4 | $B1 | 164 | 177 |
| HIGHTR | $A6 | $B3 | 166 | 179 |
| INDX | $A8 | $B5 | 168 | 181 |
| TMPEXP | $A8 | $B5 | 168 | 181 |
| EXPON | $A9 | $B6 | 169 | 182 |
| LOWTR | $AA | $B7 | 170 | 183 |
| LOWTRX | $AA | $B7 | 170 | 183 |
| EXPSGN | $AB | $B8 | 171 | 184 |
| FAC | $AC | $B9 | 172 | 185 |
| FACSIGN | $B0 | $BD | 176 | 189 |
| SERLEN | $B1 | $BE | 177 | 190 |
| SHIFTSIGNEXT | $B2 | $BF | 178 | 191 |
| ARG | $B3 | $C0 | 179 | 192 |
| ARGSIGN | $B7 | $C4 | 183 | 196 |
| STRNG1 | $B8 | $C5 | 184 | 197 |
| STRNG2 | $BA | $C7 | 186 | 199 |
| CHRGET | $BC | $C9 | 188 | 201 |
| TOKENCASE | (new) | $09 | (new) | 9 |

If a BASIC program `POKE`s around in the zero page, the addresses need
to be adjusted using the table above. Also note that the most important
one, the configuration of `USR()` has not changed. A new one has been
added, see the function above. Zero page addresses 0-3 are I/O
ports, so then can't be used. Addresses 4-7 are used internally by
the kernel, so they will be overwritten. Address 8 should still be
unused, though.


## ROM Functions

Also all ROM functions have been moved due to the porting. However, this
is not a problem, as the `SYS` command was only added by this port, so
no original OSI BASIC code could be using it.
