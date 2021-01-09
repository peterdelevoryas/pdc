#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>

fn read_file(path: *const char) -> *char:
    var file = fopen(path, "rb")
    if !file:
        printf("unable to open '%s': %s\n", path, strerror(errno))
        return NULL

    fseek(file, 0, SEEK_END)
    var size = ftell(file)
    fseek(file, 0, SEEK_SET)

    var text = (char*)malloc(size + 1)
    assert(text)
    assert(fread(text, size, 1, file) == 1)
    text[size] = 0
    fclose(file)
    return text

struct string_intern {
    index: *int;
    index_len: int;
    index_cap: int;

    big_string: *char;
    big_string_len: int;
    big_string_cap: int;
};

fn intern_string(s: *const char, n: int, si: *struct string_intern) -> int:
    return 0

enum token {
    token_fn,
    token_var,
    token_import,
    token_i8,
    token_i32,
    token_num_keywords,
    token_integer_literal,
    token_string_literal,
    token_identifier,
    token_lparen,
    token_rparen,
    token_lbrace,
    token_rbrace,
    token_lbracket,
    token_rbracket,
    token_colon,
    token_comma,
    token_semicolon,
    token_equals,
    token_star,
    token_plus,
    token_minus,
    token_arrow,
    token_dot,
    token_ellipsis,
    token_space,
    token_eol,
    token_eof,
    token_error,
};

fn token2string(t: enum token) -> *const char:
    switch t:
        case token_fn:
            return "fn"
        case token_var:
            return "var"
        case token_import:
            return "import"
        case token_i8:
            return "i8"
        case token_i32:
            return "i32"
        case token_integer_literal:
            return "integer literal"
        case token_string_literal:
            return "string literal"
        case token_identifier:
            return "identifier"
        case token_lparen:
            return "("
        case token_rparen:
            return ")"
        case token_lbrace:
            return "{"
        case token_rbrace:
            return "}"
        case token_lbracket:
            return "["
        case token_rbracket:
            return "]"
        case token_colon:
            return ":"
        case token_comma:
            return ","
        case token_semicolon:
            return ";"
        case token_equals:
            return "="
        case token_star:
            return "*"
        case token_plus:
            return "+"
        case token_minus:
            return "-"
        case token_arrow:
            return "->"
        case token_dot:
            return "."
        case token_ellipsis:
            return "..."
        case token_space:
            return "' '"
        case token_eol:
            return "'\n'"
        case token_eof:
            return "'\\0'"
        default:
            return NULL

fn char2token(c: char) -> enum token:
    switch c:
        case 'a'...'z':
        case 'A'...'Z':
        case '_':
            return token_identifier
        case '0'...'9':
            return token_integer_literal
        case '"':
            return token_string_literal
        case '(':
            return token_lparen
        case ')':
            return token_rparen
        case '{':
            return token_lbrace
        case '}':
            return token_rbrace
        case '[':
            return token_lbracket
        case ']':
            return token_rbracket
        case ':':
            return token_colon
        case ',':
            return token_comma
        case ';':
            return token_semicolon
        case '=':
            return token_equals
        case '*':
            return token_star
        case '+':
            return token_plus
        case '-':
            return token_minus
        case '.':
            return token_dot
        case ' ':
            return token_space
        case '\n':
            return token_eol
        case '\0':
            return token_eof
        default:
            return token_error

struct translation_unit {
    strings: struct string_intern;
    int keywords[token_num_keywords];
};

struct parser {
    path: *const char;
    text: *const char;
    unit: *struct translation_unit;
    token: enum token;
    line_no: int;
    start: int;
    end: int;
    string: int;
    indent: int;
    prev_indent: int;
};

fn bump(p: *struct parser) -> void:
    while 1:
        p.start = p.end

        if p.indent == -1:
            while p.text[p.end] == ' ':
                p.end += 1
            p.indent = (p.end - p.start) / 4

        if p.prev_indent < p.indent:
            p.prev_indent += 1
            p.token = token_lbrace
            break

        if p.indent < p.prev_indent:
            p.prev_indent -= 1
            p.token = token_rbrace
            break

        p.start = p.end
        p.string = 0
        var text = &p.text[p.start]
        var next = char2token(text[0])
        switch next:
            default:
                break

        break

fn parse_file(path: *const char, text: *const char, unit: *struct translation_unit) -> void:
    var p = (struct parser){}
    p.path = path
    p.text = text
    p.token = token_eof
    p.line_no = 1

    for bump(&p); p.token != token_eof; bump(&p):
        printf("%-20s %2d %2d\n", "", 1, 2)

fn main(argc: int, argv: **char) -> int:
    for var i = 1; i < argc; i++:
        if strcmp(argv[i], "-h") == 0:
            printf("usage: pdc [-h] file...\n")
            return 0

    var unit = (struct translation_unit){}
    for var i = 0; i < token_num_keywords; i++:
        var s = token2string(i)
        unit.keywords[i] = intern_string(s, strlen(s), &unit.strings)

    for var i = 1; i < argc; i++:
        var path = argv[i]
        var text = read_file(path)
        if !text:
            return 1
        parse_file(path, text, &unit)
        free(text)

    return 0
