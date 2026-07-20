// colorup.rs - Rust implementation of ColorUp syscall
// Calls all layers: Lexer -> Parser -> Renderer
// Not JavaScript - Pure Rust

use std::env;
use std::fs;

#[derive(Debug, Clone)]
enum NodeType {
    Text(String),
    Bold(Vec<Node>),
    Italic(Vec<Node>),
    BoldItalic(Vec<Node>),
    Color { color: String, children: Vec<Node> },
    Code(String),
    Link { url: String, children: Vec<Node> },
}

#[derive(Debug, Clone)]
struct Node {
    node_type: NodeType,
}

#[derive(Debug, Clone)]
enum BlockType {
    Heading { level: usize, children: Vec<Node> },
    Paragraph(Vec<Node>),
    Divider,
    Blockquote(Vec<Node>),
    List(Vec<BlockItem>),
}

#[derive(Debug, Clone)]
struct BlockItem {
    children: Vec<Node>,
}

#[derive(Debug, Clone)]
struct Document {
    children: Vec<BlockType>,
}

fn escape_html(s: &str) -> String {
    s.replace('&', "&amp;")
        .replace('<', "&lt;")
        .replace('>', "&gt;")
        .replace('"', "&quot;")
}

fn parse_inline(text: &str) -> Vec<Node> {
    let mut nodes = Vec::new();
    let chars: Vec<char> = text.chars().collect();
    let mut i = 0;

    while i < chars.len() {
        let rest: String = chars[i..].iter().collect();

        // $color[content]$color
        if rest.starts_with('$') {
            if let Some(start) = rest.find('[') {
                let color_part = &rest[1..start];
                if !color_part.is_empty() && !color_part.contains(']') {
                    let color = color_part.to_string();
                    let end_tag = format!("]${}", color);
                    let content_start = i + 1 + color.len() + 1;
                    if content_start <= chars.len() {
                        let remaining: String = chars[content_start..].iter().collect();
                        if let Some(end_idx) = remaining.find(&end_tag) {
                            let inner: String = chars[content_start..content_start+end_idx].iter().collect();
                            nodes.push(Node {
                                node_type: NodeType::Color { color: color.clone(), children: parse_inline(&inner) }
                            });
                            i = content_start + end_idx + end_tag.len();
                            continue;
                        }
                    }
                }
            }
            // $link[text|url]$link
            if rest.starts_with("$link[") {
                if let Some(end) = text[i..].find("]$link") {
                    let inner = &text[i+6..i+end];
                    let parts: Vec<&str> = inner.splitn(2, '|').collect();
                    let t = parts[0];
                    let url = if parts.len() > 1 { parts[1] } else { "#" };
                    nodes.push(Node {
                        node_type: NodeType::Link { url: url.to_string(), children: parse_inline(t) }
                    });
                    i += end + 6;
                    continue;
                }
            }
        }

        // *"text*"_
        if rest.starts_with("*\"") {
            if let Some(end) = rest.find("*\"_") {
                let inner = &rest[2..end];
                nodes.push(Node {
                    node_type: NodeType::BoldItalic(parse_inline(inner))
                });
                i += end + 3;
                continue;
            }
        }

        // 'bold'
        if rest.starts_with('\'') {
            if let Some(end) = rest[1..].find('\'') {
                let inner = &rest[1..1+end];
                nodes.push(Node {
                    node_type: NodeType::Bold(vec![Node{ node_type: NodeType::Text(inner.to_string()) }])
                });
                i += end + 2;
                continue;
            }
        }

        // ~italic~
        if rest.starts_with('~') {
            if let Some(end) = rest[1..].find('~') {
                let inner = &rest[1..1+end];
                nodes.push(Node {
                    node_type: NodeType::Italic(vec![Node{ node_type: NodeType::Text(inner.to_string()) }])
                });
                i += end + 2;
                continue;
            }
        }

        // `code`
        if rest.starts_with('`') {
            if let Some(end) = rest[1..].find('`') {
                let inner = &rest[1..1+end];
                nodes.push(Node {
                    node_type: NodeType::Code(inner.to_string())
                });
                i += end + 2;
                continue;
            }
        }

        nodes.push(Node { node_type: NodeType::Text(chars[i].to_string()) });
        i += 1;
    }

    // Merge consecutive text
    let mut merged: Vec<Node> = Vec::new();
    for n in nodes {
        if let NodeType::Text(ref v) = n.node_type {
            if let Some(last) = merged.last_mut() {
                if let NodeType::Text(ref mut lv) = last.node_type {
                    lv.push_str(v);
                    continue;
                }
            }
        }
        merged.push(n);
    }
    merged
}

fn parse_colorup_to_ast(input: &str) -> Document {
    let mut doc = Document { children: Vec::new() };
    let mut list_buffer: Option<Vec<BlockItem>> = None;

    for raw_line in input.lines() {
        let trimmed = raw_line.trim();
        if trimmed.is_empty() {
            if let Some(buf) = list_buffer.take() {
                doc.children.push(BlockType::List(buf));
            }
            continue;
        }
        if trimmed == "_-_" {
            if let Some(buf) = list_buffer.take() {
                doc.children.push(BlockType::List(buf));
            }
            doc.children.push(BlockType::Divider);
            continue;
        }
        // Heading
        if trimmed.starts_with('#') {
            let level = trimmed.chars().take_while(|&c| c == '#').count();
            if level <= 6 {
                if let Some(buf) = list_buffer.take() {
                    doc.children.push(BlockType::List(buf));
                }
                let content = trimmed[level..].trim();
                doc.children.push(BlockType::Heading { level, children: parse_inline(content) });
                continue;
            }
        }
        if trimmed.starts_with("> ") {
            if let Some(buf) = list_buffer.take() {
                doc.children.push(BlockType::List(buf));
            }
            doc.children.push(BlockType::Blockquote(parse_inline(&trimmed[2..])));
            continue;
        }
        if trimmed.starts_with("- ") {
            if list_buffer.is_none() {
                list_buffer = Some(Vec::new());
            }
            list_buffer.as_mut().unwrap().push(BlockItem { children: parse_inline(&trimmed[2..]) });
            continue;
        }
        if let Some(buf) = list_buffer.take() {
            doc.children.push(BlockType::List(buf));
        }
        doc.children.push(BlockType::Paragraph(parse_inline(raw_line)));
    }
    if let Some(buf) = list_buffer.take() {
        doc.children.push(BlockType::List(buf));
    }
    doc
}

fn render_inline(nodes: &[Node]) -> String {
    let mut out = String::new();
    for n in nodes {
        match &n.node_type {
            NodeType::Text(v) => out.push_str(&escape_html(v)),
            NodeType::Bold(ch) => out.push_str(&format!("<strong>{}</strong>", render_inline(ch))),
            NodeType::Italic(ch) => out.push_str(&format!("<em>{}</em>", render_inline(ch))),
            NodeType::BoldItalic(ch) => out.push_str(&format!("<strong><em>{}</em></strong>", render_inline(ch))),
            NodeType::Color { color, children } => out.push_str(&format!("<span style=\"color:{}\">{}</span>", escape_html(color), render_inline(children))),
            NodeType::Code(v) => out.push_str(&format!("<code>{}</code>", escape_html(v))),
            NodeType::Link { url, children } => out.push_str(&format!("<a href=\"{}\">{}</a>", escape_html(url), render_inline(children))),
        }
    }
    out
}

fn render_ast(ast: &Document) -> String {
    let mut parts = Vec::new();
    for block in &ast.children {
        match block {
            BlockType::Heading { level, children } => parts.push(format!("<h{}>{}</h{}>", level, render_inline(children), level)),
            BlockType::Paragraph(ch) => parts.push(format!("<p>{}</p>", render_inline(ch))),
            BlockType::Divider => parts.push("<hr/>".to_string()),
            BlockType::Blockquote(ch) => parts.push(format!("<blockquote>{}</blockquote>", render_inline(ch))),
            BlockType::List(items) => {
                let inner: String = items.iter().map(|it| format!("<li>{}</li>", render_inline(&it.children))).collect();
                parts.push(format!("<ul>{}</ul>", inner));
            }
        }
    }
    parts.join("\n")
}

// === SYSCALL: calls all layers ===
fn syscall(input: &str) -> String {
    let ast = parse_colorup_to_ast(input);
    render_ast(&ast)
}

fn parse(input: &str) -> String {
    syscall(input)
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        println!("Usage: colorup <file.colorup>");
        std::process::exit(1);
    }
    let content = fs::read_to_string(&args[1]).expect("Failed to read file");
    println!("{}", parse(&content));
}
