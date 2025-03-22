@echo off
set RETAIL=1
rem *** set value ***
set arcname=spextend14.zip
set arcdevname=spextend_dev14.zip
set readme=spextend.txt
set srcname=spextend.7z

rem *** main ***
rem WINCLOSE xxx
del /q *.zip 2> NUL
del /q *.7z 2> NUL
del /q *.log 2> NUL

del /q *.obj 2> NUL
call m64 -DSUSIE_DEV -DRELEASE
call m32 -DSUSIE_DEV -DRELEASE
call marm64 -DSUSIE_DEV -DRELEASE
call m64 -DRELEASE
call m32 -DRELEASE
call marm64 -DRELEASE

for %%i in (*.exe) do tfilesign sp %%i %%i
for %%i in (*.sp*) do tfilesign sp %%i %%i

rem *** Source Archive ***
if %RETAIL%==0 goto :skipsource

for %%i in (*) do CT %readme% %%i
ppb /c %%u/7-zip32.dll,a %srcname% -hide -mx=9 makefile *.vcproj *.sln *.def *.bat *.c *.cpp *.h *.rc *.rh VS2017\*.sln VS2017\axextend\*.vcxproj VS2017\ifextend\*.vcxproj
tfilesign sf %srcname% %srcname%
CT %readme% %srcname%
:skipsource

for %%i in (*.exe) do CT %readme% %%i
for %%i in (*.spi*) do CT %readme% %%i
for %%i in (*.sph*) do CT %readme% %%i
for %%i in (*.cfg) do CT %readme% %%i

ppb /c %%u/7-ZIP32.DLL,a -tzip -hide -mx=7 %arcname% %readme% ??extend.sp* %srcname% ??extend.cfg
tfilesign s %arcname% %arcname%
CT %readme% %arcname%

ppb /c %%u/7-ZIP32.DLL,a -tzip -hide -mx=7 %arcdevname% spextend_dev.txt %readme% ??extend_dev.sp* %srcname% ??extend_dev.cfg sample\*
tfilesign s %arcdevname% %arcdevname%
CT %readme% %arcdevname%

del /q %srcname% 2> NUL
