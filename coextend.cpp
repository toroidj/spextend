/*----------------------------------------------------------------------------
	Extend convert Susie Plug-in	Copyright (c) TORO
----------------------------------------------------------------------------*/
#include "susie.h"

#define CopyrightSusie	"1.3+3 Copyright (c)2017-2024 TORO"

#define LINESIZE 0x800
#define InfoCount 2
const WCHAR NilStr[] = L"";
const char *DefaultExtNamesA = "*.*";
const char *InfoText[InfoCount] = {
	PLUGININFO_ID,
	"Extend " PLUGININFO_NAME " convert Plug-in  " CopyrightSusie,
};
const WCHAR *DefaultExtType = L"Extend convert";

typedef struct {
	DWORD extoffset;
	DWORD typeoffset;
	DWORD commandoffset;
} ExtendListTable;

typedef struct {
	SUSIE_FINFOW *first, *infos, *last;
	DWORD allocsize;
} SUSIEINFOLIST;

typedef struct {
	WCHAR name[MAX_PATH];
	DWORD commandoffset;
} TLSDATA; // CheckedFile = {L"", 0};

HINSTANCE hInst;
CFG config = {NULL}; // xxextend.cfg の内容
ExtendListTable *ExtendList = NULL; // 拡張子一覧
UINT ExtendCount = 0;
#define TLS_UNUSE (MAX32 - 1)
DWORD TlsIndex = TLS_UNUSE; // TlsAlloc の戻り値, MAX32=GlobalTLSDATAを使用
TLSDATA GlobalTLSDATA = {L"", 0};

#define ERROR_GETLASTERROR 0xfffefffe
void ErrorMessage(const WCHAR *text, const WCHAR *param, DWORD errcode)
{
	WCHAR textbuf[0x1000], *p;

	if ( errcode == ERROR_GETLASTERROR ) errcode = GetLastError();
	if ( param == NULL ) param = NilStr;

	p = textbuf + wsprintf(textbuf, L"%s(%d,%s)\n\n", text, errcode, param);

	if ( errcode != NO_ERROR ){
		FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
			p, 0x800, NULL);
	}
	MessageBox(NULL, textbuf, T(P_ProductName), MB_OK);
}

void MakeCfgFilename(WCHAR *filename)
{
	WCHAR orgname[MAX_PATH], *orgptr;
	WCHAR appdataname[MAX_PATH];
									// 取りあえずオリジナルファイル名を作成
	GetModuleFileNameW(hInst, orgname, MAX_PATH);
	wcscpy(wcsrchr(orgname, '.'), L".cfg");
	orgptr = wcsrchr(orgname, '\\');
									// AppData があればそれを使う
	appdataname[0] = '\0';
	ExpandEnvironmentStringsW(L"%APPDATA%", appdataname, MAX_PATH);
	wcscat(appdataname, L"\\TOROID");
	if ( GetFileAttributesW(appdataname) != 0xffffffff ){
		wcscat(appdataname, orgptr);
	} else{
		appdataname[0] = '\0';
	}

	if ( (appdataname[0] != '\0') && (GetFileAttributesW(appdataname) != 0xffffffff) ){
		wcscpy(filename, appdataname);
	} else{
		wcscpy(filename, orgname);
	}
}

void ConfigLineFix(DWORD offset)
{
	WCHAR *ptr;

	ptr = config.bottom + offset;
	for ( ;;){
		if ( *(ptr - 1) > ' ' ) break;
		ptr--;
	}
	*ptr = '\0';
}

void LoadConfig(void)
{
	WCHAR linebuf[LINESIZE];
	WCHAR CFGPath[MAX_PATH];	// 定義ファイル名
	DWORD offset;

	ExtendCount = 0;

	MakeCfgFilename(CFGPath);

	if ( CFGOpen(&config, CFGPath) == FALSE ){
		ErrorMessage(
			(LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE) ?
			L"定義ファイルが見つからないか、設定がありません" :
			L"Config file load error",
			CFGPath, NO_ERROR);
		return;
	}

	// 登録数計数
	offset = 0;
	for ( ;;){
		if ( CFGGetLine(&config, &offset, linebuf, TSIZEOF(linebuf)) == FALSE ){
			break;
		}
		if ( linebuf[0] == '\0' ) break;
		if ( linebuf[0] == '*' ) ExtendCount++;
	}
	if ( ExtendCount == 0 ) return;
	// 場所記憶
	ExtendList = static_cast<ExtendListTable *>(HeapAlloc(GetProcessHeap(), 0, sizeof(ExtendListTable) * ExtendCount));
	if ( ExtendList != NULL ){
		int count = 0, extoffset, typeoffset = 0;
		offset = 0;

		for ( ;;){
			extoffset = offset;
			if ( CFGGetLine(&config, &offset, linebuf, TSIZEOF(linebuf)) == FALSE ){
				break;
			}
			if ( linebuf[0] == '\0' ) break;
			if ( linebuf[0] == ':' ){
				typeoffset = extoffset + 1;
				ConfigLineFix(offset);
			}
			if ( linebuf[0] != '*' ) continue;

			ExtendList[count].extoffset = extoffset;
			ExtendList[count].typeoffset = typeoffset;
			ExtendList[count].commandoffset = offset;
						// extoffset 行末の余分な文字をスキップ
			ConfigLineFix(offset);
			count++;
			typeoffset = 0;
		}
	} else{
		ExtendCount = 0;
	}
}

void FreeConfig(void)
{
	if ( ExtendCount > 0 ){
		HeapFree(GetProcessHeap(), 0, ExtendList);
		ExtendCount = 0;
	}
	CFGClose(&config);
}

#include "cotest.cpp"

TLSDATA *GetTLSDATA(void)
{
	if ( TlsIndex < TLS_UNUSE ){
		return (TLSDATA *)TlsGetValue(TlsIndex);
	}
	if ( TlsIndex == MAX32 ) return &GlobalTLSDATA;
	return NULL; // TLS_UNUSE : 未使用
}

TLSDATA *ServeTLSDATA(void)
{
	TLSDATA *tls;

	if ( TlsIndex < TLS_UNUSE ){
		tls = (TLSDATA *)TlsGetValue(TlsIndex);
	}else{
		if ( TlsIndex == TLS_UNUSE ){ // 未割当てなので新規に用意
			TlsIndex = TlsAlloc();
		}
		if ( TlsIndex == MAX32 ) return &GlobalTLSDATA;
		tls = NULL;
	}
	if ( tls == NULL ){
		tls = (TLSDATA *)HeapAlloc(GetProcessHeap(), 0, sizeof(TLSDATA));
		if ( tls == NULL ){
			tls = &GlobalTLSDATA;
		}else{
			TlsSetValue(TlsIndex, tls);
			tls->name[0] = '\0';
		}
	}
	return tls;
}

void FreeTLSDATA(void)
{
	void *tls;

	if ( TlsIndex >= TLS_UNUSE ) return; // 未使用
	tls = TlsGetValue(TlsIndex);
	if ( tls != NULL ){
		HeapFree(GetProcessHeap(), 0, tls);
		TlsSetValue(TlsIndex, NULL); // 念のため
	}
}

#define FreeTLSindex() \
	if ( TlsIndex < TLS_UNUSE ){ \
		TlsFree(TlsIndex); \
		TlsIndex = TLS_UNUSE; \
	}

/*-----------------------------------------------------------------------------
	Plug-in interface
-----------------------------------------------------------------------------*/
int GetExtensionList(UINT infono, char *ExtA, WCHAR *ExtW, int buflen)
{
	UINT count;
	char *bufA = ExtA;
	WCHAR *bufW = ExtW;
	const WCHAR *strptr;

	if ( config.bottom == NULL ) LoadConfig();

	count = (infono - InfoCount) / 2;
	if ( count >= ExtendCount ) return 0;

	if ( infono & 1 ){ // type string
		if ( ExtendList[count].typeoffset != 0 ){
			strptr = config.bottom + ExtendList[count].typeoffset;
		} else{
			strptr = DefaultExtType;
		}
	} else{ // ext.list
		strptr = config.bottom + ExtendList[count].extoffset;
	}
	if ( bufW == NULL ){
		UnicodeToAnsi(strptr, bufA, buflen - 1);
		bufA[buflen - 1] = '\0';
		return strlen(bufA);
	} else{
		int lenW;

		lenW = wcslen(strptr);
		if ( buflen <= (lenW + 1) ){
			memcpy(bufW, strptr, buflen * sizeof(WCHAR));
			bufW[buflen - 1] = '\0';
		} else{
			wcscpy(bufW, strptr);
		}
		return lenW;
	}
}

extern "C" int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen)
{
	int len;
	const char *text;

	if ( buflen <= 0 ) return 0;
	if ( (UINT)infono >= InfoCount ){
		return DumpGetPluginInfo(GetExtensionList((UINT)infono, buf, NULL, buflen));
	}
	text = InfoText[infono];
	len = strlen(text);
	if ( len >= buflen ) len = buflen - 1;
	memcpy(buf, text, len);
	buf[len] = '\0';
	return DumpGetPluginInfo(len);
}

extern "C" int __stdcall GetPluginInfoW(int infono, LPWSTR buf, int buflen)
{
	char bufA[0x400];
	int len;

	len = GetPluginInfo(infono, bufA, sizeof(bufA));
	if ( len <= 0 ){
		if ( buflen > 0 ) buf[0] = '\0';
		return DumpGetPluginInfoW(infono, buf, buflen, 0);
	}
	len = AnsiToUnicode(bufA, buf, buflen);
	buf[buflen - 1] = '\0';
	return DumpGetPluginInfoW(infono, buf, buflen, len - 1);
}

//-----------------------------------------------------------------------------
int FindCommand(LPCWSTR filename, DWORD &offset)
{
	const WCHAR *fileext;
	UINT count = 0;

	if ( filename == NULL ) return 0;
	if ( config.bottom == NULL ) LoadConfig();

	fileext = wcsrchr(filename, '.');
	if ( fileext == NULL ) return 0;

	for ( ;; count++ ){
		const WCHAR *extptr, *fileextptr;

		if ( count >= ExtendCount ) {
			return 0;
		}
		extptr = config.bottom + ExtendList[count].extoffset;
		for ( ;;){
			extptr = wcschr(extptr, '.');
			if ( extptr == NULL ) break;
			if ( *(++extptr) == '*' ){
				offset = ExtendList[count].commandoffset;
				return 1; // ワイルドカード一致
			}
			fileextptr = fileext + 1;
			for ( ;;){
				WCHAR c;

				c = *extptr;
				if ( ((c == '\0') || (c == ';')) && (*fileextptr == '\0') ){
					offset = ExtendList[count].commandoffset;
					return 1; // 名前一致
				}
				if ( TinyCharUpperW(c) != TinyCharUpperW(*fileextptr++) ){
					break; // 不一致
				}
				extptr++;
			}
		}
	}
}

BOOL OpenExeFile(const WCHAR *commandline)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	BOOL CheckExitCode = FALSE;
	DWORD createflags = CREATE_DEFAULT_ERROR_MODE, ExitCode = 0;

	si.cb = sizeof(si);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = SW_SHOWNORMAL;
	si.cbReserved2 = 0;
	si.lpReserved2 = NULL;

	if ( *commandline == '&' ){
		commandline++;
		CheckExitCode = TRUE;
	}

	if ( *commandline == '>' ){
		commandline++;
		si.wShowWindow = SW_HIDE;
		setflag(createflags, DETACHED_PROCESS);
	}

	if ( FALSE == CreateProcess(NULL, (WCHAR *)commandline, NULL, NULL, FALSE,
		createflags, NULL, NULL, &si, &pi) ){
		ErrorMessage(
			(LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE) ?
			L"ファイルが実行できません" :
			L"File execute error",
			commandline, ERROR_GETLASTERROR);
		return FALSE;
	}

	CloseHandle(pi.hThread);
	WaitForInputIdle(pi.hProcess, INFINITE);
	WaitForSingleObject(pi.hProcess, 120 * 1000);
	GetExitCodeProcess(pi.hProcess, &ExitCode);
	CloseHandle(pi.hProcess);
	return CheckExitCode ? (ExitCode == 0) : TRUE;
}

void tstrreplace(TCHAR *text, const TCHAR *targetword, const TCHAR *replaceword)
{
	TCHAR *p;

	while ( (p = tstrstr(text, targetword)) != NULL ){
		int tlen = tstrlen(targetword);
		int rlen = tstrlen(replaceword);

		if ( tlen != rlen ) memmove(p + rlen, p + tlen, TSTRSIZE(p + tlen));
		memcpy(p, replaceword, TSTROFF(rlen));
		text = p + rlen;
	}
}

#pragma argsused
extern "C" int __stdcall IsSupportedW(LPCWSTR filename, void *dw)
{
	TLSDATA *tls;
	DWORD offset;

	if ( 0 == DumpIsSupportedW(FindCommand(filename, offset)) ){
		// 該当しない→キャッシュを廃棄
		tls = GetTLSDATA();
		if ( tls != NULL ) tls->name[0] = '\0';
		return 0;
	}else{ // ファイル名を記憶
		if ( wcslen(filename) < MAX_PATH ){
			tls = ServeTLSDATA();
			wcscpy(tls->name, filename);
			tls->commandoffset = offset;
		}
		return 1;
	}
}

#pragma argsused
extern "C" int __stdcall IsSupported(LPCSTR filename, void *dw)
{
	TLSDATA *tls;
	DWORD offset;
	WCHAR nameW[MAX_PATH];

	if ( filename == NULL ){
		return 0;
	} else{
		if ( AnsiToUnicode(filename, nameW, MAX_PATH) == 0 ) return 0;
		nameW[MAX_PATH - 1] = '\0';
	}

	if ( 0 == DumpIsSupported(FindCommand(nameW, offset)) ){
		// 該当しない→キャッシュを廃棄
		tls = GetTLSDATA();
		if ( tls != NULL ) tls->name[0] = '\0';
		return 0;
	}else{ // ファイル名を記憶
		tls = ServeTLSDATA();
		wcscpy(tls->name, nameW);
		tls->commandoffset = offset;
		return 1;
	}
}

void WriteConsoleLF(HANDLE hStdOut, const WCHAR *text)
{
	DWORD size;

	WriteConsole(hStdOut, text, wcslen(text), &size, NULL);
	WriteConsole(hStdOut, L"\r\n", 2, &size, NULL);
}

BOOL ExecuteConfig(DWORD &offset, const WCHAR *srcpath, const WCHAR *dstpath)
{
	WCHAR CmdLine[0x8000];
#define usecon_no 0
#define usecon_alloc 1
#define usecon_used 2
	int UseConsole = usecon_no;
	#ifndef SUSIE_DEV
		BOOL UseLog = FALSE;
	#endif
	HANDLE hStdOut = NULL;

	for ( ;;){
		if ( CFGGetLine(&config, &offset, CmdLine, TSIZEOF(CmdLine)) == FALSE ){
			break;
		}
		if ( (CmdLine[0] == '\0') || (CmdLine[0] == '*') ) break;
		if ( (CmdLine[0] == ';') || (CmdLine[0] == ':') ) continue;
		tstrreplace(CmdLine, L"%src%", srcpath);
		tstrreplace(CmdLine, L"%dst%", dstpath);
		switch ( CmdLine[0] ){
			case '?':
				if ( UseConsole == usecon_no ){
					::MessageBox(NULL, CmdLine, T(P_ProductName), MB_OK);
				} else{
					WriteConsoleLF(hStdOut, CmdLine);
				}
				break;

			case '!':
				switch ( CmdLine[1] ){
					case 'c': // !console
						if ( UseConsole != usecon_no ) break;
						if ( AllocConsole() ){
							UseConsole = usecon_alloc;
							SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);
						} else{
							UseConsole = usecon_used;
						}
						hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
						break;

					case 'l': // !log
						#ifndef SUSIE_DEV
							UseLog = TRUE;
						#endif
						break;

					case 'p': // !pause
						if ( UseConsole == usecon_no ){
							::MessageBox(NULL, T("pause"), T(P_ProductName), MB_OK);
						} else{
							WriteFile(hStdOut, "\r\nHit enter.\r\n", 14, (DWORD *)CmdLine, NULL);
							ReadFile(GetStdHandle(STD_INPUT_HANDLE), CmdLine, 4, (DWORD *)CmdLine, NULL);
						}
						break;
				}
				break;

			default:
				#ifdef SUSIE_DEV
					printoutf("%ls\r\n", CmdLine);
				#else
					if ( UseLog ){
						if ( UseConsole == usecon_no ){
							::MessageBox(NULL, CmdLine, T(P_ProductName), MB_OK);
						} else{
							WriteConsoleLF(hStdOut, CmdLine);
						}
					}
				#endif
				if ( OpenExeFile(CmdLine) == FALSE ){
					return FALSE;
				}
		}
	}
	if ( UseConsole == usecon_alloc ) FreeConsole();
	return TRUE;
}

extern "C" int __stdcall ConfigurationDlg(HWND hWnd, int function)
{
	if ( function != 1 ){
		DumpConfigurationDlg(-1);
		return -1;
	}

	if ( ::MessageBox(hWnd,
		(LOWORD(GetUserDefaultLCID()) == LCID_JAPANESE) ?
		L"定義ファイルを読み込み直しますか？" :
		L"Reload config file ?",
		T(P_ProductName), MB_YESNO | MB_DEFBUTTON2) == IDYES ){
		FreeConfig();
	}
	DumpConfigurationDlg(0);
	return 0;
}
