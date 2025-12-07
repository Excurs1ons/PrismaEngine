# 获取所有目标文件
$files = Get-ChildItem -Recurse -Include *.cpp, *.h
$total = $files.Count
$index = 0

foreach ($f in $files) {
    $index++

    # 显示进度
    Write-Progress -Activity "转换编码为 UTF-8 无 BOM" `
                   -Status "正在处理: $($f.FullName)" `
                   -PercentComplete (($index / $total) * 100)

    # 读取原内容
    $content = Get-Content $f.FullName -Raw

    # 用 .NET StreamWriter 写 UTF-8 无 BOM
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($f.FullName, $content, $utf8NoBom)
}

Write-Host "完成：已全部转换为 UTF-8 无 BOM。" -ForegroundColor Green
