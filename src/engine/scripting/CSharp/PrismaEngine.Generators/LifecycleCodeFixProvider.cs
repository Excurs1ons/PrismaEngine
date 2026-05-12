using System.Collections.Immutable;
using System.Composition;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CodeActions;
using Microsoft.CodeAnalysis.CodeFixes;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace PrismaEngine.Generators
{
    [ExportCodeFixProvider(LanguageNames.CSharp, Name = nameof(LifecycleCodeFixProvider)), Shared]
    public class LifecycleCodeFixProvider : CodeFixProvider
    {
        public sealed override ImmutableArray<string> FixableDiagnosticIds => 
            ImmutableArray.Create(LifecycleAnalyzer.DiagnosticId, PartialClassAnalyzer.DiagnosticId);

        public sealed override FixAllProvider GetFixAllProvider() => WellKnownFixAllProviders.BatchFixer;

        public sealed override async Task RegisterCodeFixesAsync(CodeFixContext context)
        {
            var root = await context.Document.GetSyntaxRootAsync(context.CancellationToken).ConfigureAwait(false);
            var diagnostic = context.Diagnostics.First();
            
            // 情况 1：修复缺失的 override
            if (diagnostic.Id == LifecycleAnalyzer.DiagnosticId)
            {
                var diagnosticSpan = diagnostic.Location.SourceSpan;
                var methodDeclaration = root.FindToken(diagnosticSpan.Start).Parent.AncestorsAndSelf().OfType<MethodDeclarationSyntax>().First();

                context.RegisterCodeFix(
                    CodeAction.Create(
                        title: "Add 'override' modifier",
                        createChangedDocument: c => AddOverrideModifierAsync(context.Document, methodDeclaration, c),
                        equivalenceKey: "Add 'override' modifier"),
                    diagnostic);
            }
            // 情况 2：在类上提供“生成生命周期模板”
            else if (diagnostic.Id == PartialClassAnalyzer.DiagnosticId)
            {
                var diagnosticSpan = diagnostic.Location.SourceSpan;
                var classDeclaration = root.FindToken(diagnosticSpan.Start).Parent.AncestorsAndSelf().OfType<ClassDeclarationSyntax>().First();

                context.RegisterCodeFix(
                    CodeAction.Create(
                        title: "Generate Prisma Lifecycle (Start/Update)",
                        createChangedDocument: c => GenerateLifecycleAsync(context.Document, classDeclaration, c),
                        equivalenceKey: "Generate Prisma Lifecycle"),
                    diagnostic);
            }
        }

        private async Task<Document> AddOverrideModifierAsync(Document document, MethodDeclarationSyntax methodDeclaration, CancellationToken cancellationToken)
        {
            var overrideModifier = SyntaxFactory.Token(SyntaxKind.OverrideKeyword);
            var newModifiers = methodDeclaration.Modifiers.Add(overrideModifier);
            var newMethodDeclaration = methodDeclaration.WithModifiers(newModifiers);

            var root = await document.GetSyntaxRootAsync(cancellationToken).ConfigureAwait(false);
            var newRoot = root.ReplaceNode(methodDeclaration, newMethodDeclaration);

            return document.WithSyntaxRoot(newRoot);
        }

        private async Task<Document> GenerateLifecycleAsync(Document document, ClassDeclarationSyntax classDeclaration, CancellationToken cancellationToken)
        {
            // 生成 OnStart 和 OnUpdate
            var onStart = SyntaxFactory.MethodDeclaration(
                SyntaxFactory.PredefinedType(SyntaxFactory.Token(SyntaxKind.VoidKeyword)), "OnStart")
                .AddModifiers(SyntaxFactory.Token(SyntaxKind.PublicKeyword), SyntaxFactory.Token(SyntaxKind.OverrideKeyword))
                .WithBody(SyntaxFactory.Block());

            var onUpdate = SyntaxFactory.MethodDeclaration(
                SyntaxFactory.PredefinedType(SyntaxFactory.Token(SyntaxKind.VoidKeyword)), "OnUpdate")
                .AddModifiers(SyntaxFactory.Token(SyntaxKind.PublicKeyword), SyntaxFactory.Token(SyntaxKind.OverrideKeyword))
                .AddParameterListParameters(
                    SyntaxFactory.Parameter(SyntaxFactory.Identifier("time")).WithType(SyntaxFactory.ParseTypeName("TimeContext")),
                    SyntaxFactory.Parameter(SyntaxFactory.Identifier("input")).WithType(SyntaxFactory.ParseTypeName("InputContext")))
                .WithBody(SyntaxFactory.Block());

            // 检查是否已经存在
            var existingMethods = classDeclaration.Members.OfType<MethodDeclarationSyntax>().Select(m => m.Identifier.Text).ToList();
            var newMembers = classDeclaration.Members;
            
            if (!existingMethods.Contains("OnStart")) newMembers = newMembers.Add(onStart);
            if (!existingMethods.Contains("OnUpdate")) newMembers = newMembers.Add(onUpdate);

            // 同时也补全 partial (因为这个 Fix 是在 PartialAnalyzer 触发的)
            var newModifiers = classDeclaration.Modifiers;
            if (!newModifiers.Any(SyntaxKind.PartialKeyword))
                newModifiers = newModifiers.Add(SyntaxFactory.Token(SyntaxKind.PartialKeyword));

            var newClassDeclaration = classDeclaration.WithMembers(newMembers).WithModifiers(newModifiers);

            var root = await document.GetSyntaxRootAsync(cancellationToken).ConfigureAwait(false);
            var newRoot = root.ReplaceNode(classDeclaration, newClassDeclaration);

            return document.WithSyntaxRoot(newRoot);
        }
    }
}
