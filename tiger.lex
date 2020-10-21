%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/
int commentCnt = 0;
/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
  char *ptr = (char *)checked_malloc(strlen(str) + 1);
  char *ret = ptr;
  char *crt = (char *)str + 1;
  while(*crt != '\0') {
    if(*crt == '\\') {
      crt++;
      if(*crt == '\0') {
        *ptr = '\\';
        ptr++;
        break;
      }
      if(*crt >= '0' && *crt <= '9') {
        int num = *crt-'0'; crt++;
        if(*crt >= '0' && *crt <= '9') {
          num = num*10+(*crt-'0'); crt++;
          if(*crt >= '0' && *crt <= '9') {
            num = num*10+(*crt-'0'); crt++;
          }
        }
        *ptr = (char)num; ptr++;
        continue;
      }
      switch(*crt) {
        case 'n': {*ptr = '\n'; break;}
        case 't': {*ptr = '\t'; break;}
        case '\"': {*ptr = '\"'; break;}
        case '^': {
          if(*(crt+1) >= 'A' && *(crt+1) <= 'Z') {
            crt++;
            *ptr = (char)(*crt-'A'+1);
          }
          break;
        }
        default: {
          if(*crt == '\\' && !(*(crt+1) == '\n' || *(crt+1) == '\t')) {
            *ptr = '\\';
            break;
          }
          while(*crt != '\\' && *crt != '\0') crt++;
          if(*crt == '\\') ptr--;
        }  
      }
      crt++;ptr++;
    }
    else {
      *ptr = *crt;
      ptr++; crt++;
    }
  }
  ptr--;
  *ptr = '\0';
  if(strlen(ret) == 0) return NULL;
  return ret;
}

%}
  /* You can add lex definitions here. */

%START COMMENT

%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"/*" {adjust(); commentCnt++; BEGIN COMMENT;}
<INITIAL>[0-9][0-9]* {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>, {adjust(); return COMMA;}
<INITIAL>: {adjust(); return COLON;}
<INITIAL>; {adjust(); return SEMICOLON;}
<INITIAL>\( {adjust(); return LPAREN;}
<INITIAL>\) {adjust(); return RPAREN;}
<INITIAL>\[ {adjust(); return LBRACK;}
<INITIAL>\] {adjust(); return RBRACK;}
<INITIAL>\{ {adjust(); return LBRACE;}
<INITIAL>\} {adjust(); return RBRACE;}
<INITIAL>\. {adjust(); return DOT;}
<INITIAL>\+ {adjust(); return PLUS;}
<INITIAL>\- {adjust(); return MINUS;}
<INITIAL>\* {adjust(); return TIMES;}
<INITIAL>\/ {adjust(); return DIVIDE;}
<INITIAL>= {adjust(); return EQ;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>"<" {adjust(); return LT;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>">" {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"&" {adjust(); return AND;}
<INITIAL>"|" {adjust(); return OR;}
<INITIAL>":=" {adjust(); return ASSIGN;}
<INITIAL>"array" {adjust(); return ARRAY;}
<INITIAL>"if" {adjust(); return IF;}
<INITIAL>"then" {adjust(); return THEN;}
<INITIAL>"else" {adjust(); return ELSE;}
<INITIAL>"while" {adjust(); return WHILE;}
<INITIAL>"for" {adjust(); return FOR;}
<INITIAL>"to" {adjust(); return TO;}
<INITIAL>"do" {adjust(); return DO;}
<INITIAL>"let" {adjust(); return LET;}
<INITIAL>"in" {adjust(); return IN;}
<INITIAL>"end" {adjust(); return END;}
<INITIAL>"of" {adjust(); return OF;}
<INITIAL>"break" {adjust(); return BREAK;}
<INITIAL>"nil" {adjust(); return NIL;}
<INITIAL>"function" {adjust(); return FUNCTION;}
<INITIAL>"var" {adjust(); return VAR;}
<INITIAL>"type" {adjust(); return TYPE;}
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust(); yylval.sval=String(yytext); return ID;}
<INITIAL>\"(\\.|[^"\\]|\\[ \t\n]+\\)*\" {adjust(); yylval.sval=getstr(yytext); return STRING;}
<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>(" "|\t) {adjust(); continue;}
<COMMENT>"/*" {adjust(); commentCnt++;}
<COMMENT>"*/" {adjust(); if(--commentCnt == 0) BEGIN INITIAL;}
<COMMENT>([^\*\/]*)|\*|\/ {adjust();}
<INITIAL>. {adjust(); EM_error(charPos, "illegal symbol %s", yytext);}
