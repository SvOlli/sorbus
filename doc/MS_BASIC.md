The BASIC provided is based upon Microsoft BASIC for the OSI Computer
Ohio Scientific, Inc. : https://www.oldcomputers.net/osi-600.html

The sourcecode is based upon the single file version created by
Grant Searle in 2013, which in itself is based by the source code
reverse engineered by Michael Steil.

| Name      | Release | MS Version  | ROM | FP | INPUTBUFFER | extensions |
| --------- | ------- | ----------- | --- | -- | ----------- | ---------- |
| OSI BASIC |    1977 | 1.0 REV 3.2 |  Y  | 6  | ZP          | -          |


This code contains these additional changes:

 * zero page addresses totally reshuffled, no POKE works as with OSI BASIC
   * e.g. inputbuffer starts at address 25 ($19) instead of 19 ($13) and is
     79 ($4f) instead of 72 ($48) bytes long
   * vector for USR stays the same (11,12) with 10 being the jmp opcode
 * line input is handled by kernel instead of BASIC, allowing line editing
 * BASIC keywords are case insensitive (both writings can be used)
 * BASIC variables are case sensitive (A <> a, both can be used)
 * the token of the useless NULL instrution has been replaced by SYS
   * sys0 calls the machine language monitor
   * addresses 13,14,15 will be used to shadow A,X,Y registers
   * rest of tokens are unchanged, so most of the code for OSI BASIC should
     work on the Sorbus Computer as well
 * address 9 can be used to select charset in list output
   * POKE 9,PEEK(9)AND223 switches to upper case output
   * POKE 9,PEEK(9)OR32 switches to lower case output
   * this only changes the tokens, not variable names
 * LOAD and SAVE are adapted to use internal drive
   * LOAD"$" shows directory _without_ losing BASIC programm
   * loading a file not used returns an "NF error"
     (reusing "no for" as "not found")
 * memory used for BASIC is $0400-$CFFF (=52K)
 * FRE(0) fixed
 * POKE a, PEEK(b) fixed


Credits:

 * main work by Michael Steil
 * Grant Searle did the onefile version (credit as mentioned by him)
 * function names and all uppercase comments taken from Bob Sander-Cederlof's excellent AppleSoft II disassembly:
   http://www.txbobsc.com/scsc/scdocumentor/
 * Applesoft lite by Tom Greene http://cowgod.org/replica1/applesoft/ helped a lot, too.
 * Thanks to Joe Zbicak for help with Intellision Keyboard BASIC
 * This work is dedicated to the memory of my dear hacking pal Michael "acidity" Kollmann.

