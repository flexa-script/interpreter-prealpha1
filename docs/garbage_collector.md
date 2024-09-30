Implementar um garbage collector (GC) em uma linguagem de programação exige uma abordagem cuidadosa, já que o objetivo é liberar automaticamente a memória alocada, evitando vazamentos de memória. Existem várias estratégias para implementar um GC, como contagem de referências, coleta de lixo baseada em marcação ("mark and sweep") ou algoritmos generacionais.

Aqui está uma abordagem passo a passo para adicionar um **garbage collector básico de "mark and sweep"** para sua linguagem.

### 1. **Estrutura Básica**
Você deve rastrear objetos que precisam ser coletados. Para isso, cada objeto gerenciado pelo GC precisa ter um "header" que contém informações sobre seu estado (se foi marcado ou não) e referências para outros objetos que ele possa estar apontando.

### 2. **Passos do GC "Mark and Sweep"**
- **Mark**: Durante a fase de marcação, o GC começa de um conjunto de "raízes" (por exemplo, variáveis globais, a pilha de execução), percorre todas as referências e marca todos os objetos que ainda estão em uso.
- **Sweep**: Na fase de varredura, todos os objetos que não foram marcados são considerados lixo e são liberados da memória.

### 3. **Implementação Simples de GC**

#### Estrutura de Objeto Gerenciado

Cada objeto gerenciado pelo GC precisa ter um "header" com informações para rastrear seu estado. Algo assim pode ser adicionado ao seu sistema de objetos:

```cpp
struct GCObject {
    bool marked;            // Indica se o objeto foi marcado
    GCObject* next;         // Próximo objeto na lista de todos os objetos
    // Dados específicos do objeto
    // (por exemplo, valores primitivos, referências a outros objetos, etc.)
};
```

#### Estrutura do Gerenciador de Memória

Você precisa de uma estrutura para o coletor, que mantém uma lista de todos os objetos alocados e suas raízes.

```cpp
class GarbageCollector {
public:
    void* allocate(size_t size);
    void mark();
    void sweep();
    void collect(); // Combina mark e sweep

private:
    GCObject* objects; // Lista de todos os objetos alocados
    std::vector<GCObject*> roots; // Lista de referências raiz (variáveis globais, pilha, etc.)
};
```

#### Função `allocate`

Essa função aloca memória e adiciona o objeto à lista de objetos gerenciados.

```cpp
void* GarbageCollector::allocate(size_t size) {
    GCObject* object = static_cast<GCObject*>(malloc(size));
    object->marked = false;
    object->next = objects; // Adiciona ao início da lista
    objects = object;
    return object;
}
```

#### Fase de "Mark"

O coletor começa de uma lista de referências raiz (como variáveis globais ou objetos na pilha) e marca recursivamente todos os objetos que estão sendo referenciados.

```cpp
void GarbageCollector::mark() {
    for (GCObject* root : roots) {
        mark_recursive(root);
    }
}

void GarbageCollector::mark_recursive(GCObject* obj) {
    if (obj == nullptr || obj->marked) return; // Já marcado ou nulo
    obj->marked = true;
    
    // Percorra todos os campos referenciados pelo objeto
    for (GCObject* referenced : obj->get_references()) {
        mark_recursive(referenced);
    }
}
```

#### Fase de "Sweep"

Todos os objetos não marcados são liberados da memória.

```cpp
void GarbageCollector::sweep() {
    GCObject** object = &objects;
    while (*object) {
        if (!(*object)->marked) {
            // Não está marcado, então podemos liberar
            GCObject* unreached = *object;
            *object = unreached->next;
            free(unreached);
        } else {
            // Limpa a marca para a próxima coleta
            (*object)->marked = false;
            object = &(*object)->next;
        }
    }
}
```

#### Coleta de Lixo

A função `collect` realiza a coleta de lixo, combinando as fases de marcação e varredura.

```cpp
void GarbageCollector::collect() {
    mark();  // Fase de marcação
    sweep(); // Fase de varredura
}
```

### 4. **Integração no Interpretador**

Você precisará garantir que as raízes (variáveis globais e objetos na pilha) sejam acessíveis para o garbage collector. Isso pode exigir a adaptação do seu interpretador para rastrear corretamente as variáveis vivas e adicionar todas elas à lista de `roots` do coletor de lixo.

#### Exemplo de Variáveis Globais

Se o interpretador mantiver uma lista de variáveis globais, você pode adicioná-las à lista de raízes no coletor:

```cpp
void Interpreter::add_global_variable(GCObject* obj) {
    gc.roots.push_back(obj);
}
```

### 5. **Trigger Automático do GC**

Você pode fazer com que o GC seja disparado automaticamente quando a memória alocada atingir um determinado limite, ou pode chamar manualmente o `collect()` sempre que achar necessário (por exemplo, no final de uma execução ou loop).

```cpp
if (memory_usage > MEMORY_LIMIT) {
    gc.collect();
}
```

### 6. **Controle Avançado (Opcional)**

Se quiser avançar com técnicas mais sofisticadas, como **gerações** ou **coletores incrementais**, você pode evoluir a partir desse modelo básico, mas para o início, este método de "mark and sweep" fornecerá uma boa base para evitar vazamentos de memória na sua linguagem.

### Conclusão

Com essas etapas, você tem um **garbage collector básico "mark and sweep"** integrado ao seu interpretador. A chave está em garantir que todos os objetos referenciados sejam gerenciados pelo GC e que as referências raiz (variáveis globais, etc.) sejam corretamente identificadas para que o coletor possa marcá-los e varrê-los corretamente.
