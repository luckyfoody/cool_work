class Main inherits IO {
    -- 1. 测试未初始化的对象（Void）
    void_obj : Main; 

    main() : Object {
        {
            out_string(">>> Starting Ultimate Stress Test <<<\n");

            --------------------------------------------------------
            -- 阶段 1: 运行时异常触发 (Runtime Abort Tests)
            --------------------------------------------------------
            -- 注意：在实际测试中，取消下面其中一行的注释，
            -- 编译器应该打印错误并退出，而不是直接 Segment Fault。
            
            -- void_obj.main();           -- 测试 _dispatch_abort
            -- (case void_obj of x:Object => 1; esac); -- 测试 _case_abort
            
            --------------------------------------------------------
            -- 阶段 2: 深度嵌套作用域与栈平衡 (Deep Let & Stack Stress)
            --------------------------------------------------------
            out_string("Testing deep stack usage and variable resolution...\n");
            let x : Int <- 1 in
                let x : Int <- x + 1 in
                    let x : Int <- x + 1 in
                        let x : Int <- x + 1 in
                            let x : Int <- x + 1 in
                                if x = 5 then
                                    out_string("  - Deep Let Scope: PASSED\n")
                                else
                                    { out_string("  - Deep Let Scope: FAILED, x = "); out_int(x); out_string("\n"); }
                                fi;

            --------------------------------------------------------
            -- 阶段 3: 算术与临时变量栈冲突 (Arithmetic Stack Conflict)
            --------------------------------------------------------
            -- 这是一个复杂的表达式，会迫使编译器频繁将中间结果压栈
            let a : Int <- 10, b : Int <- 20, c : Int <- 30 in
                if (a * b) + (c * a) - (b / a) = 498 then
                    out_string("  - Complex Arithmetic: PASSED\n")
                else
                    out_string("  - Complex Arithmetic: FAILED\n")
                fi;

            --------------------------------------------------------
            -- 阶段 4: SELF_TYPE 动态分派 (Dynamic SELF_TYPE)
            --------------------------------------------------------
            out_string("Testing dynamic SELF_TYPE dispatch...\n");
            let derived : Derived <- (new Derived).get_self() in
                derived.identify(); -- 应该打印 "I am Derived"

            --------------------------------------------------------
            -- 阶段 5: Case 表达式的深度继承匹配 (Advanced Case)
            --------------------------------------------------------
            out_string("Testing advanced Case matching...\n");
            let alpha : Alpha <- new Gamma in
                case alpha of
                    g : Gamma => out_string("  - Case Match (Gamma): PASSED\n");
                    b : Beta  => out_string("  - Case Match (Beta): FAILED (Too shallow)\n");
                    a : Alpha => out_string("  - Case Match (Alpha): FAILED (Too shallow)\n");
                    o : Object => out_string("  - Case Match (Object): FAILED\n");
                esac;

            out_string(">>> Ultimate Stress Test Completed Successfully <<<\n");
        }
    };
};

------------------------------------------------------------------------
-- 辅助测试类层次
------------------------------------------------------------------------
class Alpha inherits IO {
    get_self() : SELF_TYPE { self };
    identify() : Object { out_string("  - Identity: I am Alpha\n") };
};

class Beta inherits Alpha {
    identify() : Object { out_string("  - Identity: I am Beta\n") };
};

class Gamma inherits Beta {
    -- 继承了 get_self()，但返回的应该是 Gamma 实例
    identify() : Object { out_string("  - Identity: I am Gamma (SELF_TYPE worked)\n") };
};

class Derived inherits Alpha {
    identify() : Object { out_string("  - Identity: I am Derived (SELF_TYPE worked)\n") };
};
