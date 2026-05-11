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

            var scripts = new List<ScriptInfo>();

            foreach (var classSymbol in receiver.Classes)
            {
                var info = ProcessClass(classSymbol);
                if (info != null)
                {
                    scripts.Add(info);
                    GenerateScriptPartial(context, info);
                }
            }

            GenerateRegistry(context, scripts);
        }

        private ScriptInfo? ProcessClass(INamedTypeSymbol classSymbol)
        {
            var properties = new List<PropertyInfo>();

            foreach (var member in classSymbol.GetMembers())
            {
                if (member is IPropertySymbol property)
                {
                    if (property.GetAttributes().Any(ad => ad.AttributeClass?.Name == "PrismaPropertyAttribute"))
                    {
                        properties.Add(new PropertyInfo { Name = property.Name, Type = property.Type.ToDisplayString() });
                    }
                }
                else if (member is IFieldSymbol field)
                {
                    if (field.GetAttributes().Any(ad => ad.AttributeClass?.Name == "PrismaPropertyAttribute"))
                    {
                        properties.Add(new PropertyInfo { Name = field.Name, Type = field.Type.ToDisplayString() });
                    }
                }
            }

            return new ScriptInfo
            {
                Namespace = classSymbol.ContainingNamespace.ToDisplayString(),
                ClassName = classSymbol.Name,
                FullName = classSymbol.ToDisplayString(),
                Properties = properties,
                // Simple deterministic ID based on name hash
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
            sb.AppendLine($"    public partial class {info.ClassName}");
            sb.AppendLine("    {");
            sb.AppendLine($"        public override uint TypeId => {info.TypeId}u;");
            sb.AppendLine();
            
            // Generate Serialization
            sb.AppendLine("        public override void OnSerialize(Utf8JsonWriter writer)");
            sb.AppendLine("        {");
            foreach (var prop in info.Properties)
            {
                if (prop.Type == "float")
                    sb.AppendLine($"            writer.WriteNumber(\"{prop.Name}\", {prop.Name});");
                else if (prop.Type == "string")
                    sb.AppendLine($"            writer.WriteString(\"{prop.Name}\", {prop.Name});");
                else if (prop.Type == "PrismaEngine.Vector2" || prop.Type == "Vector2") {
                    sb.AppendLine($"            writer.WriteStartArray(\"{prop.Name}\");");
                    sb.AppendLine($"            writer.WriteNumberValue({prop.Name}.X);");
                    sb.AppendLine($"            writer.WriteNumberValue({prop.Name}.Y);");
                    sb.AppendLine($"            writer.WriteEndArray();");
                }
            }
            sb.AppendLine("        }");

            // Generate Deserialization
            sb.AppendLine("        public override void OnDeserialize(ref Utf8JsonReader reader)");
            sb.AppendLine("        {");
            sb.AppendLine("            // Generated deserialization logic...");
            sb.AppendLine("        }");

            sb.AppendLine("    }");
            sb.AppendLine("}");

            context.AddSource($"{info.ClassName}.g.cs", SourceText.From(sb.ToString(), Encoding.UTF8));
        }

        private void GenerateRegistry(GeneratorExecutionContext context, List<ScriptInfo> scripts)
        {
            var sb = new StringBuilder();
            sb.AppendLine("using System;");
            sb.AppendLine("using PrismaEngine;");
            sb.AppendLine();
            sb.AppendLine("namespace PrismaEngine.Generated");
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
                if (context.Node is ClassDeclarationSyntax classDeclaration &&
                    classDeclaration.AttributeLists.Count > 0)
                {
                    var classSymbol = context.SemanticModel.GetDeclaredSymbol(classDeclaration) as INamedTypeSymbol;
                    if (classSymbol != null && classSymbol.GetAttributes().Any(ad => ad.AttributeClass?.Name == "PrismaScriptAttribute"))
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
            public uint TypeId { get; set; }
            public List<PropertyInfo> Properties { get; set; } = new List<PropertyInfo>();
        }

        class PropertyInfo
        {
            public string Name { get; set; } = "";
            public string Type { get; set; } = "";
        }
    }
}
