name = ifextend
axname = axextend
# nmake ��p����ꍇ�A$.��$@�A$&��*�ɏ�������

UseDebug = 0	# 1 �Ȃ� debug �w��
!ifndef UseBCC
UseBCC = 1	# 1 �Ȃ� BCC32 , 0 �Ȃ� CL
!endif
UseTASM = 1	# 1 �Ȃ� TASM�g�p
UseIlink = 0	# 1 �Ȃ� ilink32(BCC 5.5�ȍ~) ���g�p
!ifndef ARM
ARM = 0		# 1 �Ȃ� Arm/Arm64��(���UNICODE��)
!endif
#=======================================
!ifndef X64
X64 = 0		# 1 �Ȃ� x64��(���UNICODE��)
!else
X64 = 1
!endif

!ifndef UseUnicode
UseUnicode = 0	# 1 �Ȃ� UNICODE ��
!else
  UnicodeCopt	= -DUNICODE
!endif

!ifdef RELEASE
TAIL = R
UseDebug = 0
Copt = /DRELEASE
!endif

!if $(X64)
UseBCC	= 0
UseTASM = 0
#UseUnicode	= 1
TAIL	= $(TAIL)64
spi	= sph
!else
SSEOPT = /arch:SSE2
spi	= spi
!endif

!if $(ARM)
UseUnicode	= 0
TAIL	= $(TAIL)A
spi	= $(spi)a
!endif

!if $(UseUnicode)
  UnicodeCopt	= -DUNICODE
!endif

!ifdef SUSIE_DEV
  NameTail = _dev
  ExCopt = -DSUSIE_DEV
!endif
#=============================================================== Borland �p�ݒ�
!if $(UseBCC)
.autodepend
!if $(UseDebug)
  DebugCopt	= -R -v -vG -DDEBUG	# �ǉ� C �R���p�C���I�v�V����
  DebugLopt	= -v
  DebugLib	= CG32.LIB		# �ǉ����C�u����
!endif
!if $(UseTASM)
 C0D	= TINYC0D
 C0DOBJ	= TINYC0D.OBJ
!else
 C0D	= $(C0DLL)
!endif
#-------------------------------------- �R���p�C�����ʎw��
cc	= @BCC32 -4 -c -C -d -H -Hc -w -O -O1 -RT- -x- -DWINVER=0x400 $(bc56opt) $(DebugCopt) $(UnicodeCopt) $(ExCopt)
#-------------------------------------- �����J���ʎw��
!if $(UseIlink)
 bc56opt = -Oc -q -a4
 linkopt = @ilink32 -c -m -P -V4.0 -Gn -q $(DebugLopt)
!else
 linkopt = @tlink32 -c -m -P -V4.0 $(DebugLopt)
!endif
  C0CON	= C0X32
  C0GUI	= C0W32
  C0DLL	= C0D32X
#-------------------------------------- �����J(GUI)
linkexe	= $(linkopt) -Tpe -aa
#-------------------------------------- �����J(CONSOLE)
linkcon	= $(linkopt) -Tpe -ap
#-------------------------------------- �����J(DLL)
linkdll	= $(linkopt) -Tpd -aa

#-------------------------------------- �C���|�[�g
implib  = @implib
#-------------------------------------- ���\�[�X�R���p�C��
rc	= @Brc32 -r
#-------------------------------------- �Öق̎w��
.c.obj:
  $(cc) -o$@ $<

.cpp.obj:
  $(cc) -o$@ $<

.rc.res:
  $(rc) -fo$@ $<
#============================================================= Microsoft �p�ݒ�
!else
!if $(UseDebug)
Copt	= /Od /Zi /GS #/analyze
DebugLopt	= /DEBUG
!else # UseDebug
!ifdef RELEASE
Copt	= /Gz /O2 /Os /Oy /DRELEASE
!else # RELEASE
Copt	= /Gz /O2 /Os /Oy
!endif # RELEASE
!endif # UseDebug

!if $(X64) # x64, ARM64
X64Copt	= /wd4244 /wd4267
# ���T�C�Y�Ⴂ�x��������(���ł��邾�����̎w����g��Ȃ�����)
!if !$(ARM) # x64
LINK64	= /MACHINE:AMD64 /LARGEADDRESSAWARE
RC64	= /d_WIN64
!else # ARM64
LINK64	= /MACHINE:ARM64 /LARGEADDRESSAWARE
Copt	= /O2 /Os /Oy /DRELEASE
X64Copt	= /wd4244 /wd4267
RC64	= /d_WIN64 /d_ARM64_
!endif # ARM64
!else # !$(X64), x86, ARM32
!if $(ARM) # ARM32
LINK64	= /MACHINE:ARM /LARGEADDRESSAWARE
!endif # ARM32
!endif # X64

#-------------------------------------- �R���p�C��
cc	= @"cl" /GF /nologo /c /DWINVER=0x400 $(Copt) $(UnicodeCopt) $(WineCopt) $(X64Copt) $(FORVIEW) /W3 /wd4068 /wd4996 $(ExCopt)
# 4068:�s���� #pragma	4996:�Â��֐����g�p����	/Wp64:64bit�ڐA���`�F�b�N
#-------------------------------------- �����J���ʎw��
linkopt = @"link" /NOLOGO /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF /NXCOMPAT /DYNAMICBASE

C0DLL	= Kernel32.Lib AdvAPI32.Lib Ole32.Lib Gdi32.Lib\
	  Shell32.Lib User32.Lib comctl32.lib Comdlg32.lib
C0GUI	= $(C0DLL)
C0CON	= $(C0DLL)
#-------------------------------------- �����J(GUI)
linkexe	= $(linkopt) /SUBSYSTEM:WINDOWS
#-------------------------------------- �����J(CONSOLE)
linkcon	= $(linkopt) /SUBSYSTEM:CONSOLE
#-------------------------------------- �����J(DLL)
linkdll	= $(linkopt) /DLL
#-------------------------------------- ���\�[�X�R���p�C��
rc	= @%COMSPEC% /C RC /dSDKRC $(RC64) $(UnicodeCopt)
#-------------------------------------- �Öق̎w��
.c.obj:
  $(cc) /Fo$@ $<

.cpp.obj:
  $(cc) /Fo$@ $<

.rc.res:
  $(rc) $<
!endif

#------------------------------------------------------------------------------
allFiles:	makefile  CODE$(NameTail)$(TAIL).OBJ  $(name)$(NameTail).$(spi)  $(axname)$(NameTail).$(spi)

#------------------------------------------------------ code�̌n�؊��p
CODE$(NameTail)$(TAIL).OBJ:
  -@del *.obj 2> nul
  -@del *.res 2> nul
  -@if exist %CSM% del %CSM%\*.CSM
  @copy nul CODE$(NameTail)$(TAIL).OBJ > nul

#------------------------------------------------------------------------------
$(name)$(NameTail).$(spi):	$(name).obj TCFG.OBJ
!if $(UseBCC)
  $(linkdll) $(C0DLL) $(name).obj TCFG.OBJ,$<,$*,NOEH32 IMPORT32 CW32,susieif
!else
  $(linkdll) $(C0DLL) /DEF:susieif.def $(name).obj TCFG.OBJ /OUT:$(name)$(NameTail).$(spi)
!endif

$(name).obj: $(name).cpp coextend.cpp cotest.cpp
TCFG.OBJ: TCFG.C

#------------------------------------------------------------------------------
$(axname)$(NameTail).$(spi):	$(axname).obj TCFG.OBJ
!if $(UseBCC)
  $(linkdll) $(C0DLL) $(axname).obj TCFG.OBJ,$<,$*,NOEH32 IMPORT32 CW32,susieax
!else
  $(linkdll) $(C0DLL) /DEF:susieax.def $(axname).obj TCFG.OBJ /OUT:$(axname)$(NameTail).$(spi)
!endif

$(axname).obj: $(axname).cpp coextend.cpp cotest.cpp
