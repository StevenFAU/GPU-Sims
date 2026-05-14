// extract_and_find.ts - TS compiler API helper for cat2.public-symbol-used-ts.
//
// Reads common/common-web/tsconfig.json, locates index.ts, enumerates
// public exports, and for each export walks every project source file
// to find references. Emits JSON to stdout for the Python check to consume.
//
// Usage: node extract_and_find.js <repo_root>
//
// All diagnostic messages go to stderr; stdout is pure JSON.

import * as ts from 'typescript';
import * as path from 'path';
import * as fs from 'fs';

interface ReferenceSite {
    file: string;
    line: number;
}

interface SymbolInfo {
    name: string;
    kind: string;
    file: string;
    line: number;
    references: ReferenceSite[];
}

interface Output {
    symbols: SymbolInfo[];
    diagnostics: string[];
}

function fail(msg: string, code = 1): never {
    process.stderr.write(`extract_and_find: ${msg}\n`);
    process.exit(code);
}

function describeKind(symbol: ts.Symbol): string {
    const flags = symbol.flags;
    if (flags & ts.SymbolFlags.Class) return 'class';
    if (flags & ts.SymbolFlags.Interface) return 'interface';
    if (flags & ts.SymbolFlags.TypeAlias) return 'type';
    if (flags & ts.SymbolFlags.Enum) return 'enum';
    if (flags & ts.SymbolFlags.Function) return 'function';
    if (flags & ts.SymbolFlags.Method) return 'method';
    if (flags & ts.SymbolFlags.Property) return 'property';
    if (flags & ts.SymbolFlags.Variable) return 'variable';
    if (flags & ts.SymbolFlags.Module) return 'module';
    return 'symbol';
}

function lineOf(sourceFile: ts.SourceFile, pos: number): number {
    return sourceFile.getLineAndCharacterOfPosition(pos).line + 1;
}

function walkDir(dir: string, out: string[]): void {
    if (!fs.existsSync(dir)) return;
    let entries: fs.Dirent[];
    try {
        entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch {
        return;
    }
    for (const entry of entries) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            if (
                entry.name === 'node_modules' ||
                entry.name === 'dist' ||
                entry.name === 'build' ||
                entry.name.startsWith('.')
            ) continue;
            walkDir(full, out);
        } else if (entry.isFile()) {
            if (entry.name.endsWith('.ts') || entry.name.endsWith('.tsx')) {
                if (entry.name.endsWith('.d.ts')) continue;
                out.push(full);
            }
        }
    }
}

function main(): void {
    const argv = process.argv.slice(2);
    if (argv.length < 1) fail('usage: extract_and_find.js <repo_root>');
    const repoRoot = path.resolve(argv[0]);

    const tsconfigPath = path.join(repoRoot, 'common', 'common-web', 'tsconfig.json');
    if (!fs.existsSync(tsconfigPath)) {
        fail(`tsconfig.json not found at ${tsconfigPath}`);
    }

    const tsconfigRead = ts.readConfigFile(tsconfigPath, ts.sys.readFile);
    if (tsconfigRead.error) {
        fail(`tsconfig parse error: ${JSON.stringify(tsconfigRead.error.messageText)}`);
    }
    const tsconfigParsed = ts.parseJsonConfigFileContent(
        tsconfigRead.config,
        ts.sys,
        path.dirname(tsconfigPath),
    );

    const indexPath = path.join(repoRoot, 'common', 'common-web', 'src', 'index.ts');
    if (!fs.existsSync(indexPath)) {
        fail(`index.ts not found at ${indexPath}`);
    }

    const consumerRoots = [
        path.join(repoRoot, 'common', 'common-web', 'examples'),
        path.join(repoRoot, 'closed-form'),
        path.join(repoRoot, 'agent-based'),
        path.join(repoRoot, 'continuous-ca'),
        path.join(repoRoot, 'volumetric-grid'),
        path.join(repoRoot, 'particle-fluids'),
        path.join(repoRoot, 'hybrid-particle-grid'),
    ];

    const extraFiles: string[] = [];
    for (const root of consumerRoots) walkDir(root, extraFiles);

    const rootNames = Array.from(new Set([
        ...tsconfigParsed.fileNames,
        indexPath,
        ...extraFiles,
    ]));

    const program = ts.createProgram({
        rootNames,
        options: tsconfigParsed.options,
    });
    const checker = program.getTypeChecker();

    const indexSourceFile = program.getSourceFile(indexPath);
    if (!indexSourceFile) fail(`could not load source file ${indexPath}`);

    const indexSymbol = checker.getSymbolAtLocation(indexSourceFile);
    if (!indexSymbol) fail(`could not resolve module symbol for ${indexPath}`);

    const exports = checker.getExportsOfModule(indexSymbol);

    interface TargetSymbol {
        name: string;
        kind: string;
        declarationFile: string;
        declarationLine: number;
        parentClassName: string | null;
        tsSymbol: ts.Symbol;
    }

    const targets: TargetSymbol[] = [];

    for (const exp of exports) {
        let actual = exp;
        if (actual.flags & ts.SymbolFlags.Alias) {
            try { actual = checker.getAliasedSymbol(actual); } catch { /* ignore */ }
        }

        const decls = actual.getDeclarations();
        if (!decls || decls.length === 0) continue;
        const decl = decls[0]!;
        const sf = decl.getSourceFile();
        const kind = describeKind(actual);

        targets.push({
            name: exp.getName(),
            kind,
            declarationFile: sf.fileName,
            declarationLine: lineOf(sf, decl.getStart()),
            parentClassName: null,
            tsSymbol: actual,
        });

        if (kind === 'class' || kind === 'interface') {
            const members = actual.members;
            if (members) {
                members.forEach((memberSymbol, memberName) => {
                    const memberDecls = memberSymbol.getDeclarations();
                    if (!memberDecls || memberDecls.length === 0) return;
                    const memberDecl = memberDecls[0]!;
                    if (ts.canHaveModifiers(memberDecl)) {
                        const mods = ts.getModifiers(memberDecl);
                        if (mods && mods.some(m =>
                            m.kind === ts.SyntaxKind.PrivateKeyword ||
                            m.kind === ts.SyntaxKind.ProtectedKeyword
                        )) return;
                    }
                    const nameStr = String(memberName);
                    if (nameStr.startsWith('_')) return;
                    if (nameStr === '__constructor') return;

                    const memberSf = memberDecl.getSourceFile();
                    const memberKind = (memberSymbol.flags & ts.SymbolFlags.Method)
                        ? 'method'
                        : (memberSymbol.flags & ts.SymbolFlags.Property)
                            ? 'property'
                            : 'member';
                    targets.push({
                        name: nameStr,
                        kind: memberKind,
                        declarationFile: memberSf.fileName,
                        declarationLine: lineOf(memberSf, memberDecl.getStart()),
                        parentClassName: exp.getName(),
                        tsSymbol: memberSymbol,
                    });
                });
            }
        }
    }

    const targetSymbolSet = new Map<ts.Symbol, TargetSymbol>();
    for (const t of targets) targetSymbolSet.set(t.tsSymbol, t);

    const referencesByTarget = new Map<TargetSymbol, ReferenceSite[]>();
    for (const t of targets) referencesByTarget.set(t, []);

    function visit(node: ts.Node, sourceFile: ts.SourceFile, enclosingClass: string | null): void {
        if (ts.isClassDeclaration(node) || ts.isClassExpression(node)) {
            const className = node.name ? node.name.text : null;
            ts.forEachChild(node, c => visit(c, sourceFile, className));
            return;
        }

        if (ts.isInterfaceDeclaration(node)) {
            const ifaceName = node.name.text;
            ts.forEachChild(node, c => visit(c, sourceFile, ifaceName));
            return;
        }

        // Skip identifiers that are export/import specifiers — they re-bind
        // a name into a module's surface, they don't "consume" the symbol.
        if (ts.isExportSpecifier(node) || ts.isImportSpecifier(node) ||
            ts.isExportDeclaration(node) || ts.isImportDeclaration(node)) {
            // For ImportDeclaration we still want to recurse so we see uses
            // INSIDE the file, just not the import statement itself.
            if (ts.isExportSpecifier(node) || ts.isImportSpecifier(node)) {
                return;
            }
        }

        let identifierNode: ts.Node | null = null;
        if (ts.isIdentifier(node)) {
            identifierNode = node;
        } else if (ts.isPropertyAccessExpression(node)) {
            identifierNode = node.name;
        }

        if (identifierNode) {
            const symbol = checker.getSymbolAtLocation(identifierNode);
            if (symbol) {
                let resolved = symbol;
                if (resolved.flags & ts.SymbolFlags.Alias) {
                    try { resolved = checker.getAliasedSymbol(resolved); } catch { /* ignore */ }
                }
                const target = targetSymbolSet.get(resolved);
                if (target) {
                    const declarations = resolved.getDeclarations();
                    let isDeclarationSite = false;
                    if (declarations) {
                        for (const d of declarations) {
                            const ds = d.getSourceFile();
                            if (ds !== sourceFile) continue;
                            const dLine = ds.getLineAndCharacterOfPosition(d.getStart()).line;
                            const nLine = sourceFile.getLineAndCharacterOfPosition(identifierNode.getStart()).line;
                            if (dLine === nLine) {
                                isDeclarationSite = true;
                                break;
                            }
                        }
                    }
                    if (!isDeclarationSite) {
                        let skipMemberSelf = false;
                        if ((target.kind === 'property' || target.kind === 'method') &&
                            target.parentClassName && enclosingClass === target.parentClassName) {
                            skipMemberSelf = true;
                        }
                        if (!skipMemberSelf) {
                            referencesByTarget.get(target)!.push({
                                file: path.relative(repoRoot, sourceFile.fileName),
                                line: lineOf(sourceFile, identifierNode.getStart()),
                            });
                        }
                    }
                }
            }
        }

        ts.forEachChild(node, c => visit(c, sourceFile, enclosingClass));
    }

    for (const sf of program.getSourceFiles()) {
        if (sf.isDeclarationFile) continue;
        if (sf.fileName.includes('node_modules')) continue;
        if (!sf.fileName.startsWith(repoRoot)) continue;
        visit(sf, sf, null);
    }

    const symbols: SymbolInfo[] = targets.map(t => ({
        name: t.parentClassName ? `${t.parentClassName}.${t.name}` : t.name,
        kind: t.kind,
        file: path.relative(repoRoot, t.declarationFile),
        line: t.declarationLine,
        references: referencesByTarget.get(t)!,
    }));

    const output: Output = { symbols, diagnostics: [] };
    process.stdout.write(JSON.stringify(output));
}

try {
    main();
} catch (e: any) {
    fail(`runtime error: ${e?.message || e}`, 2);
}
