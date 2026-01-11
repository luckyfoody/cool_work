#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include "cgen.h"
#include "cgen_gc.h"

extern void emit_string_constant(ostream& str, char* s);
extern int cgen_debug;

static int global_label_cursor = 0;
CgenClassTable* active_codegen_table = nullptr;


#define INST_PUSH(reg, s) { emit_store(reg, 0, SP, s); emit_addiu(SP, SP, -4, s); }
#define INST_POP(reg, s)  { emit_addiu(SP, SP, 4, s); emit_load(reg, 0, SP, s); }
#define FETCH_SYM_ADDR(sym, mgr) (mgr.resolve_symbol_address(sym))


AddressDescriptor TranslationContext::resolve_symbol_address(Symbol sym) {
    // 1. 优先级：由内向外查找 Let 绑定的局部变量 (基于 SP 寻址)
    for (int i = _local_scope_stack.size() - 1; i >= 0; --i) {
        if (_local_scope_stack[i] == sym) {
            // 返回相对于 SP 的偏移，偏移量取决于当前栈深度
            return AddressDescriptor(SP, (_local_scope_stack.size() - 1 - i) + 1);
        }
    }

    // 2. 优先级：查找方法的形式参数 (基于 FP 寻址)
    if (_param_map.find(sym) != _param_map.end()) {
        int param_idx = _param_map[sym];
        int total_params = _param_map.size();
        // 根据 Cool 栈帧约定：FP+12 是最后一个参数，FP+12+4*(n-1) 是第一个
        return AddressDescriptor(FP, (total_params - 1 - param_idx) + 3);
    }

    // 3. 优先级：回退至类属性 (基于 SELF/S0 寻址)
    // 标记为无效以通知调用者去属性表查找
    return AddressDescriptor(nullptr, -1);
}




void method_class::produce_code(ostream& s, CgenNode* host_class) {
    // 实例化一个具备“地址解析能力”的生成管理器
    TranslationContext context(host_class);
    
    // 建立参数映射。
    int p_idx = 0;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        context.register_param(formals->nth(i)->get_name(), p_idx++);
    }

    // --- 方法序言 (Prologue) ---
    // 递归安全：必须在栈中保存现场
    emit_addiu(SP, SP, -12, s);
    emit_store(FP, 3, SP, s);
    emit_store(SELF, 2, SP, s); // 保护 s0，这是处理二叉树递归遍历的命门
    emit_store(RA, 1, SP, s);
    
    // 设置新的帧指针和当前对象指针
    emit_addiu(FP, SP, 4, s);
    emit_move(SELF, ACC, s);

    // --- 方法体翻译 ---
    expr->produce_code(s, context);

    // --- 方法结语 (Epilogue) ---
    // 恢复现场并清理栈空间
    emit_load(FP, 3, SP, s);
    emit_load(SELF, 2, SP, s); // 恢复父级 SELF，确保递归回溯后变量访问正确
    emit_load(RA, 1, SP, s);
    
    // 使用在 cool-tree.h 中重构的内联函数计算清理量
    int frame_size = calculate_activation_record_size();
    emit_addiu(SP, SP, frame_size, s);
    
    emit_return(s);
}

void object_class::produce_code(ostream& s, TranslationContext& context) {
    // 处理 self 特殊情况
    if (name == self) {
        emit_move(ACC, SELF, s);
        return;
    }

    // 调用解耦后的地址解算器
    AddressDescriptor addr = context.resolve_symbol_address(name);

    if (addr.is_valid) {
        // 加载局部变量或参数
        emit_load(ACC, addr.offset, (char*)addr.base_reg, s);
    } else {
        // 处理类属性访问逻辑
        int attr_offset = context.get_class_context()->resolve_attribute_offset(name);
        // 属性相对于 SELF 的偏移通常从 3 开始（0:Tag, 1:Size, 2:DispTab）
        emit_load(ACC, attr_offset + 3, SELF, s);
    }
}

//******************************************************************
// 赋值表达式逻辑实现
//******************************************************************
void assign_class::produce_code(ostream& s, TranslationContext& context) {
    // 首先计算右侧表达式的值，结果存入 $a0
    expr->produce_code(s, context);

    // 解析左侧标识符的存储位置
    AddressDescriptor addr = context.resolve_symbol_address(name);

    if (addr.is_valid) {
        // 存储至局部变量或参数空间
        emit_store(ACC, addr.offset, (char*)addr.base_reg, s);
    } else {
        // 存储至类属性空间
        int attr_offset = context.get_class_context()->resolve_attribute_offset(name);
        emit_store(ACC, attr_offset + 3, SELF, s);
    }

    // 处理垃圾回收观察者（若启用）
    if (Z_GC_CHECK) {
        emit_addiu(A1, SELF, (attr_offset + 3) * 4, s);
        emit_jal("_GenGC_Assign", s);
    }
}

//******************************************************************
// 动态分派逻辑实现 (用于实现二叉树遍历递归)
//******************************************************************


void dispatch_class::produce_code(ostream& s, TranslationContext& context) {
    // 1. 将实参从左至右依次求值并压栈
    for (int i = 0; i < actual->len(); ++i) {
        actual->nth(i)->produce_code(s, context);
        // 压栈操作改变了当前的栈顶指针位置
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
    }

    // 2. 求值调用对象 (Receiver)
    expr->produce_code(s, context);

    // 3. 运行时异常检查：确保调用对象非空 (Void Check)
    int ok_label = global_label_cursor++;
    emit_bne(ACC, ZERO, ok_label, s);

    // 若对象为 Void，载入文件名并跳转至错误处理程序
    emit_load_address(T1, "str_const0", s); 
    emit_load_imm(T2, line_number, s);
    emit_jal("_dispatch_abort", s);

    // 4. 定位虚函数表并完成跳转
    emit_label_def(ok_label, s);
    emit_load(T1, 2, ACC, s); // 载入 Dispatch Table 地址

    // 查找方法在虚表中的偏移量
    Symbol rec_type = expr->get_type();
    if (rec_type == SELF_TYPE) {
        rec_type = context.get_class_context()->get_name();
    }
    
    int method_offset = context.get_class_context()->resolve_method_offset(name);
    emit_load(T1, method_offset, T1, s);
    emit_jalr(T1, s);
}

//******************************************************************
// 条件分支逻辑实现
//******************************************************************
void cond_class::code(ostream& s, TranslationContext& context) {
    int else_branch = global_label_cursor++;
    int end_branch = global_label_cursor++;

    // 1. 计算谓词逻辑
    pred->produce_code(s, context);
    
    // 载入布尔对象的原始值 (位于偏移 3 处)
    emit_load(T1, 3, ACC, s);
    
    // 若为 false (0)，跳转至 else 分支
    emit_beq(T1, ZERO, else_branch, s);

    // 2. Then 分支逻辑
    then_exp->produce_code(s, context);
    emit_branch(end_branch, s);

    // 3. Else 分支逻辑
    emit_label_def(else_branch, s);
    else_exp->produce_code(s, context);

    // 4. 结束标记
    emit_label_def(end_branch, s);
}

//******************************************************************
// 静态分派逻辑实现
//******************************************************************
void static_dispatch_class::produce_code(ostream& s, TranslationContext& context) {
    // 参数压栈
    for (int i = 0; i < actual->len(); ++i) {
        actual->nth(i)->produce_code(s, context);
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
    }

    // 求值调用者
    expr->produce_code(s, context);

    // 空对象检查
    int ok_label = global_label_cursor++;
    emit_bne(ACC, ZERO, ok_label, s);
    emit_load_address(T1, "str_const0", s);
    emit_load_imm(T2, line_number, s);
    emit_jal("_dispatch_abort", s);

    emit_label_def(ok_label, s);
    
    // 静态分派直接查指定类的虚表
    std::string disp_tab_name = std::string(type_name->get_string()) + "_dispatch_tab";
    emit_load_address(T1, (char*)disp_tab_name.c_str(), s);
    
    int method_offset = context.get_class_context()->resolve_method_offset(name);
    emit_load(T1, method_offset, T1, s);
    emit_jalr(T1, s);
}

//******************************************************************
// Let 表达式逻辑实现
//******************************************************************
void let_class::produce_code(ostream& s, TranslationContext& context) {
    // 1. 初始化局部变量
    if (init->get_type() != nullptr) {
        init->produce_code(s, context);
    } else {
        // 默认初始化：根据类型载入原型常量
        if (type_decl == Int) {
            emit_load_int(ACC, inttable.lookup_string("0"), s);
        } else if (type_decl == Str) {
            emit_load_string(ACC, stringtable.lookup_string(""), s);
        } else if (type_decl == Bool) {
            emit_load_bool(ACC, BoolConst(false), s);
        } else {
            emit_move(ACC, ZERO, s);
        }
    }

    // 2. 将初始值压栈并注册到当前作用域
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);
    
    context.enter_block();
    context.register_local(identifier);

    // 3. 在包含新变量的环境下计算主体表达式
    body->produce_code(s, context);

    // 4. 恢复栈指针并退出作用域
    context.exit_block();
    emit_addiu(SP, SP, 4, s);
}

//******************************************************************
// 循环逻辑实现
//******************************************************************
void loop_class::produce_code(ostream& s, TranslationContext& context) {
    int start_label = global_label_cursor++;
    int exit_label = global_label_cursor++;

    emit_label_def(start_label, s);

    // 计算循环判定条件
    pred->produce_code(s, context);
    emit_load(T1, 3, ACC, s);
    
    // 若条件为 false 则跳出
    emit_beq(T1, ZERO, exit_label, s);

    // 执行循环体
    body->produce_code(s, context);
    
    // 回到循环起始点
    emit_branch(start_label, s);

    // 结束标记，循环始终返回 void (0)
    emit_label_def(exit_label, s);
    emit_move(ACC, ZERO, s);
}

//******************************************************************
// 算术运算逻辑实现 (加、减、乘、除)
//******************************************************************


void plus_class::produce_code(ostream& s, TranslationContext& context) {
    // 1. 求值左操作数并压栈
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    // 2. 求值右操作数
    e2->produce_code(s, context);

    // 3. 复制左操作数对象（Cool 要求运算返回新对象）
    emit_jal("Object.copy", s);
    
    // 4. 从栈中弹出左操作数的值，执行加法
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);    // T1: 左操作数对象地址
    emit_load(T2, 3, T1, s);    // T2: 左操作数原始整数
    emit_load(T3, 3, ACC, s);   // T3: 右操作数原始整数
    
    emit_add(T2, T2, T3, s);
    emit_store(T2, 3, ACC, s);  // 将结果存回新对象的原始值字段
}

void sub_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->produce_code(s, context);
    emit_jal("Object.copy", s);
    
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    
    emit_sub(T2, T2, T3, s);
    emit_store(T2, 3, ACC, s);
}

void mul_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->produce_code(s, context);
    emit_jal("Object.copy", s);
    
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    
    emit_mul(T2, T2, T3, s);
    emit_store(T2, 3, ACC, s);
}

void divide_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->produce_code(s, context);
    emit_jal("Object.copy", s);
    
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    
    emit_div(T2, T2, T3, s);
    emit_store(T2, 3, ACC, s);
}

//******************************************************************
// 语句块逻辑实现
//******************************************************************
void block_class::produce_code(ostream& s, TranslationContext& context) {
    // 依次计算每个表达式，最后一个表达式的值保留在 $a0 中作为结果
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        body->nth(i)->produce_code(s, context);
    }
}

//******************************************************************
// 关系运算逻辑实现 (LT, EQ, LEQ)
//******************************************************************
void lt_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);
    
    e2->produce_code(s, context);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);    // T1: e1_obj
    emit_load(T1, 3, T1, s);    // T1: e1_val
    emit_load(T2, 3, ACC, s);   // T2: e2_val
    
    int true_label = global_label_cursor++;
    int end_label = global_label_cursor++;
    
    emit_load_bool(ACC, BoolConst(true), s);
    emit_blt(T1, T2, true_label, s);
    emit_load_bool(ACC, BoolConst(false), s);
    
    emit_label_def(true_label, s);
    emit_label_def(end_label, s);
}

void eq_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);
    
    e2->produce_code(s, context);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);    // T1: e1_obj
    emit_move(T2, ACC, s);      // T2: e2_obj
    
    int end_label = global_label_cursor++;
    // 如果两个指针相等，直接返回 true
    emit_load_bool(ACC, BoolConst(true), s);
    emit_beq(T1, T2, end_label, s);
    
    // 否则调用运行时相等检查函数（处理基本类型的值比较）
    emit_load_bool(A1, BoolConst(false), s);
    emit_jal("equality_test", s);
    
    emit_label_def(end_label, s);
}

//******************************************************************
// 常量加载与一元运算
//******************************************************************
void int_const_class::produce_code(ostream& s, TranslationContext& context) {
    emit_load_int(ACC, inttable.lookup_string(token->get_string()), s);
}

void string_const_class::produce_code(ostream& s, TranslationContext& context) {
    emit_load_string(ACC, stringtable.lookup_string(token->get_string()), s);
}

void bool_const_class::produce_code(ostream& s, TranslationContext& context) {
    emit_load_bool(ACC, BoolConst(val), s);
}

void neg_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_jal("Object.copy", s);
    emit_load(T1, 3, ACC, s);
    emit_neg(T1, T1, s);
    emit_store(T1, 3, ACC, s);
}

void comp_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    emit_load(T1, 3, ACC, s);   // 载入 Bool 的原始值
    int true_label = global_label_cursor++;
    
    emit_load_bool(ACC, BoolConst(true), s);
    emit_beq(T1, ZERO, true_label, s);
    emit_load_bool(ACC, BoolConst(false), s);
    
    emit_label_def(true_label, s);
}

//******************************************************************
// 对象实例化与空值检查
//******************************************************************
void new__class::produce_code(ostream& s, TranslationContext& context) {
    if (type_name == SELF_TYPE) {
        // 动态实例化当前类
        emit_load(T1, 0, SELF, s);          // 载入 class tag
        emit_sll(T1, T1, 3, s);             // tag * 8 (每个 entry 占 8 字节)
        emit_load_address(T2, "class_objTab", s);
        emit_addu(T1, T1, T2, s);           // T1 现在指向 protObj
        
        INST_PUSH(T1, s);                   // 保存地址
        emit_load(ACC, 0, T1, s);           // 载入 protObj
        emit_jal("Object.copy", s);
        
        INST_POP(T1, s);
        emit_load(T1, 1, T1, s);            // 载入相应的 init 函数地址
        emit_jalr(T1, s);
    } else {
        // 静态实例化指定类
        std::string prot_obj = std::string(type_name->get_string()) + PROTOBJ_SUFFIX;
        std::string init_func = std::string(type_name->get_string()) + CLASSINIT_SUFFIX;
        emit_load_address(ACC, (char*)prot_obj.c_str(), s);
        emit_jal("Object.copy", s);
        emit_jal((char*)init_func.c_str(), s);
    }
}

void isvoid_class::produce_code(ostream& s, TranslationContext& context) {
    e1->produce_code(s, context);
    int true_label = global_label_cursor++;
    
    emit_move(T1, ACC, s);
    emit_load_bool(ACC, BoolConst(true), s);
    emit_beq(T1, ZERO, true_label, s);
    emit_load_bool(ACC, BoolConst(false), s);
    
    emit_label_def(true_label, s);
}

void no_expr_class::produce_code(ostream& s, TranslationContext& context) {
    emit_move(ACC, ZERO, s); // 返回 Void
}

//******************************************************************
// CgenClassTable 核心代码生成驱动
//******************************************************************


void CgenClassTable::code() {
    if (cgen_debug) cout << "Coding global data" << endl;
    code_global_data();

    if (cgen_debug) cout << "Choosing gc" << endl;
    code_select_gc();

    if (cgen_debug) cout << "Coding constants" << endl;
    code_constants();

    // 生成类辅助表
    code_class_nameTab();
    code_class_objTab();
    code_dispatchTabs();
    code_protObjs();

    if (cgen_debug) cout << "Coding global text" << endl;
    code_global_text();

    // 生成初始化函数和方法体
    code_class_inits();
    code_class_methods();
}


