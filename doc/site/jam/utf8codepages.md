
# UTF-8 Codepages

For output the UART config register (UARTCF/$DF0B) has two bits (2 & 3)
that allow for selecting an output "codepage". For every codepage, the
lower 128 characters are the same (the ASCII table). Codepage 0 is raw
output, so the software has to create the multibyte sequences itself.
The other codepages can output a multibyte character from just a single
byte. The association of bytes to characters is stated below.
## Codepage 1

| Char | UTF-8 | Glyph | Description |
| ---- | ----- | ----- | ----------- |
| $80 | 0x2500 | &#x2500; | box drawings light horizontal |
| $81 | 0x2502 | &#x2502; | box drawings light vertical |
| $82 | 0x250c | &#x250c; | box drawings light down and right |
| $83 | 0x2510 | &#x2510; | box drawings light down and left |
| $84 | 0x2514 | &#x2514; | box drawings light up and right |
| $85 | 0x2518 | &#x2518; | box drawings light up and left |
| $86 | 0x251c | &#x251c; | box drawings light vertical and right |
| $87 | 0x2524 | &#x2524; | box drawings light vertical and left |
| $88 | 0x252c | &#x252c; | box drawings light down and horizontal |
| $89 | 0x2534 | &#x2534; | box drawings light up and horizontal |
| $8a | 0x253c | &#x253c; | box drawings light vertical and horizontal |
| $8b | 0x2600 | &#x2600; | black sun with rays |
| $8c | 0x263c | &#x263c; | white sun with rays |
| $8d | 0x2605 | &#x2605; | black star |
| $8e | 0x2606 | &#x2606; | white star |
| $8f | 0x2601 | &#x2601; | cloud |
| $90 | 0x2550 | &#x2550; | box drawings double horizontal |
| $91 | 0x2551 | &#x2551; | box drawings double vertical |
| $92 | 0x2554 | &#x2554; | box drawings double down and right |
| $93 | 0x2557 | &#x2557; | box drawings double down and left |
| $94 | 0x255a | &#x255a; | box drawings double up and right |
| $95 | 0x255d | &#x255d; | box drawings double up and left |
| $96 | 0x2560 | &#x2560; | box drawings double vertical and right |
| $97 | 0x2563 | &#x2563; | box drawings double vertical and left |
| $98 | 0x2566 | &#x2566; | box drawings double down and horizontal |
| $99 | 0x2569 | &#x2569; | box drawings double up and horizontal |
| $9a | 0x256c | &#x256c; | box drawings double vertical and horizontal |
| $9b | 0xfffd | &#xfffd; | (to be defined) |
| $9c | 0xfffd | &#xfffd; | (to be defined) |
| $9d | 0xfffd | &#xfffd; | (to be defined) |
| $9e | 0xfffd | &#xfffd; | (to be defined) |
| $9f | 0xfffd | &#xfffd; | (to be defined) |
| $a0 | 0xfffd | &#xfffd; | (to be defined) |
| $a1 | 0xfffd | &#xfffd; | (to be defined) |
| $a2 | 0x00a2 | &#x00a2; | cent sign |
| $a3 | 0x00a3 | &#x00a3; | pound sign |
| $a4 | 0xfffd | &#xfffd; | (to be defined) |
| $a5 | 0x00a5 | &#x00a5; | yen sign |
| $a6 | 0xfffd | &#xfffd; | (to be defined) |
| $a7 | 0x00a7 | &#x00a7; | section sign |
| $a8 | 0xfffd | &#xfffd; | (to be defined) |
| $a9 | 0xfffd | &#xfffd; | (to be defined) |
| $aa | 0xfffd | &#xfffd; | (to be defined) |
| $ab | 0xfffd | &#xfffd; | (to be defined) |
| $ac | 0x20ac | &#x20ac; | euro sign |
| $ad | 0x2571 | &#x2571; | box drawings light diagonal upper right to lower left |
| $ae | 0x2572 | &#x2572; | box drawings light diagonal upper left to lower right |
| $af | 0x2573 | &#x2573; | box drawings light diagonal cross |
| $b0 | 0x00b0 | &#x00b0; | degree sign |
| $b1 | 0x00b9 | &#x00b9; | superscript one |
| $b2 | 0x00b2 | &#x00b2; | superscript two |
| $b3 | 0x00b3 | &#x00b3; | superscript three |
| $b4 | 0x00b1 | &#x00b1; | plus-minus sign |
| $b5 | 0x00b5 | &#x00b5; | micro sign |
| $b6 | 0x00b7 | &#x00b7; | middle dot |
| $b7 | 0x2022 | &#x2022; | bullet |
| $b8 | 0x25cb | &#x25cb; | white circle |
| $b9 | 0x25cf | &#x25cf; | black circle |
| $ba | 0x25a1 | &#x25a1; | white square |
| $bb | 0x25a0 | &#x25a0; | black square |
| $bc | 0x00bc | &#x00bc; | vulgar fraction one quarter |
| $bd | 0x00bd | &#x00bd; | vulgar fraction one half |
| $be | 0x00be | &#x00be; | vulgar fraction three quarters |
| $bf | 0xfffd | &#xfffd; | (to be defined) |
| $c0 | 0x25b2 | &#x25b2; | black up-pointing triangle |
| $c1 | 0x25bc | &#x25bc; | black down-pointing triangle |
| $c2 | 0x25c4 | &#x25c4; | black left-pointing pointer |
| $c3 | 0x25ba | &#x25ba; | black right-pointing pointer |
| $c4 | 0x263a | &#x263a; | white smiling face |
| $c5 | 0x263b | &#x263b; | black smiling face |
| $c6 | 0x2020 | &#x2020; | dagger |
| $c7 | 0x2021 | &#x2021; | double dagger |
| $c8 | 0x2190 | &#x2190; | leftwards arrow |
| $c9 | 0x2191 | &#x2191; | upwards arrow |
| $ca | 0x2192 | &#x2192; | rightwards arrow |
| $cb | 0x2193 | &#x2193; | downwards arrow |
| $cc | 0x2194 | &#x2194; | left right arrow |
| $cd | 0x2195 | &#x2195; | up down arrow |
| $ce | 0x21b5 | &#x21b5; | downwards arrow with corner leftwards |
| $cf | 0xfffd | &#xfffd; | (to be defined) |
| $d0 | 0x2669 | &#x2669; | quarter note |
| $d1 | 0x266a | &#x266a; | eighth note |
| $d2 | 0x266b | &#x266b; | beamed eighth notes |
| $d3 | 0x266c | &#x266c; | beamed sixteenth notes |
| $d4 | 0x2654 | &#x2654; | white chess king |
| $d5 | 0x2655 | &#x2655; | white chess queen |
| $d6 | 0x2656 | &#x2656; | white chess rook |
| $d7 | 0x2657 | &#x2657; | white chess bishop |
| $d8 | 0x2658 | &#x2658; | white chess knight |
| $d9 | 0x2659 | &#x2659; | white chess pawn |
| $da | 0x265a | &#x265a; | black chess king |
| $db | 0x265b | &#x265b; | black chess queen |
| $dc | 0x265c | &#x265c; | black chess rook |
| $dd | 0x265d | &#x265d; | black chess bishop |
| $de | 0x265e | &#x265e; | black chess knight |
| $df | 0x265f | &#x265f; | black chess pawn |
| $e0 | 0x2660 | &#x2660; | black spade suit |
| $e1 | 0x2661 | &#x2661; | white heart suit |
| $e2 | 0x2662 | &#x2662; | white diamond suit |
| $e3 | 0x2663 | &#x2663; | black club suit |
| $e4 | 0x2664 | &#x2664; | white spade suit |
| $e5 | 0x2665 | &#x2665; | black heart suit |
| $e6 | 0x2666 | &#x2666; | black diamond suit |
| $e7 | 0x2667 | &#x2667; | white club suit |
| $e8 | 0x2122 | &#x2122; | trade mark sign |
| $e9 | 0x00a9 | &#x00a9; | copyright sign |
| $ea | 0x00ae | &#x00ae; | registered sign |
| $eb | 0x2610 | &#x2610; | ballot box |
| $ec | 0x2611 | &#x2611; | ballot box with check |
| $ed | 0x2612 | &#x2612; | ballot box with x |
| $ee | 0x2591 | &#x2591; | light shade |
| $ef | 0x2592 | &#x2592; | medium shade |
| $f0 | 0x2593 | &#x2593; | dark shade |
| $f1 | 0x259d | &#x259d; | quadrant upper right |
| $f2 | 0x2597 | &#x2597; | quadrant lower right |
| $f3 | 0x2596 | &#x2596; | quadrant lower left |
| $f4 | 0x2598 | &#x2598; | quadrant upper left |
| $f5 | 0x259a | &#x259a; | quadrant upper left and lower right |
| $f6 | 0x259e | &#x259e; | quadrant upper right and lower left |
| $f7 | 0x2580 | &#x2580; | upper half block |
| $f8 | 0x2584 | &#x2584; | lower half block |
| $f9 | 0x258c | &#x258c; | left half block |
| $fa | 0x2590 | &#x2590; | right half block |
| $fb | 0x2599 | &#x2599; | quadrant upper left and lower left and lower right |
| $fc | 0x259b | &#x259b; | quadrant upper left and upper right and lower left |
| $fd | 0x259c | &#x259c; | quadrant upper left and upper right and lower right |
| $fe | 0x259f | &#x259f; | quadrant upper right and lower left and lower right |
| $ff | 0x2588 | &#x2588; | full block |

## Codepage 2

Right now it is the same as codepage 1.

## Codepage 3

Right now it is the same as codepage 1.
