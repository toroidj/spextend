/*-----------------------------------------------------------------------------
	��`�t�@�C������p�b�P�[�W �ȈՔ�							(c)TORO 2017
-----------------------------------------------------------------------------*/
#define STRICT
#include <windows.h>
#include <string.h>
#include "TOROWIN.H"
#include "TCFG.H"

#define MARGINE 64
//======================================================================== �G�p
/*-------------------------------------
	�s��(\0,\a,\d)���H

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
	��(space,tab�j���X�L�b�v����

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

//------------------------------------- �s���󔒂�����(b:�擪,p:����)
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
	����̃p�����[�^�𒊏o����B�擪�Ɩ����̋󔒂͏�������

RET:	�擪�̕���(�����Ȃ������� 0)
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
//============================================================ ��`�t�@�C������
//------------------------------------- UTF-8 ��`�t�@�C����ǂݍ���
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
//------------------------------------- ��`�t�@�C�������
BOOL CFGClose(CFG *cfg)
{
	if ( cfg->bottom != NULL ) HeapFree(GetProcessHeap(), 0, cfg->bottom);
	cfg->bottom = NULL;
//	cfg->size = 0;
	return TRUE;
}
/*-------------------------------------
	�w��� offset ����A��s����ǂݍ���
	string �ɏ������ށB
	offset �ɂ͎��̍s�̐擪������
-------------------------------------*/
BOOL USEFASTCALL CFGGetLine(CFG *cfg, DWORD *offset, WCHAR *string, int maxlen)
{
	WCHAR *bottom, *p, *end = NULL, *maxptr = string + maxlen - 1;

	bottom = cfg->bottom + *offset;
	if ( *bottom == '\0' ) return FALSE;
											// �擪�̋󔒂��폜 ---------------
	SkipSpaceW((const WCHAR **)&bottom);
	p = bottom;
											// �s��&�R�����g�̌��� ------------
	for ( ; *p; p++ ){
	#if 0
		if ( (*p == ';') && (end == NULL) ){				// �R�����g
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
											// �s���̋󔒂��폜 ---------------
	while ( (end > bottom) && (*(end - 1) == ' ') ) end--;
											// ���� & TAB ���� ----------------
	for ( ; bottom < end; bottom++ ){
		if ( *bottom == '\t' ) continue;
		*string++ = *bottom;
		if ( string >= maxptr ) break;
	}
	*string = '\0';
	return TRUE;
}
//------------------------------------- �p�����[�^�𒊏o����
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