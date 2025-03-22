/*----------------------------------------------------------------------------
	Extend convert Susie Plug-in	Copyright (c) TORO
----------------------------------------------------------------------------*/

#ifndef SUSIE_DEV
// co
#define DUMPFLAG
#define DumpDLLEntry(reason)
#define DumpGetPluginInfo(result) result
#define DumpGetPluginInfoW(infono, buf, buflen, result) result
#define DumpIsSupportedW(result) result
#define DumpIsSupported(result) result
#define DumpConfigurationDlg(result) result
#define SUSIERESULT(result) result

// if
#define DumpGetPicture(buf, len, flags, pHBInfo, pHBm, progressCallback, lData)
#define DumpGetPictureW(buf, len, flags, pHBInfo, pHBm, progressCallback, lData)
#define DumpGetPreview(buf, len, flags, pHBInfo, pHBm, progressCallback, lData) SUSIERESULT(SUSIEERROR_NOTSUPPORT)
#define DumpGetPreviewW(buf, len, flags, pHBInfo, pHBm, progressCallback, lData) SUSIERESULT(SUSIEERROR_NOTSUPPORT)
#define ShowPictureInfo(HBInfo, HBm)
#define DumpGetPictureInfo(buf, len, flags, lpInfo)
#define DumpGetPictureInfoW(buf, len, flags, lpInfo)

// ax
#define DumpGetArchiveInfo(buf, len, flag, lphInf)
#define DumpGetArchiveInfoW(buf, len, flag, lphInf)
#define ShowArchiveInfo(sil)
#define DumpGetFile(src, len, dest, flags, progressCallback, lData)
#define DumpGetFileW(src, len, dest, flags, progressCallback, lData)
#define DumpGetFileInfo(buf, len, filename, flag, lpInfo)
#define DumpGetFileInfoW(buf, len, filename, flags, lpInfo)
#else
HANDLE hStdOut;
DWORD CurrentThreadID = 0;

void WriteLog(const char *message)
{
	WCHAR filename[MAX_PATH];
	char temp[MAX_PATH];
	DWORD size;
	HANDLE hFile;
	SYSTEMTIME ltime;

	temp[0] = '\0';

	GetModuleFileNameW(hInst, filename, sizeof(filename) / sizeof(WCHAR));
	strcatW(filename, L".LOG");

	GetLocalTime(&ltime);

	hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		size = 0;
		SetFilePointer(hFile, 0, (PLONG)&size, FILE_END);
		size = wsprintfA(temp, "%4d-%02d-%02d %02d:%02d:%02d.%03d ",
				ltime.wYear, ltime.wMonth, ltime.wDay,
				ltime.wHour, ltime.wMinute, ltime.wSecond, ltime.wMilliseconds);
		WriteFile(hFile, temp, size, &size, NULL);
		WriteFile(hFile, message, strlen(message), &size, NULL);
		CloseHandle(hFile);
	}
}

void printoutf(const char *message, ...)
{
	char buf[0x500];
	va_list argptr;
	DWORD size;

	va_start(argptr, message);
	size = wvsprintfA(buf, message, argptr);
	WriteFile(hStdOut, buf, size, &size, NULL);
	WriteLog(buf);
}

void DumpOut(const char *memory, int offset)
{
	DWORD size;
	char dest[80], *destp;
	const char *srcp;
	int i;

	size = 16;
	srcp = memory;
	destp = dest;
	while( size ){
		destp += wsprintfA(destp, "   %04x:", offset);
		for ( i = 0 ; i < 16 ; i++ ){
			destp += wsprintfA(destp, " %02x", (unsigned char)*srcp++);
			size--;
			if ( size == 0 ) break;
		}
		destp += wsprintfA(destp, "\r\n");
	}
	printoutf(dest);
}

void DumpDLLEntry(DWORD reason)
{
	switch (reason){
		case DLL_PROCESS_ATTACH:
			CurrentThreadID = GetCurrentThreadId();
			if ( AllocConsole() ){
				SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);
			}
			hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
			printoutf(P_ProductName " DLLEntry(): Load plugin, thread: %d\r\n", CurrentThreadID);
			break;

		case DLL_THREAD_ATTACH:
			printoutf(P_ProductName " DLLEntry(): Attach thread: %d\r\n", GetCurrentThreadId());
			break;

		case DLL_THREAD_DETACH:
			printoutf(P_ProductName " DLLEntry(): Detach thread: %d\r\n", GetCurrentThreadId());
			break;

		case DLL_PROCESS_DETACH:
			printoutf(P_ProductName " DLLEntry(): Free plugin, thread: %d\r\n", GetCurrentThreadId());
			break;
	}
}

const char *errortext[] = {
	"Not Support",
	"no error",
	"User cancel",
	"Unknown format",
	"Broken DATA",
	"Empty memory",
	"Fault memory",
	"Fault read",
	"Window error",
	"Internal",
	"File write",
	"End of file"
};

const char *ResultText(int result)
{
	const char *text;

	if ( (result >= SUSIEERROR_NOTSUPPORT) && (result <= SUSIEERROR_EOF) ){
		text = errortext[result - SUSIEERROR_NOTSUPPORT];
	}else{
		text = "?";
	}
	return text;
}

void ThreadCheck(void)
{
	DWORD threadID = GetCurrentThreadId();

	if ( CurrentThreadID != threadID ){
		CurrentThreadID = threadID;
		printoutf(P_ProductName " thread: %d\r\n", threadID);
	}
}

// co
#define DUMPSILENT B31
#define DUMPFLAG | DUMPSILENT

void DumpGetPluginInfoBuflen(int infono, int buflen)
{
	int susielen;

	switch (infono){
		case 0:
			susielen = 8;
			break;

		case 1:
			susielen = 128;
			break;

		default:
			susielen = 255;
			break;
	}
	if ( buflen < susielen ){
		printoutf("  ● infono = %d のときの buflen (%d) が Susie 使用値(%d)より小さい\r\n", infono, buflen, susielen);
	}
}

#define DumpGetPluginInfo(result) DumpGetPluginInfoMain(infono, buf, buflen, result)
int DumpGetPluginInfoMain(int infono, LPSTR buf, int buflen, int result)
{
	ThreadCheck();
	printoutf("GetPluginInfo(%d, 0x%p, %d) = %d \"%s\"\r\n",
			infono, buf, buflen, result, (result > 0) ? buf : "none");
	DumpGetPluginInfoBuflen(infono, buflen);
	return result;
}

#define DumpGetPluginInfoW(infono, buf, buflen, result) DumpGetPluginInfoWMain(infono, buf, buflen, result)
int DumpGetPluginInfoWMain(int infono, LPWSTR buf, int buflen, int result)
{
	ThreadCheck();
	printoutf("GetPluginInfoW(%d, 0x%p, %d) = %d \"%ls\"\r\n",
			infono, buf, buflen, result, (result > 0) ? buf : L"none");
	DumpGetPluginInfoBuflen(infono, buflen);
	return result;
}

char *DumpIsSupported_Result(int result)
{
	return result ? "support" : "not support";
}

void DumpIsSupported_dw(const void *dw)
{
	if ( (LONG_PTR)dw & ~0xffff ){ // mem
		printoutf("  dw is memory\r\n");
		DumpOut((const char *)dw, 0);
		DumpOut((const char *)dw + 16, 16);
		printoutf("    :\r\n");
		DumpOut((const char *)dw + SUSIE_CHECK_SIZE - 16, SUSIE_CHECK_SIZE - 16);
	}else{ // handle
		printoutf("  dw is file handle(pointer: %d) ●memory に変更を推奨\r\n",
				SetFilePointer((HANDLE)dw, 0, NULL, FILE_CURRENT) );
	}
}

#define DumpIsSupported(result) DumpIsSupportedMain(filename, dw, result)
int DumpIsSupportedMain(LPCSTR filename, const void *dw, int result)
{
	ThreadCheck();
	printoutf(P_ProductName " IsSupported(\"%s\", 0x%p) = %d(%s)\r\n", filename, dw, result, DumpIsSupported_Result(result));
	DumpIsSupported_dw(dw);
	return result;
}

#define DumpIsSupportedW(result) DumpIsSupportedWMain(filename, dw, result)
int DumpIsSupportedWMain(LPCWSTR filename, const void *dw, int result)
{
	ThreadCheck();
	printoutf(P_ProductName " IsSupportedW(L\"%ls\", 0x%p) = %d(%s)\r\n", filename, dw, result, DumpIsSupported_Result(result));
	DumpIsSupported_dw(dw);
	return result;
}

#define DumpConfigurationDlg(result) DumpConfigurationDlgMain(hWnd, function, result)
int DumpConfigurationDlgMain(HWND hWnd, int function, int result)
{
	ThreadCheck();
	printoutf(P_ProductName " ConfigurationDlg(0x%p, %d) = %d\r\n", hWnd, function, result);
	return result;
}

int SUSIERESULT(int result)
{
	printoutf("  return %d (%s)\r\n", result, ResultText(result));
	return result;
}

void DumpSource(LPCSTR bufA, LPCWSTR bufW, LONG_PTR len, unsigned int flags)
{
	switch ( flags & SUSIE_SOURCE_MASK ){
		case SUSIE_SOURCE_DISK:
			printoutf("  source is disk(");
			if ( bufA != NULL ){
				printoutf("%s, offset %d).\r\n", bufA, len);
			}else{
				printoutf("%ls, offset %d).\r\n", bufW, len);
			}
			break;
		case SUSIE_SOURCE_MEM:
			printoutf("  source is memory(size=%d).\r\n", len);
			break;
		default:
			printoutf("  ● flags の source 指定が未定義\r\n");
	}
}

#ifdef PLUGININFO_IN
extern "C" int __stdcall GetPictureW(LPCWSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData);
extern "C" int __stdcall GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData);

void DumpGetPicture(LPCSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	if ( flags & DUMPSILENT ) return;
	ThreadCheck();
	printoutf("GetPicture(0x%p, %d, 0x%x, 0x%p, 0x%p, 0x%p, %d)\r\n", buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
	DumpSource(buf, NULL, len, flags);
}
void DumpGetPictureW(LPCWSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	if ( flags & DUMPSILENT ) return;
	ThreadCheck();
	printoutf("GetPictureW(0x%p, %d, 0x%x, 0x%p, 0x%p, 0x%p, %d)\r\n", buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
	DumpSource(NULL, buf, len, flags);
}
int DumpGetPreview(LPCSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	ThreadCheck();
	printoutf("GetPreview(0x%p, %d, 0x%x, 0x%p, 0x%p, 0x%p, %d)\r\n", buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
	return GetPicture(buf, len, flags | DUMPSILENT, pHBInfo, pHBm, progressCallback, lData);
}
int DumpGetPreviewW(LPCWSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	ThreadCheck();
	printoutf("GetPreviewW(0x%p, %d, 0x%x, 0x%p, 0x%p, 0x%p, %d)\r\n", buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
	return GetPictureW(buf, len, flags | DUMPSILENT, pHBInfo, pHBm, progressCallback, lData);
}

const char *Compressions[] = {
	"BI_RGB", "BI_RLE8", "BI_RLE4", "BI_BITFIELDS",
	"BI_JPEG", "BI_PNG", "BI_ALPHABITFIELDS"
};
#ifndef BI_ALPHABITFIELDS
  #define BI_ALPHABITFIELDS 6
#endif

typedef struct {
  DWORD bV5Size;
  LONG bV5Width;
  LONG bV5Height;
  WORD bV5Planes;
  WORD bV5BitCount;
  DWORD bV5Compression;
  DWORD bV5SizeImage;
  LONG bV5XPelsPerMeter;
  LONG bV5YPelsPerMeter;
  DWORD bV5ClrUsed;
  DWORD bV5ClrImportant;
  DWORD bV5RedMask;
  DWORD bV5GreenMask;
  DWORD bV5BlueMask;
  DWORD bV5AlphaMask;
  DWORD bV5CSType;
  CIEXYZTRIPLE bV5Endpoints;
  DWORD bV5GammaRed;
  DWORD bV5GammaGreen;
  DWORD bV5GammaBlue;
  DWORD bV5Intent;
  DWORD bV5ProfileData;
  DWORD bV5ProfileSize;
  DWORD bV5Reserved;
} BITMAPV5HEADER;

void ShowPictureInfo(HLOCAL HBInfo, HLOCAL HBm)
{
	BITMAPV5HEADER *bmpinfo;
	DWORD size, bmpsize, linesize, calc_bmpsize;

	if ( HBInfo == NULL ){
		printoutf(" ● pHBInfo == NULL\r\n");
	}
	if ( HBm == NULL ){
		printoutf(" ● pHBm == NULL\r\n");
	}
	bmpinfo = (BITMAPV5HEADER *)LocalLock(HBInfo);
	size = LocalSize(HBInfo);
	bmpsize = LocalSize(HBm);
	if ( size < 40 ){
		printoutf(" ● HBInfo の大きさが小さい %d < 40\r\n", size );
	}else{
		const char *wptr;

		if ( bmpinfo->bV5Size == 40 ){
			printoutf(" BITMAPINFO\r\n");
		}else if ( bmpinfo->bV5Size == sizeof(BITMAPV5HEADER) ){
			printoutf(" BITMAPV5HEADER\r\n");
		}else if ( bmpinfo->bV5Size < sizeof(BITMAPINFO) ){
			printoutf(" ● BITMAPINFO biSize が小さい %d < 40\r\n", bmpinfo->bV5Size );
		}else if ( bmpinfo->bV5Size < sizeof(BITMAPINFO) ){
			printoutf(" ● BITMAPINFO biSize 不明サイズ %d != %d\r\n", bmpinfo->bV5Size, sizeof(BITMAPINFO) );
		}
		printoutf(" biWidth: %d pix   biHeight: %d pix\r\n", bmpinfo->bV5Width, bmpinfo->bV5Height );

		if ( (bmpinfo->bV5Width <= 0) || (bmpinfo->bV5Width >= 0x20000) ){
			printoutf(" ● biWidth が異常値\r\n");
		}
		if ( (bmpinfo->bV5Height < 0) ){
			printoutf(" ● トップダウンDIB\r\n");
		}
		if ( (bmpinfo->bV5Height <= -0x20000) || (bmpinfo->bV5Height >= 0x20000) ){
			printoutf(" ● biHeight が異常値\r\n");
		}
		if ( bmpinfo->bV5Planes != 1 ){
			printoutf(" ● biPlanes が 1 以外(%d)\r\n", bmpinfo->bV5Planes);
		}
		printoutf(" biBitCount: %d bit (%d colors)\r\n", bmpinfo->bV5BitCount, 1 << bmpinfo->bV5BitCount );
		if ( !( (bmpinfo->bV5BitCount <= 1) || (bmpinfo->bV5BitCount == 4) ||
				(bmpinfo->bV5BitCount == 8) || (bmpinfo->bV5BitCount == 16) ||
				(bmpinfo->bV5BitCount == 24) || (bmpinfo->bV5BitCount == 32))){
			printoutf(" ● biBitCount が異常値\r\n");
		}
		if ( bmpinfo->bV5Compression > BI_ALPHABITFIELDS ){
			wptr = "?";
		}else{
			wptr = Compressions[bmpinfo->bV5Compression];
		}
		printoutf(" biCompression: %d (%s)\r\n", bmpinfo->bV5Compression, wptr);
		if ( bmpinfo->bV5Compression != BI_RGB ){
			printoutf(" ● BI_RGB 以外\r\n");
		}
		linesize = DwordBitSize(bmpinfo->bV5Width * bmpinfo->bV5BitCount);
		calc_bmpsize = linesize * ((bmpinfo->bV5Height >= 0) ? bmpinfo->bV5Height : -bmpinfo->bV5Height);

		printoutf(" biSizeImage: %d bytes (HBm: %d bytes. calculated: %d bytes)\r\n", bmpinfo->bV5SizeImage, bmpsize, calc_bmpsize);
		if ( ((bmpinfo->bV5Compression != BI_RGB) && (bmpinfo->bV5SizeImage == 0)) ||
			 (bmpinfo->bV5SizeImage > bmpsize) ){
			printoutf(" ● biSizeImage が異常値\r\n");
		}
		if ( ((bmpinfo->bV5Compression == BI_RGB) || (bmpinfo->bV5Compression == BI_BITFIELDS)) ){
			if ( (bmpinfo->bV5SizeImage != 0) && (bmpinfo->bV5SizeImage != calc_bmpsize) ){
				printoutf(" ● biSizeImage が calculated と一致しない\r\n");
			}
			if ( bmpsize < calc_bmpsize ){
				printoutf(" ● ビットマップが calculated より少ない\r\n");
			}
		}
		printoutf(" biXPelsPerMeter: %d (%d ppi) biYPelsPerMeter %d (%d ppi)\r\n",
				bmpinfo->bV5XPelsPerMeter, bmpinfo->bV5XPelsPerMeter / 39,
				bmpinfo->bV5YPelsPerMeter, bmpinfo->bV5YPelsPerMeter / 39);
		if ( bmpinfo->bV5XPelsPerMeter != bmpinfo->bV5YPelsPerMeter ){
			printoutf(" ● アスペクト比が 1 以外\r\n");
		}
		printoutf(" biClrUsed: %d   biClrImportant %d\r\n\r\n",
				bmpinfo->bV5ClrUsed , bmpinfo->bV5ClrImportant);
		if ( bmpinfo->bV5Compression == BI_BITFIELDS ){
			printoutf(" Bit mask R:%x G:%x B:%x  Alpha:%x\r\n",
					bmpinfo->bV5RedMask , bmpinfo->bV5GreenMask,
					bmpinfo->bV5BlueMask, bmpinfo->bV5AlphaMask);
		}
	}

	LocalUnlock(HBInfo);
	LocalUnlock(HBm);
}

void DumpGetPictureInfo(LPCSTR buf, LONG_PTR len, unsigned int flags, struct PictureInfo *lpInfo)
{
	ThreadCheck();
	printoutf("GetPictureInfo(0x%p, %d, %x, 0x%p)\r\n", buf, len, flags, lpInfo);
	DumpSource(buf, NULL, len, flags);
}
void DumpGetPictureInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flags, struct PictureInfo *lpInfo)
{
	if ( flags & DUMPSILENT ) return;
	ThreadCheck();
	printoutf("GetPictureInfoW(0x%p, %d, %x, 0x%p)\r\n", buf, len, flags, lpInfo);
	DumpSource(NULL, buf, len, flags);
}
#endif

#ifdef PLUGININFO_AM
void DumpDestination(LPCSTR destA, LPCWSTR destW, unsigned int flags)
{
	switch ( flags & SUSIE_DEST_MASK ){
		case SUSIE_DEST_DISK:
			printoutf("  destination is disk(");
			if ( destA != NULL ){
				printoutf("%s).\r\n", destA);
			}else{
				printoutf("%ls).\r\n", destW);
			}
			break;
		case SUSIE_DEST_MEM:
			printoutf("  destination is memory.\r\n");
			break;
		default:
			printoutf("  ● flags の destination 指定が未定義\r\n");
	}
}

void DumpGetArchiveInfo(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)
{
	ThreadCheck();
	printoutf("GetArchiveInfo(0x%p, %d, 0x%x, 0x%p)\r\n", buf, len, flag, lphInf);
	DumpSource(buf, NULL, len, flag);
}
void DumpGetArchiveInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *lphInf)
{
	ThreadCheck();
	printoutf("GetArchiveInfoW(0x%p, %d, 0x%x, 0x%p)\r\n", buf, len, flag, lphInf);
	DumpSource(NULL, buf, len, flag);
}

void ShowArchiveInfo(SUSIEINFOLIST &sil)
{
	printoutf("%d entries\r\n", sil.allocsize / sizeof(SUSIE_FINFOW));
}

void DumpGetFile(LPCSTR src, LONG_PTR len, LPSTR dest, unsigned int flags, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	ThreadCheck();
	printoutf("GetFile(0x%p, %d, 0x%p, %x, 0x%p, %d)\r\n", src, len, dest, flags, progressCallback, lData);
	DumpSource(src, NULL, len, flags);
	DumpDestination(dest, NULL, flags);
}
void DumpGetFileW(LPCWSTR src, LONG_PTR len, LPWSTR dest, unsigned int flags, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	if ( flags & DUMPSILENT ) return;

	ThreadCheck();
	printoutf("GetFileW(0x%p, %d, 0x%p, %x, 0x%p, %d)\r\n", src, len, dest, flags, progressCallback, lData);
	DumpSource(NULL, src, len, flags);
	DumpDestination(NULL, dest, flags);
}
void DumpGetFileInfo(LPCSTR buf, LONG_PTR len, LPCSTR filename, unsigned int flags, SUSIE_FINFO *lpInfo)
{
	ThreadCheck();
	printoutf("GetFileInfo(0x%p, %d, 0x%p, %x, 0x%p)\r\n", buf, len, filename, flags, lpInfo);
	DumpSource(buf, NULL, len, flags);
}
void DumpGetFileInfoW(LPCWSTR buf, LONG_PTR len, LPCWSTR filename, unsigned int flags, SUSIE_FINFOW *lpInfo)
{
	ThreadCheck();
	printoutf("GetFileInfoW(0x%p, %d, 0x%p, %x, 0x%p)\r\n", buf, len, filename, flags, lpInfo);
	DumpSource(NULL, buf, len, flags);
}
#endif
#endif
