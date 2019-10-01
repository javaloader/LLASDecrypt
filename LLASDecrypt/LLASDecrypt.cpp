// LLASDecrypt.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <filesystem>
#include <ShlObj.h>
using namespace std;

#pragma comment(lib, "Shell32.lib")
#include "sqlite/sqlite3.h"

inline const char* GetBasename(const char* filename)
{
	const char* basename = filename + strlen(filename);

	for (; *basename != '/' && *basename != '\\' && basename != filename; basename--) {}

	if (basename != filename) return basename + 1;
	else return filename;
}


const char *GetImageType(BYTE* buf)
{
	DWORD h1 = *(DWORD*)(buf);
	h1 = (h1 & 0x000000ff) << 24 | (h1 & 0x0000ff00) << 8 |
		(h1 & 0x00ff0000) >> 8 | (h1 & 0xff000000) >> 24;
	DWORD h2 = *(DWORD*)(buf + 4);
	h2 = (h2 & 0x000000ff) << 24 | (h2 & 0x0000ff00) << 8 |
		(h2 & 0x00ff0000) >> 8 | (h2 & 0xff000000) >> 24;
	if (h1 == 0xFFD8FFE0 || h1 == 0xFFD8FFDB)
	{
		return "jpg";
	}
	else if (h1 == 0x49492A)
	{
		return "tiff";
	}
	else if (h1 == 0x424D)
	{
		return "bmp";
	}
	else if (h1 == 0x474946)
	{
		return "gif";
	}
	else if (h1 == 0x89504E47 && h2 == 0x0D0A1A0A)
	{
		return "png";
	}
	return 0;
}
int main(int argc, char **argv)
{
	sqlite3* db;
	char* zErrMsg = 0;

	if (argc < 3)
	{
		printf("Usage: LLASDecrypt.exe <db> <input>");
		return 1;
	}

	if (sqlite3_open(argv[1], &db) != SQLITE_OK) {
		fprintf(stderr, "Can't open database: %s.\n", sqlite3_errmsg(db));
		system("pause");
		return 1;
	}

	char dir[512];
	sprintf(dir, "Decryption Results\\%s", GetBasename(argv[2]));

	CreateDirectoryA("Decryption Results", 0);

	sqlite3_stmt* stmt;

	if (sqlite3_prepare(db, "SELECT name FROM sqlite_master WHERE type='table'", -1, &stmt, 0) != SQLITE_OK) {
		printf("Could not prepare statement.\n");
		system("pause");
		return 1;
	}

	while (sqlite3_step(stmt) != SQLITE_DONE)
	{
		char* name = (char *)sqlite3_column_text(stmt, 0);
		
		char sql[128];
		sprintf(sql, "SELECT head, size, key1, key2 FROM %s WHERE pack_name='%s' ORDER BY head ASC", name, GetBasename(argv[2]));
		sqlite3_stmt* stmt_;
		if (sqlite3_prepare(db, sql, -1, &stmt_, 0) == SQLITE_OK)
		{
			while (sqlite3_step(stmt_) != SQLITE_DONE)
			{
				int head = sqlite3_column_int(stmt_, 0);
				int size = sqlite3_column_int(stmt_, 1);
				int key1 = sqlite3_column_int(stmt_, 2);
				int key2 = sqlite3_column_int(stmt_, 3);
				
				FILE* in = fopen(argv[2], "rb");
				if (in)
				{
					fseek(in, 0, SEEK_END);
					long length = ftell(in);
					fseek(in, 0, SEEK_SET);

					BYTE* buf = (BYTE*)malloc(sizeof(BYTE) * length);
					fread(buf, 1, length, in);

					BYTE* result = (BYTE*)malloc(size);
					memcpy(result, &buf[head], size);

					int key3 = 12345;
					for (int i = 0; i < size; i++)
					{
						result[i] ^= BYTE(((unsigned int)key2 ^ (unsigned int)key1 ^ (unsigned int)key3) >> 24);
						key1 = 214013 * key1 + 2531011;
						key2 = 214013 * key2 + 2531011;
						key3 = 214013 * key3 + 2531011;
					}

					char output[128];
					const char* type = GetImageType(result);
					if (type == 0)
						sprintf(output, "%s\\%s_%d", dir, GetBasename(argv[2]), head);
					else sprintf(output, "%s\\%s_%d.%s", dir, GetBasename(argv[2]), head, type);

					CreateDirectoryA(dir, 0);
					FILE* out = fopen(output, "wb");
					if (out)
					{
						fwrite(result, 1, size, out);
						fclose(out);
					}

					free(buf);
					fclose(in);
				}
			}
		}
	}

	sqlite3_close(db);
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
