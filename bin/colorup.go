package main

// colorup.go - Go implementation of ColorUp syscall
// Calls all layers: Lexer -> Parser -> Renderer
// Not JavaScript - Pure Go

import (
	"fmt"
	"os"
	"regexp"
	"strings"
)

// === LEXER (simplified, parser does regex inline like JS version) ===
type Token struct {
	Type  string
	Value string
}

// === PARSER: string -> AST ===
type Node struct {
	Type     string
	Level    int
	Color    string
	Value    string
	URL      string
	Text     string
	Children []Node
	Items    []Node
}

type Document struct {
	Type     string
	Children []Node
}

func parseInline(text string) []Node {
	nodes := []Node{}
	i := 0
	for i < len(text) {
		rest := text[i:]

		// $color[content]$color
		reColor := regexp.MustCompile(`^\$([a-zA-Z]+|#[0-9a-fA-F]{3,8})\[`)
		if m := reColor.FindStringSubmatch(rest); m != nil {
			color := m[1]
			endTag := "]$" + color
			startIdx := i + len(m[0])
			endIdx := strings.Index(text[startIdx:], endTag)
			if endIdx != -1 {
				inner := text[startIdx : startIdx+endIdx]
				nodes = append(nodes, Node{Type: "color", Color: color, Children: parseInline(inner)})
				i = startIdx + endIdx + len(endTag)
				continue
			}
		}

		// $link[text|url]$link
		if strings.HasPrefix(rest, "$link[") {
			end := strings.Index(text[i:], "]$link")
			if end != -1 {
				inner := text[i+6 : i+end]
				parts := strings.SplitN(inner, "|", 2)
				t := parts[0]
				url := "#"
				if len(parts) > 1 {
					url = parts[1]
				}
				nodes = append(nodes, Node{Type: "link", Text: t, URL: url, Children: parseInline(t)})
				i = i + end + 6
				continue
			}
		}

		// Bold+Italic *"text*"_
		reBI := regexp.MustCompile(`^\*"([^*]+?)\*"\_`)
		if m := reBI.FindStringSubmatch(rest); m != nil {
			nodes = append(nodes, Node{Type: "bolditalic", Children: parseInline(m[1])})
			i += len(m[0])
			continue
		}

		// Bold 'text'
		reBold := regexp.MustCompile(`^'([^']+?)'`)
		if m := reBold.FindStringSubmatch(rest); m != nil {
			nodes = append(nodes, Node{Type: "bold", Children: []Node{{Type: "text", Value: m[1]}}})
			i += len(m[0])
			continue
		}

		// Italic ~text~
		reItalic := regexp.MustCompile(`^~([^~]+?)~`)
		if m := reItalic.FindStringSubmatch(rest); m != nil {
			nodes = append(nodes, Node{Type: "italic", Children: []Node{{Type: "text", Value: m[1]}}})
			i += len(m[0])
			continue
		}
