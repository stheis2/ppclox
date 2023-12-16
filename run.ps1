# TODO: Should use an actual build system instead of bespoke script

try {
    Push-Location $PSScriptRoot

    try {
        Push-Location ./build

        # TODO: Have Powershell just get all the .cpp files and feed them
        # to cl, instead of listing them individually

        # NOTE! /EHsc is included to silence warning C4530: 
        #       C++ exception handler used, but unwind semantics are not enabled. 
        #       Specify /EHsc
        cl /std:c++20 /EHsc ../src/*.cpp /link /out:pplox.exe

        Write-Host ""
        ./pplox
    }
    finally {
        Pop-Location
    }
}
finally {
    Pop-Location
}