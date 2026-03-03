[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$Release
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

#------------------------------------------------------------------------------
# Logging
#------------------------------------------------------------------------------
function Write-Info { param([string]$Message) Write-Host "[INFO]    $Message" }
function Write-Warn { param([string]$Message) Write-Host "[WARN]    $Message" }
function Write-Err  { param([string]$Message) Write-Host "[ERROR]   $Message" }

#------------------------------------------------------------------------------
# Filesystem utilities
#------------------------------------------------------------------------------
function Ensure-Directory {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { New-Item -ItemType Directory -Path $Path | Out-Null }
}

function Resolve-FirstExistingPath {
    param([string]$BaseDirectory, [string[]]$Candidates, [string]$Label)
    foreach ($candidate in $Candidates) {
        $candidatePath = $candidate
        if (-not [System.IO.Path]::IsPathRooted($candidatePath)) { $candidatePath = Join-Path $BaseDirectory $candidatePath }
        $candidatePath = [System.IO.Path]::GetFullPath($candidatePath)
        if (Test-Path -LiteralPath $candidatePath) { return $candidatePath }
    }
    throw "$Label not found. Searched: $($Candidates -join ', ')"
}

#------------------------------------------------------------------------------
# MSVC environment bootstrap
#------------------------------------------------------------------------------
function Import-VcVarsEnvironment {
    param([string]$VcVarsPath)
    Write-Info "Using MSVC: $VcVarsPath"
    $command = "call `"$VcVarsPath`" >nul && set"
    $envDump = & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw "Failed to load MSVC environment from: $VcVarsPath" }
    foreach ($line in $envDump) {
        if ($line -match "^(.*?)=(.*)$") { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process") }
    }
    if (-not (Get-Command cl.exe   -ErrorAction SilentlyContinue)) { throw "cl.exe not found after loading vcvars." }
    if (-not (Get-Command link.exe -ErrorAction SilentlyContinue)) { throw "link.exe not found after loading vcvars." }
}

function Resolve-VcVarsPath {
    $attempted = New-Object System.Collections.Generic.List[string]

    $vswhereCandidates = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    foreach ($vswhere in $vswhereCandidates) {
        $attempted.Add("vswhere: $vswhere")
        if (-not (Test-Path -LiteralPath $vswhere)) { continue }

        $queryVariants = @(
            @("-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find", "VC\Auxiliary\Build\vcvars64.bat"),
            @("-latest", "-prerelease", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find", "VC\Auxiliary\Build\vcvars64.bat"),
            @("-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-find", "VC\Auxiliary\Build\vcvars64.bat")
        )
        foreach ($queryArgs in $queryVariants) {
            $result = & $vswhere @queryArgs 2>$null
            if ($LASTEXITCODE -ne 0) { continue }
            $paths = @($result | ForEach-Object { $_.ToString().Trim() } | Where-Object { $_.Length -gt 0 })
            foreach ($path in $paths) {
                $attempted.Add("vswhere result: $path")
                if (Test-Path -LiteralPath $path) {
                    return [System.IO.Path]::GetFullPath($path)
                }
            }
        }
    }

    $staticCandidates = @(
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($candidate in $staticCandidates) {
        $attempted.Add($candidate)
        if (Test-Path -LiteralPath $candidate) {
            return [System.IO.Path]::GetFullPath($candidate)
        }
    }

    throw "Visual Studio vcvars64.bat not found. Probed via vswhere and common install paths: $($attempted -join '; ')"
}

#------------------------------------------------------------------------------
# Incremental build helpers
#------------------------------------------------------------------------------
function Get-Sha256 {
    param([string]$Text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        $hash  = $sha.ComputeHash($bytes)
        return ($hash | ForEach-Object { $_.ToString("x2") }) -join ""
    } finally { $sha.Dispose() }
}

function Test-PathUnderAnyRoot {
    param([string]$Path, [string[]]$Roots)
    foreach ($root in $Roots) {
        if ($Path.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) { return $true }
    }
    return $false
}

function Get-SourceOutputKey {
    param([string]$SourcePath, [string]$ProjectRoot, [string]$ImguiRoot)
    $fullSourcePath = [System.IO.Path]::GetFullPath($SourcePath)
    if ($fullSourcePath.StartsWith($ProjectRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fullSourcePath.Substring($ProjectRoot.Length).TrimStart("\", "/")
    }
    if ($fullSourcePath.StartsWith($ImguiRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return Join-Path "External\imgui" ($fullSourcePath.Substring($ImguiRoot.Length).TrimStart("\", "/"))
    }
    $sanitized = $fullSourcePath -replace "[:\\/\s]", "_"
    return Join-Path "External\misc" $sanitized
}

function Get-CompileReason {
    param([string]$ObjectPath, [string]$DependencyFile, [string]$CommandFile, [string]$CompileSignature)
    if (-not (Test-Path -LiteralPath $ObjectPath))      { return "missing object" }
    if (-not (Test-Path -LiteralPath $DependencyFile))  { return "missing dependency cache" }
    if (-not (Test-Path -LiteralPath $CommandFile))     { return "missing command signature" }
    $storedSignature = (Get-Content -LiteralPath $CommandFile -Raw).Trim()
    if ($storedSignature -ne $CompileSignature)         { return "compile settings changed" }
    $objectTime      = (Get-Item -LiteralPath $ObjectPath).LastWriteTimeUtc
    $dependencyPaths = @(Get-Content -LiteralPath $DependencyFile | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 })
    if ($dependencyPaths.Count -eq 0)                  { return "empty dependency cache" }
    foreach ($dep in $dependencyPaths) {
        if (-not (Test-Path -LiteralPath $dep))                                      { return "dependency removed: $dep" }
        if ((Get-Item -LiteralPath $dep).LastWriteTimeUtc -gt $objectTime)           { return "dependency updated: $dep" }
    }
    return $null
}

function Get-LinkReason {
    param([string]$ExecutablePath, [string[]]$ObjectPaths, [string]$LinkSignatureFile, [string]$LinkSignature)
    if (-not (Test-Path -LiteralPath $ExecutablePath))      { return "missing executable" }
    if (-not (Test-Path -LiteralPath $LinkSignatureFile))   { return "missing link signature" }
    $storedSignature = (Get-Content -LiteralPath $LinkSignatureFile -Raw).Trim()
    if ($storedSignature -ne $LinkSignature)                { return "link settings changed" }
    $exeTime = (Get-Item -LiteralPath $ExecutablePath).LastWriteTimeUtc
    foreach ($obj in $ObjectPaths) {
        if (-not (Test-Path -LiteralPath $obj))                                     { return "missing object: $obj" }
        if ((Get-Item -LiteralPath $obj).LastWriteTimeUtc -gt $exeTime)             { return "updated object: $obj" }
    }
    return $null
}

function Get-ProcessLockingPath {
    param([string]$Path)
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    foreach ($process in Get-Process -ErrorAction SilentlyContinue) {
        try {
            if ($process.Path -and ([System.IO.Path]::GetFullPath($process.Path).Equals($normalizedPath, [System.StringComparison]::OrdinalIgnoreCase))) { return $process }
        } catch { continue }
    }
    return $null
}

#------------------------------------------------------------------------------
# Shader compilation
#------------------------------------------------------------------------------
function Find-ShaderCompiler {
    param([string]$VulkanDirectory)
    if ($env:VULKAN_SDK) {
        $fromEnv = Join-Path $env:VULKAN_SDK "Bin\glslangValidator.exe"
        if (Test-Path -LiteralPath $fromEnv) { return $fromEnv }
    }
    $localPath = Join-Path $VulkanDirectory "Bin\glslangValidator.exe"
    if (Test-Path -LiteralPath $localPath) { return $localPath }
    if (Test-Path -LiteralPath "C:\VulkanSDK") {
        $candidate = Get-ChildItem -Path "C:\VulkanSDK" -Directory |
            Sort-Object Name -Descending |
            Select-Object -First 1 |
            ForEach-Object { Join-Path $_.FullName "Bin\glslangValidator.exe" }
        if ($candidate -and (Test-Path -LiteralPath $candidate)) { return $candidate }
    }
    $fromPathCmd = Get-Command glslangValidator.exe -ErrorAction SilentlyContinue
    if ($fromPathCmd) { return $fromPathCmd.Source }
    return $null
}

function Compile-ShaderIfNeeded {
    param([string]$ShaderCompiler, [string]$SourcePath, [string]$OutputPath)
    $needsCompile = $true
    if (Test-Path -LiteralPath $OutputPath) {
        $srcTime = (Get-Item -LiteralPath $SourcePath).LastWriteTimeUtc
        $outTime = (Get-Item -LiteralPath $OutputPath).LastWriteTimeUtc
        if ($outTime -ge $srcTime) { $needsCompile = $false }
    }
    if ($needsCompile) {
        Write-Host "[SHADER] $(Split-Path $SourcePath -Leaf)"
        & $ShaderCompiler -V $SourcePath -o $OutputPath
        if ($LASTEXITCODE -ne 0) { throw "Shader compilation failed: $SourcePath" }
    } else {
        Write-Host "[SKIP]   $(Split-Path $SourcePath -Leaf)"
    }
}

function Get-RelativePathCompat {
    param([string]$BasePath, [string]$TargetPath)
    $base = ([System.IO.Path]::GetFullPath($BasePath)).TrimEnd("\", "/") + "\"
    $target = [System.IO.Path]::GetFullPath($TargetPath)
    $baseUri = New-Object System.Uri($base)
    $targetUri = New-Object System.Uri($target)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\")
}

#------------------------------------------------------------------------------
# Entry point
#------------------------------------------------------------------------------
try {
    $ProjectName   = "Filament"
    $scriptRoot    = [System.IO.Path]::GetFullPath((Split-Path $MyInvocation.MyCommand.Path -Parent))
    $workspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot "..\.."))
    $extRoot       = Resolve-FirstExistingPath -BaseDirectory "" -Candidates @(
        (Join-Path $workspaceRoot "ExternalPackages"),
        (Join-Path ([System.IO.Path]::GetFullPath((Join-Path $workspaceRoot ".."))) "ExternalPackages")
    ) -Label "ExternalPackages root"

    $Config = if ($Release) { "Release" } else { "Debug" }

    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host " $ProjectName - $Config Build"           -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    #--------------------------------------------------------------------------
    # Resolve external packages
    #--------------------------------------------------------------------------
    $vulkanDir = $null
    try {
        $vulkanDir = Resolve-FirstExistingPath -BaseDirectory $extRoot -Candidates @(
            "vulkan", "Vulkan", "VulkanSDK"
        ) -Label "Vulkan SDK (ExternalPackages\vulkan)"
    } catch {
        if ($env:VULKAN_SDK -and (Test-Path -LiteralPath $env:VULKAN_SDK)) {
            $vulkanDir = [System.IO.Path]::GetFullPath($env:VULKAN_SDK)
        } elseif (Test-Path -LiteralPath "C:\VulkanSDK") {
            $latestSdk = Get-ChildItem -Path "C:\VulkanSDK" -Directory |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($latestSdk) {
                $vulkanDir = [System.IO.Path]::GetFullPath($latestSdk.FullName)
            }
        }
    }
    if (-not $vulkanDir) {
        throw "Vulkan SDK not found. Checked ExternalPackages and system installs (VULKAN_SDK / C:\VulkanSDK)."
    }

    $imguiDir = Resolve-FirstExistingPath -BaseDirectory $extRoot -Candidates @(
        "imgui", "ImGui"
    ) -Label "ImGui (ExternalPackages\imgui)"

    Write-Info "Workspace:  $workspaceRoot"
    Write-Info "Vulkan SDK: $vulkanDir"
    Write-Info "ImGui:      $imguiDir"

    #--------------------------------------------------------------------------
    # Resolve MSVC toolchain
    #--------------------------------------------------------------------------
    $vcVarsPath = Resolve-VcVarsPath
    Import-VcVarsEnvironment -VcVarsPath $vcVarsPath

    #--------------------------------------------------------------------------
    # Output directories
    #--------------------------------------------------------------------------
    $buildDir          = Join-Path $workspaceRoot "BuildArtifacts\$ProjectName"
    $objDir            = Join-Path $buildDir "obj"
    $stateDir          = Join-Path $buildDir "state"
    $shaderOutDir      = Join-Path $buildDir "Shaders"
    $exePath           = Join-Path $buildDir "$ProjectName.exe"
    $linkSignatureFile = Join-Path $stateDir "link.sig"

    if ($Clean) {
        Write-Info "Clean build - removing: $buildDir"
        if (Test-Path -LiteralPath $buildDir) { Remove-Item -LiteralPath $buildDir -Recurse -Force }
    }

    Ensure-Directory $buildDir
    Ensure-Directory $objDir
    Ensure-Directory $stateDir
    Ensure-Directory $shaderOutDir

    #--------------------------------------------------------------------------
    # Shader compilation
    #--------------------------------------------------------------------------
    $shaderCompiler = Find-ShaderCompiler -VulkanDirectory $vulkanDir
    if ($shaderCompiler) {
        Write-Info "Shader compiler: $shaderCompiler"
        $shaderSrcDir = Join-Path $scriptRoot "Internal\ShaderSubsystem"
        if (Test-Path -LiteralPath $shaderSrcDir) {
            $shaderFiles = Get-ChildItem -LiteralPath $shaderSrcDir -Recurse -File |
                Where-Object { $_.Extension -in @(".vert", ".frag", ".comp") }

            foreach ($shader in $shaderFiles) {
                $relative = Get-RelativePathCompat -BasePath $shaderSrcDir -TargetPath $shader.FullName
                $relativeDir = Split-Path -Parent $relative
                $stem = [System.IO.Path]::GetFileNameWithoutExtension($relative)

                $outputName = switch ($shader.Extension.ToLowerInvariant()) {
                    ".vert" { "{0}_vert.spv" -f $stem }
                    ".frag" { "{0}_frag.spv" -f $stem }
                    ".comp" { "{0}.spv" -f $stem }
                    default { $null }
                }
                if (-not $outputName) { continue }

                $outDir = if ([string]::IsNullOrWhiteSpace($relativeDir)) { $shaderOutDir } else { Join-Path $shaderOutDir $relativeDir }
                Ensure-Directory $outDir
                $outputPath = Join-Path $outDir $outputName

                Compile-ShaderIfNeeded -ShaderCompiler $shaderCompiler -SourcePath $shader.FullName -OutputPath $outputPath
            }
        }
    } else {
        Write-Warn "glslangValidator.exe not found - skipping shader compilation."
    }

    #--------------------------------------------------------------------------
    # Source file lists
    #--------------------------------------------------------------------------
    $srcRoot = $scriptRoot

    $projectSourceRelPaths = @(
        "FilamentEntry.cpp",
        "Internal\Auxiliary\FilamentLog.cpp",
        "Internal\Algebra\Vector.cpp",
        "Internal\Algebra\Matrix.cpp",
        "Internal\Algebra\Quaternion.cpp",
        "Internal\Algebra\Utilities.cpp",
        "Internal\VulkanGraphicsInterface\Context\VkBootstrap.cpp",
        "Internal\VulkanGraphicsInterface\Resources\VkSwapchain.cpp",
        "Internal\VulkanGraphicsInterface\Commands\VkCommandDispatch.cpp",
        "Internal\VulkanGraphicsInterface\Sync\VkSyncPrimitives.cpp",
        "Internal\VulkanGraphicsInterface\Memory\VkMemoryReservoir.cpp",
        "Internal\VulkanGraphicsInterface\Pipeline\VkPipelineForge.cpp",
        "Internal\VulkanGraphicsInterface\Descriptors\VkDescriptorBank.cpp",
        "Internal\VulkanGraphicsInterface\Debug\VkValidation.cpp",
        "Internal\RenderSubsystem\FilamentPipeline.cpp",
        "Internal\RenderSubsystem\FilamentGBuffer.cpp",
        "Internal\RenderSubsystem\FilamentLighting.cpp",
        "Internal\RenderSubsystem\FilamentRefraction.cpp",
        "Internal\SceneContext\FilamentScene.cpp",
        "Internal\SceneContext\FilamentCamera.cpp",
        "Internal\SceneContext\Components\CameraAdapter\Optics\Lens.cpp",
        "Internal\SceneContext\Components\CameraAdapter\Optics\Frustum.cpp",
        "Internal\SceneContext\Components\CameraAdapter\Optics\Exposure.cpp",
        "Internal\Interface\FilamentUI.cpp"
    )

    $imguiSourceRelPaths = @(
        "imgui.cpp",
        "imgui_demo.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "backends\imgui_impl_win32.cpp",
        "backends\imgui_impl_vulkan.cpp"
    )

    $allSourcePaths = @()
    foreach ($rel in $projectSourceRelPaths) {
        $full = [System.IO.Path]::GetFullPath((Join-Path $srcRoot $rel))
        if (-not (Test-Path -LiteralPath $full)) { throw "Missing source file: $full" }
        $allSourcePaths += $full
    }
    foreach ($rel in $imguiSourceRelPaths) {
        $full = [System.IO.Path]::GetFullPath((Join-Path $imguiDir $rel))
        if (-not (Test-Path -LiteralPath $full)) { throw "Missing imgui source: $full" }
        $allSourcePaths += $full
    }

    Write-Info "Found $($allSourcePaths.Count) source files."

    #--------------------------------------------------------------------------
    # Compiler configuration
    #--------------------------------------------------------------------------
    $includeDirs = @(
        (Join-Path $vulkanDir "include"),
        $imguiDir,
        (Join-Path $imguiDir "backends"),
        $srcRoot
    )
    $includeArgs = $includeDirs | ForEach-Object { "/I$_" }

    $compileArgsCommon = @(
        "/nologo", "/EHsc", "/std:c++17", "/W3", "/utf-8",
        "/DUNICODE", "/D_UNICODE",
        "/DVULKAN_HPP_NO_EXCEPTIONS", "/DNOMINMAX",
        "/DIMGUI_DEFINE_MATH_OPERATORS",

        "/showIncludes"
    )
    if ($Config -eq "Debug") {
        $compileArgsCommon += @("/Od", "/Zi", "/MDd", "/D_DEBUG", "/DDEBUG")
    } else {
        $compileArgsCommon += @("/O2", "/MD", "/DNDEBUG")
    }

    $libraries = @(
        "user32.lib",
        "gdi32.lib",
        "shell32.lib",
        "imm32.lib",
        "dwmapi.lib",
        (Join-Path $vulkanDir "Lib\vulkan-1.lib")
    )

    $trackedDependencyRoots = @(
        ([System.IO.Path]::GetFullPath($srcRoot).TrimEnd("\")   + "\"),
        ([System.IO.Path]::GetFullPath($imguiDir).TrimEnd("\")  + "\"),
        ([System.IO.Path]::GetFullPath((Join-Path $vulkanDir "include")).TrimEnd("\") + "\")
    )

    $compileSignature = Get-Sha256 -Text (@(
        ($compileArgsCommon -join " "),
        ($includeDirs -join ";"),
        ($libraries   -join ";"),
        "profile=$Config"
    ) -join "`n")

    #--------------------------------------------------------------------------
    # Incremental C++ compilation
    #--------------------------------------------------------------------------
    Write-Info "Adaptive C++ build started..."
    $compiledCount = 0
    $skippedCount  = 0
    $objectPaths   = New-Object System.Collections.Generic.List[string]

    foreach ($sourcePath in $allSourcePaths) {
        $outputKey      = Get-SourceOutputKey -SourcePath $sourcePath `
                            -ProjectRoot ([System.IO.Path]::GetFullPath($srcRoot).TrimEnd("\")   + "\") `
                            -ImguiRoot   ([System.IO.Path]::GetFullPath($imguiDir).TrimEnd("\")  + "\")
        $objectPath     = Join-Path $objDir   ([System.IO.Path]::ChangeExtension($outputKey, ".obj"))
        $dependencyFile = Join-Path $stateDir ([System.IO.Path]::ChangeExtension($outputKey, ".deps"))
        $commandFile    = Join-Path $stateDir ([System.IO.Path]::ChangeExtension($outputKey, ".cmd"))

        Ensure-Directory (Split-Path -Parent $objectPath)
        Ensure-Directory (Split-Path -Parent $dependencyFile)
        Ensure-Directory (Split-Path -Parent $commandFile)

        $reason = Get-CompileReason -ObjectPath $objectPath -DependencyFile $dependencyFile -CommandFile $commandFile -CompileSignature $compileSignature
        if ($reason) {
            Write-Host "[BUILD]  $outputKey ($reason)"
            $singleCompileArgs = @("/c") + $compileArgsCommon + $includeArgs + @("/Fo$objectPath", $sourcePath)
            if ($Config -eq "Debug") {
                $pdbPath = Join-Path $objDir ([System.IO.Path]::ChangeExtension($outputKey, ".pdb"))
                Ensure-Directory (Split-Path -Parent $pdbPath)
                $singleCompileArgs += "/Fd$pdbPath"
            }
            $compilerOutput = & cl.exe @singleCompileArgs 2>&1

            $dependencies = New-Object "System.Collections.Generic.HashSet[string]" ([System.StringComparer]::OrdinalIgnoreCase)
            [void]$dependencies.Add([System.IO.Path]::GetFullPath($sourcePath))

            foreach ($line in $compilerOutput) {
                $text = [string]$line
                if ($text -match "^\s*Note:\s+including file:\s*(.+)$") {
                    $dep = $matches[1].Trim()
                    if ($dep.Length -eq 0) { continue }
                    if (-not [System.IO.Path]::IsPathRooted($dep)) { $dep = [System.IO.Path]::GetFullPath((Join-Path $srcRoot $dep)) }
                    else { $dep = [System.IO.Path]::GetFullPath($dep) }
                    if ((Test-Path -LiteralPath $dep) -and (Test-PathUnderAnyRoot -Path $dep -Roots $trackedDependencyRoots)) { [void]$dependencies.Add($dep) }
                    continue
                }
                if ($text.Trim().Length -gt 0) { Write-Host "         $text" }
            }

            if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $sourcePath" }
            $dependencies | Sort-Object | Set-Content -LiteralPath $dependencyFile -Encoding UTF8
            Set-Content -LiteralPath $commandFile -Value $compileSignature -Encoding UTF8
            $compiledCount++
        } else {
            Write-Host "[SKIP]   $outputKey"
            $skippedCount++
        }
        $objectPaths.Add($objectPath)
    }

    #--------------------------------------------------------------------------
    # Link step
    #--------------------------------------------------------------------------
    $linkSignature = Get-Sha256 -Text (@(
        ($objectPaths -join ";"),
        ($libraries   -join ";"),
        "subsystem=windows"
    ) -join "`n")

    $linkReason = Get-LinkReason -ExecutablePath $exePath -ObjectPaths $objectPaths.ToArray() -LinkSignatureFile $linkSignatureFile -LinkSignature $linkSignature

    if ($linkReason) {
        $lockingProcess = Get-ProcessLockingPath -Path $exePath
        if ($lockingProcess) { throw "Cannot link: $ProjectName.exe is running (PID $($lockingProcess.Id)). Close it and retry." }

        Write-Info "Linking $ProjectName.exe ($linkReason)"
        $linkArgs   = @("/nologo", "/SUBSYSTEM:WINDOWS", "/ENTRY:WinMainCRTStartup", "/OUT:$exePath") + $objectPaths.ToArray() + $libraries
        if ($Config -eq "Debug") {
            $linkArgs += "/DEBUG"
        }
        $linkOutput = & link.exe @linkArgs 2>&1
        foreach ($line in $linkOutput) {
            $text = [string]$line
            if ($text.Trim().Length -gt 0) { Write-Host "         $text" }
        }
        if ($LASTEXITCODE -ne 0) { throw "Link failed." }
        Set-Content -LiteralPath $linkSignatureFile -Value $linkSignature -Encoding UTF8
    } else {
        Write-Host "[SKIP]   Link step (up to date)"
    }

    #--------------------------------------------------------------------------
    # Copy runtime DLLs
    #--------------------------------------------------------------------------
    $vulkanDll = Join-Path $vulkanDir "Bin\vulkan-1.dll"
    if (Test-Path -LiteralPath $vulkanDll) { Copy-Item -LiteralPath $vulkanDll -Destination $buildDir -Force }

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host " BUILD SUCCESSFUL"                        -ForegroundColor Green
    Write-Host " Output:  $exePath"                       -ForegroundColor Green
    Write-Host " Shaders: $shaderOutDir"                  -ForegroundColor Green
    Write-Host " Compiled: $compiledCount  Skipped: $skippedCount  Total: $($allSourcePaths.Count)" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    exit 0
}
catch {
    Write-Err $_.Exception.Message
    exit 1
}
