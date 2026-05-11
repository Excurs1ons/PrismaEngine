using System.Collections.Immutable;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Diagnostics;

namespace PrismaEngine.Generators
{
    [DiagnosticAnalyzer(LanguageNames.CSharp)]
    public class PartialClassAnalyzer : DiagnosticAnalyzer
    {
        public const string DiagnosticId = "PRISMA001";

        private static readonly LocalizableString Title = "Script class must be partial";
        private static readonly LocalizableString MessageFormat = "The class '{0}' must be declared as partial to support Prisma source generation (TypeId, Serialization, etc.)";
        private static readonly LocalizableString Description = "Classes inheriting from Script or using Prisma attributes must have the 'partial' modifier.";
        private const string Category = "Usage";

        private static readonly DiagnosticDescriptor Rule = new DiagnosticDescriptor(
            DiagnosticId, Title, MessageFormat, Category, DiagnosticSeverity.Error, isEnabledByDefault: true, description: Description);

        public override ImmutableArray<DiagnosticDescriptor> SupportedDiagnostics => ImmutableArray.Create(Rule);

        public override void Initialize(AnalysisContext context)
        {
            context.ConfigureGeneratedCodeAnalysis(GeneratedCodeAnalysisFlags.None);
            context.EnableConcurrentExecution();
            context.RegisterSyntaxNodeAction(AnalyzeNode, SyntaxKind.ClassDeclaration);
        }

        private void AnalyzeNode(SyntaxNodeAnalysisContext context)
        {
            var classDeclaration = (ClassDeclarationSyntax)context.Node;
            
            // 1. 语法检查 (最鲁棒)：检查源代码中是否直接写了 ": Script"
            // Syntax check: Check if the source code contains ": Script"
            bool appearsToBeScript = classDeclaration.BaseList?.Types.Any(t => 
                t.ToString().EndsWith("Script")) ?? false;

            // 2. 语义检查 (更精确)：检查真实的符号继承链
            // Semantic check: Check the actual symbol inheritance chain
            bool isScript = false;
            var symbol = context.SemanticModel.GetDeclaredSymbol(classDeclaration) as INamedTypeSymbol;
            
            if (symbol != null)
            {
                var baseType = symbol.BaseType;
                while (baseType != null)
                {
                    if (baseType.Name == "Script")
                    {
                        isScript = true;
                        break;
                    }
                    baseType = baseType.BaseType;
                }
            }

            // 如果两者都不符合，则跳过
            if (!appearsToBeScript && !isScript) return;

            // 如果已经标记了 partial，则跳过
            if (classDeclaration.Modifiers.Any(SyntaxKind.PartialKeyword))
            {
                return;
            }

            // 报告错误
            var diagnostic = Diagnostic.Create(Rule, classDeclaration.Identifier.GetLocation(), classDeclaration.Identifier.Text);
            context.ReportDiagnostic(diagnostic);
        }
    }
}
