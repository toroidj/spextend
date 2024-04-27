/*-----------------------------------------------------------------------------
	定義ファイル操作パッケージ	※縮小版						(c)TORO 2017
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		WCHAR	*bottom;			// メモリ上のテキストの先頭
	//	DWORD	size;				// テキストの大きさ(メモリ上は MARGINE 分大きい)
	//	int		write;				// 変更があれば真
	//	WCHAR	filename[MAX_PATH];	// 定義ファイル名(書き込み用)
	} CFG;
	//======================================================================== 雑用
	extern void USEFASTCALL SkipSpaceB(const WCHAR *b, WCHAR *p);
	extern WCHAR GetLineParam(const WCHAR **str, WCHAR *param);
	extern BYTE USEFASTCALL SkipSpaceA(const char **str);
#ifdef UNICODE
	extern WCHAR USEFASTCALL SkipSpaceW(const WCHAR **str);
#define SkipSpace SkipSpaceW
#else
#define SkipSpace SkipSpaceA
#endif

//============================================================ 定義ファイル操作
// filename で示した定義ファイルをメモリ上に取込み、操作可能にする
	extern BOOL CFGOpen(CFG *cfg, WCHAR *filename);

	// メモリ上の定義ファイルを閉じる。
	extern BOOL CFGClose(CFG *cfg);

	// 指定の offset から、一行分を読み込み string に書き込む。
	// offset には次の行の先頭を書く
	extern BOOL USEFASTCALL CFGGetLine(CFG *cfg, DWORD *offset, WCHAR *string, int maxlen);

	// パラメータを抽出する
	extern WCHAR *CFGFixParam(WCHAR *string, WCHAR **next);

#ifdef __cplusplus
}
#endif // __cplusplus
