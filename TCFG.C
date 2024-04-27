/*-----------------------------------------------------------------------------
	定義ファイル操作パッケージ 簡易版							(c)TORO 2017
-----------------------------------------------------------------------------*/
#define STRICT
#include <windows.h>
#include <string.h>
#include "TOROWIN.H"
#include "TCFG.H"

#define MARGINE 64
//======================================================================== 雑用
/*-------------------------------------
	行末(\0,\a,\d)か？

RET:	==0 :EOL	!=0 :Code
--------------------------------------*/
#ifndef UNICODE
BYTE USEFASTCALL IsEOLA(const char **str)
{
	BYTE code;

	code = **str;
	if ( code == '\0' ) return '\0';
	if ( code == 0xd ){
		if ( *(*str + 1) == 0x0a ) (*str)++;
		return '\0';
	}
	if ( code == 0xa ){
		if ( *(*str + 1) == 0x0d ) (*str)++;
		return '\0';
	}
	return code;
}
#endif
WCHAR USEFASTCALL IsEOLW(const WCHAR **str)
{
	WCHAR code;

	code = **str;
	if ( code == '\0' ) return '\0';
	if ( code == 0xd ){
		if ( *(*str + 1) == 0x0a ) (*str)++;
		return '\0';
	}
	if ( code == 0xa ){
		if ( *(*str + 1) == 0x0d ) (*str)++;
		return '\0';
	}
	return code;
}

/*-------------------------------------
	空白(space,tab）をスキップする

RET:	==0 :EOL	!=0 :Code
--------------------------------------*/
#ifndef UNICODE
BYTE USEFASTCALL SkipSpaceA(const char **str)
{
	BYTE code;

	for ( ;; ){
		code = IsEOLA(str);
		if ( (code != ' ') && (code != '\t') ) break;
		(*str)++;
	}
	return code;
}
#endif

WCHAR USEFASTCALL SkipSpaceW(const WCHAR **str)
{
	WCHAR code;

	for ( ;; ){
		code = IsEOLW(str);
		if ( (code != ' ') && (code != '\t') ) break;
		(*str)++;
	}
	return code;
}

//------------------------------------- 行末空白を除去(b:先頭,p:末尾)
void USEFASTCALL SkipSpaceB(const WCHAR *b, WCHAR *p)
{
	while ( b < p ){
		if ( (*p == ' ') || (*p == '\t') ){
			p--;
			continue;
		}
		break;
	}
	*(p + 1) = '\0';
}
/*-----------------------------------------------------------------------------
	一つ分のパラメータを抽出する。先頭と末尾の空白は除去する

RET:	先頭の文字(何もなかったら 0)
-----------------------------------------------------------------------------*/
WCHAR GetLineParam(const WCHAR **str, WCHAR *param)
{
	WCHAR code, bottom;
	const WCHAR *src;
	WCHAR *dst;

	src = *str;
	dst = param;
	bottom = SkipSpaceW(&src);
	if ( bottom == '\0' ){
		*str = src;
		return '\0';
	}
	src++;
	if ( bottom == '\"' ){
		bottom = *src;
		while ( '\0' != (code = IsEOLW(&src)) ){
			src++;
			if ( (code == '\"') && (*src <= ' ') ) break;
			*dst++ = code;
		}
	} else{
		*dst++ = bottom;
		while ( '\0' != (code = IsEOLW(&src)) ){
			src++;
			if ( code <= ' ' ) break;
			*dst++ = code;
		}
	}
	*dst = '\0';
	*str = src;
	return bottom;
}
//============================================================ 定義ファイル操作
//------------------------------------- UTF-8 定義ファイルを読み込む
BOOL CFGOpen(CFG *cfg, WCHAR *filename)
{
	HANDLE hFile;
	DWORD filesize, fileoffset, cfglen;
	char *fileimage;

	cfg->bottom = NULL;
//	cfg->size = 0;
//	cfg->write = 0;
//	tstrcpy(cfg->filename,filename);

	hFile = CreateFileW(filename, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;

	filesize = GetFileSize(hFile, NULL);
	fileimage = (char *)HeapAlloc(GetProcessHeap(), 0, filesize + MARGINE);
	if ( fileimage == NULL ){
		CloseHandle(hFile);
		return FALSE;
	}
	ReadFile(hFile, fileimage, filesize, &filesize, NULL);
	CloseHandle(hFile);

	fileoffset = memcmp(fileimage, UTF8HEADER, UTF8HEADERSIZE) ? 0 : UTF8HEADERSIZE;
	cfglen = MultiByteToWideChar(CP_UTF8, 0, fileimage + fileoffset, filesize - fileoffset, NULL, 0);
	cfg->bottom = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (cfglen * sizeof(WCHAR)) + MARGINE);
	if ( cfg->bottom == NULL ){
		HeapFree(GetProcessHeap(), 0, fileimage);
	//	cfg->size = 0;
		return FALSE;
	}
	memset((char *)(WCHAR *)(cfg->bottom + cfglen), 0, MARGINE);
	MultiByteToWideChar(CP_UTF8, 0, fileimage + fileoffset, filesize - fileoffset, cfg->bottom, cfglen);
	HeapFree(GetProcessHeap(), 0, fileimage);
	return TRUE;
}
//------------------------------------- 定義ファイルを閉じる
BOOL CFGClose(CFG *cfg)
{
	if ( cfg->bottom != NULL ) HeapFree(GetProcessHeap(), 0, cfg->bottom);
	cfg->bottom = NULL;
//	cfg->size = 0;
	return TRUE;
}
/*-------------------------------------
	指定の offset から、一行分を読み込み
	string に書き込む。
	offset には次の行の先頭を書く
-------------------------------------*/
BOOL USEFASTCALL CFGGetLine(CFG *cfg, DWORD *offset, WCHAR *string, int maxlen)
{
	WCHAR *bottom, *p, *end = NULL, *maxptr = string + maxlen - 1;

	bottom = cfg->bottom + *offset;
	if ( *bottom == '\0' ) return FALSE;
											// 先頭の空白を削除 ---------------
	SkipSpaceW((const WCHAR **)&bottom);
	p = bottom;
											// 行末&コメントの検索 ------------
	for ( ; *p; p++ ){
	#if 0
		if ( (*p == ';') && (end == NULL) ){				// コメント
			end = p;
			continue;
		}
	#endif
		if ( (*p == '\r') || (*p == '\n') ){	// CR/LF
			if ( end == NULL ) end = p;
			p++;
			while ( (*p == '\r') || (*p == '\n') ) p++;
			break;
		}
	}
	*offset = (DWORD)(p - cfg->bottom);
	if ( end == NULL ) end = p;
											// 行末の空白を削除 ---------------
	while ( (end > bottom) && (*(end - 1) == ' ') ) end--;
											// 複写 & TAB 除去 ----------------
	for ( ; bottom < end; bottom++ ){
		if ( *bottom == '\t' ) continue;
		*string++ = *bottom;
		if ( string >= maxptr ) break;
	}
	*string = '\0';
	return TRUE;
}
//------------------------------------- パラメータを抽出する
WCHAR *CFGFixParam(WCHAR *string, WCHAR **next)
{
	WCHAR *p, *q;

	SkipSpaceW((const WCHAR **)&string);
	p = string;
	while ( *p && (*p != ',') ) p++;
	q = p;
	if ( *p == ',' ) p++;
	while ( (q > string) && (*(q - 1) == ' ') ) q--;
	*q = '\0';

	if ( next != '\0' ) *next = p;
	return string;
}
