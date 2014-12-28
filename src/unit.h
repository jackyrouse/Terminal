/*
 * unit.h
 *
 *  Created on: 2014-7-26
 *      Author: root
 */

#ifndef UNIT_H_
#define UNIT_H_

#include <stdlib.h>
#include <stdio.h>

void print_hex(const char *array, int count, const char * Tags = NULL);
void Hex2Str(const char * Hex, char * Str, int Len); //Len为hex的位数
bool Str2Hex(const char * Str, char * Hex, int Len); ////Len为Str的位数
unsigned int Hex2Int(const char * Hex, int count = 4);
bool Int2Hex(const unsigned int src, char * Hexstr, int len = 4);
bool Str2Bin(const char szStr[8], char* cpRst);
void Bin2Str(const char ch, char * BinStr);
char StrBCC(const char * str, int len);
void Hexstrncpy(char * dst, const char * src, int len);
bool Hexstrcmp(const char * src, const char * dst, int srclen);
unsigned char CRC8(unsigned char * p, unsigned char counter);

#endif /* UNIT_H_ */
