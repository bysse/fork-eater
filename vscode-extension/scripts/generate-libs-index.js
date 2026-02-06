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
    const funcRegex = /^([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)/gm;
    let match;

    const lines = content.split('\n');
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const funcMatch = /^([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)/.exec(line);
        
        if (funcMatch) {
            const returnType = funcMatch[1];
            const funcName = funcMatch[2];
            const args = funcMatch[3];
            const signature = `${returnType} ${funcName}(${args})`;

            // Skip main function and keywords
            if (funcName === 'main') continue;
            if (['return', 'if', 'else', 'while', 'for', 'switch'].includes(returnType)) continue;

            // Look for comments above the function
            let documentation = signature;
            const docLines = [];
            let j = i - 1;
            
            while (j >= 0) {
                const prevLine = lines[j].trim();
                if (prevLine.startsWith('//')) {
                    // If it's a separator line like //////, stop here
                    if (/^\/\/[-=/!]{3,}/.test(prevLine)) break;
                    
                    docLines.unshift(prevLine.replace(/^\/\/\s*/, ''));
                    j--;
                } else if (prevLine === '') {
                    // Stop if we hit an empty line after finding some comments
                    if (docLines.length > 0) break;
                    j--;
                } else {
                    break;
                }
            }
            
            if (docLines.length > 0) {
                documentation = docLines.join('\n') + '\n\n' + signature;
            }

            libsFunctions.push({
                label: funcName,
                kind: KIND_FUNCTION,
                detail: `${signature} [${file}]`,
                insertText: funcName,
                documentation: {
                    kind: 'markdown',
                    value: documentation
                }
            });
        }
    }
});

const output = {
    libsFunctions,
    libsFiles
};

fs.writeFileSync(OUTPUT_FILE, JSON.stringify(output, null, 2));
console.log(`Generated index for ${libsFiles.length} files and ${libsFunctions.length} functions to ${OUTPUT_FILE}`);
