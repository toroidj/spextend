/*----------------------------------------------------------------------------
	Extend Images convert Susie Plug-in	Copyright (c) TORO
----------------------------------------------------------------------------*/
#define STRICT
#define UNICODE
#include <windows.h>
#include "TOROWIN.H"
#include "TCFG.H"
#pragma hdrstop

#define P_ProductName "ifextend"
#define PLUGININFO_IN
#define PLUGININFO_ID "00IN"
#define PLUGININFO_NAME "Images"
#define PLUGIN_IN

#include "coextend.cpp"

int EnableGetPictureInfo = -1;

#define margin 64
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
		FreeTLSDATA();
		FreeTLSindex();
		FreeConfig();
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
DWORD CalcBmpHeaderSize(BITMAPINFOHEADER *dumpdata)
{
	DWORD offset, color, palette;

	offset = dumpdata->biSize;
	if ( offset < sizeof(BITMAPINFOHEADER) ){
												// OS/2 形式
		color = ((BITMAPCOREHEADER *)dumpdata)->bcBitCount;
		if ( color <= 8 ) offset += (DWORD)(1 << color) * sizeof(RGBTRIPLE);
	} else{								// WINDOWS 形式
		color = dumpdata->biBitCount;
		palette = dumpdata->biClrUsed;
		if ( (offset < (sizeof(BITMAPINFOHEADER) + 12)) && (dumpdata->biCompression == BI_BITFIELDS) ){
			offset += 12;	// 16/32bit のときはビット割り当てがある
		}

		palette = palette ? palette * sizeof(RGBQUAD) :
			((color <= 8) ? (DWORD)(1 << color) * sizeof(RGBQUAD) : 0);

		if ( offset == 0x7c/*BITMAPV5HEADER*/ ){
			DWORD ProfileData = *(DWORD *)((BYTE *)dumpdata + 0x70); //bV5ProfileData
			if ( (offset + palette) == ProfileData ){
				offset += *(DWORD *)((BYTE *)dumpdata + 0x74); //bV5ProfileSize
			}
		}
		offset += palette;
	}
	return offset;
}

//-----------------------------------------------------------------------------
extern "C" int __stdcall GetPictureW(LPCWSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	const WCHAR *filename;
	WCHAR TempDir[MAX_PATH], TempBMPfile[MAX_PATH], TempORGfile[MAX_PATH];
	DWORD sizeL, sizeH;	// イメージの大きさ
	char *pMemImage, *pHeapImage;
	DWORD offset, OffBits;

	BITMAPINFOHEADER *bih, *bmpinfo;
	BITMAPFILEHEADER *bif;
	DWORD ProfileData = 0;
	DWORD ProfileSize;
	DWORD bfhsize;
	BYTE *bits;
	int anum, anumtest = 100;
	int SusieResult = SUSIEERROR_BROKENDATA;

	DumpGetPictureW(buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
	if ( progressCallback != NULL ) progressCallback(0, 100, lData);

	TempORGfile[0] = '\0';
	switch ( flags & SUSIE_SOURCE_MASK ){
		case SUSIE_SOURCE_DISK: // filename
			if ( FindCommand(buf, offset) == 0 ) return SUSIERESULT(SUSIEERROR_UNKNOWNFORMAT);
			filename = buf;
			break;

		case SUSIE_SOURCE_MEM: { // memory  IsSupported で記憶したファイル名を利用する
			TLSDATA *tls;

			tls = GetTLSDATA();
			if ( (tls == NULL) || (tls->name[0] == '\0') ){
				return SUSIERESULT(SUSIEERROR_NOTSUPPORT);
			}
			filename = tls->name;
			offset = tls->commandoffset;

			// 記憶したファイル名が実在しているならそのまま使用
			if ( !(GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY) ){
				break;
			}

			{ // 実在していないので一時ファイルを用意する
				WCHAR *ext;
				HANDLE hFile;
				DWORD tempsize = 0;

				GetTempPath(MAX_PATH, TempDir);
				ext = wcsrchr(tls->name, '.');
				if ( ext == NULL ) return SUSIERESULT(SUSIEERROR_UNKNOWNFORMAT);

				anum = (len + 1) & 0x7ff;
				for ( ;;){
					wsprintf(TempORGfile, L"%s\\ifex%d%s", TempDir, anum, ext);
					if ( GetFileAttributes(TempORGfile) == BADATTR ) break;
					anum = (anum + (len / 2) + 3) & 0x7ff;
					if ( --anumtest == 0 ) break;
				}
				hFile = CreateFile(TempORGfile, GENERIC_WRITE, 0, NULL,
						CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				if ( hFile == INVALID_HANDLE_VALUE ){
					return SUSIERESULT(SUSIEERROR_FAULTREAD);
				}
				WriteFile(hFile, buf, len, &tempsize, NULL);
				CloseHandle(hFile);
				if ( len != tempsize ){
					DeleteFile(TempORGfile);
					return SUSIERESULT(SUSIEERROR_FAULTREAD);
				}
				filename = TempORGfile;
			}
			break;
		}

		default:
			return SUSIERESULT(SUSIEERROR_NOTSUPPORT);
	}

	if ( GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY ){
		return SUSIERESULT(SUSIEERROR_FAULTREAD); // エラー or dir
	}

	if ( progressCallback != NULL ) progressCallback(10, 100, lData);
	GetTempPath(MAX_PATH, TempDir);
	anum = (len + 1) & 0x7ff;
	for ( ;;){
		wsprintf(TempBMPfile, L"%s\\ifex%d.bmp", TempDir, anum);
		if ( GetFileAttributes(TempBMPfile) == BADATTR ) break;
		anum = (anum + (len / 2) + 3) & 0x7ff;
		if ( --anumtest == 0 ) break;
	}

	// 変換
	if ( ExecuteConfig(offset, filename, TempBMPfile) == FALSE ){
		if ( TempORGfile[0] != '\0' ) DeleteFile(TempORGfile);
		return SUSIERESULT(SUSIEERROR_BROKENDATA);
	}

	if ( progressCallback != NULL ) progressCallback(50, 100, lData);
	if ( TempORGfile[0] != '\0' ) DeleteFile(TempORGfile);

	{ // ファイル読み込み
		HANDLE hFile;
		BOOL result;
										// ファイルを開く ---------------------
		hFile = CreateFileW(TempBMPfile, GENERIC_READ,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if ( hFile == INVALID_HANDLE_VALUE ){
			ErrorMessage(L"変換したファイルを開けません。", TempBMPfile, ERROR_GETLASTERROR);
			return SUSIERESULT(SUSIEERROR_FAULTREAD);
		}
										// ファイルサイズの確認 ---------------
		sizeL = GetFileSize(hFile, &sizeH);
		if ( sizeH ) sizeL = 0xffffffff;
										// 読み込み準備 -----------------------
		pMemImage = pHeapImage = (char *)HeapAlloc(GetProcessHeap(), 0, sizeL + margin);
		if ( pHeapImage == NULL ){
			CloseHandle(hFile);
			return SUSIERESULT(SUSIEERROR_EMPTYMEMORY);
		}
										// 読み込み & 0 padding ---------------
		result = ReadFile(hFile, pHeapImage, sizeL, &sizeL, NULL);
		CloseHandle(hFile);
		#ifdef SUSIE_DEV
			printoutf("  Bitmap file: %ls\r\n", TempBMPfile);
		#else
			DeleteFile(TempBMPfile);
		#endif
		if ( result == FALSE ){
			SusieResult = SUSIEERROR_FAULTREAD;
			goto error_free;
		}
		memset(pHeapImage + sizeL, 0, margin);
	}
	if ( progressCallback != NULL ) progressCallback(75, 100, lData);

	if ( sizeL < 0x30 ) goto error_free;
	bif = (BITMAPFILEHEADER *)pMemImage;
	if ( bif->bfType != ('B' + ('M' << 8)) ) goto error_free;

	// BITMAPINFOHEADER 関連のメモリブロックを作成する ----
	bih = (BITMAPINFOHEADER *)(pMemImage + sizeof(BITMAPFILEHEADER));
	if ( bih->biSize == 0x7c /*BITMAPV5HEADER*/ ){
		ProfileData = *(DWORD *)((BYTE *)bih + 0x70); //bV5ProfileData
	}
	bfhsize = CalcBmpHeaderSize(bih);

	OffBits = bif->bfOffBits;
	if ( OffBits == 0 ) OffBits = sizeof(BITMAPFILEHEADER) + bfhsize;

	if ( ProfileData <= bfhsize ){ // ICC profile がない一般BMP file
		ProfileSize = 0;
		bmpinfo = static_cast<BITMAPINFOHEADER *>(*pHBInfo = LocalAlloc(LPTR, OffBits - sizeof(BITMAPFILEHEADER)));
		if ( bmpinfo == NULL ) goto error_free;
		memcpy((char *)bmpinfo, bih, OffBits - sizeof(BITMAPFILEHEADER));
	} else{	// ICC profile がビットマップの後ろにあるので
			// bmpinfo の後ろに移動させる
		ProfileSize = *(DWORD *)((BYTE *)bih + 0x74); //bV5ProfileSize
		bmpinfo = static_cast<BITMAPINFOHEADER *>(*pHBInfo = LocalAlloc(LPTR, OffBits - sizeof(BITMAPFILEHEADER) + ProfileSize));
		if ( bmpinfo == NULL ) goto error_free;
		memcpy((char *)bmpinfo + bfhsize, (BYTE *)bih + ProfileData, ProfileSize);
		memcpy((char *)bmpinfo, bih, bfhsize);
		*(DWORD *)((BYTE *)bmpinfo + 0x70) /*bV5ProfileData */ = bfhsize;
	}

	// ビットマップのメモリブロックを作成する ----
	bits = static_cast<LPBYTE>(*pHBm = LocalAlloc(LPTR, sizeL - OffBits));
	if ( bits == NULL ) goto error_free;
	memcpy(bits, pMemImage + OffBits, sizeL - OffBits - ProfileSize);

	ShowPictureInfo(*pHBInfo, *pHBm);

	SusieResult = SUSIEERROR_NOERROR;
	if ( progressCallback != NULL ) progressCallback(100, 100, lData);
error_free:
	if ( SusieResult != SUSIEERROR_NOERROR ){
		if ( *pHBm != NULL ) LocalFree(*pHBm);
		if ( *pHBInfo != NULL ) LocalFree(*pHBInfo);
	}
	if ( pHeapImage != NULL ) HeapFree(GetProcessHeap(), 0, pHeapImage);
	return SUSIERESULT(SusieResult);
}

extern "C" int __stdcall GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	DumpGetPicture(buf, len, flag, pHBInfo, pHBm, progressCallback, lData);
	if ( (flag & 7) == 0 ){ // ファイル名指定
		WCHAR filenameW[MAX_PATH];

		filenameW[0] = '\0';
		AnsiToUnicode(buf, filenameW, MAX_PATH);
		filenameW[MAX_PATH - 1] = '\0';
		return GetPictureW(filenameW, len, flag DUMPFLAG, pHBInfo, pHBm, progressCallback, lData);
	} else{
		return GetPictureW((LPCWSTR)buf, len, flag DUMPFLAG, pHBInfo, pHBm, progressCallback, lData);
	}
}

//-----------------------------------------------------------------------------
#pragma argsused
extern "C" int __stdcall GetPreview(LPCSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	return DumpGetPreview(buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
}

#pragma argsused
extern "C" int __stdcall GetPreviewW(LPCWSTR buf, LONG_PTR len, unsigned int flags, HLOCAL *pHBInfo, HLOCAL *pHBm, SUSIE_PROGRESS progressCallback, LONG_PTR lData)
{
	return DumpGetPreviewW(buf, len, flags, pHBInfo, pHBm, progressCallback, lData);
}

#pragma argsused
extern "C" int __stdcall GetPictureInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flags, struct PictureInfo *lpInfo)
{
	int SusieResult;
	HLOCAL pHBInfo, pHBm;

	DumpGetPictureInfoW(buf, len, flags, lpInfo);
	if ( EnableGetPictureInfo < 0 ){
		if ( config.bottom == NULL ) LoadConfig();
		if ( config.bottom != NULL ){
			EnableGetPictureInfo = wcsstr(config.bottom, L"\nEnableGetPictureInfo") != NULL;
		}
	}

	if ( EnableGetPictureInfo == 0 ){
		if ( flags == 0 ) return SUSIERESULT(SUSIEERROR_NOTSUPPORT);

		// ダミー情報を返す
		lpInfo->left = lpInfo->top = 0;
		lpInfo->width = lpInfo->height = 100;
		lpInfo->x_density = lpInfo->y_density = 96;
		lpInfo->colorDepth = 24;
		lpInfo->hInfo = NULL;
		return SUSIERESULT(SUSIEERROR_NOERROR);
	}

	SusieResult = GetPictureW(buf, len, flags, &pHBInfo, &pHBm, NULL, 0);
	if ( SusieResult != SUSIEERROR_NOERROR ) return SUSIERESULT(SusieResult);

	BITMAPINFOHEADER *bmpinfo = static_cast<BITMAPINFOHEADER *>(pHBInfo);
	lpInfo->left = lpInfo->top = 0;
	lpInfo->width = bmpinfo->biWidth;
	lpInfo->height = bmpinfo->biHeight;
	lpInfo->x_density = static_cast<WORD>(bmpinfo->biXPelsPerMeter / 39);
	lpInfo->y_density = static_cast<WORD>(bmpinfo->biYPelsPerMeter / 39);
	lpInfo->colorDepth = bmpinfo->biBitCount;
	lpInfo->hInfo = NULL;
	LocalFree(pHBm);
	LocalFree(pHBInfo);
	return SUSIERESULT(SUSIEERROR_NOERROR);
}

#pragma argsused
extern "C" int __stdcall GetPictureInfo(LPCSTR buf, LONG_PTR len, unsigned int flags, struct PictureInfo *lpInfo)
{
	DumpGetPictureInfo(buf, len, flags, lpInfo);
	if ( (flags & SUSIE_SOURCE_MASK) == SUSIE_SOURCE_DISK ){ // ファイル名指定
		WCHAR filenameW[MAX_PATH];

		filenameW[0] = '\0';
		AnsiToUnicode(buf, filenameW, MAX_PATH);
		filenameW[MAX_PATH - 1] = '\0';
		return GetPictureInfoW(filenameW, len, flags DUMPFLAG, lpInfo);
	} else{
		return GetPictureInfoW((LPCWSTR)buf, len, flags DUMPFLAG, lpInfo);
	}
}
