# TODO: Should use an actual build system instead of bespoke script

try {
    Push-Location $PSScriptRoot

    try {
        Push-Location ./build

        # NOTE! /EHsc is included to silence warning C4530: 
        #       C++ exception handler used, but unwind semantics are not enabled. 
        #       Specify /EHsc
        cl /std:c++latest /EHsc ../src/*.cpp /link /out:pplox.exe
        if ($LASTEXITCODE -ne 0) {
            Write-Host ""
            Write-Host "Non-zero exit code from cl: $LASTEXITCODE"
            # Return from this script
            return
        }
    }
    finally {
        Pop-Location
    }

    Write-Host ""
    ./build/pplox ./src/test_file.lox
}
finally {
    Pop-Location
}