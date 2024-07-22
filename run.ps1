try {
    Push-Location $PSScriptRoot

    if (!(Test-Path -PathType Container -Path "./build")) {
        New-Item -ItemType Directory -Path "./build"
    }

    try {
        Push-Location ./build

        # NOTE! /EHsc is included to silence warning C4530: 
        #       C++ exception handler used, but unwind semantics are not enabled. 
        #       Specify /EHsc
        cl /std:c++latest /EHsc ../*.cpp /link /out:pplox.exe
        if ($LASTEXITCODE -ne 0) {
            Write-Host ""
            Write-Host "Non-zero exit code from cl: $LASTEXITCODE"
            return
        }
    }
    finally {
        Pop-Location
    }

    Write-Host ""
    ./build/pplox ./test_file.lox
}
finally {
    Pop-Location
}