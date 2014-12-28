/*
 * unit.cpp
 *
 *  Created on: 2014-7-26
 *      Author: root
 */
#include "unit.h"

void print_hex(const char *array, int count, const char * Tags)
{
	int i;
	if (Tags != NULL)
		printf("<%s> --- Write Data is : ", Tags);
	printf("[%02X", array[0] & 0xFF);
	for (i = 1; i < count; i++)
	{
		printf(" %02X", array[i] & 0xFF);
	}
	printf("]\n");
}

void Hex2Str(const char * Hex, char * Str, int Len)
{ //Len为hex的位数
	int i;
	for (i = 0; i < Len; i++)
		sprintf(Str + i * 2, "%02X ", Hex[i] & 0xFF);
	*(Str + i * 2) = 0;
}

bool Str2Hex(const char * Str, char * Hex, int Len)
{ //Len为Str的位数

	if (Len % 2 != 0)
		return false;
	int i;
	int tmp = 0;
	char StrTmp[3];
	for (i = 0; i < Len / 2; i++)
	{
		StrTmp[0] = Str[2 * i];
		StrTmp[1] = Str[2 * i + 1];
		StrTmp[2] = 0;
		sscanf(StrTmp, "%02X", &tmp);
		*(Hex + i) = tmp;
	}
	return true;
}

unsigned int Hex2Int(const char * Hex, int count)
{
	unsigned int res = 0;
	int j = 1;
	if (count > 4)
		return res;
	for (int i = 0; i < count; i++)
	{
		j = 1;
		j <<= (8 * (count - i - 1));
		res += (Hex[i] & 0xFF) * j;
	}
	return res;
}

bool Int2Hex(const unsigned int src, char * Hexstr, int strlen)
{
	int len = 4;
	if (strlen < 4)
		len = strlen;
	int j = 1;
	bool res = true;
	char tmp[4];
	unsigned int n = src;
	for (int i = 0; i < 4; i++)
	{
		j = 1;
		j <<= (8 * (4 - i - 1));
		tmp[i] = n / j;
		n %= j;
		if ((4 - i) <= len)
		{
			Hexstr[len - (4 - i)] = tmp[i];
		}
	}
	//print_hex (tmp , 4 , "Int2Hex") ;
	for (int k = 0; k < 4 - len; k++)
	{
		if (tmp[k] != 0x00)
		{
			res = false;
			break;
		}
	}
	return res;
}

//解析二进制字符串
bool Str2Bin(const char szStr[8], char* cpRst)
{
	char cRtn = '\0';
	if (szStr == NULL)
		return false;
	for (int i = 0; i < 8; i++)
	{
		if (szStr[i] != '0' && szStr[i] != '1')
			return false;
		cRtn = szStr[i] - 48; // '0' = 48
		cRtn = cRtn << (8 - i - 1);
		*cpRst |= cRtn;
	}
	return true;
}

//二进制转二进制字符串
void Bin2Str(const char ch, char * BinStr)
{
	int j = 1;
	for (int i = 7; i >= 0; i--)
	{
		BinStr[i] = (char) (bool) (ch & j) + '0';
		j <<= 1;
	}
	BinStr[8] = 0;
}

char StrBCC(const char * str, int len)
{
	char BCC = 0x00;
	BCC = str[0];
	for (int i = 1; i < len; i++)
	{
		BCC = BCC ^ str[i];
	}
	return BCC;
}//--将获取实际服务器IP＆PORT的指令加入发送队列
//--加入完毕

void Hexstrncpy(char * dst, const char * src, int len)
{
	for (int i = 0; i < len; i++)
	{
		dst[i] = src[i];
	}
}

bool Hexstrcmp(const char * src, const char * dst, int srclen)
{
	for (int i = 0; i < srclen; i++)
	{
		if (src[i] != dst[i])
			return false;
	}
	return true;
}

static unsigned char CRC8_Tab[] = { 0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
		0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79,
		0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
		0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1,
		0xC4, 0xC3, 0xCA, 0xCD, 0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
		0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE,
		0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
		0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86,
		0x93, 0x94, 0x9D, 0x9A, 0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
		0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E,
		0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
		0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8,
		0xAD, 0xAA, 0xA3, 0xA4, 0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
		0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60,
		0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
		0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28,
		0x3D, 0x3A, 0x33, 0x34, 0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
		0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37,
		0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
		0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F,
		0x8A, 0x8D, 0x84, 0x83, 0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
		0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3 };

unsigned char CRC8(unsigned char *p, unsigned char counter)
{
	unsigned char crc8 = 0x55;
	for(; counter > 0; counter--)
	{
		crc8 = CRC8_Tab[crc8^*p];
		p++;
	}
	crc8 ^= 0x00;
	return (crc8);
}