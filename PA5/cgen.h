#ifndef CGEN_H
#define CGEN_H

#include <assert.h>
#include <stdio.h>
#include <stack>
#include <vector>
#include <map>
#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"

enum Basicness { Basic, NotBasic };
#define TRUE 1
#define FALSE 0

extern Symbol No_class;

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
typedef CgenNode *CgenNodeP;

/**
 * @brief 地址描述符结构
 */
struct AddressDescriptor {
    const char* base_reg;
    int offset;
    bool is_valid;

    AddressDescriptor() : base_reg(nullptr), offset(0), is_valid(false) {}
    AddressDescriptor(const char* reg, int off) : base_reg(reg), offset(off), is_valid(true) {}
};

/**
 * @brief 翻译上下文管理器 
 * 采用多级查找逻辑，确保在处理二叉树递归时符号解析的准确性
 */
class TranslationContext {
private:
    CgenNode* _current_class;
    std::vector<Symbol> _local_scope_stack; // 追踪 let 变量
    std::map<Symbol, int> _param_map;       // 追踪方法参数
    std::vector<int> _scope_boundaries;     // 作用域边界标记

public:
    TranslationContext(CgenNode* node) : _current_class(node) {}

    // 作用域生命周期管理
    void enter_block() { _scope_boundaries.push_back(_local_scope_stack.size()); }
    void exit_block() {
        if (!_scope_boundaries.empty()) {
            int prev_size = _scope_boundaries.back();
            while ((int)_local_scope_stack.size() > prev_size) {
                _local_scope_stack.pop_back();
            }
            _scope_boundaries.pop_back();
        }
    }

    // 符号注册
    void register_param(Symbol s, int idx) { _param_map[s] = idx; }
    void register_local(Symbol s) { _local_scope_stack.push_back(s); }

    /**
     * @brief 核心寻址逻辑
     
     */
    AddressDescriptor resolve_symbol_address(Symbol sym);

    CgenNode* get_class_context() { return _current_class; }
};

class CgenClassTable : public SymbolTable<Symbol, CgenNode> {
private:
    List<CgenNode> *nds;
    ostream& str;
    int stringclasstag;
    int intclasstag;
    int boolclasstag;
    std::vector<CgenNode*> m_class_nodes;
    std::map<Symbol, int> m_class_tags;

    void code_global_data();
    void code_global_text();
    void code_bools(int);
    void code_select_gc();
    void code_constants();
    void code_class_nameTab();
    void code_class_objTab();
    void code_dispatchTabs();
    void code_protObjs();
    void code_class_inits();
    void code_class_methods();

public:
    CgenClassTable(Classes, ostream& str);
    void code();
    CgenNodeP root();
    
    std::vector<CgenNode*> GetClassNodes() { return m_class_nodes; }
    int GetClassTag(Symbol class_name) { return m_class_tags[class_name]; }
};

class CgenNode : public class__class {
private: 
    CgenNodeP parentnd;                        
    List<CgenNode> *children;                  
    Basicness basic_status;                    
    int m_class_tag;
    std::map<Symbol, int> m_attrib_idx_tab;
    std::map<Symbol, std::pair<Symbol, int>> m_method_idx_tab;

public:
    CgenNode(Class_ c, Basicness bstatus, CgenClassTableP class_table);

    void add_child(CgenNodeP child);
    List<CgenNode> *get_children() { return children; }
    void set_parentnd(CgenNodeP p) { parentnd = p; }
    CgenNodeP get_parentnd() { return parentnd; }
    
    int GetClassTag() { return m_class_tag; }
    void SetClassTag(int tag) { m_class_tag = tag; }

    // 提供给 TranslationContext 使用的元数据访问接口
    int resolve_attribute_offset(Symbol sym);
    int resolve_method_offset(Symbol sym);
    
    void code_init(ostream& s);
    void code_methods(ostream& s);
};

#endif
