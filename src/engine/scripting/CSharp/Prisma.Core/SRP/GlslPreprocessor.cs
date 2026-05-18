using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text.RegularExpressions;

namespace Prisma.SRP;

/// <summary>
/// Iris/OptiFine GLSL 预处理器。
/// 1. 解析 #include 指令（带缓存，防重复）
/// 2. 升级 #version 120 → 450 core
/// 3. #version 120 → 450 core 兼容性转换 (attribute/varying/ftransform)
/// 4. 按 #ifdef VSH / #ifdef FSH 分割源码
/// </summary>
internal static class GlslPreprocessor
{
    private static readonly Regex IncludeRegex = new(
        @"^\s*#include\s+[""<]([^""<>]+)[""<>]",
        RegexOptions.Compiled | RegexOptions.Multiline
    );

    // 循环检测：当前正在解析的文件路径链
    private static readonly HashSet<string> s_inProgress = new();

    // 解析缓存：virtualPath → 已解析内容（跨 VSH/FSH 共享）
    private static readonly Dictionary<string, string> s_cache = new();

    // 最大递归深度
    private const int MaxDepth = 64;

    // ============================================================
    // Public API
    // ============================================================

    /// <summary>从 .vsh/.fsh 入口文件预处理 GLSL。</summary>
    public static string PreprocessEntryPoint(ZipArchive archive, string entryPathInZip)
    {
        s_inProgress.Clear();
        s_cache.Clear();
        string source = ResolveFile(archive, entryPathInZip, 0);
        return ApplyCompatTransform(source);
    }

    /// <summary>从 .glsl 统一文件分离 VSH 和 FSH 源码。</summary>
    public static (string vertexSource, string fragmentSource) SplitGlslProgram(
        ZipArchive archive, string programGlslPath)
    {
        s_inProgress.Clear();
        s_cache.Clear();
        string combined = ResolveFile(archive, programGlslPath, 0);
        var (vshPart, fshPart) = SplitVshFsh(combined);
        return (
            ApplyCompatTransform(vshPart),
            ApplyCompatTransform(fshPart)
        );
    }

    // ============================================================
    // Recursive Include Resolver (with cache + depth guard)
    // ============================================================

    private static string ResolveFile(ZipArchive archive, string virtualPath, int depth)
    {
        if (depth > MaxDepth)
            throw new InvalidOperationException(
                $"GLSL include depth exceeded {MaxDepth} at: {virtualPath}");

        // 规范化路径
        string normPath = NormalizePath(virtualPath);

        // 缓存命中
        if (s_cache.TryGetValue(normPath, out var cached))
            return cached;

        // 循环检测
        if (!s_inProgress.Add(normPath))
        {
            s_cache[normPath] = "";
            return $"// [CYCLE: {normPath}]\n";
        }

        try
        {
            // 查找 zip entry
            string zipPath = normPath.StartsWith("shaders/") ? normPath : "shaders/" + normPath;
            var entry = archive.GetEntry(zipPath)
                     ?? archive.GetEntry("shaders/" + normPath)   // fallback
                     ?? archive.GetEntry(normPath);

            if (entry == null)
            {
                string msg = $"// [MISSING: {normPath}]\n";
                s_cache[normPath] = msg;
                return msg;
            }

            string source;
            using (var reader = new StreamReader(entry.Open()))
                source = reader.ReadToEnd();

            // 递归解析所有 #include
            source = IncludeRegex.Replace(source, match =>
            {
                string inc = match.Groups[1].Value;
                string resolved;
                if (inc.StartsWith("/"))
                    resolved = "shaders" + inc;
                else
                {
                    string dir = Path.GetDirectoryName(normPath)!.Replace('\\', '/');
                    resolved = dir + "/" + inc;
                }
                resolved = resolved.TrimStart('/');
                return ResolveFile(archive, resolved, depth + 1);
            });

            // 升级 #version
            source = UpgradeVersion(source);

            // 写入缓存
            s_cache[normPath] = source;
            return source;
        }
        finally
        {
            s_inProgress.Remove(normPath);
        }
    }

    private static string NormalizePath(string virtualPath)
    {
        return virtualPath.Replace('\\', '/').TrimStart('/');
    }

    private static string UpgradeVersion(string source)
    {
        return Regex.Replace(source, @"#version\s+\d+(\s+\w+)?", "#version 450 core");
    }

    // ============================================================
    // VSH/FSH Splitter
    // ============================================================

    public static (string vertexSource, string fragmentSource) SplitVshFsh(string combined)
    {
        var vLines = new List<string>();
        var fLines = new List<string>();
        var lines = combined.Split('\n');
        var stack = new List<char> { 'B' };

        foreach (var raw in lines)
        {
            string t = raw.Trim();
            char cur = stack[^1];

            if (t.StartsWith("#ifdef VSH"))
                stack.Add('V');
            else if (t.StartsWith("#ifdef FSH"))
                stack.Add('F');
            else if (t.StartsWith("#ifndef VSH"))
                stack.Add(cur == 'V' ? 'N' : 'F');
            else if (t.StartsWith("#ifndef FSH"))
                stack.Add(cur == 'F' ? 'N' : 'V');
            else if ((t.StartsWith("#if") || t.StartsWith("#ifdef ") || t.StartsWith("#ifndef ")) && !t.Contains("VSH") && !t.Contains("FSH"))
                stack.Add(cur);
            else if (t.StartsWith("#else") && stack.Count >= 2)
                stack[^1] = InvertStage(stack[^1], stack[^2]);
            else if (t.StartsWith("#endif") && stack.Count > 1)
                stack.RemoveAt(stack.Count - 1);
            else
            {
                if (cur == 'V' || cur == 'B') vLines.Add(raw);
                if (cur == 'F' || cur == 'B') fLines.Add(raw);
            }
        }

        return (string.Join("\n", vLines), string.Join("\n", fLines));
    }

    private static char InvertStage(char cur, char parent)
    {
        return parent == 'B'
            ? (cur == 'V' ? 'N' : cur == 'F' ? 'N' : 'B')
            : (cur == 'V' ? 'N' : cur == 'F' ? 'N' : parent);
    }

    // ============================================================
    // #version 120 → 450 core Compatibility Transform
    // ============================================================

    private static readonly Regex VaryingRegex = new(
        @"\bvarying\s+(?<type>\w+(?:\s+\w+)?)\s+(?<names>\w+(?:\s*,\s*\w+)*)\s*;",
        RegexOptions.Compiled
    );

    private static readonly Regex AttribRegex = new(
        @"\battribute\s+(?<type>\w+(?:\s+\w+)?)\s+(?<names>\w+(?:\s*,\s*\w+)*)\s*;",
        RegexOptions.Compiled
    );

    // 侦测 standalone uniform（非 opaque 类型）
    private static readonly Regex StandaloneUniformRegex = new(
        @"\buniform\s+(float|vec[234]|mat[34]|int|uint|bool|double)\s+(\w+)\s*;",
        RegexOptions.Compiled
    );

    public static string ApplyCompatTransform(string source)
    {
        if (!NeedsCompat(source))
            return source;

        // 根据 #define VSH / #define FSH 确定着色器阶段
        bool hasDefineVsh = source.Contains("#define VSH");
        bool hasDefineFsh = source.Contains("#define FSH");
        bool isVertex;
        if (hasDefineVsh && !hasDefineFsh)
            isVertex = true;
        else if (hasDefineFsh && !hasDefineVsh)
            isVertex = false;
        else
            isVertex = source.Contains("gl_Vertex"); // heuristic fallback

        // Pass 1: 收集 attribute/varying
        var userAttribs = new List<(string type, string name)>();
        var userVaryings = new List<(string type, string name)>();
        foreach (Match m in AttribRegex.Matches(source))
        {
            string type = m.Groups["type"].Value.Trim();
            foreach (string n in m.Groups["names"].Value.Split(',').Select(x => x.Trim()))
                userAttribs.Add((type, n));
        }
        foreach (Match m in VaryingRegex.Matches(source))
        {
            string type = m.Groups["type"].Value.Trim();
            foreach (string n in m.Groups["names"].Value.Split(',').Select(x => x.Trim()))
                userVaryings.Add((type, n));
        }

        // Pass 2: 检测内置变量
        bool hasVtx = source.Contains("gl_Vertex");
        bool hasNml = source.Contains("gl_Normal");
        bool hasClr = source.Contains("gl_Color");
        bool hasTex = source.Contains("gl_MultiTexCoord");
        bool hasTM  = source.Contains("gl_TextureMatrix");
        bool hasNM  = source.Contains("gl_NormalMatrix");
        bool hasFT  = source.Contains("ftransform");

        // Pass 3: 替换

        // 3a. attribute → 处理多变量 (attribute vec3 a, b;)
        int aLoc = 0;
        source = AttribRegex.Replace(source, m =>
        {
            string type = m.Groups["type"].Value.Trim();
            var names = m.Groups["names"].Value.Split(',').Select(x => x.Trim()).ToArray();
            var decls = new List<string>();
            foreach (string n in names)
                decls.Add($"layout(location = {aLoc++}) in {type} {n};");
            return string.Join("\n", decls);
        });

        // 3b. varying (VSH→out, FSH→in) — 处理多变量 (varying vec3 a, b;)
        int vLoc = 0;
        var vMap = new Dictionary<string, int>();
        source = VaryingRegex.Replace(source, m =>
        {
            string type = m.Groups["type"].Value.Trim();
            var names = m.Groups["names"].Value.Split(',').Select(x => x.Trim()).ToArray();
            var decls = new List<string>();
            foreach (string n in names)
            {
                if (!vMap.TryGetValue(n, out int loc))
                { loc = vLoc++; vMap[n] = loc; }
                decls.Add($"layout(location = {loc}) {(isVertex ? "out" : "in")} {type} {n};");
            }
            return string.Join("\n", decls);
        });

        // 3c. ftransform
        if (hasFT)
            source = source.Replace("ftransform()",
                "(_glCompat.gbufferProjection * _glCompat.gbufferModelView * vec4(_glVertex, 1.0))");

        // 3d. texture2D → texture
        source = Regex.Replace(source, @"\btexture2DLod\b", "textureLod");
        source = Regex.Replace(source, @"\btexture2DGrad\b", "textureGrad");
        source = Regex.Replace(source, @"\btexture2D\b", "texture");

        // 3e. gl_FragData[N]
        var fdu = new HashSet<int>();
        source = Regex.Replace(source, @"gl_FragData\[(\d+)\]", m =>
        { int i = int.Parse(m.Groups[1].Value); fdu.Add(i); return $"fragData{i}"; });

        // 3f. gl_FragColor
        bool hasFC = source.Contains("gl_FragColor");
        if (hasFC) source = source.Replace("gl_FragColor", "fragColor0");

        // 3g. 矩阵名 → push constant
        source = source.Replace("gl_ProjectionMatrix", "_glCompat.gbufferProjection");
        source = source.Replace("gl_ModelViewMatrix", "_glCompat.gbufferModelView");

        // 3h. 内置顶点属性名
        if (hasVtx) source = ReplaceWord(source, "gl_Vertex", "_glVertex");
        if (hasNml) source = ReplaceWord(source, "gl_Normal", "_glNormal");
        if (hasClr) source = ReplaceWord(source, "gl_Color", "_glColor");
        if (hasTex) source = ReplaceWord(source, "gl_MultiTexCoord0", "_glMultiTexCoord0");

        // Pass 4: 注入声明块

        string inject = "\n// ---- GLSL 120→450 compat ----\n";

        // Push constant block (128 bytes = 2×mat4)
        inject += "layout(push_constant) uniform GLCompat {\n";
        inject += "    mat4 gbufferProjection;\n";
        inject += "    mat4 gbufferModelView;\n";
        inject += "} _glCompat;\n\n";

        // 内置顶点属性 (location after user attribs)
        if (hasVtx) inject += $"layout(location = {aLoc++}) in vec3 _glVertex;\n";
        if (hasNml) inject += $"layout(location = {aLoc++}) in vec3 _glNormal;\n";
        if (hasClr) inject += $"layout(location = {aLoc++}) in vec4 _glColor;\n";
        if (hasTex) inject += $"layout(location = {aLoc++}) in vec2 _glMultiTexCoord0;\n";

        // Extra uniforms (NormalMatrix, TextureMatrix) — 不可用 gl_ 前缀，450 core 保留
        if (hasNM || hasTM)
        {
            inject += "layout(std140, binding = 1) uniform GLCompatExtra {\n";
            if (hasNM) inject += "    mat3 normalMatrix;\n";
            if (hasTM) inject += "    mat4 textureMatrix[8];\n";
            inject += "} _glExtra;\n\n";
            if (hasNM) source = ReplaceWord(source, "gl_NormalMatrix", "_glExtra.normalMatrix");
            if (hasTM) source = ReplaceWord(source, "gl_TextureMatrix", "_glExtra.textureMatrix");
        }

        // FragData outputs (fragment only) — 跳过 location 0 如果有 gl_FragColor 冲突
        if (fdu.Count > 0 && !isVertex)
        {
            foreach (int i in fdu)
            {
                if (hasFC && i == 0) continue; // gl_FragColor 已占用 location 0
                inject += $"layout(location = {i}) out vec4 fragData{i};\n";
            }
        }

        // gl_FragColor output → location = 0
        if (hasFC && !isVertex)
            inject += "layout(location = 0) out vec4 fragColor0;\n";

        inject += "// ---- end compat ----\n";

        // ------ Pass 4b: 给 BSL 的 standalone uniform 加 explicit binding ------
        // Vulkan 不允许 uniform 裸声明（必须 layout(binding=N) 或在 block 内）
        int nextBinding = 2; // binding 0=GLCompat(通过 push constant), 1=GLCompatExtra
        source = StandaloneUniformRegex.Replace(source, m =>
        {
            string type = m.Groups[1].Value;
            string name = m.Groups[2].Value;
            int b = nextBinding++;
            return $"layout(binding = {b}) uniform {type} {name};";
        });

        source = Regex.Replace(source,
            @"(#version\s+450\s+core\s*)",
            $"$1{inject}");

        return source;
    }

    private static bool NeedsCompat(string src)
    {
        return src.Contains("attribute ") || src.Contains("varying ") ||
               src.Contains("ftransform") || src.Contains("texture2D") ||
               src.Contains("gl_FragData") || src.Contains("gl_FragColor") ||
               src.Contains("gl_ModelViewMatrix") || src.Contains("gl_Vertex");
    }

    private static string ReplaceWord(string text, string word, string replacement)
    {
        return Regex.Replace(text, $@"(?<!\w){Regex.Escape(word)}(?!\w)", replacement);
    }
}
