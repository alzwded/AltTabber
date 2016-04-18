@ECHO OFF

IF #%1 == # GOTO :fullRebuild
IF #%1 == #compile GOTO :compile
IF #%1 == #clean GOTO :clean
IF #%1 == #rebuild GOTO :fullRebuild
GOTO :options

:options
ECHO Usage: %0 ^(compile^|clean^|rebuild^)
EXIT /B 255

:fullRebuild
msbuild /property:Configuration=Release /property:Platform=x64 /target:Clean
msbuild /property:Configuration=Release /property:Platform=Win32 /target:Clean
GOTO :compile

:compile
msbuild /property:Configuration=Release /property:Platform=x64 && msbuild /property:Configuration=Release /property:Platform=Win32
GOTO :EOF

:clean
msbuild /property:Configuration=Release /property:Platform=x64 /target:Clean
msbuild /property:Configuration=Release /property:Platform=Win32 /target:Clean
GOTO :EOF
