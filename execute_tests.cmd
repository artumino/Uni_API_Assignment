@echo off
for /r %%i in (tests\*.in) do main < %%i > taken\%%~ni.out
pause
