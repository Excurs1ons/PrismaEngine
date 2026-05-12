using System.Collections.Immutable;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Diagnostics;

namespace Prisma.Generators
{
    [DiagnosticAnalyzer(LanguageNames.CSharp)]
    public class LifecycleAnalyzer : DiagnosticAnalyzer
    {
        public const string DiagnosticId = "PRISMA002";

        private static readonly LocalizableString Title = "Missing override keyword";
        private static readonly LocalizableString MessageFormat = "Method '{0}' matches a Prisma lifecycle name but is missing the 'override' modifier. It will not be called by the engine.";
        private static readonly LocalizableString Description = "Lifecycle methods like OnUpdate or OnStart must be declared with 'override'.";
        private const string Category = "Usage";

        private static readonly DiagnosticDescriptor Rule = new DiagnosticDescriptor(
            DiagnosticId, Title, MessageFormat, Category, DiagnosticSeverity.Warning, isEnabledByDefault: true, description: Description);

        private static readonly string[] LifecycleNames = { "OnCreate", "OnStart", "OnUpdate", "OnLateUpdate", "OnDestroy" };

        public override ImmutableArray<DiagnosticDescriptor> SupportedDiagnostics => ImmutableArray.Create(Rule);

        public override void Initialize(AnalysisContext context)
        {
            context.ConfigureGeneratedCodeAnalysis(GeneratedCodeAnalysisFlags.None);
            context.EnableConcurrentExecution();
            context.RegisterSyntaxNodeAction(AnalyzeMethod, SyntaxKind.MethodDeclaration);
        }

        private void AnalyzeMethod(SyntaxNodeAnalysisContext context)
        {
            var methodDeclaration = (MethodDeclarationSyntax)context.Node;
            var methodName = methodDeclaration.Identifier.Text;

            if (!LifecycleNames.Contains(methodName)) return;

            // 检查所在类是否继承�?Script
            var classDeclaration = methodDeclaration.Parent as ClassDeclarationSyntax;
            if (classDeclaration == null) return;

            var symbol = context.SemanticModel.GetDeclaredSymbol(classDeclaration) as INamedTypeSymbol;
            if (symbol == null) return;

            bool isScript = false;
            var baseType = symbol.BaseType;
            while (baseType != null)
            {
                if (baseType.Name == "Script") { isScript = true; break; }
                baseType = baseType.BaseType;
            }

            if (!isScript) return;

            // 如果已经标记�?override，则正常
            if (methodDeclaration.Modifiers.Any(SyntaxKind.OverrideKeyword)) return;

            // 检查签名是否匹配（简单检查参数数量）
            int paramCount = methodDeclaration.ParameterList.Parameters.Count;
            if (methodName == "OnUpdate" || methodName == "OnLateUpdate")
            {
                if (paramCount != 2) return;
            }
            else
            {
                if (paramCount != 0) return;
            }

            var diagnostic = Diagnostic.Create(Rule, methodDeclaration.Identifier.GetLocation(), methodName);
            context.ReportDiagnostic(diagnostic);
        }
    }
}
