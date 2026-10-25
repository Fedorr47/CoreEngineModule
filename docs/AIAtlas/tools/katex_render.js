// Рендер формул KaTeX при сборке атласа: stdin — JSON [{tex, display}], stdout — JSON [html].
// Ошибка разбора любой формулы завершает процесс с кодом 1 (strict-сборка падает).
'use strict';
const path = require('path');
const katex = require(path.join(__dirname, '..', '.toolchain', 'node', 'node_modules', 'katex'));
let input = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', (chunk) => { input += chunk; });
process.stdin.on('end', () => {
    const items = JSON.parse(input || '[]');
    const out = [];
    for (const item of items) {
        try {
            out.push(katex.renderToString(item.tex, {
                displayMode: Boolean(item.display), throwOnError: true, output: 'htmlAndMathml', strict: 'ignore',
            }));
        } catch (error) {
            process.stderr.write(`KaTeX error in "${item.tex}": ${error.message}\n`);
            process.exit(1);
        }
    }
    process.stdout.write(JSON.stringify(out));
});
