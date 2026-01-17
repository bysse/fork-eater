const fs = require('fs');
const path = require('path');

const LIBS_DIR = path.resolve(__dirname, '../../libs');
const OUTPUT_FILE = path.resolve(__dirname, '../server/src/libs-index.json');

// CompletionItemKind.Function = 3
// CompletionItemKind.File = 17
const KIND_FUNCTION = 3;
const KIND_FILE = 17;

if (!fs.existsSync(LIBS_DIR)) {
    console.error(`Libs directory not found at ${LIBS_DIR}`);
    process.exit(1);
}

const libsFunctions = [];
const libsFiles = [];

const files = fs.readdirSync(LIBS_DIR);

files.forEach(file => {
    if (!file.endsWith('.glsl')) return;

    const fullPath = path.join(LIBS_DIR, file);
    const content = fs.readFileSync(fullPath, 'utf8');

    // Add file to completion items
    libsFiles.push({
        label: file,
        kind: KIND_FILE,
        detail: `System Library: ${file}`,
        insertText: file
    });

    // Parse functions
    // Regex to capture return type, function name, and args
    // Matches "type name(args)" at the start of a line
    // Handling simplified GLSL function signatures
    const regex = /^([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)/gm;
    let match;

    while ((match = regex.exec(content)) !== null) {
        const returnType = match[1];
        const funcName = match[2];
        const args = match[3];
        const signature = `${returnType} ${funcName}(${args})`;

        // Skip main function and keywords
        if (funcName === 'main') continue;
        if (['return', 'if', 'else', 'while', 'for', 'switch'].includes(returnType)) continue;

        libsFunctions.push({
            label: funcName,
            kind: KIND_FUNCTION,
            detail: `${signature} [${file}]`,
            insertText: funcName,
            documentation: signature
        });
    }
});

const output = {
    libsFunctions,
    libsFiles
};

fs.writeFileSync(OUTPUT_FILE, JSON.stringify(output, null, 2));
console.log(`Generated index for ${libsFiles.length} files and ${libsFunctions.length} functions to ${OUTPUT_FILE}`);
