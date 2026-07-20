#!/usr/bin/env python3
# colorup.py - Python implementation of ColorUp syscall
# Calls all layers: Lexer -> Parser -> Renderer
# Not JavaScript - Pure Python

import sys
import re
import html

# === PARSER: string -> AST ===
def parse_inline(text):
    nodes = []
    i = 0
    while i < len(text):
        rest = text[i:]

        # $color[content]$color
        m = re.match(r'^\$([a-zA-Z]+|#[0-9a-fA-F]{3,8})\[', rest)
        if m:
            color = m.group(1)
            end_tag = f']${color}'
            start_idx = i + len(m.group(0))
            end_idx = text.find(end_tag, start_idx)
            if end_idx != -1:
                inner = text[start_idx:end_idx]
                nodes.append({'type': 'color', 'color': color, 'children': parse_inline(inner)})
                i = end_idx + len(end_tag)
                continue

        # $link[text|url]$link
        if rest.startswith('$link['):
            end = text.find(']$link', i)
            if end != -1:
                inner = text[i+6:end]
                parts = inner.split('|', 1)
                t = parts[0]
                url = parts[1] if len(parts) > 1 else '#'
                nodes.append({'type': 'link', 'text': t, 'url': url, 'children': parse_inline(t)})
                i = end + 6
                continue

        # Bold+Italic *"text*"_
        m = re.match(r'^\*"([^*]+?)\*"\_', rest)
        if m:
            nodes.append({'type': 'bolditalic', 'children': parse_inline(m.group(1))})
            i += len(m.group(0))
            continue

        # Bold 'text'
        m = re.match(r"^'([^']+?)'", rest)
        if m:
            nodes.append({'type': 'bold', 'children': [{'type': 'text', 'value': m.group(1)}]})
            i += len(m.group(0))
            continue

        # Italic ~text~
        m = re.match(r'^~([^~]+?)~', rest)
        if m:
            nodes.append({'type': 'italic', 'children': [{'type': 'text', 'value': m.group(1)}]})
            i += len(m.group(0))
            continue

        # Code `text`
        m = re.match(r'^`([^`]+?)`', rest)
        if m:
            nodes.append({'type': 'code', 'value': m.group(1)})
            i += len(m.group(0))
            continue

        nodes.append({'type': 'text', 'value': text[i]})
        i += 1

    # Merge consecutive text
    merged = []
    for n in nodes:
        if n['type'] == 'text' and merged and merged[-1]['type'] == 'text':
            merged[-1]['value'] += n['value']
        else:
            merged.append(n)
    return merged

def parse_colorup_to_ast(input_str):
    lines = input_str.split('\n')
    ast = {'type': 'document', 'children': []}
    list_buffer = None

    for raw_line in lines:
        trimmed = raw_line.strip()
        if not trimmed:
            if list_buffer:
                ast['children'].append({'type': 'list', 'items': list_buffer})
                list_buffer = None
            continue
        if trimmed == '_-_':
            if list_buffer:
                ast['children'].append({'type': 'list', 'items': list_buffer})
                list_buffer = None
            ast['children'].append({'type': 'divider'})
            continue

        m = re.match(r'^(#{1,6})\s*(.*)$', trimmed)
        if m:
            if list_buffer:
                ast['children'].append({'type': 'list', 'items': list_buffer})
                list_buffer = None
            ast['children'].append({'type': 'heading', 'level': len(m.group(1)), 'children': parse_inline(m.group(2))})
            continue

        if trimmed.startswith('> '):
            if list_buffer:
                ast['children'].append({'type': 'list', 'items': list_buffer})
                list_buffer = None
            ast['children'].append({'type': 'blockquote', 'children': parse_inline(trimmed[2:])})
            continue

        if trimmed.startswith('- '):
            if list_buffer is None:
                list_buffer = []
            list_buffer.append({'type': 'listItem', 'children': parse_inline(trimmed[2:])})
            continue

        if list_buffer:
            ast['children'].append({'type': 'list', 'items': list_buffer})
            list_buffer = None
        ast['children'].append({'type': 'paragraph', 'children': parse_inline(raw_line)})

    if list_buffer:
        ast['children'].append({'type': 'list', 'items': list_buffer})
    return ast

# === RENDERER: AST -> HTML ===
def render_inline(nodes):
    out = ''
    for n in nodes:
        if n['type'] == 'text':
            out += html.escape(n['value'])
        elif n['type'] == 'bold':
            out += f"<strong>{render_inline(n['children'])}</strong>"
        elif n['type'] == 'italic':
            out += f"<em>{render_inline(n['children'])}</em>"
        elif n['type'] == 'bolditalic':
            out += f"<strong><em>{render_inline(n['children'])}</em></strong>"
        elif n['type'] == 'color':
            out += f"<span style=\"color:{html.escape(n['color'])}\">{render_inline(n['children'])}</span>"
        elif n['type'] == 'code':
            out += f"<code>{html.escape(n['value'])}</code>"
        elif n['type'] == 'link':
            out += f"<a href=\"{html.escape(n['url'])}\">{render_inline(n['children'])}</a>"
    return out

def render_ast(ast):
    parts = []
    for node in ast['children']:
        if node['type'] == 'heading':
            parts.append(f"<h{node['level']}>{render_inline(node['children'])}</h{node['level']}>")
        elif node['type'] == 'paragraph':
            parts.append(f"<p>{render_inline(node['children'])}</p>")
        elif node['type'] == 'divider':
            parts.append("<hr/>")
        elif node['type'] == 'blockquote':
            parts.append(f"<blockquote>{render_inline(node['children'])}</blockquote>")
        elif node['type'] == 'list':
            items = ''.join([f"<li>{render_inline(it['children'])}</li>" for it in node['items']])
            parts.append(f"<ul>{items}</ul>")
    return '\n'.join(parts)

# === SYSCALL: calls all layers ===
def syscall(input_str, debug=False):
    ast = parse_colorup_to_ast(input_str)
    html_out = render_ast(ast)
    if debug:
        print(f"[SYSCALL] AST: {ast}")
    return html_out

def parse(input_str):
    return syscall(input_str)

def main():
    if len(sys.argv) < 2:
        print("Usage: colorup <file.colorup>")
        sys.exit(1)
    file = sys.argv[1]
    with open(file, 'r', encoding='utf-8') as f:
        content = f.read()
    print(parse(content))

if __name__ == '__main__':
    main()
