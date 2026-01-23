Write-Host "Testing quiet build..."
Write-Host "Preset: windows-x64-debug"
Write-Host ""
Write-Host "[1/2] Configuring..."
cmake --preset windows-x64-debug --log-level=ERROR
if ($LASTEXITCODE -eq 0) {
    Write-Host "[2/2] Building..."
    cmake --build --preset windows-x64-debug -- /v:minimal
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build completed: build\windows-x64-debug" -ForegroundColor Green
    } else {
        Write-Host "Build failed" -ForegroundColor Red
    }
} else {
    Write-Host "Configure failed" -ForegroundColor Red
}
