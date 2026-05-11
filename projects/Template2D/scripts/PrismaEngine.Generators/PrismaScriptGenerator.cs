using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace PrismaEngine.Generators
{
    [Generator]
    public class PrismaScriptGenerator : ISourceGenerator
    {
        public void Initialize(GeneratorInitializationContext context)
        {
            // Register a syntax receiver that will be created for each compilation
            context.RegisterForSyntaxNotifications(() => new SyntaxReceiver());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is SyntaxReceiver receiver))
                return;

            var scriptsToRegister = new List<ScriptInfo>();

            foreach (var classSymbol in receiver.Classes)
            {
                var info = ProcessClass(classSymbol);
                if (info != null)
                {
                    GenerateScriptPartial(context, info);
                    
                    // 只有是 Script 的类才需要注册到 ScriptRegistry
                    if (info.IsScript)
                    {
                        scriptsToRegister.Add(info);
                    }
                }
            }

            GenerateRegistry(context, scriptsToRegister);
        }

        private ScriptInfo? ProcessClass(INamedTypeSymbol classSymbol)
        {
            bool isScript = false;
            var baseType = classSymbol.BaseType;
            while (baseType != null)
            {
                if (baseType.Name == "Script") { isScript = true; break; }
                baseType = baseType.BaseType;
            }

            // 提取完整的继承列表 / Extract full inheritance list
            var baseList = new List<string>();
            if (classSymbol.BaseType != null && classSymbol.BaseType.SpecialType != SpecialType.System_Object)
            {
                baseList.Add(classSymbol.BaseType.ToDisplayString());
            }
            foreach (var iface in classSymbol.Interfaces)
            {
                baseList.Add(iface.ToDisplayString());
            }

            bool isSerializable = classSymbol.GetAttributes().Any(ad => ad.AttributeClass?.Name == "SerializableAttribute");

            var properties = new List<PropertyInfo>();
            if (isSerializable)
            {
                foreach (var member in classSymbol.GetMembers())
                {
                    if (member.GetAttributes().Any(ad => ad.AttributeClass?.Name == "NonSerializedAttribute"))
                        continue;

                    bool shouldSerialize = false;
                    if (member is IFieldSymbol field)
                    {
                        if (field.IsConst) continue;
                        if (field.DeclaredAccessibility == Accessibility.Public || 
                            field.GetAttributes().Any(ad => ad.AttributeClass?.Name == "SerializedFieldAttribute"))
                        {
                            shouldSerialize = true;
                        }
                    }
                    else if (member is IPropertySymbol property)
                    {
                        if (property.GetAttributes().Any(ad => ad.AttributeClass?.Name == "SerializedPropertyAttribute"))
                        {
                            if (!property.IsIndexer && property.GetMethod != null) shouldSerialize = true;
                        }
                    }

                    if (shouldSerialize)
                    {
                        var type = (member is IFieldSymbol f) ? f.Type : ((IPropertySymbol)member).Type;
                        properties.Add(new PropertyInfo { Name = member.Name, Type = type.ToDisplayString() });
                    }
                }
            }

            return new ScriptInfo
            {
                Namespace = classSymbol.ContainingNamespace.ToDisplayString(),
                ClassName = classSymbol.Name,
                FullName = classSymbol.ToDisplayString(),
                BaseList = string.Join(", ", baseList),
                Properties = properties,
                IsScript = isScript,
                IsSerializable = isSerializable,
                TypeId = (uint)Math.Abs(classSymbol.ToDisplayString().GetHashCode())
            };
        }

        private void GenerateScriptPartial(GeneratorExecutionContext context, ScriptInfo info)
        {
            var sb = new StringBuilder();
            sb.AppendLine("using System;");
            sb.AppendLine("using PrismaEngine;");
            sb.AppendLine("using System.Text.Json;");
            sb.AppendLine();
            sb.AppendLine($"namespace {info.Namespace}");
            sb.AppendLine("{");
            
            // 在 partial 类声明中包含基类和接口
            sb.Append($"    public partial class {info.ClassName}");
            if (!string.IsNullOrEmpty(info.BaseList))
            {
                sb.Append($" : {info.BaseList}");
            }
            sb.AppendLine();
            sb.AppendLine("    {");

            // 1. 如果是 Script，生成 TypeId / If Script, generate TypeId
            if (info.IsScript)
            {
                sb.AppendLine($"        public override uint TypeId => {info.TypeId}u;");
                sb.AppendLine();
            }

            // 2. 如果标记了 Serializable，生成序列化覆盖 / If Serializable, generate serialization overrides
            if (info.IsSerializable && info.IsScript)
            {
                sb.AppendLine("        public override void OnSerialize(Utf8JsonWriter writer)");
                sb.AppendLine("        {");
                foreach (var prop in info.Properties)
                {
                    if (prop.Type == "float") sb.AppendLine($"            writer.WriteNumber(\"{prop.Name}\", {prop.Name});");
                    else if (prop.Type == "string") sb.AppendLine($"            writer.WriteString(\"{prop.Name}\", {prop.Name});");
                    else if (prop.Type == "PrismaEngine.Vector2" || prop.Type == "Vector2") {
                        sb.AppendLine($"            writer.WriteStartArray(\"{prop.Name}\");");
                        sb.AppendLine($"            writer.WriteNumberValue({prop.Name}.X);");
                        sb.AppendLine($"            writer.WriteNumberValue({prop.Name}.Y);");
                        sb.AppendLine($"            writer.WriteEndArray();");
                    }
                }
                sb.AppendLine("        }");
                sb.AppendLine();
                sb.AppendLine("        public override void OnDeserialize(ref Utf8JsonReader reader) { }");
            }

            sb.AppendLine("    }");
            sb.AppendLine("}");

            context.AddSource($"{info.ClassName}.g.cs", SourceText.From(sb.ToString(), Encoding.UTF8));
        }

        private void GenerateRegistry(GeneratorExecutionContext context, List<ScriptInfo> scripts)
        {
            var assemblyName = context.Compilation.AssemblyName?.Replace(".", "") ?? "Unknown";
            var sb = new StringBuilder();
            sb.AppendLine("using System;");
            sb.AppendLine("using PrismaEngine;");
            sb.AppendLine();
            sb.AppendLine($"namespace {assemblyName}.Generated");
            sb.AppendLine("{");
            sb.AppendLine("    public static class ScriptRegistry");
            sb.AppendLine("    {");
            sb.AppendLine("        public static void RegisterAll()");
            sb.AppendLine("        {");
            foreach (var script in scripts)
            {
                sb.AppendLine($"            PrismaEngine.ScriptRegistry.Register<{script.FullName}>({script.TypeId}u);");
            }
            sb.AppendLine("        }");
            sb.AppendLine("    }");
            sb.AppendLine("}");

            context.AddSource("ScriptRegistry.g.cs", SourceText.From(sb.ToString(), Encoding.UTF8));
        }

        class SyntaxReceiver : ISyntaxContextReceiver
        {
            public List<INamedTypeSymbol> Classes { get; } = new List<INamedTypeSymbol>();

            public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
            {
                if (context.Node is ClassDeclarationSyntax classDeclaration)
                {
                    var classSymbol = context.SemanticModel.GetDeclaredSymbol(classDeclaration) as INamedTypeSymbol;
                    if (classSymbol == null) return;

                    // 检查是否有 [Serializable] 特性
                    bool hasSerializable = classSymbol.GetAttributes().Any(ad => ad.AttributeClass?.Name == "SerializableAttribute");

                    // 检查是否继承自 Script
                    bool inheritsFromScript = false;
                    var baseType = classSymbol.BaseType;
                    while (baseType != null)
                    {
                        if (baseType.Name == "Script")
                        {
                            inheritsFromScript = true;
                            break;
                        }
                        baseType = baseType.BaseType;
                    }

                    if (hasSerializable || inheritsFromScript)
                    {
                        Classes.Add(classSymbol);
                    }
                }
            }
        }

        class ScriptInfo
        {
            public string Namespace { get; set; } = "";
            public string ClassName { get; set; } = "";
            public string FullName { get; set; } = "";
            public string BaseList { get; set; } = "";
            public uint TypeId { get; set; }
            public bool IsScript { get; set; }
            public bool IsSerializable { get; set; }
            public List<PropertyInfo> Properties { get; set; } = new List<PropertyInfo>();
        }

        class PropertyInfo
        {
            public string Name { get; set; } = "";
            public string Type { get; set; } = "";
        }
    }
}
