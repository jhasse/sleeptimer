SET DataDir=..\build\exe.win32-3.4
move %DataDir%\sleeptimer.exe sleeptimer.exe
"%WIX%bin\heat.exe" dir "%DataDir%" -cg DataFiles -var env.DataDir -gg -scom -sreg -sfrag -srd -dr data -out data.wxs || goto :error
"%WIX%bin\candle.exe" main.wxs -ext WixUtilExtension || goto :error
"%WIX%bin\candle.exe" data.wxs -ext WixUtilExtension || goto :error
"%WIX%bin\light.exe" -out Sleeptimer.msi -ext WixUtilExtension -ext WixUIExtension main.wixobj data.wixobj || goto :error

:error
move sleeptimer.exe %DataDir%\sleeptimer.exe
