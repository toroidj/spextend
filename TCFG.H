/*-----------------------------------------------------------------------------
	��`�t�@�C������p�b�P�[�W	���k����						(c)TORO 2017
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		WCHAR	*bottom;			// ��������̃e�L�X�g�̐擪
	//	DWORD	size;				// �e�L�X�g�̑傫��(��������� MARGINE ���傫��)
	//	int		write;				// �ύX������ΐ^
	//	WCHAR	filename[MAX_PATH];	// ��`�t�@�C����(�������ݗp)
	} CFG;
	//======================================================================== �G�p
	extern void USEFASTCALL SkipSpaceB(const WCHAR *b, WCHAR *p);
	extern WCHAR GetLineParam(const WCHAR **str, WCHAR *param);
	extern BYTE USEFASTCALL SkipSpaceA(const char **str);
#ifdef UNICODE
	extern WCHAR USEFASTCALL SkipSpaceW(const WCHAR **str);
#define SkipSpace SkipSpaceW
#else
#define SkipSpace SkipSpaceA
#endif

//============================================================ ��`�t�@�C������
// filename �Ŏ�������`�t�@�C������������Ɏ捞�݁A����\�ɂ���
	extern BOOL CFGOpen(CFG *cfg, WCHAR *filename);

	// ��������̒�`�t�@�C�������B
	extern BOOL CFGClose(CFG *cfg);

	// �w��� offset ����A��s����ǂݍ��� string �ɏ������ށB
	// offset �ɂ͎��̍s�̐擪������
	extern BOOL USEFASTCALL CFGGetLine(CFG *cfg, DWORD *offset, WCHAR *string, int maxlen);

	// �p�����[�^�𒊏o����
	extern WCHAR *CFGFixParam(WCHAR *string, WCHAR **next);

#ifdef __cplusplus
}
#endif // __cplusplus