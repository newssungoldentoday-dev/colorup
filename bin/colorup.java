// colorup.java - Java implementation of ColorUp syscall
// Calls all layers: Lexer -> Parser -> Renderer
// Not JavaScript - Pure Java

import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class colorup {

    // === Utils ===
    static String escapeHtml(String s) {
        return s.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("\"","&quot;");
    }

    // === PARSER: inline ===
    static String renderInline(String text) {
        StringBuilder out = new StringBuilder();
        int i=0;
        while(i < text.length()) {
            String rest = text.substring(i);

            // $color[content]$color
            Matcher mColor = Pattern.compile("^\\$([a-zA-Z]+|#[0-9a-fA-F]{3,8})\\[").matcher(rest);
            if(mColor.find()) {
                String color = mColor.group(1);
                String endTag = "]$"+color;
                int startIdx = i + mColor.group(0).length();
                int endIdx = text.indexOf(endTag, startIdx);
                if(endIdx != -1) {
                    String inner = text.substring(startIdx, endIdx);
                    out.append("<span style=\"color:").append(escapeHtml(color)).append("\">").append(renderInline(inner)).append("</span>");
                    i = endIdx + endTag.length();
                    continue;
                }
            }
            // $link[text|url]$link
            if(rest.startsWith("$link[")) {
                int end = text.indexOf("]$link", i);
                if(end != -1) {
                    String inner = text.substring(i+6, end);
                    String[] parts = inner.split("\\|",2);
                    String t = parts[0];
                    String url = parts.length>1? parts[1] : "#";
                    out.append("<a href=\"").append(escapeHtml(url)).append("\">").append(renderInline(t)).append("</a>");
                    i = end + 6;
                    continue;
                }
            }
            // *"text*"_
            Matcher mBI = Pattern.compile("^\\*\"([^*]+?)\\*\"_").matcher(rest);
            if(mBI.find()) { out.append("<strong><em>").append(renderInline(mBI.group(1))).append("</em></strong>"); i+=mBI.group(0).length(); continue; }
            // 'bold'
            Matcher mB = Pattern.compile("^'([^']+?)'").matcher(rest);
            if(mB.find()) { out.append("<strong>").append(escapeHtml(mB.group(1))).append("</strong>"); i+=mB.group(0).length(); continue; }
            // ~italic~
            Matcher mI = Pattern.compile("^~([^~]+?)~").matcher(rest);
            if(mI.find()) { out.append("<em>").append(escapeHtml(mI.group(1))).append("</em>"); i+=mI.group(0).length(); continue; }
            // `code`
            Matcher mC = Pattern.compile("^`([^`]+?)`").matcher(rest);
            if(mC.find()) { out.append("<code>").append(escapeHtml(mC.group(1))).append("</code>"); i+=mC.group(0).length(); continue; }

            out.append(escapeHtml(String.valueOf(text.charAt(i))));
            i++;
        }
        return out.toString();
    }

    // === PARSER + RENDERER: lines -> HTML ===
    static String parseAndRender(String input) {
        List<String> htmlParts = new ArrayList<>();
        List<String> listBuffer = null;
        String[] lines = input.split("\n");

        for(String rawLine : lines) {
            String trimmed = rawLine.trim();
            if(trimmed.isEmpty()) {
                if(listBuffer != null) { htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>"); listBuffer=null; }
                continue;
            }
            if(trimmed.equals("_-_")) {
                if(listBuffer != null) { htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>"); listBuffer=null; }
                htmlParts.add("<hr/>");
                continue;
            }
            Matcher mH = Pattern.compile("^(#{1,6})\\s*(.*)$").matcher(trimmed);
            if(mH.find()) {
                if(listBuffer != null) { htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>"); listBuffer=null; }
                int lvl = mH.group(1).length();
                htmlParts.add("<h"+lvl+">"+renderInline(mH.group(2))+"</h"+lvl+">");
                continue;
            }
            if(trimmed.startsWith("> ")) {
                if(listBuffer != null) { htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>"); listBuffer=null; }
                htmlParts.add("<blockquote>"+renderInline(trimmed.substring(2))+"</blockquote>");
                continue;
            }
            if(trimmed.startsWith("- ")) {
                if(listBuffer==null) listBuffer=new ArrayList<>();
                listBuffer.add("<li>"+renderInline(trimmed.substring(2))+"</li>");
                continue;
            }
            if(listBuffer != null) { htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>"); listBuffer=null; }
            htmlParts.add("<p>"+renderInline(rawLine)+"</p>");
        }
        if(listBuffer != null) htmlParts.add("<ul>"+String.join("", listBuffer)+"</ul>");
        return String.join("\n", htmlParts);
    }

    // === SYSCALL ===
    public static String syscall(String input) {
        return parseAndRender(input);
    }
    public static String parse(String input) { return syscall(input); }

    public static void main(String[] args) throws Exception {
        if(args.length < 1) { System.out.println("Usage: colorup <file.colorup>"); System.exit(1); }
        String content = Files.readString(Paths.get(args[0]));
        System.out.println(parse(content));
    }
              }
