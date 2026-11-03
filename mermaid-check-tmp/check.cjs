const fs = require('node:fs');
const { JSDOM } = require('jsdom');

const mdPath = 'D:/Project/CPP_Porject/API/API/白痴式技术解析文档.md';
const md = fs.readFileSync(mdPath, 'utf8');
const blocks = [...md.matchAll(/```mermaid\n([\s\S]*?)```/g)].map((m) => m[1]);

console.log(`found ${blocks.length} mermaid blocks\n`);

const dom = new JSDOM('<!DOCTYPE html><html><body></body></html>', { pretendToBeVisual: true });
global.window = dom.window;
global.document = dom.window.document;
Object.defineProperty(global, 'navigator', { value: dom.window.navigator, configurable: true });

// 使用自带 dompurify 的 bundled 版本，避免 ESM 下 DOMPurify 工厂函数问题
require('mermaid/dist/mermaid.min.js');
const mermaid = global.window.mermaid || global.mermaid;
if (!mermaid || typeof mermaid.initialize !== 'function') {
  console.error('cannot resolve mermaid API from bundled build');
  process.exit(2);
}
mermaid.initialize({ startOnLoad: false, securityLevel: 'loose' });

let fail = 0;
(async () => {
  for (let i = 0; i < blocks.length; i++) {
    const first = blocks[i].trim().split('\n')[0];
    try {
      await mermaid.parse(blocks[i]);
      console.log(`[OK  ] block ${String(i + 1).padStart(2)}  ${first}`);
    } catch (e) {
      fail++;
      console.log(`[FAIL] block ${String(i + 1).padStart(2)}  ${first}`);
      console.log(`       ${(e && e.message) || e}`);
    }
  }
  console.log(fail === 0 ? `\nALL ${blocks.length} blocks parse OK` : `\n${fail}/${blocks.length} blocks FAILED`);
  process.exit(fail === 0 ? 0 : 1);
})();
