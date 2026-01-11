#ifndef COOL_TREE_H
#define COOL_TREE_H
//////////////////////////////////////////////////////////
//
// file: cool-tree.h
//
//
//////////////////////////////////////////////////////////

#include "tree.h"
#include "cool-tree.handcode.h"
#include <vector>

class TranslationContext;

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Program
// ---------------------------------------------------------------------
typedef class Program_class *Program;

class Program_class : public tree_node {
public:
   tree_node *copy()     { return copy_Program(); }
   virtual Program copy_Program() = 0;

#ifdef Program_EXTRAS
   Program_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Class_
// ---------------------------------------------------------------------
typedef class Class__class *Class_;

class Class__class : public tree_node {
public:
   tree_node *copy()     { return copy_Class_(); }
   virtual Class_ copy_Class_() = 0;

#ifdef Class__EXTRAS
   Class__EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Feature
// ---------------------------------------------------------------------
typedef class Feature_class *Feature;

class Feature_class : public tree_node {
public:
   tree_node *copy()     { return copy_Feature(); }
   virtual Feature copy_Feature() = 0;
   
   virtual bool is_method_node() const = 0; 

#ifdef Feature_EXTRAS
   Feature_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Formal
// ---------------------------------------------------------------------
typedef class Formal_class *Formal;

class Formal_class : public tree_node {
public:
   tree_node *copy()     { return copy_Formal(); }
   virtual Formal copy_Formal() = 0;

#ifdef Formal_EXTRAS
   Formal_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Expression
// ---------------------------------------------------------------------
typedef class Expression_class *Expression;

class Expression_class : public tree_node {
public:
   tree_node *copy()     { return copy_Expression(); }
   virtual Expression copy_Expression() = 0;

#ifdef Expression_EXTRAS
   Expression_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 基础 Phylum 定义：Case
// ---------------------------------------------------------------------
typedef class Case_class *Case;

class Case_class : public tree_node {
public:
   tree_node *copy()     { return copy_Case(); }
   virtual Case copy_Case() = 0;

#ifdef Case_EXTRAS
   Case_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 核心逻辑：method_class (方法节点)
// ---------------------------------------------------------------------

class method_class : public Feature_class {
public:
   Symbol name;
   Formals formals;
   Symbol return_type;
   Expression expr;
public:
   method_class(Symbol a1, Formals a2, Symbol a3, Expression a4) :
     name(a1), formals(a2), return_type(a3), expr(a4) {}

   Feature copy_Feature();
   void dump(ostream& stream, int n);
   
   // 实现谓词逻辑
   bool is_method_node() const { return true; }

   /**
    * @brief 计算该方法激活记录（Stack Frame）所需的总字节数
    * 包括固定的 FP, SELF, RA (12字节) 以及所有参数压栈空间
    */
   int calculate_activation_record_size() {
      int formal_count = 0;
      for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
          formal_count++;
      }
      // 3个固定寄存器槽位 (12 bytes) + 参数数量 * 4
      return 12 + (formal_count * 4);
   }

   // 配合 TranslationContext 的新生成接口
   void emit_mips_asm(ostream& stream, CgenNode* host_class);

#ifdef Feature_SHARED_EXTRAS
   Feature_SHARED_EXTRAS
#endif
#ifdef method_EXTRAS
   method_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 属性节点定义：attr_class
// ---------------------------------------------------------------------
class attr_class : public Feature_class {
public:
   Symbol name;
   Symbol type_decl;
   Expression init;
public:
   attr_class(Symbol a1, Symbol a2, Expression a3) {
      name = a1;
      type_decl = a2;
      init = a3;
   }
   Feature copy_Feature();
   void dump(ostream& stream, int n);
   bool is_method_node() const { return false; }

#ifdef Feature_SHARED_EXTRAS
   Feature_SHARED_EXTRAS
#endif
#ifdef attr_EXTRAS
   attr_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：assign_class (赋值)
// ---------------------------------------------------------------------
class assign_class : public Expression_class {
public:
   Symbol name;
   Expression expr;
public:
   assign_class(Symbol a1, Expression a2) {
      name = a1;
      expr = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   
   /**
    * @brief 翻译赋值逻辑
    * 222
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef assign_EXTRAS
   assign_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：static_dispatch_class (静态分派)
// ---------------------------------------------------------------------
class static_dispatch_class : public Expression_class {
public:
   Expression expr;
   Symbol type_name;
   Symbol name;
   Expressions actual;
public:
   static_dispatch_class(Expression a1, Symbol a2, Symbol a3, Expressions a4) {
      expr = a1;
      type_name = a2;
      name = a3;
      actual = a4;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef static_dispatch_EXTRAS
   static_dispatch_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：dispatch_class (动态分派)
// ---------------------------------------------------------------------
class dispatch_class : public Expression_class {
public:
   Expression expr;
   Symbol name;
   Expressions actual;
public:
   dispatch_class(Expression a1, Symbol a2, Expressions a3) {
      expr = a1;
      name = a2;
      actual = a3;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   
   /**
    * @brief 翻译分派逻辑
    * 222
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef dispatch_EXTRAS
   dispatch_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：cond_class (条件分支)
// ---------------------------------------------------------------------
class cond_class : public Expression_class {
public:
   Expression pred;
   Expression then_exp;
   Expression else_exp;
public:
   cond_class(Expression a1, Expression a2, Expression a3) {
      pred = a1;
      then_exp = a2;
      else_exp = a3;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   
   /**
    * @brief 条件跳转生成
    * 使用独立的 label 生成器
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef cond_EXTRAS
   cond_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：loop_class (循环)
// ---------------------------------------------------------------------
class loop_class : public Expression_class {
public:
   Expression pred;
   Expression body;
public:
   loop_class(Expression a1, Expression a2) {
      pred = a1;
      body = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef loop_EXTRAS
   loop_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：typcase_class (分支选择)
// ---------------------------------------------------------------------
class typcase_class : public Expression_class {
public:
   Expression expr;
   Cases cases;
public:
   typcase_class(Expression a1, Cases a2) {
      expr = a1;
      cases = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef typcase_EXTRAS
   typcase_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：block_class (语句块)
// ---------------------------------------------------------------------
class block_class : public Expression_class {
public:
   Expressions body;
public:
   block_class(Expressions a1) {
      body = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef block_EXTRAS
   block_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 表达式子类：let_class (局部变量绑定)
// ---------------------------------------------------------------------
class let_class : public Expression_class {
public:
   Symbol identifier;
   Symbol type_decl;
   Expression init;
   Expression body;
public:
   let_class(Symbol a1, Symbol a2, Expression a3, Expression a4) {
      identifier = a1;
      type_decl = a2;
      init = a3;
      body = a4;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);

   /**
    * @brief 翻译 Let 绑定
    * 111
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef let_EXTRAS
   let_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 算术运算：基础加/减/乘/除
// ---------------------------------------------------------------------
class plus_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   plus_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef plus_EXTRAS
   plus_EXTRAS
#endif
};

class sub_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   sub_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef sub_EXTRAS
   sub_EXTRAS
#endif
};

class mul_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   mul_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef mul_EXTRAS
   mul_EXTRAS
#endif
};

class divide_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   divide_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef divide_EXTRAS
   divide_EXTRAS
#endif
};

class neg_class : public Expression_class {
public:
   Expression e1;
public:
   neg_class(Expression a1) {
      e1 = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef neg_EXTRAS
   neg_EXTRAS
#endif
};

class lt_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   lt_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef lt_EXTRAS
   lt_EXTRAS
#endif
};

class eq_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   eq_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef eq_EXTRAS
   eq_EXTRAS
#endif
};

class leq_class : public Expression_class {
public:
   Expression e1;
   Expression e2;
public:
   leq_class(Expression a1, Expression a2) {
      e1 = a1;
      e2 = a2;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef leq_EXTRAS
   leq_EXTRAS
#endif
};

class comp_class : public Expression_class {
public:
   Expression e1;
public:
   comp_class(Expression a1) {
      e1 = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef comp_EXTRAS
   comp_EXTRAS
#endif
};


class int_const_class : public Expression_class {
public:
   Symbol token;
public:
   int_const_class(Symbol a1) {
      token = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef int_const_EXTRAS
   int_const_EXTRAS
#endif
};

class bool_const_class : public Expression_class {
public:
   Boolean val;
public:
   bool_const_class(Boolean a1) {
      val = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef bool_const_EXTRAS
   bool_const_EXTRAS
#endif
};

class string_const_class : public Expression_class {
public:
   Symbol token;
public:
   string_const_class(Symbol a1) {
      token = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef string_const_EXTRAS
   string_const_EXTRAS
#endif
};

class object_class : public Expression_class {
public:
   Symbol name;
public:
   object_class(Symbol a1) {
      name = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);

   /**
    * @brief 翻译变量加载逻辑
    * 111
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef object_EXTRAS
   object_EXTRAS
#endif
};

class new__class : public Expression_class {
public:
   Symbol type_name;
public:
   new__class(Symbol a1) {
      type_name = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef new__EXTRAS
   new__EXTRAS
#endif
};

class isvoid_class : public Expression_class {
public:
   Expression e1;
public:
   isvoid_class(Expression a1) {
      e1 = a1;
   }
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef isvoid_EXTRAS
   isvoid_EXTRAS
#endif
};

class no_expr_class : public Expression_class {
public:
   no_expr_class() {}
   Expression copy_Expression();
   void dump(ostream& stream, int n);
   
   /**
    * @brief 空表达式生成
    * 111
    */
   void produce_code(ostream& s, CodeGenManager& manager);

#ifdef Expression_SHARED_EXTRAS
   Expression_SHARED_EXTRAS
#endif
#ifdef no_expr_EXTRAS
   no_expr_EXTRAS
#endif
};

// ---------------------------------------------------------------------
// 构造函数工厂声明 (用于词法/语法解析器)
// ---------------------------------------------------------------------
Program program(Classes);
Class_ class_(Symbol, Symbol, Features, Symbol);
Feature method(Symbol, Formals, Symbol, Expression);
Feature attr(Symbol, Symbol, Expression);
Formal formal(Symbol, Symbol);
Case branch(Symbol, Symbol, Expression);
Expression assign(Symbol, Expression);
Expression static_dispatch(Expression, Symbol, Symbol, Expressions);
Expression dispatch(Expression, Symbol, Expressions);
Expression cond(Expression, Expression, Expression);
Expression loop(Expression, Expression);
Expression typcase(Expression, Cases);
Expression block(Expressions);
Expression let(Symbol, Symbol, Expression, Expression);
Expression plus(Expression, Expression);
Expression sub(Expression, Expression);
Expression mul(Expression, Expression);
Expression divide(Expression, Expression
