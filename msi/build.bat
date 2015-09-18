@echo off

SET DataDir=..\build\exe.win32-3.4

echo moving sleeptimer.exe out of build directory
move %DataDir%\sleeptimer.exe sleeptimer.exe || goto :error
echo.
echo creating data.wxs
"%WIX%bin\heat.exe" dir "%DataDir%" -nologo -cg DataFiles -var env.DataDir -gg -scom -sreg -sfrag -srd -dr data -out data.wxs || goto :error
echo.
echo candle
"%WIX%bin\candle.exe" main.wxs -nologo -ext WixUtilExtension || goto :error
echo.
echo candle
"%WIX%bin\candle.exe" data.wxs -nologo -ext WixUtilExtension || goto :error
echo.
echo creating Sleeptimer.msi
"%WIX%bin\light.exe" -out Sleeptimer.msi -nologo -ext WixUtilExtension -ext WixUIExtension main.wixobj data.wixobj || goto :error

goto finally

:error
echo Command failed!
pause

:finally
echo Success!
move sleeptimer.exe %DataDir%\sleeptimer.exe
