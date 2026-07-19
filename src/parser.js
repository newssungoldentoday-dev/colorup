
// Parser: tokens / string -> AST

function parseColorUpToAST(input) {
  // AST nodes: { type, ... }
  // Root: { type: 'document', children: [] }

  const lines = input.split('\n');
  const ast = { type: 'document', children: [] };
  let listBuffer = null;

  function parseInlineAST(text) {
    // Returns array of inline nodes
    const nodes = [];
    let i = 0;
    while (i < text.length) {
      const rest = text.slice(i);

      // Color $red[content]$red
      const colorMatch = rest.match(/^\$([a-zA-Z]+|#[0-9a-fA-F]{3,8})\[/);
      if (colorMatch) {
        const color = colorMatch[1];
        // find matching ]$color
        const endTag = `]$${color}`;
        const startIdx = i + colorMatch[0].length;
        const endIdx = text.indexOf(endTag, startIdx);
        if (endIdx !== -1) {
          const inner = text.slice(startIdx, endIdx);
          nodes.push({ type: 'color', color, children: parseInlineAST(inner) });
          i = endIdx + endTag.length;
          continue;
        }
      }

      // Link $link[text|url]$link
      if (rest.startsWith('$link[')) {
        const end = text.indexOf(']$link', i);
        if (end !== -1) {
          const inner = text.slice(i + 6, end);
          const [t, url] = inner.split('|');
          nodes.push({ type: 'link', text: t, url: url || '#', children: parseInlineAST(t) });
          i = end + 6;
          continue;
        }
      }

      // Bold+Italic *"text*"_ 
      const bi1 = rest.match(/^\*"([^*]+?)\*"_/);
      if (bi1) {
        nodes.push({ type: 'bolditalic', children: parseInlineAST(bi1[1]) });
        i += bi1[0].length;
        continue;
      }
      const bi2 = rest.match(/^\*~(.+?)~\*/);
      if (bi2) {
        nodes.push({ type: 'bolditalic', children: parseInlineAST(bi2[1]) });
        i += bi2[0].length;
        continue;
      }

      // Bold 'text'
      const b = rest.match(/^'([^']+?)'/);
      if (b) {
        nodes.push({ type: 'bold', children: [{ type: 'text', value: b[1] }] });
        i += b[0].length;
        continue;
      }

      // Italic ~text~
      const it = rest.match(/^~([^~]+?)~/);
      if (it) {
        nodes.push({ type: 'italic', children: [{ type: 'text', value: it[1] }] });
        i += it[0].length;
        continue;
      }

      // Code `text`
      const cd = rest.match(/^`([^`]+?)`/);
      if (cd) {
        nodes.push({ type: 'code', value: cd[1] });
        i += cd[0].length;
        continue;
      }

      // Text
      nodes.push({ type: 'text', value: text[i] });
      i++;
    }

    // Merge consecutive text nodes
    const merged = [];
    for (let n of nodes) {
      if (n.type === 'text' && merged.length && merged[merged.length-1].type === 'text') {
        merged[merged.length-1].value += n.value;
      } else {
        merged.push(n);
      }
    }
    return merged;
  }

  for (let rawLine of lines) {
    const trimmed = rawLine.trim();
    if (!trimmed) {
      if (listBuffer) { ast.children.push({ type: 'list', items: listBuffer }); listBuffer = null; }
      continue;
    }
    if (trimmed === '_-_') {
      if (listBuffer) { ast.children.push({ type: 'list', items: listBuffer }); listBuffer = null; }
      ast.children.push({ type: 'divider' });
      continue;
    }
    const hMatch = trimmed.match(/^(#{1,6})\s*(.*)$/);
    if (hMatch) {
      if (listBuffer) { ast.children.push({ type: 'list', items: listBuffer }); listBuffer = null; }
      ast.children.push({ type: 'heading', level: hMatch[1].length, children: parseInlineAST(hMatch[2]) });
      continue;
    }
    if (trimmed.startsWith('> ')) {
      if (listBuffer) { ast.children.push({ type: 'list', items: listBuffer }); listBuffer = null; }
      ast.children.push({ type: 'blockquote', children: parseInlineAST(trimmed.slice(2)) });
      continue;
    }
    if (trimmed.startsWith('- ')) {
      if (!listBuffer) listBuffer = [];
      listBuffer.push({ type: 'listItem', children: parseInlineAST(trimmed.slice(2)) });
      continue;
    }
    // paragraph
    if (listBuffer) { ast.children.push({ type: 'list', items: listBuffer }); listBuffer = null; }
    ast.children.push({ type: 'paragraph', children: parseInlineAST(rawLine) });
  }
  if (listBuffer) ast.children.push({ type: 'list', items: listBuffer });

  return ast;
}

module.exports = { parseColorUpToAST };
