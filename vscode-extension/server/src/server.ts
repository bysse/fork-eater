import {
	createConnection,
	TextDocuments,
	ProposedFeatures,
	InitializeParams,
	DidChangeConfigurationNotification,
	CompletionItem,
	CompletionItemKind,
	TextDocumentPositionParams,
	TextDocumentSyncKind,
	InitializeResult
} from 'vscode-languageserver/node';

import {
	TextDocument
} from 'vscode-languageserver-textdocument';

import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath } from 'url';

import * as libsIndex from './libs-index.json';

// Create a connection for the server, using Node's IPC as a transport.
// Also include all preview / proposed LSP features.
const connection = createConnection(ProposedFeatures.all);

// Create a simple text document manager.
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasDiagnosticRelatedInformationCapability = false;

let workspaceRoot: string | null = null;
let libsFunctions: CompletionItem[] = (libsIndex as any).libsFunctions || [];
let libsFiles: CompletionItem[] = (libsIndex as any).libsFiles || [];

connection.onInitialize((params: InitializeParams) => {
	const capabilities = params.capabilities;

	// Does the client support the `workspace/configuration` request?
	// If not, we fall back using global settings.
	hasConfigurationCapability = !!(
		capabilities.workspace && !!capabilities.workspace.configuration
	);
hasWorkspaceFolderCapability = !!(
		capabilities.workspace && !!capabilities.workspace.workspaceFolders
	);
hasDiagnosticRelatedInformationCapability = !!(
		capabilities.textDocument &&
		capabilities.textDocument.publishDiagnostics &&
		capabilities.textDocument.publishDiagnostics.relatedInformation
	);

	if (params.workspaceFolders && params.workspaceFolders.length > 0) {
		workspaceRoot = fileURLToPath(params.workspaceFolders[0].uri);
	} else if (params.rootUri) {
		workspaceRoot = fileURLToPath(params.rootUri);
	}

	const result: InitializeResult = {
		capabilities: {
			textDocumentSync: TextDocumentSyncKind.Incremental,
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
		connection.client.register(DidChangeConfigurationNotification.type, undefined);
	}
	if (hasWorkspaceFolderCapability) {
		connection.workspace.onDidChangeWorkspaceFolders(_event => {
			connection.console.log('Workspace folder change event received.');
		});
	}
});

// System uniforms (standard usage)
const systemUniforms: CompletionItem[] = [
	{ label: 'u_time', kind: CompletionItemKind.Variable, detail: 'float: Time in seconds' },
	{ label: 'iTime', kind: CompletionItemKind.Variable, detail: 'float: Shadertoy compatibility' },
	{ label: 'u_resolution', kind: CompletionItemKind.Variable, detail: 'vec2: Viewport resolution' },
	{ label: 'iResolution', kind: CompletionItemKind.Variable, detail: 'vec3: Shadertoy compatibility' },
	{ label: 'u_mouse', kind: CompletionItemKind.Variable, detail: 'vec4: Mouse coordinates' },
	{ label: 'iMouse', kind: CompletionItemKind.Variable, detail: 'vec4: Shadertoy compatibility' },
	{ label: 'u_mouse_rel', kind: CompletionItemKind.Variable, detail: 'vec2: Relative mouse (wrap-around)' },
	{ label: 'TexCoord', kind: CompletionItemKind.Variable, detail: 'vec2: Texture coordinates' },
	{ label: 'FragColor', kind: CompletionItemKind.Variable, detail: 'vec4: Output color' }
];

const switchFlags: CompletionItem[] = [
	{ label: 'FORK_DISABLE_MOUSE_LOOK', kind: CompletionItemKind.EnumMember, detail: 'Disable mouse-based camera rotation [camera.glsl]' }
];

// Uniform declaration suggestions (includes type)
const uniformTypeSuggestions: CompletionItem[] = [
	{ label: 'float u_time', kind: CompletionItemKind.Snippet, detail: 'Standard time uniform', insertText: 'float u_time;' },
	{ label: 'vec2 u_resolution', kind: CompletionItemKind.Snippet, detail: 'Standard resolution uniform', insertText: 'vec2 u_resolution;' },
	{ label: 'vec4 u_mouse', kind: CompletionItemKind.Snippet, detail: 'Standard mouse uniform', insertText: 'vec4 u_mouse;' },
	{ label: 'vec2 u_mouse_rel', kind: CompletionItemKind.Snippet, detail: 'Relative mouse uniform', insertText: 'vec2 u_mouse_rel;' },
	
	// Shadertoy compatibility
	{ label: 'float iTime', kind: CompletionItemKind.Snippet, detail: 'Shadertoy time', insertText: 'float iTime;' },
	{ label: 'vec3 iResolution', kind: CompletionItemKind.Snippet, detail: 'Shadertoy resolution', insertText: 'vec3 iResolution;' },
	{ label: 'vec4 iMouse', kind: CompletionItemKind.Snippet, detail: 'Shadertoy mouse', insertText: 'vec4 iMouse;' },
	
	// Generic types
	{ label: 'float', kind: CompletionItemKind.Keyword, detail: 'Floating point' },
	{ label: 'int', kind: CompletionItemKind.Keyword, detail: 'Integer' },
	{ label: 'bool', kind: CompletionItemKind.Keyword, detail: 'Boolean' },
	{ label: 'vec2', kind: CompletionItemKind.Class, detail: 'Vector 2' },
	{ label: 'vec3', kind: CompletionItemKind.Class, detail: 'Vector 3' },
	{ label: 'vec4', kind: CompletionItemKind.Class, detail: 'Vector 4' },
	{ label: 'mat3', kind: CompletionItemKind.Class, detail: 'Matrix 3x3' },
	{ label: 'mat4', kind: CompletionItemKind.Class, detail: 'Matrix 4x4' },
	{ label: 'sampler2D', kind: CompletionItemKind.Class, detail: '2D Texture Sampler' }
];

function findProjectRoot(startPath: string): string | null {
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

function getRelativePath(from: string, to: string): string {
	let rel = path.relative(from, to);
	// Normalize path separators to forward slashes for GLSL
	rel = rel.split(path.sep).join('/');
	return rel;
}

function scanDirectoryForIncludes(dir: string, currentFileDir: string, kind: string): CompletionItem[] {
	const items: CompletionItem[] = [];
	if (!fs.existsSync(dir)) return items;

	try {
		const files = fs.readdirSync(dir);
		for (const file of files) {
			if (file.endsWith('.glsl') || file.endsWith('.frag') || file.endsWith('.vert')) {
				const fullPath = path.join(dir, file);
				const relPath = getRelativePath(currentFileDir, fullPath);
				
				items.push({
					label: relPath, // Show relative path in the list
					kind: CompletionItemKind.File,
					detail: `${kind}: ${file}`,
					insertText: relPath
				});
			}
		}
	} catch (e) {
		connection.console.error(`Error scanning directory ${dir}: ${e}`);
	}
	return items;
}

connection.onCompletion(
	(textDocumentPosition: TextDocumentPositionParams): CompletionItem[] => {
		const document = documents.get(textDocumentPosition.textDocument.uri);
		if (!document) return [];

		const content = document.getText();
		const offset = document.offsetAt(textDocumentPosition.position);
		const line = content.substring(0, offset).split('\n').pop() || '';
		const trimmedLine = line.trim();
		
		// Pragma completion
		if (trimmedLine.startsWith('#pragma')) {
			if (line.includes('include(')) {
				const currentFilePath = fileURLToPath(textDocumentPosition.textDocument.uri);
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
			if (line.includes('switch(')) {
				return switchFlags;
			}
			if (trimmedLine === '#pragma') {
				return [
					{ label: 'include', kind: CompletionItemKind.Keyword, insertText: ' include(' },
					{ label: 'switch', kind: CompletionItemKind.Keyword, insertText: ' switch(' },
					{ label: 'label', kind: CompletionItemKind.Keyword, insertText: ' label(' },
					{ label: 'range', kind: CompletionItemKind.Keyword, insertText: ' range(' },
					{ label: 'group', kind: CompletionItemKind.Keyword, insertText: ' group(' },
					{ label: 'endgroup', kind: CompletionItemKind.Keyword, insertText: ' endgroup(' }
				];
			}
		}

		// Pragma argument completions
		if (trimmedLine.startsWith('#pragma label(') || trimmedLine.startsWith('#pragma range(')) {
			// Extract uniforms from current document
			const uniformRegex = /uniform\s+(?:float|vec2|vec3|vec4)\s+([a-zA-Z0-9_]+);/g;
			let match;
			const uniforms: CompletionItem[] = [];
			while ((match = uniformRegex.exec(content)) !== null) {
				uniforms.push({ label: match[1], kind: CompletionItemKind.Variable });
			}
			return uniforms;
		}

		if (trimmedLine.startsWith('#pragma switch(')) {
			if (trimmedLine.includes(',')) {
				return [
					{ label: 'true', kind: CompletionItemKind.Keyword },
					{ label: 'false', kind: CompletionItemKind.Keyword },
					{ label: 'on', kind: CompletionItemKind.Keyword },
					{ label: 'off', kind: CompletionItemKind.Keyword }
				];
			}
			return switchFlags;
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
	}
);

connection.onCompletionResolve(
	(item: CompletionItem): CompletionItem => {
		return item;
	}
);

documents.listen(connection);
connection.listen();
