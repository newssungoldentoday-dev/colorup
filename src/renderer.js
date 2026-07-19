
function escapeHtml(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;'); }

function renderInline(nodes){
  return nodes.map(n=>{
    if(n.type==='text') return escapeHtml(n.value);
    if(n.type==='bold') return `<strong>${renderInline(n.children)}</strong>`;
    if(n.type==='italic') return `<em>${renderInline(n.children)}</em>`;
    if(n.type==='bolditalic') return `<strong><em>${renderInline(n.children)}</em></strong>`;
    if(n.type==='color') return `<span style="color:${escapeHtml(n.color)}">${renderInline(n.children)}</span>`;
    if(n.type==='code') return `<code>${escapeHtml(n.value)}</code>`;
    if(n.type==='link') return `<a href="${escapeHtml(n.url)}">${renderInline(n.children)}</a>`;
    return '';
  }).join('');
}

function renderAST(ast){
  return ast.children.map(node=>{
    if(node.type==='heading') return `<h${node.level}>${renderInline(node.children)}</h${node.level}>`;
    if(node.type==='paragraph') return `<p>${renderInline(node.children)}</p>`;
    if(node.type==='divider') return `<hr/>`;
    if(node.type==='blockquote') return `<blockquote>${renderInline(node.children)}</blockquote>`;
    if(node.type==='list') return `<ul>${node.items.map(it=>`<li>${renderInline(it.children)}</li>`).join('')}</ul>`;
    return '';
  }).join('\n');
}

module.exports = { renderAST, renderInline };
