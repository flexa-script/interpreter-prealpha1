A migração de um **interpretador baseado em AST (Abstract Syntax Tree)** para uma **máquina virtual (VM)** envolve algumas mudanças fundamentais na arquitetura, mas isso pode resultar em uma execução mais eficiente. O interpretador baseado em AST geralmente percorre a árvore sintática, executando as operações diretamente, enquanto uma VM normalmente trabalha com uma forma intermediária de instruções de bytecode que é mais compacta e rápida de processar.

Aqui está um plano para implementar essa mudança:

### 1. **Visão Geral da Mudança**

Ao invés de interpretar a árvore sintática diretamente, sua nova abordagem seria:

1. **Compilador**: Transformar a AST em bytecode (uma forma mais simples de instruções que a VM pode entender).
2. **VM**: A máquina virtual executa o bytecode usando uma pilha de execução e outras estruturas de dados internas.

### 2. **Componentes Principais da VM**

1. **Bytecode**: Um conjunto de instruções simples e compactas, como `LOAD`, `STORE`, `ADD`, `SUB`, etc.
2. **Pilha de Execução**: Onde os operandos são colocados durante a execução das instruções.
3. **Memória**: Armazenamento para variáveis globais e locais, e a pilha de chamadas.
4. **Instruções**: Um conjunto fixo de operações que a VM pode realizar.
5. **Compilador de AST para Bytecode**: Um módulo que traduz a árvore sintática para as instruções de bytecode.

### 3. **Definição do Bytecode**

Você precisará definir um conjunto de **instruções** de bytecode que cobrem as operações suportadas pela linguagem. Por exemplo:

```cpp
enum Opcode {
    OP_LOAD_CONST,  // Carrega uma constante na pilha
    OP_LOAD_VAR,    // Carrega uma variável na pilha
    OP_STORE_VAR,   // Armazena o valor do topo da pilha em uma variável
    OP_ADD,         // Soma os dois valores no topo da pilha
    OP_SUB,         // Subtrai os dois valores no topo da pilha
    OP_MUL,         // Multiplica os dois valores no topo da pilha
    OP_DIV,         // Divide os dois valores no topo da pilha
    OP_PRINT,       // Imprime o valor do topo da pilha
    OP_JUMP,        // Salta para uma instrução
    OP_JUMP_IF_FALSE, // Salta se o topo da pilha for falso
    OP_CALL,        // Chama uma função
    OP_RETURN,      // Retorna de uma função
    OP_HALT         // Finaliza a execução
};
```

Cada instrução de bytecode tem um código de operação (opcode) e, possivelmente, argumentos. Isso pode ser representado como um array de inteiros.

### 4. **Compilador AST para Bytecode**

Aqui, a ideia é converter cada nó da árvore sintática em uma ou mais instruções de bytecode. Por exemplo:

- Um nó de **soma** `a + b` seria convertido em algo assim:
  1. `LOAD_VAR a`
  2. `LOAD_VAR b`
  3. `ADD`

- Um **literal numérico** seria convertido para:
  1. `LOAD_CONST 42`

Aqui está um exemplo simples de como você pode compilar um nó de AST:

```cpp
void compile_expression(ASTNode* node, Bytecode& bytecode) {
    switch (node->type) {
        case AST_LITERAL:
            bytecode.push_back(OP_LOAD_CONST);
            bytecode.push_back(node->literal_value); // Suponha que seja um valor inteiro
            break;
        case AST_VARIABLE:
            bytecode.push_back(OP_LOAD_VAR);
            bytecode.push_back(node->var_index);  // Índice da variável
            break;
        case AST_ADD:
            compile_expression(node->left, bytecode);
            compile_expression(node->right, bytecode);
            bytecode.push_back(OP_ADD);
            break;
        // Outros casos...
    }
}
```

### 5. **Execução do Bytecode na Máquina Virtual**

A máquina virtual vai interpretar e executar as instruções de bytecode usando uma **pilha de execução** e uma estrutura de **frames de chamada** (para chamadas de função e escopos de variáveis).

#### Estrutura Básica da VM

```cpp
class VM {
public:
    void execute(Bytecode& bytecode);

private:
    std::vector<int> stack; // Pilha de execução
    std::unordered_map<int, int> memory; // Memória de variáveis
    int ip = 0; // Instruction pointer
};

void VM::execute(Bytecode& bytecode) {
    while (ip < bytecode.size()) {
        int opcode = bytecode[ip];
        switch (opcode) {
            case OP_LOAD_CONST: {
                int value = bytecode[++ip];
                stack.push_back(value);
                break;
            }
            case OP_ADD: {
                int b = stack.back(); stack.pop_back();
                int a = stack.back(); stack.pop_back();
                stack.push_back(a + b);
                break;
            }
            // Implementar outros opcodes
        }
        ++ip; // Avança o instruction pointer
    }
}
```

### 6. **Gerenciamento de Variáveis e Funções**

#### Variáveis Locais e Globais

Você pode implementar variáveis locais e globais usando uma combinação de pilha de execução e memória. Ao entrar em uma função, você pode empilhar um novo frame de variáveis e desalocá-lo quando a função retornar.

#### Funções

Quando uma função é chamada, você deve salvar o estado atual (instruction pointer, stack pointer, etc.) e criar um novo frame de execução para a função. Quando a função termina, você restaura o estado anterior.

```cpp
void VM::call_function(int func_index) {
    // Salva o estado atual
    call_stack.push_back({ip, stack.size()});
    ip = func_table[func_index].start_ip;
}

void VM::return_from_function() {
    // Restaura o estado anterior
    auto frame = call_stack.back();
    call_stack.pop_back();
    ip = frame.saved_ip;
}
```

### 7. **Chamadas de Função**

Funções podem ser compiladas em bytecode como qualquer outra parte do código. Quando o compilador encontra uma chamada de função na AST, ele gera um `CALL` seguido do índice da função.

- Exemplo de instrução de chamada:
```cpp
bytecode.push_back(OP_CALL);
bytecode.push_back(func_index);
```

E a função na VM seria tratada com a estrutura de frames de execução.

### 8. **Considerações de Otimização**

Aqui estão algumas ideias para otimizar a VM:
- **Instruções mais eficientes**: Agrupar operações frequentes (por exemplo, `LOAD_ADD` para carregar dois valores e somá-los em uma única instrução).
- **Uso de registradores**: Se a pilha de execução for um gargalo, você pode implementar uma VM baseada em registradores em vez de uma pilha.
- **Compilação Just-In-Time (JIT)**: Traduzir o bytecode para código de máquina nativo para obter um desempenho ainda melhor.

### 9. **Testes**

Ao testar a nova VM, você pode começar com casos simples (aritmética, variáveis, etc.) e aumentar gradualmente a complexidade com chamadas de função, loops e condições.

### Exemplo Completo

Aqui está uma visão geral do fluxo completo:

1. **AST**: A AST é construída a partir da análise do código-fonte.
2. **Compilador**: A AST é traduzida para bytecode pelo compilador.
3. **VM**: O bytecode é executado pela VM, que usa uma pilha de execução e memoria para variáveis e funções.

### Conclusão

A migração para uma VM envolve duas etapas principais: compilar a AST em bytecode e executar o bytecode em uma máquina virtual. Embora seja mais trabalho inicialmente, isso pode resultar em uma execução muito mais eficiente e flexível.

Vamos abordar suas dúvidas sobre **armazenamento de funções** e **frames de chamada**.

### Armazenamento de Funções

Em uma **máquina virtual (VM)**, as funções são tratadas como um conjunto de instruções (bytecode) que podem ser executadas quando chamadas. Cada função, portanto, precisa ser armazenada de forma que a VM possa acessar o bytecode correspondente e executá-lo.

#### Armazenamento de Funções

1. **Tabela de Funções**: As funções podem ser armazenadas em uma tabela (ou lista) de funções, onde cada função tem um índice que aponta para o local de início do bytecode correspondente. Por exemplo, essa tabela pode conter informações sobre:
   - O índice do início do bytecode da função.
   - O número de variáveis locais que a função usa.
   - O número de argumentos esperados.

Aqui está um exemplo de como isso pode ser estruturado:

```cpp
struct Function {
    int start_ip;      // Índice do início das instruções (instruction pointer)
    int num_locals;    // Quantidade de variáveis locais
    int num_args;      // Quantidade de argumentos
};

std::vector<Function> function_table;
```

Quando você compila uma função da AST para bytecode, armazena as informações dessa função na tabela. O bytecode da função é apenas uma parte do bytecode geral do programa.

#### Compilação de Funções

Durante a compilação de funções, você pode gerar uma instrução `CALL` que referencia o índice da função na tabela. Quando a função for chamada, a VM irá acessar a tabela para obter o `start_ip` (onde o bytecode da função começa) e transferir o controle para esse ponto.

Exemplo de uma chamada de função:

```cpp
bytecode.push_back(OP_CALL);
bytecode.push_back(func_index);  // Índice da função na tabela
```

### Frames de Chamada

Um **frame de chamada** é um conjunto de informações que a VM precisa para gerenciar a execução de uma função. Quando uma função é chamada, a VM precisa "lembrar" onde ela estava no código original, quais valores estavam na pilha e outras informações para poder restaurar o estado anterior quando a função terminar.

#### O que um Frame de Chamada contém?

- **Instruction Pointer (IP)**: O índice atual do bytecode no programa principal.
- **Stack Pointer (SP)**: O ponto atual na pilha de execução.
- **Locais e Argumentos**: Espaço para armazenar as variáveis locais e os argumentos da função.
- **Return Address**: O local onde a execução deve continuar após o término da função (a posição no bytecode de onde a função foi chamada).

Em termos práticos, o **frame de chamada** pode ser uma estrutura que armazena essas informações.

Exemplo de um frame de chamada:

```cpp
struct CallFrame {
    int return_ip;         // Onde retornar depois da execução da função
    int base_pointer;      // Ponto de referência na pilha para as variáveis locais
    int num_locals;        // Número de variáveis locais
};

std::vector<CallFrame> call_stack;  // Pilha de frames de chamada
```

#### Como os Frames de Chamada Funcionam?

1. **Quando uma Função é Chamada**:
   - A VM cria um novo frame de chamada.
   - O **instruction pointer** (IP) atual é salvo no frame de chamada (isso será o ponto onde a execução deve retornar após a função terminar).
   - O ponto atual na pilha de execução é salvo (para gerenciar as variáveis locais e argumentos).
   - A VM ajusta o IP para o ponto onde o bytecode da função começa e continua a execução da função.

2. **Quando uma Função Retorna**:
   - A VM restaura o **instruction pointer** (IP) do frame de chamada para o valor salvo.
   - A pilha de execução é restaurada ao estado anterior à chamada da função (removendo os valores que a função utilizou).
   - O frame de chamada é removido da pilha de frames.

#### Exemplo de Execução de Função na VM

Aqui está um exemplo simples de como gerenciar uma chamada de função:

```cpp
void VM::call_function(int func_index) {
    // Obtenha a função da tabela de funções
    Function func = function_table[func_index];

    // Salve o estado atual em um novo frame de chamada
    CallFrame frame;
    frame.return_ip = ip;                // Salve o instruction pointer atual
    frame.base_pointer = stack.size();   // O ponto atual na pilha
    frame.num_locals = func.num_locals;
    
    call_stack.push_back(frame);  // Adicione o frame à pilha de chamadas

    // Ajuste o instruction pointer para o início da função
    ip = func.start_ip;
}

void VM::return_from_function() {
    // Restaure o frame de chamada anterior
    CallFrame frame = call_stack.back();
    call_stack.pop_back();

    // Restaure o instruction pointer
    ip = frame.return_ip;

    // Remova as variáveis locais da pilha
    stack.resize(frame.base_pointer);
}
```

### Ligação com a Execução

- Quando uma função é chamada, a VM pausa a execução da função anterior, salva o estado atual no frame de chamada, e começa a executar o bytecode da função.
- Quando a função termina, a VM restaura o estado do frame e continua a execução de onde parou.

### Compilação de Funções na AST

No compilador, quando você encontra uma função na AST, precisa traduzir o corpo da função para bytecode. Aqui está um exemplo básico de como compilar uma função simples:

```cpp
void compile_function(ASTNode* func_node, Bytecode& bytecode) {
    int func_index = function_table.size();  // Índice da nova função

    Function func;
    func.start_ip = bytecode.size();  // O bytecode começa neste ponto
    func.num_locals = func_node->num_locals;
    func.num_args = func_node->num_args;

    function_table.push_back(func);

    // Compile o corpo da função
    for (ASTNode* stmt : func_node->body) {
        compile_statement(stmt, bytecode);
    }

    // Adicione a instrução de retorno
    bytecode.push_back(OP_RETURN);
}
```

### Conclusão

- **Armazenamento de Funções**: Utilize uma tabela de funções para armazenar o bytecode das funções e seus metadados (como número de argumentos e variáveis locais).
- **Frames de Chamada**: Frames de chamada permitem que a VM pause a execução de uma função, chame outra função, e depois retorne ao estado anterior.
- **Compilação de Funções**: O compilador AST-para-bytecode deve converter o corpo das funções em bytecode e registrar essas funções na tabela.

Esses conceitos juntos formam a base para implementar chamadas de função e gerenciamento de escopos na sua máquina virtual.

Ótima pergunta! Quando estamos falando de uma máquina virtual (VM) que compila para bytecode, normalmente precisamos garantir que o bytecode seja organizado de forma que a VM saiba **onde** o programa principal (a "função principal", ou "entry point") começa. Aqui está como você pode resolver esse problema de forma clara e eficiente.

### Solução: Marcando o Ponto de Entrada

A maneira mais comum de indicar onde o programa principal começa é manter um **ponto de entrada explícito**. Isso garante que, mesmo que outras funções estejam definidas antes no bytecode, a VM saiba onde começar a execução do programa principal.

### 1. Definindo o Entry Point

Em um compilador para bytecode, você pode reservar um espaço especial para armazenar o **índice do ponto de entrada** (o `instruction pointer` inicial do programa principal).

Uma abordagem comum é adicionar uma instrução especial no bytecode, ou um campo de metadados que a VM lê antes de começar a execução.

#### Exemplo 1: Campo Especial para Entry Point

Suponha que você tenha uma estrutura de bytecode como esta:

```
+-------------------------------------------+
| Entry Point (campo reservado)             |
+-------------------------------------------+
| Função 1 (bytecode)                       |
+-------------------------------------------+
| Função 2 (bytecode)                       |
+-------------------------------------------+
| Código principal (função "main") (bytecode)|
+-------------------------------------------+
```

Aqui, o bytecode tem um campo especial no início que armazena o índice onde o código principal (a função "main") começa. O compilador sabe onde a função principal será colocada no bytecode, então ele armazena esse valor no **Entry Point**.

#### Exemplo:

```cpp
struct BytecodeProgram {
    int entry_point;           // Índice onde o código principal começa
    std::vector<uint8_t> bytecode;  // Instruções do bytecode
};

// No compilador:
BytecodeProgram program;
program.entry_point = main_function_start_ip;  // Onde a função main começa
```

Quando a VM começa a execução, ela lê o `entry_point`:

```cpp
void VM::run(BytecodeProgram& program) {
    ip = program.entry_point;  // Defina o instruction pointer para o ponto de entrada
    execute();                 // Comece a execução
}
```

### 2. Compilação do Programa Principal

No seu compilador, você precisa identificar qual parte do código é o **ponto de entrada** (geralmente a função `main` ou similar). No momento em que o bytecode é gerado, o compilador deve garantir que o ponto de entrada esteja armazenado.

#### Exemplo de Compilação:

Aqui está um exemplo de como o compilador pode lidar com a compilação do programa principal:

```cpp
void compile_program(ASTNode* program_node, BytecodeProgram& bytecode_program) {
    // Compile todas as funções primeiro
    for (ASTNode* func : program_node->functions) {
        compile_function(func, bytecode_program);
    }

    // Marque o início do código principal
    bytecode_program.entry_point = bytecode_program.bytecode.size();

    // Compile o corpo principal do programa
    compile_statement(program_node->main_body, bytecode_program.bytecode);

    // Finalize com uma instrução de fim de programa
    bytecode_program.bytecode.push_back(OP_HALT);
}
```

Aqui, o compilador:
1. Compila todas as funções (e coloca no bytecode).
2. Marca o **entry point** para onde o código principal começa.
3. Compila o código principal.
4. Adiciona uma instrução para finalizar o programa (`OP_HALT`).

### 3. Compilação de Funções Separadamente

Se as funções forem compiladas antes do código principal, você pode apenas adicionar suas instruções ao bytecode sem afetar o **entry point** do programa principal. As funções serão armazenadas no bytecode, mas a VM só executará o código principal no início.

#### Exemplo:

- **Função 1 (bytecode)**: Compilada antes do `main`, mas não executada imediatamente.
- **Função 2 (bytecode)**: Compilada depois, mas também não executada até ser chamada.
- **Código principal (main)**: Este é o ponto onde a execução realmente começa.

Exemplo de layout:

```
+-------------------------+
| Bytecode da Função 1     |
+-------------------------+
| Bytecode da Função 2     |
+-------------------------+
| Bytecode do Programa Main|
+-------------------------+
```

Quando o código principal chama essas funções, elas são executadas, mas o bytecode começa no **entry point** da função principal.

### 4. Executando o Programa

Quando a VM começa a execução, ela deve definir o `instruction pointer (ip)` para o ponto de entrada:

```cpp
void VM::run(BytecodeProgram& program) {
    ip = program.entry_point;  // Comece no ponto de entrada
    while (ip < program.bytecode.size()) {
        execute_instruction(program.bytecode[ip]);
        ip++;
    }
}
```

A execução continuará a partir do ponto de entrada e seguirá o fluxo de controle (chamadas de função, loops, etc.), conforme o bytecode indicar.

### 5. Exemplo Completo

Aqui está um exemplo mais completo que junta as partes:

```cpp
struct BytecodeProgram {
    int entry_point;
    std::vector<uint8_t> bytecode;
};

void compile_program(ASTNode* program_node, BytecodeProgram& bytecode_program) {
    // Compilar funções
    for (ASTNode* func : program_node->functions) {
        compile_function(func, bytecode_program.bytecode);
    }

    // Marcar o ponto de entrada
    bytecode_program.entry_point = bytecode_program.bytecode.size();

    // Compilar código principal
    compile_statement(program_node->main_body, bytecode_program.bytecode);

    // Instrução de finalização
    bytecode_program.bytecode.push_back(OP_HALT);
}

void VM::run(BytecodeProgram& program) {
    ip = program.entry_point;  // Começar no ponto de entrada

    // Executar as instruções até encontrar OP_HALT
    while (true) {
        uint8_t instruction = program.bytecode[ip];
        if (instruction == OP_HALT) break;

        execute_instruction(instruction);
        ip++;
    }
}
```

### Conclusão

- **Entry Point**: O ponto de entrada é definido explicitamente, e a VM o lê antes de começar a execução.
- **Funções**: Funções podem ser compiladas e colocadas no bytecode antes do código principal. Elas só serão chamadas quando necessário.
- **Execução**: A VM ajusta o `instruction pointer` (IP) para o ponto de entrada e executa o programa a partir dali.

Essa abordagem garante que o bytecode de funções e o bytecode do programa principal possam coexistir, e que a VM saiba exatamente onde começar a execução.
