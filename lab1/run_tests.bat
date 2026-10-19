@echo off
setlocal

if not exist a.exe (
  echo ERROR: a.exe not found in current folder
  exit /b 2
)

python --version >nul 2>&1
if errorlevel 1 (
  echo ERROR: Python not found. Install Python 3.x and ensure it's in PATH.
  exit /b 2
)

python run_tests.py --exe a.exe %*
exit /b %ERRORLEVEL%
