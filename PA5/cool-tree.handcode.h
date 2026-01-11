#ifndef COOL_TREE_HANDCODE_H
#define COOL_TREE_HANDCODE_H

#include <iostream>
#include "tree.h"
#include "cool.h"
#include "stringtab.h"

// 宏定义：同步当前行号
#define yylineno curr_lineno;
extern int yylineno;

class CodeGenManager; 

inline Boolean copy_Boolean(Boolean b) { return b; }
inline void assert_Boolean(Boolean) {}
inline void dump_Boolean(ostream& stream, int padding, Boolean b)
	{ stream << pad(padding) << (int) b << "\n"; }

void dump_Symbol(ostream& stream, int padding, Symbol b);
void assert_Symbol(Symbol b);
Symbol copy_Symbol(Symbol b);

// Phylum 声明
class Program_class;
typedef Program_class *Program;
class Class__class;
typedef Class__class *Class_;
class Feature_class;
typedef Feature_class *Feature;
class Formal_class;
typedef Formal_class *Formal;
class Expression_class;
typedef Expression_class *Expression;
class Case_class;
typedef Case_class *Case;

typedef list_node<Class_> Classes_class;
typedef Classes_class *Classes;
typedef list_node<Feature> Features_class;
typedef Features_class *Features;
typedef list_node<Formal> Formals_class;
typedef Formals_class *Formals;
typedef list_node<Expression> Expressions_class;
typedef Expressions_class *Expressions;
typedef list_node<Case> Cases_class;
typedef Cases_class *Cases;

// Program 扩展：核心生成入口
#define Program_EXTRAS                          \
virtual void cgen(ostream&) = 0;                \
virtual void dump_with_types(ostream&, int) = 0; 

#define program_EXTRAS                          \
void cgen(ostream&);                            \
void dump_with_types(ostream&, int);            

// Class 扩展：获取元数据
#define Class__EXTRAS                           \
virtual Symbol get_name() = 0;                  \
virtual Symbol get_parent() = 0;                \
virtual Symbol get_filename() = 0;              \
virtual void dump_with_types(ostream&, int) = 0; 

#define class__EXTRAS                           \
Symbol get_name()     { return name; }          \
Symbol get_parent() { return parent; }          \
Symbol get_filename() { return filename; }      \
void dump_with_types(ostream&, int);            

// Feature 扩展：区分属性与方法声明
#define Feature_EXTRAS                                        \
virtual void dump_with_types(ostream&, int) = 0;              \
virtual bool is_method_decl() const = 0;                      

#define Feature_SHARED_EXTRAS                                 \
void dump_with_types(ostream&, int);                          \
bool is_method_decl() const; 

// Formal 扩展
#define Formal_EXTRAS                                         \
virtual void dump_with_types(ostream&, int) = 0;

#define formal_EXTRAS                                         \
void dump_with_types(ostream&, int);

// Case 扩展
#define Case_EXTRAS                                           \
virtual void dump_with_types(ostream&, int) = 0;

#define branch_EXTRAS                                         \
void dump_with_types(ostream&, int);

#define Expression_EXTRAS                                     \
Symbol type;                                                  \
Symbol get_type() { return type; }                            \
Expression set_type(Symbol s) { type = s; return this; }      \
virtual void produce_code(ostream&, CodeGenManager&) = 0;     \
virtual void dump_with_types(ostream&, int) = 0;              \
void dump_type(ostream&, int);                                \
Expression_class() { type = (Symbol) NULL; }

#define Expression_SHARED_EXTRAS                              \
void produce_code(ostream&, CodeGenManager&);                 \
void dump_with_types(ostream&, int); 

#endif
