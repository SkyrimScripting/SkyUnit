# SkyUnit

> SkyUnit is a Unit Testing Framework for Skyrim Papyrus Scripts

![SkyUnit](Images/Logo.jpg)

[![Download SkyUnit](https://github.com/SkyrimScripting/SkyrimScripting/raw/main/Resources/DownloadButton.png)](https://github.com/SkyrimScripting/SkyUnit/releases/download/v1/SkyUnit.7z)

# `v1`

The goal for `v1` of `SkyUnit` is to be _as minimal as possible_:

- Any script with a name ending in "`UnitTest`" is automatically run
- Only `global` functions are supported
    - Functions with names starting with `Test` are each run as test cases
- Test results are written to `SkyUnit.TestResults.log`
    - A summary of all test results is printed at the very end
- `SkyUnit.Assert(bool, [string message])` is the only assertion provided
    - On a failure, the Papyrus source code file name and line number are provided
- The game automatically exits after all unit tests have been run
- None of this is configurable
