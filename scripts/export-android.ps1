<#
.SYNOPSIS
    PrismaEngine Android export tool - generates a buildable Android project from source.

.DESCRIPTION
    Reads project config (.jsonc), generates Android project from template, optionally runs CMake build,
    dotnet publish, asset copy, and Gradle APK build.

    This script is CLI-driven with no GUI dependency.

.PARAMETER Source
    Project source directory path (e.g. projects/PathTracing3D). Must contain assets/{ProjectName}.jsonc config file.

.PARAMETER Output
    Output directory path. Default: build/export/android/{ProjectName}

.PARAMETER Package
    Android package name. Default: com.prismaengine.{projectname}

.PARAMETER Preset
    CMake preset name. Default: android-arm64-debug

.PARAMETER EngineRoot
    Engine root directory path. Default: auto-detected (parent of script directory)

.PARAMETER SkipBuild
    Skip CMake and dotnet build steps (generate project only)

.PARAMETER BuildApk
    Run Gradle APK build after generating project

.PARAMETER Clean
    Clean output directory before generating

.EXAMPLE
    .\export-android.ps1 -Source projects\PathTracing3D

    Export PathTracing3D project with default parameters.

.EXAMPLE
    .\export-android.ps1 -Source projects\PathTracing3D -BuildApk

    Export and build APK.

.EXAMPLE
    .\export-android.ps1 -Source projects\PrismaCraft -Package com.example.craft -SkipBuild

    Export with custom package name, skip native build.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,

    [string]$Output,

    [string]$Package,

    [string]$Preset = "android-arm64-debug",

    [string]$EngineRoot,

    [switch]$SkipBuild,

    [switch]$BuildApk,

    [switch]$Clean
)

# ============================================================
# Helper functions
# ============================================================

function Write-Step {
    param([string]$Message)
    Write-Host "`n=== $Message ===" -ForegroundColor Cyan
}

function Write-Info {
    param([string]$Message)
    Write-Host "  $Message" -ForegroundColor Gray
}

function Write-Success {
    param([string]$Message)
    Write-Host "  [OK] $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "  [FAIL] $Message" -ForegroundColor Red
}

function Resolve-EngineRoot {
    if ($EngineRoot) {
        return (Resolve-Path $EngineRoot -ErrorAction Stop).Path
    }
    # Auto-detect: script is in scripts/, engine root is parent
    $scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $PSCommandPath }
    if ($scriptDir) {
        $candidate = Resolve-Path (Join-Path $scriptDir "..") -ErrorAction SilentlyContinue
        if ($candidate -and (Test-Path (Join-Path $candidate "CMakeLists.txt"))) {
            return $candidate.Path
        }
    }
    # Fallback: current working directory
    return (Get-Location).Path
}

function Resolve-NdkPath {
    # Search order:
    #   1. ANDROID_NDK_HOME env var (if valid)
    #   2. ANDROID_SDK_ROOT/ANDROID_HOME + ndk/<version> subdirectories (pick latest)
    #   3. Common install locations
    # Returns the NDK root directory (containing toolchain, platforms, etc.) or $null.

    # 1. Explicit env var
    $ndkHome = $env:ANDROID_NDK_HOME
    if ($ndkHome -and (Test-Path (Join-Path $ndkHome "build\cmake\android.toolchain.cmake"))) {
        Write-Info "NDK found via ANDROID_NDK_HOME: $ndkHome"
        return $ndkHome
    }

    # 2. SDK directory -> scan ndk/ subdirs for latest version
    $sdkDirs = @()
    if ($env:ANDROID_SDK_ROOT) { $sdkDirs += $env:ANDROID_SDK_ROOT }
    if ($env:ANDROID_HOME) { $sdkDirs += $env:ANDROID_HOME }
    # Windows default
    $winDefault = Join-Path $env:LOCALAPPDATA "Android\Sdk"
    if (Test-Path $winDefault) { $sdkDirs += $winDefault }
    # macOS default
    if ($env:HOME) {
        $macDefault = Join-Path $env:HOME "Library\Android\sdk"
        if (Test-Path $macDefault) { $sdkDirs += $macDefault }
        # Linux default
        $linuxDefault = Join-Path $env:HOME "Android\Sdk"
        if (Test-Path $linuxDefault) { $sdkDirs += $linuxDefault }
    }

    $sdkDirs = $sdkDirs | Select-Object -Unique

    foreach ($sdk in $sdkDirs) {
        $ndkDir = Join-Path $sdk "ndk"
        if (Test-Path $ndkDir) {
            # Pick the latest version (sort by directory name, which is version string)
            $latestNdk = Get-ChildItem -Path $ndkDir -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($latestNdk -and (Test-Path (Join-Path $latestNdk.FullName "build\cmake\android.toolchain.cmake"))) {
                Write-Info "NDK found in SDK: $($latestNdk.FullName)"
                return $latestNdk.FullName
            }
        }
    }

    # 3. Common standalone NDK locations
    $standalonePaths = @(
        Join-Path $env:LOCALAPPDATA "Android\Ndk"
        "C:\Android\Ndk"
        "D:\Android\Ndk"
        "/usr/local/lib/android/sdk/ndk"
        "/opt/android-ndk"
    )
    foreach ($p in $standalonePaths) {
        if ($p -and (Test-Path $p)) {
            # Could be the NDK root itself or a parent with version subdirs
            if (Test-Path (Join-Path $p "build\cmake\android.toolchain.cmake")) {
                Write-Info "NDK found at: $p"
                return $p
            }
            # Check for version subdirs
            $latestNdk = Get-ChildItem -Path $p -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($latestNdk -and (Test-Path (Join-Path $latestNdk.FullName "build\cmake\android.toolchain.cmake"))) {
                Write-Info "NDK found at: $($latestNdk.FullName)"
                return $latestNdk.FullName
            }
        }
    }

    return $null
}

function Read-JsoncConfig {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        Write-Error "Config file not found: $Path"
        exit 1
    }

    $content = Get-Content -Path $Path -Raw -Encoding UTF8

    # Strip JSONC comments:
    #   Single-line // ...
    #   Multi-line /* ... */
    # Note: does not handle comments inside strings (sufficient for engine config files)
    $content = $content -replace '//.*$', '' -replace '/\*[\s\S]*?\*/', ''

    # Remove trailing commas (JSON disallows, JSONC allows)
    $content = $content -replace ',\s*([}\]])', '$1'

    try {
        return $content | ConvertFrom-Json
    }
    catch {
        Write-Error "Failed to parse JSONC config: $Path`n$($_.Exception.Message)"
        exit 1
    }
}

function Get-ProjectConfig {
    param(
        [string]$ProjectDir,
        [string]$ProjectName
    )

    # Find .jsonc config file
    $configPath = Join-Path $ProjectDir "assets\$ProjectName.jsonc"
    if (-not (Test-Path $configPath)) {
        # Try project root directory
        $configPath = Join-Path $ProjectDir "$ProjectName.jsonc"
    }
    if (-not (Test-Path $configPath)) {
        # Search for any .jsonc file
        $jsoncFiles = Get-ChildItem -Path $ProjectDir -Filter "*.jsonc" -Recurse -Depth 1
        if ($jsoncFiles.Count -eq 1) {
            $configPath = $jsoncFiles[0].FullName
        }
        elseif ($jsoncFiles.Count -gt 1) {
            Write-Error "Multiple .jsonc files found in $ProjectDir, please specify project name"
            exit 1
        }
        else {
            Write-Error "No .jsonc config file found in $ProjectDir"
            exit 1
        }
    }

    Write-Info "Config file: $configPath"
    return (Read-JsoncConfig -Path $configPath), $configPath
}

function Get-NativeLibraries {
    param(
        [string]$ProjectName,
        [string]$ProjectDir,
        [string]$EngineRoot
    )

    # Core libraries: SDL3 + Prisma.Core (always required)
    $libs = @("SDL3", "Prisma.Core")

    # Project plugin library: {ProjectName}.Core
    # Check CMakeLists.txt to confirm plugin target exists
    $cmakePath = Join-Path $ProjectDir "CMakeLists.txt"
    if (Test-Path $cmakePath) {
        $cmakeContent = Get-Content $cmakePath -Raw -Encoding UTF8

        # Find SHARED library target (plugin)
        # Match add_library(... SHARED ...) and extract OUTPUT_NAME
        $pluginMatch = [regex]::Match($cmakeContent, 'OUTPUT_NAME\s+"([^"]+\.Core)"')
        if ($pluginMatch.Success) {
            $libs += $pluginMatch.Groups[1].Value
        }
        else {
            # Fallback: use convention {ProjectName}.Core
            $libs += "$ProjectName.Core"
        }
    }
    else {
        # No CMakeLists.txt, use convention
        $libs += "$ProjectName.Core"
    }

    return $libs
}

function Copy-Template {
    param(
        [string]$TemplateDir,
        [string]$OutputDir,
        [hashtable]$Replacements
    )

    # Copy template directory to output directory
    if ($Clean -and (Test-Path $OutputDir)) {
        Remove-Item -Path $OutputDir -Recurse -Force
        Write-Info "Cleaned output directory: $OutputDir"
    }

    # Copy entire template directory first, then process .tpl files
    # Copy entire template directory
    Copy-Item -Path $TemplateDir -Destination (Split-Path $OutputDir -Parent) -Recurse -Force

    # Rename if output directory name differs from template directory name
    $templateDirName = Split-Path $TemplateDir -Leaf
    $copiedDir = Join-Path (Split-Path $OutputDir -Parent) $templateDirName
    if ($copiedDir -ne $OutputDir) {
        if (Test-Path $OutputDir) {
            Remove-Item -Path $OutputDir -Recurse -Force
        }
        Move-Item -Path $copiedDir -Destination $OutputDir -Force
    }

    # Process .tpl files: replace placeholders and remove .tpl extension
    $tplFiles = Get-ChildItem -Path $OutputDir -Filter "*.tpl" -Recurse
    foreach ($tplFile in $tplFiles) {
        $content = Get-Content -Path $tplFile.FullName -Raw -Encoding UTF8

        foreach ($key in $Replacements.Keys) {
            $content = $content -replace [regex]::Escape("{{$key}}"), $Replacements[$key]
        }

        $outputPath = $tplFile.FullName -replace '\.tpl$', ''
        Set-Content -Path $outputPath -Value $content -Encoding UTF8 -NoNewline
        Remove-Item -Path $tplFile.FullName -Force
        Write-Info "Template processed: $($tplFile.Name) -> $(Split-Path $outputPath -Leaf)"
    }
}

function New-AndroidProject {
    param(
        [string]$TemplateDir,
        [string]$OutputDir,
        [string]$ProjectName,
        [string]$PackageName,
        [string]$PackagePath,
        [string]$AppLabel,
        [string]$Orientation,
        [string[]]$NativeLibraries
    )

    Write-Step "Generate Android project"

    # Build native library array string (Java format)
    $libsArray = ($NativeLibraries | ForEach-Object { "`"$_`"" }) -join ", "

    $replacements = @{
        "PROJECT_NAME"            = $ProjectName
        "PACKAGE_NAME"            = $PackageName
        "PACKAGE_PATH"            = $PackagePath
        "APP_LABEL"               = $AppLabel
        "NATIVE_LIBRARIES_ARRAY"  = $libsArray
        "SCREEN_ORIENTATION"      = $Orientation
    }

    Copy-Template -TemplateDir $TemplateDir -OutputDir $OutputDir -Replacements $replacements

    # Create package path directory and move PrismaActivity.java
    $javaBaseDir = Join-Path $OutputDir "app\src\main\java"
    $packageDir = Join-Path $javaBaseDir $PackagePath
    New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

    $activitySrc = Join-Path $javaBaseDir "PrismaActivity.java"
    $activityDst = Join-Path $packageDir "PrismaActivity.java"

    if (Test-Path $activitySrc) {
        Move-Item -Path $activitySrc -Destination $activityDst -Force
        Write-Info "PrismaActivity.java → $PackagePath\PrismaActivity.java"
    }

    # Generate local.properties (SDK path)
    $sdkPath = $env:ANDROID_HOME
    if (-not $sdkPath) {
        $sdkPath = $env:ANDROID_SDK_ROOT
    }
    if (-not $sdkPath) {
        # Windows default path
        $defaultSdkPath = Join-Path $env:LOCALAPPDATA "Android\Sdk"
        if (Test-Path $defaultSdkPath) {
            $sdkPath = $defaultSdkPath
        }
    }

    if ($sdkPath) {
        # Use forward slashes to avoid Gradle path issues
        $sdkPathGradle = $sdkPath.Replace('\', '/')
        Set-Content -Path (Join-Path $OutputDir "local.properties") -Value "sdk.dir=$sdkPathGradle" -Encoding UTF8
        Write-Info "local.properties → sdk.dir=$sdkPathGradle"
    }
    else {
        Write-Host "  WARNING: Android SDK path not detected, please set local.properties manually" -ForegroundColor Yellow
    }

    Write-Success "Android project generated: $OutputDir"
}

function Copy-NativeLibs {
    param(
        [string]$OutputDir,
        [string]$BuildDir,
        [string[]]$NativeLibraries,
        [string]$Architecture = "arm64-v8a"
    )

    Write-Step "Copy native libraries (.so)"

    $jniLibsDir = Join-Path $OutputDir "app\src\main\jniLibs\$Architecture"
    New-Item -ItemType Directory -Path $jniLibsDir -Force | Out-Null

    $libDir = Join-Path $BuildDir "lib"
    $copiedCount = 0

    foreach ($lib in $NativeLibraries) {
        $soFile = Join-Path $libDir "lib$lib.so"

        if (Test-Path $soFile) {
            Copy-Item -Path $soFile -Destination $jniLibsDir -Force
            Write-Info "lib$lib.so ✓"
            $copiedCount++
        }
        else {
            # SDL3 may be in _deps
            if ($lib -eq "SDL3") {
                $sdlSo = Get-ChildItem -Path (Join-Path $BuildDir "_deps") -Filter "libSDL3.so" -Recurse -ErrorAction SilentlyContinue |
                          Select-Object -First 1
                if ($sdlSo) {
                    Copy-Item -Path $sdlSo.FullName -Destination $jniLibsDir -Force
                    Write-Info "libSDL3.so ✓ (from _deps)"
                    $copiedCount++
                    continue
                }
            }

            Write-Host "  WARNING: lib$lib.so not found" -ForegroundColor Yellow
        }
    }

    # Copy CoreCLR runtime .so files
    $csRuntimeDir = Join-Path $BuildDir "PrismaEngine.Host\android-arm64\publish"
    if (Test-Path $csRuntimeDir) {
        $runtimeSoFiles = Get-ChildItem -Path $csRuntimeDir -Filter "lib*.so"
        foreach ($soFile in $runtimeSoFiles) {
            Copy-Item -Path $soFile.FullName -Destination $jniLibsDir -Force
            Write-Info "$($soFile.Name) ✓ (CoreCLR runtime)"
            $copiedCount++
        }
    }

    Write-Success "Copied $copiedCount native libraries to jniLibs/$Architecture/"
}

function Copy-ProjectAssets {
    param(
        [string]$OutputDir,
        [string]$ProjectDir,
        [string]$EngineRoot
    )

    Write-Step "Copy project assets"

    $assetsDir = Join-Path $OutputDir "app\src\main\assets"
    New-Item -ItemType Directory -Path $assetsDir -Force | Out-Null

    # Copy project assets (scenes, materials, models, .jsonc config, etc.)
    $projectAssetsDir = Join-Path $ProjectDir "assets"
    if (Test-Path $projectAssetsDir) {
        # Copy all assets except .spv files (shaders handled separately)
        Get-ChildItem -Path $projectAssetsDir -Recurse -File |
            Where-Object { $_.Extension -ne '.spv' } |
            ForEach-Object {
                $relativePath = $_.FullName.Substring($projectAssetsDir.Length + 1)
                $destPath = Join-Path $assetsDir $relativePath
                $destDir = Split-Path $destPath -Parent
                if (-not (Test-Path $destDir)) {
                    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                }
                Copy-Item -Path $_.FullName -Destination $destPath -Force
            }
        Write-Info "Project assets copied"
    }

    # 生成标准化 project.jsonc（不依赖项目名，供 native 层读取 plugin 库名）
    $jsoncFile = Get-ChildItem -Path $assetsDir -Filter "*.jsonc" -File | Select-Object -First 1
    if ($jsoncFile) {
        $standardPath = Join-Path $assetsDir "project.jsonc"
        Copy-Item -Path $jsoncFile.FullName -Destination $standardPath -Force
        Write-Info "Standard project.jsonc generated from $($jsoncFile.Name)"
    }

    # Copy engine common shaders (.spv) — 根 assets/shaders/
    $rootShadersDir = Join-Path $EngineRoot "assets\shaders"
    if (Test-Path $rootShadersDir) {
        $spvFiles = Get-ChildItem -Path $rootShadersDir -Filter "*.spv" -Recurse
        if ($spvFiles.Count -gt 0) {
            $shadersDestDir = Join-Path $assetsDir "shaders"
            New-Item -ItemType Directory -Path $shadersDestDir -Force | Out-Null

            foreach ($spvFile in $spvFiles) {
                $relativePath = $spvFile.FullName.Substring($rootShadersDir.Length + 1)
                $destPath = Join-Path $shadersDestDir $relativePath
                $destDir = Split-Path $destPath -Parent
                if (-not (Test-Path $destDir)) {
                    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                }
                Copy-Item -Path $spvFile.FullName -Destination $destPath -Force
            }
            Write-Info "Engine shaders copied ($($spvFiles.Count) .spv files)"
        }
    }

    # Also copy from resources/common/shaders/glsl (water, particles in subdirs)
    $glslShadersDir = Join-Path $EngineRoot "resources\common\shaders\glsl"
    if (Test-Path $glslShadersDir) {
        $spvFiles = Get-ChildItem -Path $glslShadersDir -Filter "*.spv" -Recurse
        if ($spvFiles.Count -gt 0) {
            $shadersDestDir = Join-Path $assetsDir "shaders"
            New-Item -ItemType Directory -Path $shadersDestDir -Force | Out-Null

            foreach ($spvFile in $spvFiles) {
                $relativePath = $spvFile.FullName.Substring($glslShadersDir.Length + 1)
                $destPath = Join-Path $shadersDestDir $relativePath
                $destDir = Split-Path $destPath -Parent
                if (-not (Test-Path $destDir)) {
                    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                }
                Copy-Item -Path $spvFile.FullName -Destination $destPath -Force
            }
            Write-Info "Common GLSL shaders copied ($($spvFiles.Count) .spv files)"
        }
    }

    # Copy project shaders (.spv)
    $projectShadersDir = Join-Path $projectAssetsDir "shaders"
    if (Test-Path $projectShadersDir) {
        $spvFiles = Get-ChildItem -Path $projectShadersDir -Filter "*.spv" -Recurse
        if ($spvFiles.Count -gt 0) {
            $shadersDestDir = Join-Path $assetsDir "shaders"
            if (-not (Test-Path $shadersDestDir)) {
                New-Item -ItemType Directory -Path $shadersDestDir -Force | Out-Null
            }

            foreach ($spvFile in $spvFiles) {
                $relativePath = $spvFile.FullName.Substring($projectShadersDir.Length + 1)
                $destPath = Join-Path $shadersDestDir $relativePath
                $destDir = Split-Path $destPath -Parent
                if (-not (Test-Path $destDir)) {
                    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                }
                Copy-Item -Path $spvFile.FullName -Destination $destPath -Force
            }
            Write-Info "Project shaders copied ($($spvFiles.Count) .spv files)"
        }
    }

    # Copy C# runtime (CoreCLR)
    $buildDir = Join-Path $EngineRoot "build\$Preset"
    $csRuntimeDir = Join-Path $buildDir "PrismaEngine.Host\android-arm64\publish"
    if (Test-Path $csRuntimeDir) {
        $runtimeDestDir = Join-Path $assetsDir "runtime"
        New-Item -ItemType Directory -Path $runtimeDestDir -Force | Out-Null

        # Copy all .dll files (excluding .so, those are already in jniLibs)
        Get-ChildItem -Path $csRuntimeDir -Filter "*.dll" -Recurse |
            ForEach-Object {
                $relativePath = $_.FullName.Substring($csRuntimeDir.Length + 1)
                $destPath = Join-Path $runtimeDestDir $relativePath
                $destDir = Split-Path $destPath -Parent
                if (-not (Test-Path $destDir)) {
                    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
                }
                Copy-Item -Path $_.FullName -Destination $destPath -Force
            }

        # Copy project script DLLs
        $projectScriptsDll = Get-ChildItem -Path $csRuntimeDir -Filter "*.Scripts.dll" -Recurse
        if ($projectScriptsDll.Count -eq 0) {
            # Fallback: cmake builds scripts DLL to bin/ directory
            $binDir = Join-Path $buildDir "bin"
            if (Test-Path $binDir) {
                $projectScriptsDll = Get-ChildItem -Path $binDir -Filter "*.Scripts.dll"
            }
        }
        foreach ($dll in $projectScriptsDll) {
            Copy-Item -Path $dll.FullName -Destination $runtimeDestDir -Force
        }

        Write-Info "C# runtime copied"
    }

    Write-Success "Asset copy complete"
}

function Invoke-CMakeBuild {
    param(
        [string]$EngineRoot,
        [string]$Preset,
        [string]$ProjectName = ""
    )

    Write-Step "CMake build ($Preset)"

    # Resolve NDK and set ANDROID_NDK_HOME for CMake preset
    $ndkPath = Resolve-NdkPath
    if ($ndkPath) {
        $env:ANDROID_NDK_HOME = $ndkPath
        Write-Info "ANDROID_NDK_HOME set to: $ndkPath"
    }
    else {
        Write-Error "Android NDK not found. Set ANDROID_NDK_HOME or install NDK under your Android SDK."
        Write-Info "  Expected: <sdk>/ndk/<version>/ with toolchain at build/cmake/android.toolchain.cmake"
        exit 1
    }

    $buildDir = Join-Path $EngineRoot "build\$Preset"

    # Clear stale CMakeCache if NDK path changed (old cache may reference a removed NDK)
    $cmakeCache = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cmakeCache) {
        $cacheNdk = Select-String -Path $cmakeCache -Pattern "ANDROID_NDK:" -ErrorAction SilentlyContinue
        if ($cacheNdk) {
            $cachedNdkPath = ($cacheNdk.Line -split '=')[1].Trim()
            $toolchainInCache = Join-Path $cachedNdkPath "build\cmake\android.toolchain.cmake"
            if (-not (Test-Path $toolchainInCache)) {
                Write-Info "Clearing stale CMakeCache (NDK path changed or removed)"
                Remove-Item -Path $cmakeCache -Force
                $cmakeFilesDir = Join-Path $buildDir "CMakeFiles"
                if (Test-Path $cmakeFilesDir) {
                    Remove-Item -Path $cmakeFilesDir -Recurse -Force -ErrorAction SilentlyContinue
                }
            }
        }
    }

# Build only the target project (or just Engine if no project specified)
    $cmakeArgs = @("--preset", $Preset)

    $allProjects = @("PRISMACRAFT", "PACMAN", "PRISMA2D", "PATHTRACING3D", "CLUSTEREDFORWARD3D", "DEFERRED3D", "METROIDVANIADEMO", "NEOEDITOR")
    foreach ($proj in $allProjects) {
        $flagName = "PRISMA_BUILD_PROJECT_$proj"
        if ($ProjectName -and ($proj -eq $ProjectName.ToUpper())) {
            $cmakeArgs += "-D$flagName=ON"
        } else {
            $cmakeArgs += "-D$flagName=OFF"
        }
    }
    if ($ProjectName) {
        Write-Info "Building only project: $ProjectName (others disabled)"
    } else {
        Write-Info "Building Engine only (all projects disabled)"
    }

    # Configure
    Write-Info "Configuring CMake..."
    & cmake @cmakeArgs 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configure failed (exit code: $LASTEXITCODE)"
        exit 1
    }

    # Build
    Write-Info "Building..."
    & cmake --build $buildDir --parallel 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake build failed (exit code: $LASTEXITCODE)"
        exit 1
    }

    Write-Success "CMake build complete"
}

function Invoke-DotNetPublish {
    param(
        [string]$ProjectDir,
        [string]$EngineRoot,
        [string]$Preset
    )

    Write-Step "dotnet publish (C# scripts)"

    # Find project .csproj files
    $csprojFiles = Get-ChildItem -Path (Join-Path $ProjectDir "scripts") -Filter "*.csproj" -ErrorAction SilentlyContinue
    if ($csprojFiles.Count -eq 0) {
        Write-Info "No C# script project found, skipping"
        return
    }

    foreach ($csproj in $csprojFiles) {
        Write-Info "Publishing: $($csproj.Name)"
        & dotnet publish $csproj.FullName -r android-arm64 -c Release --nologo 2>&1 | Write-Host
        if ($LASTEXITCODE -ne 0) {
            Write-Error "dotnet publish failed (exit code: $LASTEXITCODE)"
            exit 1
        }
    }

    Write-Success "C# script publish complete"
}

function Invoke-GradleBuild {
    param([string]$OutputDir)

    Write-Step "Gradle build APK"

    Push-Location $OutputDir
    try {
        if ($IsWindows -or ($env:OS -eq "Windows_NT")) {
            & ".\gradlew.bat" assembleDebug 2>&1 | Write-Host
        }
        else {
            & "./gradlew" assembleDebug 2>&1 | Write-Host
        }

        if ($LASTEXITCODE -ne 0) {
            Write-Error "Gradle build failed (exit code: $LASTEXITCODE)"
            exit 1
        }

        $apkPath = Get-ChildItem -Path (Join-Path $OutputDir "app\build\outputs\apk\debug") -Filter "*.apk" -ErrorAction SilentlyContinue |
                   Select-Object -First 1

        if ($apkPath) {
            Write-Success "APK: $($apkPath.FullName)"
        }
        else {
            Write-Host "  WARNING: APK file not found" -ForegroundColor Yellow
        }
    }
    finally {
        Pop-Location
    }
}

# ============================================================
# Main flow
# ============================================================

Write-Host @"

  +---------------------------------------------------+
  |       PrismaEngine - Android Export v1.0           |
  +---------------------------------------------------+

"@ -ForegroundColor Cyan

# Resolve source path
$Source = (Resolve-Path $Source -ErrorAction Stop).Path
if (-not (Test-Path $Source -PathType Container)) {
    Write-Error "Source directory not found: $Source"
    exit 1
}

# Detect engine root
$EngineRoot = Resolve-EngineRoot
Write-Info "Engine root: $EngineRoot"
Write-Info "Project dir: $Source"

# Infer project name
$ProjectName = Split-Path $Source -Leaf
Write-Info "Project name: $ProjectName"

# Read project config
$config, $configPath = Get-ProjectConfig -ProjectDir $Source -ProjectName $ProjectName

# Get display name from config
# Take part before parentheses or whole name, strip special chars
$AppLabel = if ($config.name) {
    $label = $config.name -split '\(' | Select-Object -First 1
    $label = $label -split '\(' | Select-Object -First 1
    $label.Trim()
} else { $ProjectName }

# Determine package name
if (-not $Package) {
    # Generate package name from project name: PathTracing3D -> com.prismaengine.pathtracing3d
    $Package = "com.prismaengine.$($ProjectName.ToLower())"
}
$PackagePath = $Package.Replace('.', '\')

Write-Info "Package: $Package"
Write-Info "App label: $AppLabel"

# Determine output directory
if (-not $Output) {
    $Output = Join-Path $EngineRoot "build\export\android\$ProjectName"
}
# Ensure output parent directory exists
$outputParent = Split-Path $Output -Parent
if (-not (Test-Path $outputParent)) {
    New-Item -ItemType Directory -Path $outputParent -Force | Out-Null
}

Write-Info "Output dir: $Output"

# Determine native libraries
$NativeLibraries = Get-NativeLibraries -ProjectName $ProjectName -ProjectDir $Source -EngineRoot $EngineRoot
Write-Info "Native libs: $($NativeLibraries -join ', ')"

# Template directory
$TemplateDir = Join-Path $EngineRoot "src\editor\export\android-template"
if (-not (Test-Path $TemplateDir)) {
    Write-Error "Template directory not found: $TemplateDir"
    exit 1
}

# Determine orientation (default: landscape)
$Orientation = if ($config.window.orientation) { $config.window.orientation } else { "landscape" }
Write-Info "Screen orientation: $Orientation"

# ============================================================
# Step 1: CMake build (optional)
# ============================================================
if (-not $SkipBuild) {
    Invoke-CMakeBuild -EngineRoot $EngineRoot -Preset $Preset -ProjectName $ProjectName
    Invoke-DotNetPublish -ProjectDir $Source -EngineRoot $EngineRoot -Preset $Preset
}

# ============================================================
# Step 2: Generate Android project
# ============================================================
New-AndroidProject -TemplateDir $TemplateDir -OutputDir $Output `
    -ProjectName $ProjectName -PackageName $Package -PackagePath $PackagePath `
    -AppLabel $AppLabel -Orientation $Orientation -NativeLibraries $NativeLibraries

# ============================================================
# Step 3: Copy native libraries (if built)
# ============================================================
if (-not $SkipBuild) {
    $buildDir = Join-Path $EngineRoot "build\$Preset"
    if (Test-Path $buildDir) {
        Copy-NativeLibs -OutputDir $Output -BuildDir $buildDir -NativeLibraries $NativeLibraries
    }
    else {
        Write-Host "  WARNING: Build directory not found, skipping native lib copy: $buildDir" -ForegroundColor Yellow
    }
}

# ============================================================
# Step 4: Copy project assets
# ============================================================
Copy-ProjectAssets -OutputDir $Output -ProjectDir $Source -EngineRoot $EngineRoot

# ============================================================
# Step 5: Gradle build APK (optional)
# ============================================================
if ($BuildApk) {
    Invoke-GradleBuild -OutputDir $Output
}

# ============================================================
# Done
# ============================================================
Write-Host @"

  ===================================================
  [OK] Android project export complete!
  ===================================================

  Project:  $ProjectName
  Package:   $Package
  Output:    $Output

  Next steps:
    1. cd $Output
    2. .\gradlew assembleDebug        (Build Debug APK)
    3. .\gradlew assembleRelease      (Build Release APK)

"@ -ForegroundColor Green