%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

extern int curr_lineno;
extern YYSTYPE cool_yylval;

#define MAX_STR_CONST 1025
static char string_buf[MAX_STR_CONST];
static char *string_buf_ptr;
static int comment_level;
%}

%option noyywrap

%x COMMENT
%x STRING

DARROW          =>
ASSIGN          <-
LE              <=
DIGIT           [0-9]
LOWER           [a-z]
UPPER           [A-Z]
LETTER          [a-zA-Z]
ALNUM           [a-zA-Z0-9_]

%%

[ \f\r\t\v]+                    { }
\n                              { curr_lineno++; }

[cC][lL][aA][sS][sS]            { return CLASS; }
[eE][lL][sS][eE]                { return ELSE; }
[fF][iI]                        { return FI; }
[iI][fF]                        { return IF; }
[iI][nN]                        { return IN; }
[iI][nN][hH][eE][rR][iI][tT][sS] { return INHERITS; }
[iI][sS][vV][oO][iI][dD]        { return ISVOID; }
[lL][eE][tT]                    { return LET; }
[lL][oO][oO][pP]                { return LOOP; }
[pP][oO][oO][lL]                { return POOL; }
[tT][hH][eE][nN]                { return THEN; }
[wW][hH][iI][lL][eE]            { return WHILE; }
[cC][aA][sS][eE]                { return CASE; }
[eE][sS][aA][cC]                { return ESAC; }
[nN][eE][wW]                    { return NEW; }
[oO][fF]                        { return OF; }
[nN][oO][tT]                    { return NOT; }

t[rR][uU][eE]                   { cool_yylval.boolean = 1; return BOOL_CONST; }
f[aA][lL][sS][eE]               { cool_yylval.boolean = 0; return BOOL_CONST; }

{UPPER}({ALNUM})*               { cool_yylval.symbol = stringtable.add_string(yytext); return TYPEID; }
{LOWER}({ALNUM})*               { cool_yylval.symbol = stringtable.add_string(yytext); return OBJECTID; }

{DIGIT}+                        { cool_yylval.symbol = stringtable.add_string(yytext); return INT_CONST; }

{DARROW}                        { return DARROW; }
{ASSIGN}                        { return ASSIGN; }
{LE}                            { return LE; }

"+"                             { return '+'; }
"-"                             { return '-'; }
"*"                             { return '*'; }
"/"                             { return '/'; }
"~"                             { return '~'; }
"<"                             { return '<'; }
"="                             { return '='; }
"."                             { return '.'; }
"@"                             { return '@'; }
","                             { return ','; }
":"                             { return ':'; }
";"                             { return ';'; }
"("                             { return '('; }
")"                             { return ')'; }
"{"                             { return '{'; }
"}"                             { return '}'; }

"--".*                          { }

"(*"                            { BEGIN(COMMENT); comment_level = 1; }
<COMMENT>"(*"                   { comment_level++; }
<COMMENT>"*)"                   { comment_level--; if (comment_level == 0) BEGIN(INITIAL); }
<COMMENT>\n                     { curr_lineno++; }
<COMMENT>.                      { }
<COMMENT><<EOF>>                { cool_yylval.error_msg = "EOF in comment"; BEGIN(INITIAL); return ERROR; }
"*)"                            { cool_yylval.error_msg = "Unmatched *)"; return ERROR; }

\"                              { BEGIN(STRING); string_buf_ptr = string_buf; }
<STRING>\"                      {
                                  BEGIN(INITIAL);
                                  *string_buf_ptr = '\0';
                                  if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                                      cool_yylval.error_msg = "String constant too long";
                                      return ERROR;
                                  }
                                  cool_yylval.symbol = stringtable.add_string(string_buf);
                                  return STR_CONST;
                                }
<STRING>\n                      { curr_lineno++; BEGIN(INITIAL); cool_yylval.error_msg = "Unterminated string constant"; return ERROR; }
<STRING><<EOF>>                 { BEGIN(INITIAL); cool_yylval.error_msg = "EOF in string constant"; return ERROR; }
<STRING>\\n                     { *string_buf_ptr++ = '\n'; }
<STRING>\\t                     { *string_buf_ptr++ = '\t'; }
<STRING>\\b                     { *string_buf_ptr++ = '\b'; }
<STRING>\\f                     { *string_buf_ptr++ = '\f'; }
<STRING>\\.                     { *string_buf_ptr++ = yytext[1]; }
<STRING>.                       {
                                  if (yytext[0] == '\0') {
                                      cool_yylval.error_msg = "String contains null character.";
                                      BEGIN(INITIAL);
                                      return ERROR;
                                  }
                                  if (string_buf_ptr - string_buf >= MAX_STR_CONST - 1) {
                                      cool_yylval.error_msg = "String constant too long";
                                      BEGIN(INITIAL);
                                      return ERROR;
                                  }
                                  *string_buf_ptr++ = yytext[0];
                                }

.                               {
                                  cool_yylval.error_msg = yytext;
                                  fprintf(stderr, "DEBUG: Unmatched char '%s' line %d\n", yytext, curr_lineno);
                                  return ERROR;
                                }

%%
int cool_yylex(void) {
    return yylex();
}
