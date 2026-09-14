Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$ToolsDirectory = Join-Path $RepositoryRoot ".tools"
$VcpkgRoot = Join-Path $ToolsDirectory "vcpkg"
$VcpkgExecutable = Join-Path $VcpkgRoot "vcpkg.exe"

# vcpkg.json의 builtin-baseline과 동일한 커밋을 사용합니다.
$VcpkgCommit = "a1cae005c39be7b18ba319fced856b68d7276271"

New-Item -ItemType Directory -Force -Path $ToolsDirectory | Out-Null

if (-not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    if (Test-Path $VcpkgRoot) {
        throw "vcpkg 경로가 존재하지만 Git 저장소가 아닙니다: $VcpkgRoot"
    }

    git clone --filter=blob:none `
        https://github.com/microsoft/vcpkg.git `
        $VcpkgRoot

    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg 저장소를 복제하지 못했습니다."
    }
}

git -C $VcpkgRoot fetch origin $VcpkgCommit --depth 1

if ($LASTEXITCODE -ne 0) {
    throw "고정된 vcpkg 커밋을 가져오지 못했습니다."
}

git -C $VcpkgRoot checkout --detach $VcpkgCommit

if ($LASTEXITCODE -ne 0) {
    throw "고정된 vcpkg 커밋으로 전환하지 못했습니다."
}

& (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics

if ($LASTEXITCODE -ne 0) {
    throw "vcpkg bootstrap에 실패했습니다."
}

Push-Location $RepositoryRoot

try {
    & $VcpkgExecutable install "--triplet=x64-windows"

    if ($LASTEXITCODE -ne 0) {
        throw "프로젝트 의존성 설치에 실패했습니다."
    }
}
finally {
    Pop-Location
}

Write-Host "Macaw dependencies are ready."