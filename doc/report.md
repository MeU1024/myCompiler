### 第一章 序言

### 第二章 词法分析

### 第三章 语法分析

### 第四章 语义分析及中间代码生成

#### 4.1 概述

##### 4.1.1 LLVM IR

LLVM(Low Level Virtual Machine)是以C++编写的编译器基础设施，包含一系列模块化的编译器组件和工具教练用俩开发编译器前端和后端。

而LLVM IR是LLVM的核心所在，通过将不同高级语言的前端变换成LLVM IR进行优化、链接后再传给不同目标的后端转换成为二进制代码，前端、优化、后端三个阶段互相解耦，这种模块化的设计使得LLVM优化不依赖于任何源码和目标机器。

![image-20210619141517616](C:\Users\15426\AppData\Roaming\Typora\typora-user-images\image-20210619141517616.png)

##### 3.1.1 IR布局

每个IR文件称为一个Module，它是其他所有IR对象的顶级容器，包含了目标信息、全局符号和所依赖的其他模块和符号表等对象的列表，其中全局符号又包括了全局变量、函数声明和函数定义。

函数由参数和多个基本块组成，其中第一个基本块称为entry基本块，这是函数开始执行的起点，另外LLVM的函数拥有独立的符号表，可以对标识符进行查询和搜索。

每一个基本块包含了标签和各种指令的集合，标签作为指令的索引用于实现指令间的跳转，指令包含Phi指令、一般指令以及终止指令等。

![image-20210619143036459](C:\Users\15426\AppData\Roaming\Typora\typora-user-images\image-20210619143036459.png)

##### 4.1.2 IR 核心类

- llvm::IRBuilder 

  提供创建LLVM指令并将其插入基础块的API

- llvm::Value

  llvm::Argument, llvm::BasicBlock,llvm::Constant,llvm::Instruction这些很重要的类都是它的子类。

  llvm::Value有一个llvm::Type*成员和一个use list。后者可以跟踪有哪些其他Value使用了自己。

- llvm::Type

  表示类型类，LLVM支持17种数据类型，可以通过Type ID判断类型



#### 4.2 IR生成

##### 4.2.1 运行环境设计

LLVM IR的生成依赖上下文环境，构造CodegenContext类保证IR的生成。其中包括的环境配置如下：

- private成员 _module和builder

  ```c++
  //_module模块包含了代码中的所有函数和变量
  std::unique_ptr<llvm::Module> _module;
  //Builder对象帮助生成LLVM IR 并且记录程序的当前点，以插入LLVM指令，以及Builder对象有创建新指令的函数
  llvm::IRBuilder<> builder;  
  ```

- private成员 使用map存储

- public 成员

  ```c++
  //是否为main函数
  bool is_subroutine;
  //记录trace
  std::list<std::string> traces;
  //系统函数
  llvm::Function *printfFunc, *sprintfFunc, *scanfFunc, *absFunc, *fabsFunc, *sqrtFunc, *strcpyFunc, *strcatFunc, *getcharFunc, *strlenFunc, *atoiFunc;
  ```

- 函数

  该类下的所有函数如下图所示。此处仅对几个关键函数进行解释说明，其余函数于图中进行说明。

  <img src="C:\Users\15426\AppData\Roaming\Typora\typora-user-images\image-20210619173620980.png" alt="image-20210619173620980" style="zoom:33%;" />

##### 4.2.2 类型确定

Node中对应的类型，我们通过findType函数来获取。其中，IRBuilder提供了创建方式，我们使用相应函数构造变量类型。

<img src="C:\Users\15426\AppData\Roaming\Typora\typora-user-images\image-20210619173152792.png" alt="image-20210619173152792" style="zoom: 33%;" />

此处以简单类型为例：

根据Type选择恰当的方法确定llvm::Type

```c++
llvm::Type *SimpleTypeNode::findType(CodegenContext &context)
    {
        switch (type)
        {
            case Type::Bool: return context.getBuilder().getInt1Ty();
            case Type::Char: return context.getBuilder().getInt8Ty();
            case Type::Int: return context.getBuilder().getInt32Ty();
            case Type::Long: return context.getBuilder().getInt32Ty();
            case Type::Real: return context.getBuilder().getDoubleTy();
            default: throw CodegenExcep("Unknown");
        }
    }
```

- 内置类型Int，Char，Bool分别使用32、8、1位的Int类型
- Real使用Double类型

除此之外，还有以下函数：

```c++
//确定某个别名的type
llvm::Type *AliasTypeNode::findType(CodegenContext &context) 
//确定string的type
llvm::Type *StringTypeNode::findType(CodegenContext &context)
//确定array的type，除了需要元素类型还要计算range范围
llvm::Type *ArrayTypeNode::findType(CodegenContext &context)
```



##### 4.2.3 常量获取

直接通过IRBuilder获取常量的值

此处以RealNode为例：

首先确定llvm::Type, 将具体的值和type作为参数，使用get方法完成整个过程。

```c++
 llvm::Value *RealNode::codegen(CodegenContext &context)
    {
        auto *ty = context.getBuilder().getDoubleTy();
        return llvm::ConstantFP::get(ty, val);
    }
```

此外还有以下函数也用于获取值：

```c++
llvm::Value *BooleanNode::codegen(CodegenContext &context)

llvm::Value *IntegerNode::codegen(CodegenContext &context)
    
llvm::Value *CharNode::codegen(CodegenContext &context)

llvm::Value *StringNode::codegen(CodegenContext &context) 
```



##### 4.2.4 变量创建与存取

##### 4.2.5 数组引用

##### 4.2.6 二元操作

LLVM的IRBuilder集成了丰富的二元操作接口，包括ADD, SUB, MUL, DIV等

通过 `llvm::Value *BinaryExprNode::codegen(CodegenContext &context)` 实现了二元操作的生成。

基本的思路是先判断两个操作数的类型，如果其中一个为double，则查看是否需要类型转换。然后匹配对应的二元操作。

最终都通过CreateBinOp，CreateICmp，CreateFCmp实现。

以实数的加减乘除为例：

```c++
switch(op) 
            {
                case BinaryOp::Plus: binop = llvm::Instruction::FAdd; break;
                case BinaryOp::Minus: binop = llvm::Instruction::FSub; break;
                case BinaryOp::Truediv: binop = llvm::Instruction::FDiv; break;
                case BinaryOp::Mul: binop = llvm::Instruction::FMul; break;
                default: throw CodegenExcep("Invaild operator for REAL type");
            }
            return context.getBuilder().CreateBinOp(binop, lexp, rexp);
```

除此之外还支持浮点数的比较、整数的计算与比较、bool型的与或异或、字符的比较。



##### 4.2.7 赋值语句

赋值语句的大致流程为，获取左值的assign指针，对右值使用codegen方法。然后对左右值的type进行一个判断。如有需要进行一个转换。如整数浮点数：

```c++
 if (lhs_type->isDoubleTy() && rhs_type->isIntegerTy(32))
        {
            rhs = context.getBuilder().CreateSIToFP(rhs, context.getBuilder().getDoubleTy());
        }
 else if (lhs_type->isIntegerTy(32) && rhs_type->isDoubleTy())
        {
            auto *rhsSI = context.getBuilder().CreateFPToSI(rhs, lhs_type);
            std::cerr << "Warning: Assigning REAL type to INTEGER type, this may lose information" << std::endl;
            context.getBuilder().CreateStore(rhsSI, lhs);
            return nullptr;
        }
```

最后使用CreateStore方法完成赋值。

```c++
context.getBuilder().CreateStore(rhs, lhs);
```



##### 4.2.8 main函数

构建main函数的函数类型并创建函数实例，将main函数存入trace，之后构造一个基础块作为指令的插入点，递归调用Routine的代码生成。同时
由于main函数直接存在于模块中。

```c++
 llvm::Value *ProgramNode::codegen(CodegenContext &context)
    {
        //添加main函数
        //function中添加basic block
        //创建BasicBlock可以使用BasicBlock类的静态函数Create
        context.is_subroutine = false;

        auto *funcT = llvm::FunctionType::get(context.getBuilder().getInt32Ty(), false);
        auto *mainFunc = llvm::Function::Create(funcT, llvm::Function::ExternalLinkage, "main", *context.getModule());
        auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "entry", mainFunc);
        context.getBuilder().SetInsertPoint(block);
		
        //添加initial part
        if(init_part->content !="")init_part->codegen(context);

        //添加变量常量list
        routine_head->constList->codegen(context);
        routine_head->typeList->codegen(context);
        routine_head->varList->codegen(context);

        //添加subroutine
        context.is_subroutine = true;
        routine_head->subroutineList->codegen(context);
        context.is_subroutine = false;

        context.getBuilder().SetInsertPoint(block);
        routine_body->codegen(context);
        context.getBuilder().CreateRet(context.getBuilder().getInt32(0));

        return nullptr;
    }
```



##### 4.2.9 routine实现

Routine包含了常量声明、变量声明、类型声明、子routine声明、函数体声明，该节点只需要先确定参数、返回值，创建函数和基本块，然后依此调用子节点的codegen方法即可。

```c++
//处理参数和返回值
//...

//insert block
auto *funcTy = llvm::FunctionType::get(retTy, types, false);
auto *func = llvm::Function::Create(funcTy, llvm::Function::ExternalLinkage, name->name, *context.getModule());
auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "entry", func);
context.getBuilder().SetInsertPoint(block);

//....

routine_head->constList->codegen(context);
routine_head->typeList->codegen(context);
routine_head->varList->codegen(context);
routine_head->subroutineList->codegen(context);

routine_body->codegen(context);

//...

context.traces.pop_back();  
```



##### 4.2.10 声明

- 常量声明 
- 变量声明

##### 4.2.11 调用

- 函数及过程调用

- 系统函数

##### 4.2.12 分支语句

- if 语句

  if语句可以被抽象为四个基础块：

  - cond：条件判断基础块，计算条件表达式的值并创建跳转分支

  - then：条件为真执行的基础块，结束之后跳转到cont基础块

  - else：条件为假执行的基础块，结束之后跳转到cont基础块

  - cont：if语句结束之后的基础块，作为后面代码的插入点

  ```c++
   llvm::Value *IfStmtNode::codegen(CodegenContext &context)
      {
          auto *cond = expr->codegen(context);
          if (!cond->getType()->isIntegerTy(1))       
              throw CodegenExcep("Incompatible type in if condition: expected boolean");
  
          //创建blcok
          auto *func = context.getBuilder().GetInsertBlock()->getParent();
          auto *then_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "then", func);
          auto *else_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "else");
          auto *cont_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
          context.getBuilder().CreateCondBr(cond, then_block, else_block);
  
          //插入语句
          context.getBuilder().SetInsertPoint(then_block);
          if_stmt->codegen(context);
          context.getBuilder().CreateBr(cont_block);
  
          func->getBasicBlockList().push_back(else_block);
          context.getBuilder().SetInsertPoint(else_block);
          if (else_stmt != nullptr)
              else_stmt->codegen(context);
          context.getBuilder().CreateBr(cont_block);
  
          func->getBasicBlockList().push_back(cont_block);
          context.getBuilder().SetInsertPoint(cont_block);
          return nullptr;
      }
  ```

  首先通过getParent()方法获取函数，然后分别创建then,else,cont的BasicBlock。CreateCondBr方法设置条件跳转语句，再将then设置为插入点，对条件为真时执行的指令进行生成，之后无条件跳转到cont块。else部分同理。

- switch语句

  switch语句通过createSwitch来实现，branch里的语句通过遍历所有的caseNode，然后使用addCase方法添加。无论是哪个分支，最后都跳转至cont块。

  ```c++
   auto *func = context.getBuilder().GetInsertBlock()->getParent();
          auto *cont = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
          auto *switch_inst = context.getBuilder().CreateSwitch(value, cont, static_cast<unsigned int>(branches.size()));
          for (auto &branch : branches)
          {
              llvm::ConstantInt *constant;
              //获得该case的value
              if (is_ptr_of<ConstValueNode>(branch->branch))
                  constant = llvm::cast<llvm::ConstantInt>(branch->branch->codegen(context));
              else // ID node
                  constant = context.getConstInt(cast_node<IdNode>(branch->branch)->name);
              auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "case", func);
              context.getBuilder().SetInsertPoint(block);
              branch->stmt->codegen(context);
              context.getBuilder().CreateBr(cont);
              switch_inst->addCase(constant, block);
          }
          func->getBasicBlockList().push_back(cont);
          context.getBuilder().SetInsertPoint(cont);
  ```

  

##### 4.2.13 循环语句

- while 语句

  while我们通过while，loop，cont这几个基本块实现。首先无条件跳转至while块，在while块插入代码，然后是根据cond判断的条件跳转，如果为真，转至loop块，否则为cont块。loop块末尾插入无条件跳转至while块代码。

  ```c++
   auto *func = context.getBuilder().GetInsertBlock()->getParent();
          auto *while_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "while", func);
          auto *loop_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "loop", func);
          auto *cont_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
          context.getBuilder().CreateBr(while_block);
  
          context.getBuilder().SetInsertPoint(while_block);
          auto *cond = expr->codegen(context);
          if (!cond->getType()->isIntegerTy(1))
          { throw CodegenExcep("Incompatible type in while condition: expected boolean"); }
          context.getBuilder().CreateCondBr(cond, loop_block, cont_block);
  
          context.getBuilder().SetInsertPoint(loop_block);
          stmt->codegen(context);
          context.getBuilder().CreateBr(while_block);
  
          func->getBasicBlockList().push_back(cont_block);
          context.getBuilder().SetInsertPoint(cont_block);
          return nullptr;
  ```

  

- for 语句

  for语句在while的基础上实现，将相应的条件转为比较的BinaryExprNode，在原循环语句最后添加一个自增语句。然后新建节点WhileStmtNode，使用codegen方法生成。

  ```c++
  if (!id->codegen(context)->getType()->isIntegerTy(32))
              throw CodegenExcep("Incompatible type in for iterator: expected int");
  
          auto init = make_new_node<AssignStmtNode>(id, init_val);
          auto upto = direction == ForDirection::To;
          auto cond = make_new_node<BinaryExprNode>(upto ? BinaryOp::Leq : BinaryOp::Geq, id, end_val);
          auto iter_stmt = make_new_node<AssignStmtNode>(id,
                  make_new_node<BinaryExprNode>(upto ? BinaryOp::Plus :BinaryOp::Minus, id, make_new_node<IntegerNode>(1)));
          auto compound = make_new_node<CompoundStmtNode>();
          compound->merge(stmt); 
          compound->append(iter_stmt);
          auto while_stmt = make_new_node<WhileStmtNode>(cond, compound);
  
          init->codegen(context);
          while_stmt->codegen(context);
          return nullptr;
  ```

  

- repeat 语句

  pascal中原有的repeat语句和while较为相像，因此我们对repeat语法稍作改动。大致如下：

  ```
  repeatdo
  	循环体
  	times 循环次数 counter 计数器变量;
  ```

  实现也是基于while语句的实现，和for语句不同的是，还要新增赋值语句节点，即给计数器变量赋值为1。后面的逻辑是类似的。

  ```c++
  llvm::Value *RepeatStmtNode::codegen(CodegenContext &context)
      {
          auto init_value = make_new_node<IntegerNode>(1);
          auto init = make_new_node<AssignStmtNode>(id, init_value);
          auto cond = make_new_node<BinaryExprNode>(BinaryOp::Leq, id, expr);
          auto iter_stmt = make_new_node<AssignStmtNode>(id,
                  make_new_node<BinaryExprNode>(BinaryOp::Plus, id, make_new_node<IntegerNode>(1)));
          auto compound = make_new_node<CompoundStmtNode>();
          compound->merge(stmt); 
          compound->append(iter_stmt);
          auto while_stmt = make_new_node<WhileStmtNode>(cond, compound);
  
          init->codegen(context);
          while_stmt->codegen(context);
          return nullptr;
      }
  ```

  

##### 4.2.14 initial part

该部分为我们设计的新增的部分。功能为在程序开始时打印语句。

```c++
llvm::Value *InitNode::codegen(CodegenContext &context){

        context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr(content));
        context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr("\n"));
        return nullptr;

    }
```

将InitNode中的content作为需要打印的内容，使用CreateCall方法输出。

### 第五章 测试案例

#### 5.1 quick sort

##### 5.1.1 测试代码

```pascal
program main;
var
        one,n,s:integer;
        a:array[0..2000000]of integer;

procedure qsort(l,r:longint);
var
  i,j,m,t:longint;
begin
  one :=1;
  i:=l;
  j:=r;
  m:=a[(l+r) div 2];
  while i<=j do begin
    while a[i]<m do i:=i+one;
    while a[j]>m do j:=j-one;
    if i<=j then
    begin
      t:=a[i];
      a[i]:=a[j];
      a[j]:=t;
      i:=i+one;
      j:=j-one;
    end;
  end;
  if l<j then qsort(l,j);
  if i<r then qsort(i,r);
end;

begin
    read(n);
    for s:=1 to n do begin
    	read(a[s]);
    end;
    qsort(1,n);
    for s:=1 to n do begin
        writeln(a[s]);
    end;
end.

```

##### 5.1.2测试内容

- 数据类型：整数、整数数组
- 声明：变量声明、过程声明
- 二元操作：整数compare、整数加减
- 语句：assign语句、while语句、if语句、for语句
- 系统函数：read、writeln

##### 5.1.3测试结果



#### 5.2 矩阵乘法

```pascal
program main;
var
    a,b,c,d,i,j,k,tmp,m,n:longint;
    x,y,f:array[1..10000] of longint;

begin

read(a);
read(b);
i := 1;

repeatdo
    j := 1;
    repeatdo
        tmp := (i - 1) * b + j;
        read(x[tmp]);
    times b counter j;
times a counter i;

read(c);
read(d);

for i:=1 to c do begin
    for j:=1 to d do begin
        tmp := (i - 1) * d + j;
        read(y[tmp]);
    end;
end;

if b<>c then begin
    writeln('Incompatible Dimensions');
    end
else begin

    for i:=1 to a do begin
        for j:=1 to d do begin
            tmp := (i - 1) * d + j;
            f[tmp] := 0;
            for k:=1 to b do begin
                m := (i - 1 ) * b + k;
                n := (k - 1 ) * d + j;
                f[tmp]:=f[tmp]+x[m]*y[n];
            end;
            if tmp mod d = 0 then begin
                writeln(f[tmp]:10);
            end
            else begin        
                write(f[tmp]:10);
            end;
        end;
    end;

end;
end. 


```

- 数据类型：整数、整数数组

- 声明：变量声明

- 二元操作：整数compare、整数加减、mod运算

- 语句：assign语句、if语句、for语句、repeatdo语句

- 系统函数：read、writeln（支持设定输出宽度）

  

#### 5.3 initial part

```pascal
program test;
init
'test init'
initend
begin 
end.
```



#### 5.4 switch语句

#### 5.5 数据类型测试

#### 5.6 运算测试

