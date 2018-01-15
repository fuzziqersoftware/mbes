#ifndef __GLTEXTFONT_H
#define __GLTEXTFONT_H

#include <vector>

using namespace std;

// all characters have height 7 pixels, but may have variable width. so each
// vector is a bitmap for a character, and the vector's length must be a
// multiple of 7 - the character's width is the bitmap size / 7.

static const vector<vector<bool>> font({
  {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, // 0x00-0x0F
  {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, // 0x10-0x1F
  {   0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x20 (space)

  {   0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,0,0,
      0,1,0,
      0,0,0,
      0,0,0}, // 0x21 !

  {   1,0,1,
      1,0,1,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x22 "

  { 0,0,0,0,0,
    0,1,0,1,0,
    1,1,1,1,1,
    0,1,0,1,0,
    1,1,1,1,1,
    0,1,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x23 #

  { 0,0,1,0,0,
    0,1,1,1,0,
    1,0,1,0,0,
    0,1,1,1,0,
    0,0,1,0,1,
    0,1,1,1,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x24 $

  { 1,1,0,0,1,
    1,1,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,1,1,
    1,0,0,1,1,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x25 %

  { 0,1,1,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    0,1,0,0,0,
    1,0,1,0,1,
    1,0,0,1,0,
    0,1,1,0,1,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x26 &

  {     1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0}, // 0x27 '

  {   0,0,1,
      0,1,0,
      1,0,0,
      1,0,0,
      1,0,0,
      0,1,0,
      0,0,1,
      0,0,0,
      0,0,0}, // 0x28 ()

  {   1,0,0,
      0,1,0,
      0,0,1,
      0,0,1,
      0,0,1,
      0,1,0,
      1,0,0,
      0,0,0,
      0,0,0}, // 0x29 )

  { 0,0,1,0,0,
    1,0,1,0,1,
    0,1,1,1,0,
    0,0,1,0,0,
    0,1,1,1,0,
    1,0,1,0,1,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x2A *

  { 0,0,0,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x2B +

  {   0,0,
      0,0,
      0,0,
      0,0,
      0,0,
      0,1,
      0,1,
      1,0,
      0,0}, // 0x2C ,

  { 0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x2D -

  {   0,0,
      0,0,
      0,0,
      0,0,
      0,0,
      1,1,
      1,1,
      0,0,
      0,0}, // 0x2E .

  { 0,0,0,0,0,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x2F /

  { 0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,1,1,
    1,0,1,0,1,
    1,1,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x30 0

  { 0,0,1,0,0,
    0,1,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x31 1

  { 0,1,1,1,0,
    1,0,0,0,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x32 2

  { 0,1,1,1,0,
    1,0,0,0,1,
    0,0,0,0,1,
    0,0,1,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x33 3

  { 0,0,0,1,0,
    0,0,1,1,0,
    0,1,0,1,0,
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,0,1,0,
    0,0,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x34 4

  { 1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,0,
    0,0,0,0,1,
    0,0,0,0,1,
    1,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x35 5

  { 0,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x36 6

  { 1,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x37 7

  { 0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x38 8

  { 0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,1,
    0,0,0,0,1,
    0,0,0,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x39 9

  {   0,0,0,
      0,1,0,
      0,1,0,
      0,0,0,
      0,1,0,
      0,1,0,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x3A :

  { 0,0,0,0,
    0,0,1,0,
    0,0,1,0,
    0,0,0,0,
    0,0,1,0,
    0,0,1,0,
    0,1,0,0,
    0,0,0,0,
    0,0,0,0}, // 0x3B ;

  {   0,0,0,
      0,0,1,
      0,1,0,
      1,0,0,
      0,1,0,
      0,0,1,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x3C <

  {   0,0,0,
      0,0,0,
      1,1,1,
      0,0,0,
      1,1,1,
      0,0,0,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x3D =

  {   0,0,0,
      1,0,0,
      0,1,0,
      0,0,1,
      0,1,0,
      1,0,0,
      0,0,0,
      0,0,0,
      0,0,0}, // 0x3E >

  { 0,1,1,1,0,
    1,0,0,0,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x3F ?
  { 0,1,1,1,0,
    1,0,0,0,1,
    1,0,1,1,1,
    1,0,1,0,1,
    1,0,1,1,0,
    1,0,0,0,0,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0}, // 0x40 @
  {   0,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,1,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0}, // 0x41 A
  {   1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      0,0,0,0,
      0,0,0,0}, // 0x42 B
  {   0,1,1,1,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      0,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x43 C
  {   1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x44 D
  {   1,1,1,1,
      1,0,0,0,
      1,0,0,0,
      1,1,1,0,
      1,0,0,0,
      1,0,0,0,
      1,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x45 E
  {   1,1,1,1,
      1,0,0,0,
      1,0,0,0,
      1,1,1,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      0,0,0,0,
      0,0,0,0},//0x46 F
  {   0,1,1,1,
      1,0,0,0,
      1,0,0,0,
      1,0,1,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x47 G
  {   1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,1,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x48 H
  {   1,1,1,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      1,1,1,
      0,0,0,
      0,0,0},//0x49 I
  {   0,0,1,1,
      0,0,0,1,
      0,0,0,1,
      0,0,0,1,
      0,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x4A J
  {   1,0,0,1,
      1,0,0,1,
      1,0,1,0,
      1,1,0,0,
      1,0,1,0,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x4B K
  {   1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x4C L
  { 1,0,0,0,1,
    1,1,0,1,1,
    1,0,1,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x4D M
  {   1,0,0,1,
      1,1,0,1,
      1,0,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x4E N
  {   0,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x4F O
  {   1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      0,0,0,0,
      0,0,0,0},//0x50 P
  { 0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,1,0,1,
    1,0,0,1,1,
    0,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x51 Q
  {   1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      1,1,0,0,
      1,0,1,0,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x52 R
  {   0,1,1,1,
      1,0,0,0,
      1,0,0,0,
      0,1,1,0,
      0,0,0,1,
      0,0,0,1,
      1,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x53 S
  { 1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x54 T
  {   1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x55 U
  { 1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,1,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x56 V
  { 1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,1,0,1,
    1,0,1,0,1,
    0,1,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x57 W
  { 1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x58 X
  { 1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x59 Y
  { 1,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x5A Z
  {   1,1,
      1,0,
      1,0,
      1,0,
      1,0,
      1,0,
      1,1,
      0,0,
      0,0},//0x5B [
  { 0,0,0,0,0,
    1,0,0,0,0,
    0,1,0,0,0,
    0,0,1,0,0,
    0,0,0,1,0,
    0,0,0,0,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x5C \ (backslash)
  {   1,1,
      0,1,
      0,1,
      0,1,
      0,1,
      0,1,
      1,1,
      0,0,
      0,0},//0x5D ]
  { 0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x5E ^
  {   0,0,0,0,
      0,0,0,0,
      0,0,0,0,
      0,0,0,0,
      0,0,0,0,
      0,0,0,0,
      1,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x5F _
  {   1,0,
      0,1,
      0,0,
      0,0,
      0,0,
      0,0,
      0,0,
      0,0,
      0,0},//0x60 `
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x61 a
  {   1,0,0,0,
      1,0,0,0,
      1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x62 b
  {   0,0,0,
      0,0,0,
      0,1,1,
      1,0,0,
      1,0,0,
      1,0,0,
      0,1,1,
      0,0,0,
      0,0,0},//0x63 c
  {   0,0,0,1,
      0,0,0,1,
      0,1,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,1,
      0,0,0,0,
      0,0,0,0},//0x64 d
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,0,
      1,0,0,1,
      1,1,1,0,
      1,0,0,0,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x65 e
  {   0,0,1,
      0,1,0,
      1,1,1,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,0,0,
      0,0,0},//0x66 f
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,1,
      0,0,0,1,
      0,1,1,0},//0x67 g
  {   1,0,0,0,
      1,0,0,0,
      1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x68 h
  {     1,
        0,
        1,
        1,
        1,
        1,
        1,
        0,
        0},//0x69 i
  {   0,1,
      0,0,
      0,1,
      0,1,
      0,1,
      0,1,
      0,1,
      1,0,
      0,0},//0x6A j
  {   1,0,0,0,
      1,0,0,0,
      1,0,1,0,
      1,1,0,0,
      1,0,1,0,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x6B k
  {     1,
        1,
        1,
        1,
        1,
        1,
        1,
        0,
        0},//0x6C l
  { 0,0,0,0,0,
    0,0,0,0,0,
    1,1,1,1,0,
    1,0,1,0,1,
    1,0,1,0,1,
    1,0,1,0,1,
    1,0,1,0,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x6D m
  {   0,0,0,0,
      0,0,0,0,
      1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,0,0,0,
      0,0,0,0},//0x6E n
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x6F o
  {   0,0,0,0,
      0,0,0,0,
      1,1,1,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,1,1,0,
      1,0,0,0,
      1,0,0,0},//0x70 p
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,1,
      0,0,0,1,
      0,0,0,1},//0x71 q
  {   0,0,0,
      0,0,0,
      0,1,1,
      1,0,0,
      1,0,0,
      1,0,0,
      1,0,0,
      0,0,0,
      0,0,0},//0x72 r
  {   0,0,0,0,
      0,0,0,0,
      0,1,1,1,
      1,0,0,0,
      0,1,1,0,
      0,0,0,1,
      1,1,1,0,
      0,0,0,0,
      0,0,0,0},//0x73 s
  {   0,1,0,
      0,1,0,
      1,1,1,
      0,1,0,
      0,1,0,
      0,1,0,
      0,0,1,
      0,0,0,
      0,0,0},//0x74 t
  { 0,0,0,0,
    0,0,0,0,
    1,0,0,1,
    1,0,0,1,
    1,0,0,1,
    1,0,0,1,
    0,1,1,1,
    0,0,0,0,
    0,0,0,0},//0x75 u
  { 0,0,0,0,0,
    0,0,0,0,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x76 v
  { 0,0,0,0,0,
    0,0,0,0,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,1,0,1,
    1,0,1,0,1,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x77 w
  { 0,0,0,0,0,
    0,0,0,0,0,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x78 x
  {   0,0,0,0,
      0,0,0,0,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      1,0,0,1,
      0,1,1,1,
      0,0,0,1,
      0,1,1,0},//0x79 y
  { 0,0,0,0,0,
    0,0,0,0,0,
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x7A z
  {   0,0,1,
      0,1,0,
      0,1,0,
      1,0,0,
      0,1,0,
      0,1,0,
      0,0,1,
      0,0,0,
      0,0,0},//0x7B {
  {   0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,1,0,
      0,0,0,
      0,0,0},//0x7C |
  {   1,0,0,
      0,1,0,
      0,1,0,
      0,0,1,
      0,1,0,
      0,1,0,
      1,0,0,
      0,0,0,
      0,0,0},//0x7D }
  { 0,0,0,0,0,
    0,0,0,0,0,
    0,1,0,0,0,
    1,0,1,0,1,
    0,0,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x7E ~
  { 1,1,1,1,1,
    1,0,0,0,1,
    1,1,0,1,1,
    1,0,1,0,1,
    1,1,0,1,1,
    1,0,0,0,1,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0},//0x7F (blank)
  { 0,0,0,0,0,
    1,1,1,1,1,
    1,1,1,1,1,
    1,1,1,1,1,
    1,1,1,1,1,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x80 (filled square)
  { 0,0,0,0,0,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x81 (empty square)
  { 0,0,0,0,0,
    0,0,0,0,0,
    0,1,1,1,0,
    0,1,1,1,0,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x82 (small filled square)
  { 0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,1,0,
    0,0,0,1,1,
    0,0,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0},//0x83 (small right arrow)
});

#endif // __GLTEXTFONT_H
