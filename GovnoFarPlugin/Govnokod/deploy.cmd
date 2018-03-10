set FROM=%1
set TO= "%APPDATA%\far manager\Profile\Plugins\Govnokod"
echo %FROM% " --> " %TO%
rmdir %TO%
mkdir %TO%
copy %FROM%\*.dll %TO%\*
