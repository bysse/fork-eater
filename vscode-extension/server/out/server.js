"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const node_1 = require("vscode-languageserver/node");
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
const fs = require("fs");
const path = require("path");
const url_1 = require("url");
// Create a connection for the server, using Node's IPC as a transport.
// Also include all preview / proposed LSP features.
const connection = (0, node_1.createConnection)(node_1.ProposedFeatures.all);
// Create a simple text document manager.
const documents = new node_1.TextDocuments(vscode_languageserver_textdocument_1.TextDocument);
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasDiagnosticRelatedInformationCapability = false;
let workspaceRoot = null;
let libsFunctions = [];
let libsFiles = [];
connection.onInitialize((params) => {
    const capabilities = params.capabilities;
    // Does the client support the `workspace/configuration` request?
    // If not, we fall back using global settings.
    hasConfigurationCapability = !!(capabilities.workspace && !!capabilities.workspace.configuration);
    hasWorkspaceFolderCapability = !!(capabilities.workspace && !!capabilities.workspace.workspaceFolders);
    hasDiagnosticRelatedInformationCapability = !!(capabilities.textDocument &&
        capabilities.textDocument.publishDiagnostics &&
        capabilities.textDocument.publishDiagnostics.relatedInformation);
    if (params.workspaceFolders && params.workspaceFolders.length > 0) {
        workspaceRoot = (0, url_1.fileURLToPath)(params.workspaceFolders[0].uri);
    }
    else if (params.rootUri) {
        workspaceRoot = (0, url_1.fileURLToPath)(params.rootUri);
    }
    const result = {
        capabilities: {
            textDocumentSync: node_1.TextDocumentSyncKind.Incremental,
            // Tell the client that this server supports code completion.
            completionProvider: {
                resolveProvider: true,
                triggerCharacters: ['(', ' ']
            }
        }
    };
    if (hasWorkspaceFolderCapability) {
        result.capabilities.workspace = {
            workspaceFolders: {
                supported: true
            }
        };
    }
    return result;
});
connection.onInitialized(() => {
    if (hasConfigurationCapability) {
        // Register for all configuration changes.
        connection.client.register(node_1.DidChangeConfigurationNotification.type, undefined);
    }
    if (hasWorkspaceFolderCapability) {
        connection.workspace.onDidChangeWorkspaceFolders(_event => {
            connection.console.log('Workspace folder change event received.');
        });
    }
    // Scan libs on startup
    scanLibs();
});
function scanLibs() {
    // Look for bundled libs (copied by Makefile to server/libs)
    const bundledLibsDir = path.resolve(__dirname, '../../libs');
    connection.console.log(`Scanning bundled libs at: ${bundledLibsDir}`);
    if (!fs.existsSync(bundledLibsDir)) {
        connection.console.error(`Bundled libs directory not found: ${bundledLibsDir}`);
        return;
    }
    libsFunctions = [];
    libsFiles = [];
    try {
        const files = fs.readdirSync(bundledLibsDir);
        for (const file of files) {
            if (file.endsWith('.glsl')) {
                // Add to files list
                libsFiles.push({
                    label: `lib/${file}`,
                    kind: node_1.CompletionItemKind.File,
                    detail: 'System Library',
                    insertText: `lib/${file}`
                });
                // Parse functions
                const content = fs.readFileSync(path.join(bundledLibsDir, file), 'utf8');
                const functionRegex = /^\s*(?:float|vec2|vec3|vec4|void|int|bool)\s+([a-zA-Z0-9_]+)\s*\(/gm;
                let match;
                while ((match = functionRegex.exec(content)) !== null) {
                    libsFunctions.push({
                        label: match[1],
                        kind: node_1.CompletionItemKind.Function,
                        detail: `Function from ${file}`
                    });
                }
            }
        }
    }
    catch (e) {
        connection.console.error(`Error scanning libs: ${e}`);
    }
}
// System uniforms (standard usage)
const systemUniforms = [
    { label: 'u_time', kind: node_1.CompletionItemKind.Variable, detail: 'float: Time in seconds' },
    { label: 'iTime', kind: node_1.CompletionItemKind.Variable, detail: 'float: Shadertoy compatibility' },
    { label: 'u_resolution', kind: node_1.CompletionItemKind.Variable, detail: 'vec2: Viewport resolution' },
    { label: 'iResolution', kind: node_1.CompletionItemKind.Variable, detail: 'vec3: Shadertoy compatibility' },
    { label: 'u_mouse', kind: node_1.CompletionItemKind.Variable, detail: 'vec4: Mouse coordinates' },
    { label: 'iMouse', kind: node_1.CompletionItemKind.Variable, detail: 'vec4: Shadertoy compatibility' },
    { label: 'TexCoord', kind: node_1.CompletionItemKind.Variable, detail: 'vec2: Texture coordinates' },
    { label: 'FragColor', kind: node_1.CompletionItemKind.Variable, detail: 'vec4: Output color' }
];
// Uniform declaration suggestions (includes type)
const uniformTypeSuggestions = [
    { label: 'float u_time', kind: node_1.CompletionItemKind.Snippet, detail: 'Standard time uniform', insertText: 'float u_time;' },
    { label: 'vec2 u_resolution', kind: node_1.CompletionItemKind.Snippet, detail: 'Standard resolution uniform', insertText: 'vec2 u_resolution;' },
    { label: 'vec4 u_mouse', kind: node_1.CompletionItemKind.Snippet, detail: 'Standard mouse uniform', insertText: 'vec4 u_mouse;' },
    // Shadertoy compatibility
    { label: 'float iTime', kind: node_1.CompletionItemKind.Snippet, detail: 'Shadertoy time', insertText: 'float iTime;' },
    { label: 'vec3 iResolution', kind: node_1.CompletionItemKind.Snippet, detail: 'Shadertoy resolution', insertText: 'vec3 iResolution;' },
    { label: 'vec4 iMouse', kind: node_1.CompletionItemKind.Snippet, detail: 'Shadertoy mouse', insertText: 'vec4 iMouse;' },
    // Generic types
    { label: 'float', kind: node_1.CompletionItemKind.Keyword, detail: 'Floating point' },
    { label: 'int', kind: node_1.CompletionItemKind.Keyword, detail: 'Integer' },
    { label: 'bool', kind: node_1.CompletionItemKind.Keyword, detail: 'Boolean' },
    { label: 'vec2', kind: node_1.CompletionItemKind.Class, detail: 'Vector 2' },
    { label: 'vec3', kind: node_1.CompletionItemKind.Class, detail: 'Vector 3' },
    { label: 'vec4', kind: node_1.CompletionItemKind.Class, detail: 'Vector 4' },
    { label: 'mat3', kind: node_1.CompletionItemKind.Class, detail: 'Matrix 3x3' },
    { label: 'mat4', kind: node_1.CompletionItemKind.Class, detail: 'Matrix 4x4' },
    { label: 'sampler2D', kind: node_1.CompletionItemKind.Class, detail: '2D Texture Sampler' }
];
function findProjectRoot(startPath) {
    let currentDir = startPath;
    while (true) {
        if (fs.existsSync(path.join(currentDir, '4k-eater.project'))) {
            return currentDir;
        }
        const parentDir = path.dirname(currentDir);
        if (parentDir === currentDir) {
            return null;
        }
        currentDir = parentDir;
    }
}
function getRelativePath(from, to) {
    let rel = path.relative(from, to);
    // Normalize path separators to forward slashes for GLSL
    rel = rel.split(path.sep).join('/');
    return rel;
}
function scanDirectoryForIncludes(dir, currentFileDir, kind) {
    const items = [];
    if (!fs.existsSync(dir))
        return items;
    try {
        const files = fs.readdirSync(dir);
        for (const file of files) {
            if (file.endsWith('.glsl') || file.endsWith('.frag') || file.endsWith('.vert')) {
                const fullPath = path.join(dir, file);
                const relPath = getRelativePath(currentFileDir, fullPath);
                items.push({
                    label: relPath,
                    kind: node_1.CompletionItemKind.File,
                    detail: `${kind}: ${file}`,
                    insertText: relPath
                });
            }
        }
    }
    catch (e) {
        connection.console.error(`Error scanning directory ${dir}: ${e}`);
    }
    return items;
}
connection.onCompletion((textDocumentPosition) => {
    const document = documents.get(textDocumentPosition.textDocument.uri);
    if (!document)
        return [];
    const content = document.getText();
    const offset = document.offsetAt(textDocumentPosition.position);
    const line = content.substring(0, offset).split('\n').pop() || '';
    const trimmedLine = line.trim();
    // Pragma completion
    if (trimmedLine.startsWith('#pragma')) {
        if (line.includes('include(')) {
            const currentFilePath = (0, url_1.fileURLToPath)(textDocumentPosition.textDocument.uri);
            const currentFileDir = path.dirname(currentFilePath);
            let items = [...libsFiles]; // Always include system libs
            // Scan local directory
            items = items.concat(scanDirectoryForIncludes(currentFileDir, currentFileDir, 'Local'));
            // Scan project directories if available
            const projectRoot = findProjectRoot(currentFileDir);
            if (projectRoot) {
                items = items.concat(scanDirectoryForIncludes(path.join(projectRoot, 'shaders'), currentFileDir, 'Project Shader'));
                items = items.concat(scanDirectoryForIncludes(path.join(projectRoot, 'libs'), currentFileDir, 'Project Lib'));
            }
            return items;
        }
        if (trimmedLine === '#pragma') {
            return [
                { label: 'include', kind: node_1.CompletionItemKind.Keyword, insertText: ' include(' },
                { label: 'switch', kind: node_1.CompletionItemKind.Keyword, insertText: ' switch(' }
            ];
        }
    }
    // Uniform declaration suggestions
    // If line starts with "uniform ", suggest types and standard uniform definitions
    if (trimmedLine.startsWith('uniform ') || (trimmedLine === 'uniform')) {
        return uniformTypeSuggestions;
    }
    // If we are just typing code, suggest uniforms and lib functions
    return [
        ...systemUniforms,
        ...libsFunctions
    ];
});
connection.onCompletionResolve((item) => {
    return item;
});
documents.listen(connection);
connection.listen();
//# sourceMappingURL=server.js.map