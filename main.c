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

fn intern_string(s: *const char, n: int, intern: *struct string_intern) -> int:
    for var i = 0; i < intern.index_len; i++:
        var j = intern.index[i]
        var t = &intern.big_string[j]
        if strlen(t) != n:
            continue
        if memcmp(t, s, n) != 0:
            continue
        return j

    var j = intern.big_string_len
    if j + n + 1 > intern.big_string_cap:
        intern.big_string_cap += 4096
        intern.big_string = realloc(intern.big_string, intern.big_string_cap)

    memcpy(&intern.big_string[j], s, n)
    intern.big_string[j + n] = 0
    intern.big_string_len = j + n + 1

    if intern.index_len >= intern.index_cap:
        intern.index_cap += 32
        intern.index = realloc(intern.index, intern.index_cap * sizeof(int))

    intern.index[intern.index_len] = j
    intern.index_len += 1

    return j

fn get_string(j: int, intern: *struct string_intern) -> *const char:
    return &intern.big_string[j]

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
            return "undefined"

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
    keywords: [token_num_keywords]int;
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

var RED = "\x1b[0;31m";
var NORMAL = "\x1b[0m";

fn print_parse_error(p: *struct parser, msg: *const char) -> void:
    var line_start = p.start
    for ; line_start != 0 && p.text[line_start - 1] != '\n'; line_start -= 1:
        continue

    var line_end = p.end
    for ; p.text[line_end] != '\0' && p.text[line_end] != '\n'; line_end += 1:
        continue

    var line_offset = p.start - line_start

    printf("%s:%d:%d: %serror:%s %s\n", p.path, p.line_no, line_offset, RED, NORMAL, msg)
    printf(" %4d | %.*s\n", p.line_no, line_end - line_start, &p.text[line_start])
    printf("      | ");
    for var i = line_start; i < p.start; i++:
        if p.text[i] == '\t':
            printf("\t")
            continue
        printf(" ")
    printf("%s", RED)
    for var i = p.start; i < p.end; i++:
        printf("^")
    printf("%s\n", NORMAL)

fn bump_identifier(p: *struct parser) -> enum token:
    while 1:
        switch p.text[p.end]:
            case 'a'...'z':
            case 'A'...'Z':
            case '0'...'9':
            case '_':
                p.end += 1
                continue
        break
    var s = &p.text[p.start]
    var n = p.end - p.start
    p.string = intern_string(s, n, &p.unit.strings)
    for var i = 0; i < token_num_keywords; i++:
        if p.unit.keywords[i] == p.string:
            return i
    return token_identifier

fn bump_integer_literal(p: *struct parser) -> void:
    while 1:
        switch p.text[p.end]:
            case '0'...'9':
                p.end += 1
                continue
        break
    var s = &p.text[p.start]
    var n = p.end - p.start
    p.string = intern_string(s, n, &p.unit.strings)

fn bump_string_literal(p: *struct parser) -> void:
    p.end += 1
    var s: [256]char
    var n = 0
    while 1:
        var c = p.text[p.end]
        if c == '\0':
            print_parse_error(p, "unterminated string literal")
            exit(1)
        if c == '"':
            p.end += 1
            break
        if c == '\\':
            p.end += 1
            var d = p.text[p.end]
            switch d:
                case '\0':
                    print_parse_error(p, "unterminated string literal")
                    exit(1)
                case '"':
                    c = '"'
                    break
                case 'n':
                    c = '\n'
                    break
                case 't':
                    c = '\t'
                    break
                case 'r':
                    c = '\r'
                    break
                case '0':
                    c = '\0'
                    break
                default:
                    print_parse_error(p, "invalid escape sequence");
                    exit(1)

        s[n] = c
        n += 1
        p.end += 1

    p.string = intern_string(s, n, &p.unit.strings)

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

        var prev = p.token
        var next = char2token(p.text[p.end])
        switch next:
            case token_identifier:
                next = bump_identifier(p)
                break
            case token_integer_literal:
                bump_integer_literal(p)
                break
            case token_string_literal:
                bump_string_literal(p)
                break
            case token_lparen:
            case token_rparen:
            case token_lbrace:
            case token_rbrace:
            case token_lbracket:
            case token_rbracket:
            case token_colon:
            case token_comma:
            case token_semicolon:
            case token_equals:
            case token_star:
            case token_plus:
            case token_eof:
                p.end += 1
                break
            case token_space:
                p.end += 1
                continue
            case token_minus:
                if p.text[p.end + 1] == '>':
                    next = token_arrow
                    p.end += 2
                    break
                p.end += 1
                break
            case token_dot:
                if p.text[p.end + 1] == '.' &&
                   p.text[p.end + 2] == '.':
                    next = token_ellipsis
                    p.end += 3
                    break
                p.end += 1
                break
            case token_eol:
                p.indent = -1
                while p.text[p.end] == '\n':
                    p.line_no += 1
                    p.end += 1
                switch prev:
                    case token_rparen:
                    case token_i32:
                    case token_identifier:
                    case token_integer_literal:
                        next = token_semicolon
                        break
                    default:
                        continue
                break
            default:
                p.end += 1
                print_parse_error(p, "unexpected character")
                exit(1)

        p.token = next
        break

fn parse_file(path: *const char, text: *const char, unit: *struct translation_unit) -> void:
    var p = (struct parser){}
    p.path = path
    p.text = text
    p.unit = unit
    p.token = token_eof
    p.line_no = 1

    for bump(&p); p.token != token_eof; bump(&p):
        var s = token2string(p.token)
        var t = (const char*)""
        if p.string != 0:
            t = get_string(p.string, &unit.strings)
        printf("%-20s %2d %2d %s\n", s, p.start, p.end, t)

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
