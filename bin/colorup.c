// colorup.c - C implementation of ColorUp syscall
// Calls all layers: Lexer -> Parser -> Renderer
// Not JavaScript - Pure C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_OUT 65536

// === Utils ===
char* escape_html(const char* s) {
    static char out[MAX_OUT];
    int j=0;
    for(int i=0; s[i] && j < MAX_OUT-10; i++) {
        if(s[i]=='&'){ strcpy(out+j,"&amp;"); j+=5; }
        else if(s[i]=='<'){ strcpy(out+j,"&lt;"); j+=4; }
        else if(s[i]=='>'){ strcpy(out+j,"&gt;"); j+=4; }
        else if(s[i]=='"'){ strcpy(out+j,"&quot;"); j+=6; }
        else out[j++]=s[i];
    }
    out[j]='\0';
    return out;
}

// Simple inline parser -> directly renders to HTML for C (syscall style)
// For clarity, we merge Parser+Renderer in one pass, but syscall still calls all layers logically

void render_inline_to(const char* text, char* out);

void syscall_parse_inline(const char* text, char* out) {
    // This is Lexer + Parser + Renderer combined for C simplicity
    render_inline_to(text, out);
}

void render_inline_to(const char* text, char* out) {
    out[0]='\0';
    int len = strlen(text);
    int i=0;
    while(i < len) {
        // $color[content]$color
        if(text[i]=='$' && i+1 < len) {
            // $link[
            if(strncmp(text+i, "$link[", 6)==0) {
                char* end = strstr(text+i, "]$link");
                if(end) {
                    char inner[1024]; strncpy(inner, text+i+6, end-(text+i+6)); inner[end-(text+i+6)]='\0';
                    char* sep = strchr(inner, '|');
                    char t[512], url[512];
                    if(sep) {
                        strncpy(t, inner, sep-inner); t[sep-inner]='\0';
                        strcpy(url, sep+1);
                    } else { strcpy(t, inner); strcpy(url, "#"); }
                    char rendered_t[2048]; render_inline_to(t, rendered_t);
                    char tmp[4096]; sprintf(tmp, "<a href=\"%s\">%s</a>", escape_html(url), rendered_t);
                    strcat(out, tmp);
                    i = (end - text) + 6;
                    continue;
                }
            }
            // $red[ etc
            char color[64]; int ci=0;
            int j=i+1;
            while(j < len && (isalpha(text[j]) || text[j]=='#' ) && ci<60) color[ci++]=text[j++];
            color[ci]='\0';
            if(j < len && text[j]=='[' && ci>0) {
                char endTag[128]; sprintf(endTag, "]$%s", color);
                char* remaining = (char*)text + j + 1;
                char* endPos = strstr(remaining, endTag);
                if(endPos) {
                    int innerLen = endPos - remaining;
                    char inner[4096]; strncpy(inner, remaining, innerLen); inner[innerLen]='\0';
                    char renderedInner[8192]; render_inline_to(inner, renderedInner);
                    char tmp[10000]; sprintf(tmp, "<span style=\"color:%s\">%s</span>", escape_html(color), renderedInner);
                    strcat(out, tmp);
                    i = (endPos - text) + strlen(endTag);
                    continue;
                }
            }
        }
        // *"text*"_
        if(strncmp(text+i, "*\"", 2)==0) {
            char* end = strstr(text+i+2, "*\"_");
            if(end) {
                char inner[1024]; strncpy(inner, text+i+2, end-(text+i+2)); inner[end-(text+i+2)]='\0';
                char rendered[2048]; render_inline_to(inner, rendered);
                char tmp[4096]; sprintf(tmp, "<strong><em>%s</em></strong>", rendered);
                strcat(out, tmp);
                i = (end - text) + 3;
                continue;
            }
        }
        // 'bold'
        if(text[i]=='\'') {
            char* end = strchr(text+i+1, '\'');
            if(end) {
                char inner[1024]; strncpy(inner, text+i+1, end-(text+i+1)); inner[end-(text+i+1)]='\0';
                char tmp[2048]; sprintf(tmp, "<strong>%s</strong>", escape_html(inner));
                strcat(out, tmp);
                i = (end - text) + 1;
                continue;
            }
        }
        // ~italic~
        if(text[i]=='~') {
            char* end = strchr(text+i+1, '~');
            if(end) {
                char inner[1024]; strncpy(inner, text+i+1, end-(text+i+1)); inner[end-(text+i+1)]='\0';
                char tmp[2048]; sprintf(tmp, "<em>%s</em>", escape_html(inner));
                strcat(out, tmp);
                i = (end - text) + 1;
                continue;
            }
        }
        // `code`
        if(text[i]=='`') {
            char* end = strchr(text+i+1, '`');
            if(end) {
                char inner[1024]; strncpy(inner, text+i+1, end-(text+i+1)); inner[end-(text+i+1)]='\0';
                char tmp[2048]; sprintf(tmp, "<code>%s</code>", escape_html(inner));
                strcat(out, tmp);
                i = (end - text) + 1;
                continue;
            }
        }
        // normal char
        char ch[2] = {text[i], '\0'};
        char esc[16]; strcpy(esc, escape_html(ch));
        strcat(out, esc);
        i++;
    }
}

// === PARSER + RENDERER: lines -> HTML ===
void parse_and_render(const char* input, char* htmlOut) {
    htmlOut[0]='\0';
    char* copy = strdup(input);
    char* line = strtok(copy, "\n");
    int first=1;
    int inList=0;
    char listBuf[16384]; listBuf[0]='\0';

    while(line) {
        char* trimmed = line;
        while(isspace(*trimmed)) trimmed++;
        // trim end
        char* end = trimmed + strlen(trimmed) -1;
        while(end>trimmed && isspace(*end)) { *end='\0'; end--; }

        if(strlen(trimmed)==0) {
            if(inList){ char tmp[20000]; sprintf(tmp, "<ul>%s</ul>\n", listBuf); strcat(htmlOut, tmp); listBuf[0]='\0'; inList=0; }
            line = strtok(NULL, "\n");
            continue;
        }
        if(strcmp(trimmed, "_-_")==0) {
            if(inList){ char tmp[20000]; sprintf(tmp, "<ul>%s</ul>\n", listBuf); strcat(htmlOut, tmp); listBuf[0]='\0'; inList=0; }
            if(!first) strcat(htmlOut, "\n");
            strcat(htmlOut, "<hr/>");
            first=0;
            line = strtok(NULL, "\n");
            continue;
        }
        // Heading
        if(trimmed[0]=='#') {
            int lvl=0; while(trimmed[lvl]=='#') lvl++;
            if(inList){ char tmp[20000]; sprintf(tmp, "<ul>%s</ul>\n", listBuf); strcat(htmlOut, tmp); listBuf[0]='\0'; inList=0; }
            char* content = trimmed+lvl;
            while(isspace(*content)) content++;
            char rendered[8192]; render_inline_to(content, rendered);
            char tmp[10000]; sprintf(tmp, "%s<h%d>%s</h%d>", first?"":"\n", lvl, rendered, lvl);
            strcat(htmlOut, tmp);
            first=0;
            line = strtok(NULL, "\n");
            continue;
        }
        // blockquote
        if(strncmp(trimmed, "> ", 2)==0) {
            if(inList){ char tmp[20000]; sprintf(tmp, "<ul>%s</ul>\n", listBuf); strcat(htmlOut, tmp); listBuf[0]='\0'; inList=0; }
            char rendered[8192]; render_inline_to(trimmed+2, rendered);
            char tmp[10000]; sprintf(tmp, "%s<blockquote>%s</blockquote>", first?"":"\n", rendered);
            strcat(htmlOut, tmp);
            first=0;
            line = strtok(NULL, "\n");
            continue;
        }
        // list
        if(strncmp(trimmed, "- ", 2)==0) {
            char rendered[8192]; render_inline_to(trimmed+2, rendered);
            char tmp[10000]; sprintf(tmp, "<li>%s</li>", rendered);
            strcat(listBuf, tmp);
            inList=1;
            line = strtok(NULL, "\n");
            continue;
        }
        if(inList){ char tmp[20000]; sprintf(tmp, "<ul>%s</ul>\n", listBuf); strcat(htmlOut, tmp); listBuf[0]='\0'; inList=0; first=0; }

        char rendered[8192]; render_inline_to(line, rendered);
        char tmp[10000]; sprintf(tmp, "%s<p>%s</p>", first?"":"\n", rendered);
        strcat(htmlOut, tmp);
        first=0;

        line = strtok(NULL, "\n");
    }
    if(inList){ char tmp[20000]; sprintf(tmp, "%s<ul>%s</ul>", first?"":"\n", listBuf); strcat(htmlOut, tmp); }
    free(copy);
}

// === SYSCALL ===
void syscall_parse(const char* input, char* out) {
    // 1. LEXER (implicit)
    // 2. PARSER (parse_and_render does parsing)
    // 3. RENDERER (same)
    parse_and_render(input, out);
}

int main(int argc, char* argv[]) {
    if(argc < 2) { printf("Usage: colorup <file.colorup>\n"); return 1; }
    FILE* f = fopen(argv[1], "r");
    if(!f){ perror("Error"); return 1; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    char* content = malloc(sz+1);
    fread(content,1,sz,f); content[sz]='\0';
    fclose(f);

    char html[MAX_OUT*4];
    syscall_parse(content, html);
    printf("%s\n", html);
    free(content);
    return 0;
}
