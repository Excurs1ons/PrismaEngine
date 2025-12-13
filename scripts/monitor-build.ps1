# GitHub Actions 构建状态监控脚本
# 使用方法: .\scripts\monitor-build.ps1 [workflow-name] [branch]

param(
    [Parameter(Mandatory=$false)]
    [string]$Workflow = "ci.yml",

    [Parameter(Mandatory=$false)]
    [string]$Branch = "main"
)

# 配置 GitHub CLI 路径
$ghPath = "C:/Program Files/GitHub CLI/gh.exe"

# 颜色输出函数
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "GitHub Actions 构建状态监控" -ForegroundColor Cyan
Write-Host "仓库: Excurs1ons/PrismaEngine" -ForegroundColor Cyan
Write-Host "工作流: $Workflow" -ForegroundColor Cyan
Write-Host "分支: $Branch" -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan

# 获取最新的运行
Write-Host "`n正在获取最新的构建运行..." -ForegroundColor Yellow
try {
    $runs = & $ghPath run list --repo Excurs1ons/PrismaEngine --workflow $Workflow --branch $Branch --limit 1 --json status,conclusion,createdAt,displayTitle,url,headBranch,databaseId
    $run = $runs | ConvertFrom-Json | Select-Object -First 1

    if (-not $run) {
        Write-ColorOutput Red "未找到任何构建运行"
        exit 1
    }

    # 显示当前构建状态
    Write-Host "`n当前构建信息:" -ForegroundColor Green
    Write-Host "------------------------" -ForegroundColor Gray
    Write-Host "标题: $($run.displayTitle)" -ForegroundColor White
    Write-Host "分支: $($run.headBranch)" -ForegroundColor White
    Write-Host "创建时间: $($run.createdAt)" -ForegroundColor White
    Write-Host "URL: $($run.url)" -ForegroundColor Blue

    # 状态显示
    switch ($run.status) {
        "queued" {
            Write-Host "状态: 等待中 (Queued)" -ForegroundColor Yellow
        }
        "in_progress" {
            Write-Host "状态: 进行中 (In Progress)" -ForegroundColor Blue
        }
        "completed" {
            if ($run.conclusion -eq "success") {
                Write-Host "状态: 成功 (Success)" -ForegroundColor Green
            } elseif ($run.conclusion -eq "failure") {
                Write-Host "状态: 失败 (Failure)" -ForegroundColor Red
            } elseif ($run.conclusion -eq "cancelled") {
                Write-Host "状态: 已取消 (Cancelled)" -ForegroundColor Gray
            } else {
                Write-Host "状态: 已完成 ($($run.conclusion))" -ForegroundColor Yellow
            }
        }
        default {
            Write-Host "状态: $($run.status)" -ForegroundColor Yellow
        }
    }

    # 如果正在进行，监控实时状态
    if ($run.status -eq "in_progress" -or $run.status -eq "queued") {
        Write-Host "`n正在监控构建状态... (按 Ctrl+C 停止)" -ForegroundColor Cyan
        Write-Host "----------------------------------" -ForegroundColor Gray

        $runId = $run.databaseId
        $lastStatus = $run.status

        while ($true) {
            # 获取实时日志
            try {
                $runInfo = & $ghPath run view $runId --repo Excurs1ons/PrismaEngine --json status,conclusion,jobs

                $runData = $runInfo | ConvertFrom-Json

                # 显示作业状态
                Write-Host "`n[$(Get-Date -Format 'HH:mm:ss')] 作业状态:" -ForegroundColor White

                foreach ($job in $runData.jobs) {
                    switch ($job.status) {
                        "queued" { $color = "Yellow" }
                        "in_progress" { $color = "Blue" }
                        "completed" {
                            if ($job.conclusion -eq "success") { $color = "Green" }
                            elseif ($job.conclusion -eq "failure") { $color = "Red" }
                            elseif ($job.conclusion -eq "cancelled") { $color = "Gray" }
                            else { $color = "Yellow" }
                        }
                        default { $color = "Yellow" }
                    }

                    $statusText = if ($job.status -eq "completed") { "$($job.status) - $($job.conclusion)" } else { $job.status }
                    Write-Host "  - $($job.name): $statusText" -ForegroundColor $color
                }

                # 检查是否完成
                if ($runData.status -eq "completed") {
                    Write-Host "`n构建完成!" -ForegroundColor Green
                    Write-Host "结论: $($runData.conclusion)" -ForegroundColor $(if ($runData.conclusion -eq "success") { "Green" } else { "Red" })

                    # 打开构建页面
                    Write-Host "`n是否在浏览器中打开构建页面? (Y/n)" -ForegroundColor Yellow
                    $response = Read-Host
                    if ($response -ne "n") {
                        Start-Process $run.url
                    }
                    break
                }

                Start-Sleep -Seconds 10
            }
            catch {
                Write-ColorOutput Red "获取构建状态时出错: $_"
                Start-Sleep -Seconds 5
            }
        }
    }
    else {
        # 已完成的构建，询问是否查看日志
        Write-Host "`n是否查看构建日志? (Y/n)" -ForegroundColor Yellow
        $response = Read-Host
        if ($response -ne "n") {
            & $ghPath run view $($run.databaseId) --repo Excurs1ons/PrismaEngine --log
        }
    }
}
catch {
    Write-ColorOutput Red "错误: $_"
    Write-Host "`n请确保:" -ForegroundColor Yellow
    Write-Host "1. GitHub CLI 已正确安装和配置" -ForegroundColor Gray
    Write-Host "2. 有访问该仓库的权限" -ForegroundColor Gray
    Write-Host "3. 工作流名称和分支名称正确" -ForegroundColor Gray
    exit 1
}