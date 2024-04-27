name = ifextend
axname = axextend
# nmake を用いる場合、$.→$@、$&→*に書き換え

UseDebug = 0	# 1 なら debug 指定
!ifndef UseBCC
UseBCC = 1	# 1 なら BCC32 , 0 なら CL
!endif
UseTASM = 1	# 1 なら TASM使用
UseIlink = 0	# 1 なら ilink32(BCC 5.5以降) を使用
!ifndef ARM
ARM = 0		# 1 なら Arm/Arm64版(常にUNICODE版)
!endif
#=======================================
!ifndef X64
X64 = 0		# 1 なら x64版(常にUNICODE版)
!else
X64 = 1
!endif

!ifndef UseUnicode
UseUnicode = 0	# 1 なら UNICODE 版
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
#=============================================================== Borland 用設定
!if $(UseBCC)
.autodepend
!if $(UseDebug)
  DebugCopt	= -R -v -vG -DDEBUG	# 追加 C コンパイルオプション
  DebugLopt	= -v
  DebugLib	= CG32.LIB		# 追加ライブラリ
!endif
!if $(UseTASM)
 C0D	= TINYC0D
 C0DOBJ	= TINYC0D.OBJ
!else
 C0D	= $(C0DLL)
!endif
#-------------------------------------- コンパイラ共通指定
cc	= @BCC32 -4 -c -C -d -H -Hc -w -O -O1 -RT- -x- -DWINVER=0x400 $(bc56opt) $(DebugCopt) $(UnicodeCopt) $(ExCopt)
#-------------------------------------- リンカ共通指定
!if $(UseIlink)
 bc56opt = -Oc -q -a4
 linkopt = @ilink32 -c -m -P -V4.0 -Gn -q $(DebugLopt)
!else
 linkopt = @tlink32 -c -m -P -V4.0 $(DebugLopt)
!endif
  C0CON	= C0X32
  C0GUI	= C0W32
  C0DLL	= C0D32X
#-------------------------------------- リンカ(GUI)
linkexe	= $(linkopt) -Tpe -aa
#-------------------------------------- リンカ(CONSOLE)
linkcon	= $(linkopt) -Tpe -ap
#-------------------------------------- リンカ(DLL)
linkdll	= $(linkopt) -Tpd -aa

#-------------------------------------- インポート
implib  = @implib
#-------------------------------------- リソースコンパイラ
rc	= @Brc32 -r
#-------------------------------------- 暗黙の指定
.c.obj:
  $(cc) -o$@ $<

.cpp.obj:
  $(cc) -o$@ $<

.rc.res:
  $(rc) -fo$@ $<
#============================================================= Microsoft 用設定
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
# ↑サイズ違い警告を消す(※できるだけこの指定を使わないこと)
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

#-------------------------------------- コンパイラ
cc	= @"cl" /GF /nologo /c /DWINVER=0x400 $(Copt) $(UnicodeCopt) $(WineCopt) $(X64Copt) $(FORVIEW) /W3 /wd4068 /wd4996 $(ExCopt)
# 4068:不明な #pragma	4996:古い関数を使用した	/Wp64:64bit移植性チェック
#-------------------------------------- リンカ共通指定
linkopt = @"link" /NOLOGO /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF /NXCOMPAT /DYNAMICBASE

C0DLL	= Kernel32.Lib AdvAPI32.Lib Ole32.Lib Gdi32.Lib\
	  Shell32.Lib User32.Lib comctl32.lib Comdlg32.lib
C0GUI	= $(C0DLL)
C0CON	= $(C0DLL)
#-------------------------------------- リンカ(GUI)
linkexe	= $(linkopt) /SUBSYSTEM:WINDOWS
#-------------------------------------- リンカ(CONSOLE)
linkcon	= $(linkopt) /SUBSYSTEM:CONSOLE
#-------------------------------------- リンカ(DLL)
linkdll	= $(linkopt) /DLL
#-------------------------------------- リソースコンパイラ
rc	= @%COMSPEC% /C RC /dSDKRC $(RC64) $(UnicodeCopt)
#-------------------------------------- 暗黙の指定
.c.obj:
  $(cc) /Fo$@ $<

.cpp.obj:
  $(cc) /Fo$@ $<

.rc.res:
  $(rc) $<
!endif

#------------------------------------------------------------------------------
allFiles:	makefile  CODE$(NameTail)$(TAIL).OBJ  $(name)$(NameTail).$(spi)  $(axname)$(NameTail).$(spi)

#------------------------------------------------------ code体系切換用
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
