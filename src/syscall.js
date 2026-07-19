// syscall.js - System Call / Orchestrator
// This file calls ALL JS files in order: lexer -> parser -> renderer
// This is the main entry point GitHub / Node / Browser will use

const { lexer } = require('./lexer.js');
const { parseColorUpToAST } = require('./parser.js');
const { renderAST } = require('./renderer.js');

// Main syscall function
function syscall(input, options = {}) {
  const debug = options.debug || false;

  // 1. LEXER: string -> tokens
  const tokens = lexer(input);
  if (debug) console.log('[SYSCALL] Lexer tokens:', tokens);

  // 2. PARSER: tokens/string -> AST
  const ast = parseColorUpToAST(input);
  if (debug) console.log('[SYSCALL] Parser AST:', JSON.stringify(ast, null, 2));

  // 3. RENDERER: AST -> HTML
  const html = renderAST(ast);
  if (debug) console.log('[SYSCALL] Renderer HTML:', html);

  return { tokens, ast, html };
}

// Simple API - just return HTML
function parse(input) {
  const { html } = syscall(input);
  return html;
}

// Browser-safe export
if (typeof window !== 'undefined') {
  window.ColorUp = { syscall, parse, lexer, parseColorUpToAST, renderAST };
}

module.exports = { syscall, parse, lexer, parseColorUpToAST, renderAST };
