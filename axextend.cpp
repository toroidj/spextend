/*----------------------------------------------------------------------------
	Extend Archive convert Susie Plug-in	Copyright (c) TORO
----------------------------------------------------------------------------*/
#define STRICT
#define UNICODE
#include <windows.h>
#include "TOROWIN.H"
#include "TCFG.H"
#pragma hdrstop

#define P_ProductName "axextend"
#define PLUGININFO_AM
#define PLUGININFO_ID "00AM"
#define PLUGININFO_NAME "Archives"

#include "coextend.cpp"

struct {
	SUSIEINFOLIST sil;
	WCHAR filename[MAX_PATH];
} CachedInfo = {{NULL, NULL, NULL, 0}, L""};

WCHAR WorkDirectory[MAX_PATH] = L"";

extern void FreeSusieInfoList(SUSIEINFOLIST &sil);
extern BOOL DeleteDirectories(const TCHAR *path);

/*-----------------------------------------------------------------------------
	dll の初期化／終了処理
-----------------------------------------------------------------------------*/
BOOL WINAPI DLLEntry(HINSTANCE hInstance, DWORD reason, LPVOID)
{
	hInst = hInstance;
	DumpDLLEntry(reason);

	if ( reason == DLL_THREAD_DETACH ){
		FreeTLSDATA();
	}else if ( reason == DLL_PROCESS_DETACH ){
		FreeSusieInfoList(CachedInfo.sil);
		DeleteDirectories(WorkDirectory);

		FreeTLSDATA();
		FreeTLSindex();
		FreeConfig();
	}
	return TRUE;
}

void InitSusieInfoList(SUSIEINFOLIST &sil)
{
	sil.allocsize = 4 * sizeof(SUSIE_FINFOW);
	sil.first = (SUSIE_FINFOW *)HeapAlloc(GetProcessHeap(), 0, sil.allocsize);
	if ( sil.first == NULL ){ // 確保失敗
		sil.infos = sil.last = NULL;
		sil.allocsize = 0;
	} else{
		sil.infos = sil.first;
		sil.last = (SUSIE_FINFOW *)(BYTE *)((BYTE *)sil.first + sil.allocsize);
	}
}

SUSIE_FINFOW *CheckSizeSusieInfoList(SUSIEINFOLIST &sil)
{
	SUSIE_FINFOW *newinfos;

	if ( sil.infos < sil.last ) return sil.infos++;
	newinfos = (SUSIE_FINFOW *)HeapReAlloc(GetProcessHeap(), 0, sil.first, sil.allocsize * 2);
	if ( newinfos == NULL ) return NULL;
	sil.allocsize *= 2;
	sil.infos = newinfos + (sil.infos - sil.first);
	sil.first = newinfos;
	sil.last = (SUSIE_FINFOW *)(BYTE *)((BYTE *)newinfos + sil.allocsize);
	return sil.infos++;
}

void EndListSusieInfoList(SUSIEINFOLIST &sil)
{
	DWORD size;
	SUSIE_FINFOW *newinfos;

	if ( sil.first == NULL ) return;
	size = ((BYTE *)sil.infos - (BYTE *)sil.first) + sizeof(SUSIE_FINFOW);
	newinfos = (SUSIE_FINFOW *)HeapReAlloc(GetProcessHeap(), 0, sil.first, size);
	if ( newinfos == NULL ){
		if ( sil.infos != sil.first ) sil.infos--;
	} else{
		sil.allocsize = size;
		sil.first = newinfos;
		sil.last = (SUSIE_FINFOW *)(BYTE *)((BYTE *)sil.first + size);
		sil.infos = sil.last - 1;
	}
	memset(sil.infos, 0, sizeof(SUSIE_FINFOW));
	sil.infos++;
}

void FreeSusieInfoList(SUSIEINFOLIST &sil)
{
	if ( sil.first == NULL ) return;
	HeapFree(GetProcessHeap(), 0, sil.first);
	sil.first = NULL;
}

HLOCAL CopySusieInfoList(SUSIEINFOLIST &sil)
{
	HLOCAL hImage;

	if ( sil.first == NULL ) return NULL;
	hImage = LocalAlloc(0, sil.allocsize);
	if ( hImage == NULL ) return NULL;
	memcpy(LocalLock(hImage), sil.first, sil.allocsize);
	LocalUnlock(hImage);
	return hImage;
}

//-----------------------------------------------------------------------------
BOOL FindSubDir(SUSIEINFOLIST &sil, ULONG_PTR &position, const WCHAR *subdir, const WCHAR *subname)
{
	WIN32_FIND_DATA ff;
	HANDLE hFF;
	WCHAR dirname[MAX_PATH * 3 + 16];
	WCHAR subdirname[SUSIE_PATH_MAX];
	int len;

	len = wsprintf(dirname, L"%s\\%s\\%s\\*", WorkDirectory, subdir, subname);
	if ( len >= MAX_PATH ) return TRUE; // 長すぎのため、列挙省略
	hFF = FindFirstFile(dirname, &ff);
	if ( hFF == INVALID_HANDLE_VALUE ) return FALSE;
	if ( subdir[0] == '\0' ){
		wcscpy(dirname, subname);
	} else{
		wsprintf(dirname, L"%s\\%s", subdir, subname);
	}
	if ( len < (SUSIE_PATH_MAX - 1) ){
		wcscpy(subdirname, dirname);
	} else{
		memcpy(subdirname, dirname, (SUSIE_PATH_MAX - 2) * sizeof(WCHAR));
		subdirname[SUSIE_PATH_MAX - 2] = '\0'; // subdirname に入りきらなくてもとりあえず列挙できるようにする
	}
	wcscat(subdirname, L"\\");

	do {
		SUSIE_FINFOW *infos;

		if ( IsRelativeDir(ff.cFileName) ) continue;
		if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
			if ( FindSubDir(sil, position, dirname, ff.cFileName) == FALSE ){
				break;
			}
			continue;
		}
		if ( NULL == (infos = CheckSizeSusieInfoList(sil)) ) break;
		memcpy(infos->filename, ff.cFileName, SUSIE_PATH_MAX * sizeof(WCHAR));
		infos->filename[SUSIE_PATH_MAX - 1] = '\0';
		infos->position = position++;

		#ifdef _WIN64
			infos->compsize = infos->filesize = (ULONG_PTR)ff.nFileSizeLow +
					((ULONG_PTR)ff.nFileSizeHigh << 32);

			infos->timestamp =
					(((ULONG_PTR)ff.ftLastWriteTime.dwLowDateTime) +
					 ((ULONG_PTR)ff.ftLastWriteTime.dwHighDateTime << 32) -
					 0x19db1ded53e8000) / 10000000;
		#else
			if ( ff.nFileSizeHigh ) ff.nFileSizeLow = 0xffffffff;
			infos->compsize = infos->filesize = ff.nFileSizeLow;

			// 日時は概算値
			SubDD(ff.ftLastWriteTime.dwLowDateTime,
					ff.ftLastWriteTime.dwHighDateTime,
					0xd53e8000, 0x19db1de);
			infos->timestamp = (ff.ftLastWriteTime.dwLowDateTime / 10000000) +
					(DWORD)MulDiv((int)ff.ftLastWriteTime.dwHighDateTime, 0x4000000, 10000000 / 64);
		#endif
		wcscpy(infos->path, subdirname);
		strcpy((char *)infos->method, "file");
		infos->crc = 0;
	} while ( IsTrue(FindNextFile(hFF, &ff)) );
	FindClose(hFF);
	return TRUE;
}

int GetArchiveInfoLocal(LPCWSTR buf, LONG_PTR len, unsigned int flags, SUSIEINFOLIST &sil)
{
	WCHAR filenameW[MAX_PATH];;
	DWORD offset;

	if ( WorkDirectory[0] == '\0' ){ // 展開先を用意
		int anum, anumtest = 100;

		GetTempPath(MAX_PATH, filenameW);

		anum = len & 0x7ff; // 適当な名前を生成する
		for ( ;;){
			wsprintf(WorkDirectory, L"%s\\axex%d.tmp", filenameW, anum);
			if ( GetFileAttributes(WorkDirectory) == BADATTR ) break;
			anum = (anum + (len / 2) + 3) & 0x7ff;
			if ( --anumtest == 0 ) break;
		}
		if ( CreateDirectory(WorkDirectory, NULL) == FALSE ){
			WorkDirectory[0] = '\0';
			return SUSIEERROR_FAULTREAD;
		}
	}
	sil.first = NULL;

	switch ( flags & SUSIE_SOURCE_MASK ){
		case SUSIE_SOURCE_DISK: // filename
			// キャッシュがあるなら利用する
			if ( (CachedInfo.sil.first != NULL) &&
				!wcscmp(CachedInfo.filename, buf) ){
				sil = CachedInfo.sil;
				return SUSIEERROR_NOERROR;
			}
			if ( wcslen(buf) >= MAX_PATH ) return SUSIEERROR_FAULTREAD;
			wcscpy(filenameW, buf);
			if ( FindCommand(filenameW, offset) == 0 ){
				return SUSIEERROR_UNKNOWNFORMAT;
			}
			break;

		case SUSIE_SOURCE_MEM: { // memory
			TLSDATA *tls;

			tls = GetTLSDATA();
			if ( (tls == NULL) || (tls->name[0] == '\0') ){
				return SUSIERESULT(SUSIEERROR_NOTSUPPORT);
			}
			// キャッシュがあるなら利用する
			if ( (CachedInfo.sil.first != NULL) &&
				!wcscmp(CachedInfo.filename, tls->name) ){
				sil = CachedInfo.sil;
				return SUSIEERROR_NOERROR;
			} else{
				wcscpy(filenameW, tls->name);
				offset = tls->commandoffset;
			}
			break;
		}

		default:
			return SUSIEERROR_NOTSUPPORT;
	}

	if ( GetFileAttributes(filenameW) & FILE_ATTRIBUTE_DIRECTORY ){
		return SUSIEERROR_FAULTREAD;
	}

	FreeSusieInfoList(CachedInfo.sil);

	// 展開先を再作成
	DeleteDirectories(WorkDirectory);
	CreateDirectory(WorkDirectory, NULL);

	// 展開
	if ( ExecuteConfig(offset, filenameW, WorkDirectory) == FALSE ){
		return SUSIEERROR_BROKENDATA;
	}

	{	// ディレクトリ内の一覧をリスト化する
		WIN32_FIND_DATA ff;
		HANDLE hFF;
		ULONG_PTR position = 0;
		WCHAR dirname[MAX_PATH];

		InitSusieInfoList(sil);
		wsprintf(dirname, L"%s\\*", WorkDirectory);
		hFF = FindFirstFile(dirname, &ff);
		if ( hFF != INVALID_HANDLE_VALUE ){
			do {
				SUSIE_FINFOW *infos;

				if ( IsRelativeDir(ff.cFileName) ) continue;
				if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					if ( FindSubDir(sil, position, L"", ff.cFileName) == FALSE ){
						break;
					}
					continue;
				}
				if ( NULL == (infos = CheckSizeSusieInfoList(sil)) ) break;
				memcpy(infos->filename, ff.cFileName, SUSIE_PATH_MAX * sizeof(WCHAR));
				infos->filename[SUSIE_PATH_MAX - 1] = '\0';
				infos->position = position++;

				#ifdef _WIN64
					infos->compsize = infos->filesize =
							(ULONG_PTR)ff.nFileSizeLow +
							((ULONG_PTR)ff.nFileSizeHigh << 32);

					infos->timestamp =
						(((ULONG_PTR)ff.ftLastWriteTime.dwLowDateTime) +
						 ((ULONG_PTR)ff.ftLastWriteTime.dwHighDateTime << 32) -
						 0x19db1ded53e8000) / 10000000;
				#else
					if ( ff.nFileSizeHigh ) ff.nFileSizeLow = 0xffffffff;
					infos->compsize = infos->filesize = ff.nFileSizeLow;

					// 日時は概算値
					SubDD(ff.ftLastWriteTime.dwLowDateTime,
							ff.ftLastWriteTime.dwHighDateTime,
							0xd53e8000, 0x19db1de);
					infos->timestamp = (ff.ftLastWriteTime.dwLowDateTime / 10000000) +
							(DWORD)MulDiv((int)ff.ftLastWriteTime.dwHighDateTime, 0x4000000, 10000000 / 64);
				#endif
				infos->path[0] = '\0';
				strcpy((char *)infos->method, "file");
				infos->crc = 0;
			} while ( IsTrue(FindNextFile(hFF, &ff)) );
			FindClose(hFF);
		}
		EndListSusieInfoList(sil);
	}

	if ( (flags & SUSIE_SOURCE_MASK) == SUSIE_SOURCE_DISK ){ // filename のときは、キャッシュする
		wcscpy(CachedInfo.filename, filenameW);
		CachedInfo.sil = sil;
	}
	return SUSIEERROR_NOERROR;
}

extern "C" int __stdcall GetArchiveInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)
{
	int result;
	SUSIEINFOLIST sil;

	DumpGetArchiveInfoW(buf, len, flag, lphInf);
	result = GetArchiveInfoLocal(buf, len, flag, sil);
	if ( result != SUSIEERROR_NOERROR ){
		*lphInf = NULL;
		return SUSIERESULT(result);
	}

	*lphInf = CopySusieInfoList(sil);
	return SUSIERESULT(SUSIEERROR_NOERROR);
}

extern "C" int __stdcall GetArchiveInfo(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)
{
	int result;
	SUSIEINFOLIST sil;
	HLOCAL hImage;
	WCHAR filenameW[MAX_PATH];;

	DumpGetArchiveInfo(buf, len, flag, lphInf);
	AnsiToUnicode(buf, filenameW, MAX_PATH);
	filenameW[MAX_PATH - 1] = '\0';

	result = GetArchiveInfoLocal(filenameW, len, flag, sil);
	if ( result != SUSIEERROR_NOERROR ){
		*lphInf = NULL;
		return SUSIERESULT(result);
	}

	if ( sil.first == NULL ){
		hImage = NULL;
	} else{ // SUSIE_FINFOW → SUSIE_FINFO 変換
		int infocount;

		infocount = sil.allocsize / sizeof(SUSIE_FINFOW);
		hImage = LocalAlloc(0, infocount * sizeof(SUSIE_FINFO));
		if ( hImage == NULL ){
			*lphInf = NULL;
			return SUSIERESULT(SUSIEERROR_EMPTYMEMORY);
		}

		int i;
		SUSIE_FINFO *infosA = (SUSIE_FINFO *)LocalLock(hImage);
		SUSIE_FINFOW *infosW = sil.first;

		for ( i = 0; i < infocount; i++ ){
			memcpy(infosA, infosW, (8 * sizeof(unsigned char)) + (sizeof(ULONG_PTR) * 3) + sizeof(susie_time_t));
			UnicodeToAnsi(infosW->path, infosA->path, SUSIE_PATH_MAX);
			infosA->path[SUSIE_PATH_MAX - 1] = '\0';
			UnicodeToAnsi(infosW->filename, infosA->filename, SUSIE_PATH_MAX);
			infosA->filename[SUSIE_PATH_MAX - 1] = '\0';
			infosA->crc = 0;
			infosA++;
			infosW++;
		}
		LocalUnlock(hImage);
	}
	*lphInf = hImage;
	return SUSIERESULT(SUSIEERROR_NOERROR);
}

extern "C" int __stdcall GetFileW(LPCWSTR src, LONG_PTR len, LPWSTR dest, unsigned int flags, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	SUSIEINFOLIST sil;
	SUSIE_FINFOW *infos;
	char *bin;
	HANDLE hFile;
	HLOCAL himage = NULL;
	int result;

	DumpGetFileW(src, len, dest, flags, progressCallback, lData);
	result = GetArchiveInfoLocal(src, len, flags, sil);
	if ( result != SUSIEERROR_NOERROR ) return SUSIERESULT(result);

	if ( progressCallback != NULL ) progressCallback(0, 100, lData);
	infos = sil.first;

	while ( infos->method[0] != '\0' ){
		WCHAR filepath[MAX_PATH];

		if ( infos->position != len ){
			infos++;
			continue;
		}

		if ( infos->path[0] != '\0' ){
			wsprintf(filepath, L"%s\\%s\\%s", WorkDirectory, infos->path, infos->filename);
		} else{
			wsprintf(filepath, L"%s\\%s", WorkDirectory, infos->filename);
		}

		if ( (flags & SUSIE_DEST_MASK) == SUSIE_DEST_MEM ){ // dest がメモリ
			DWORD temp;

			himage = LocalAlloc(0, infos->filesize);
			if ( himage == NULL ) break;
			bin = (char *)LocalLock(himage);

			hFile = CreateFile(filepath, GENERIC_READ,
				FILE_SHARE_WRITE | FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hFile == INVALID_HANDLE_VALUE ){
				result = SUSIEERROR_INTERNAL;
			}
			temp = 0;
			if ( ReadFile(hFile, bin, infos->filesize, &temp, NULL) == FALSE ){
				result = SUSIEERROR_FAULTREAD;
			}
			if ( temp != infos->filesize ) temp = 0;
			CloseHandle(hFile);
			if ( temp == 0 ) break;

			LocalUnlock(himage);
			*(HLOCAL *)dest = himage;
		} else{ // dest がfile
			WCHAR destpath[MAX_PATH * 2];

			wsprintf(destpath, L"%s\\%s", dest, infos->filename);

			if ( CopyFile(filepath, destpath, FALSE) == FALSE ){
				result = SUSIEERROR_INTERNAL;
			}
		}
		if ( progressCallback != NULL ) progressCallback(100, 100, lData);
		return SUSIERESULT(result);
	}
	if ( himage != NULL ){
		LocalUnlock(himage);
		LocalFree(himage);
	}
	if ( progressCallback != NULL ) progressCallback(50, 100, lData);
	return SUSIERESULT(SUSIEERROR_FAULTREAD);
}

extern "C" int __stdcall GetFile(LPCSTR src, LONG_PTR len, LPSTR dest, unsigned int flags, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	WCHAR srcW[MAX_PATH];
	WCHAR destW[MAX_PATH];

	DumpGetFile(src, len, dest, flags, progressCallback, lData);
	if ( (flags & SUSIE_SOURCE_MASK) == SUSIE_SOURCE_DISK ){ // ファイル名指定
		srcW[0] = '\0';
		AnsiToUnicode(src, srcW, MAX_PATH);
		srcW[MAX_PATH - 1] = '\0';
		src = (const char *)srcW;
	}
	if ( (flags & SUSIE_DEST_MASK) == SUSIE_DEST_DISK ){ // dest がfile
		destW[0] = '\0';
		AnsiToUnicode(dest, destW, MAX_PATH);
		destW[MAX_PATH - 1] = '\0';
		dest = (char *)destW;
	}
	return GetFileW((LPCWSTR)src, len, (LPWSTR)dest, flags DUMPFLAG, progressCallback, lData);
}

extern "C" int __stdcall GetFileInfoW(LPCWSTR buf, LONG_PTR len, LPCWSTR filename, unsigned int flag, SUSIE_FINFOW *lpInfo)
{
	SUSIE_FINFOW *infos;
	SUSIEINFOLIST sil;
	int result;

	DumpGetFileInfoW(buf, len, filename, flag, lpInfo);
	result = GetArchiveInfoLocal(buf, len, flag, sil);
	if ( result != SUSIEERROR_NOERROR ) return SUSIERESULT(result);

	infos = sil.first;
	while ( infos->method[0] != '\0' ){
		if ( infos->path[0] == '\0' ){
			if ( stricmpW(filename, infos->filename) ){
				infos++;
				continue;
			}
		} else{
			int pathlen;

			pathlen = wcslen(infos->path);
			if ( memicmp(filename, infos->path, pathlen * sizeof(WCHAR)) ||
				stricmpW(filename + pathlen +
				((*(filename + pathlen) == '\\') ? 1 : 0), // '\\'被り?
					infos->filename) ){
				infos++;
				continue;
			}
		}
		*lpInfo = *infos;
		return SUSIERESULT(SUSIEERROR_NOERROR);
	}
	return SUSIERESULT(SUSIEERROR_NOTSUPPORT);
}

extern "C" int __stdcall GetFileInfo(LPCSTR buf, LONG_PTR len, LPCSTR filename, unsigned int flags, SUSIE_FINFO *lpInfo)
{
	SUSIE_FINFOW infosW;
	int result;
	WCHAR filenameW[MAX_PATH];

	DumpGetFileInfo(buf, len, filename, flags, lpInfo);
	filenameW[0] = '\0';
	AnsiToUnicode(filename, filenameW, MAX_PATH);
	filenameW[MAX_PATH - 1] = '\0';

	if ( (flags & SUSIE_SOURCE_MASK) == SUSIE_SOURCE_DISK ){ // ファイル名指定
		WCHAR bufW[MAX_PATH];

		bufW[0] = '\0';
		AnsiToUnicode(buf, bufW, MAX_PATH);
		bufW[MAX_PATH - 1] = '\0';
		result = GetFileInfoW(bufW, len, filenameW, flags, &infosW);
	} else{
		result = GetFileInfoW((LPCWSTR)buf, len, filenameW, flags, &infosW);
	}
	if ( result != SUSIEERROR_NOERROR ) return SUSIERESULT(result);

	// SUSIE_FINFOW → SUSIE_FINFO 変換
	memcpy(lpInfo, &infosW, (8 * sizeof(unsigned char)) + (sizeof(ULONG_PTR) * 3) + sizeof(susie_time_t));
	UnicodeToAnsi(infosW.path, lpInfo->path, SUSIE_PATH_MAX);
	lpInfo->path[SUSIE_PATH_MAX - 1] = '\0';
	UnicodeToAnsi(infosW.filename, lpInfo->filename, SUSIE_PATH_MAX);
	lpInfo->filename[SUSIE_PATH_MAX - 1] = '\0';
	lpInfo->crc = 0;

	return SUSIERESULT(SUSIEERROR_NOERROR);
}

BOOL DeleteDirectories(const TCHAR *path)
{
	TCHAR buf[MAX_PATH * 2 + 8];
	WIN32_FIND_DATA ff;
	HANDLE hFF;

	if ( tstrlen(path) < 8 ) return FALSE; // ルート削除防止(簡易対策)
	wsprintf(buf, T("%s\\*"), path);
	hFF = FindFirstFile(buf, &ff);
	if ( INVALID_HANDLE_VALUE != hFF ){
		do{
			if ( IsRelativeDir(ff.cFileName) ) continue;
			wsprintf(buf, T("%s\\%s"), path, ff.cFileName);
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_READONLY ){
				SetFileAttributes(buf, FILE_ATTRIBUTE_NORMAL);
			}
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				if ( tstrlen(buf) < MAX_PATH ) DeleteDirectories(buf);
			} else{
				DeleteFile(buf);
			}
		} while ( IsTrue(FindNextFile(hFF, &ff)) );
		FindClose(hFF);
	}
	if ( IsTrue(RemoveDirectory(path)) ) return TRUE;
	return FALSE;
}
