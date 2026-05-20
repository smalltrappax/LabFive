# Документация по коду — лабораторная «Инвертированный индекс»

Проект: поисковый движок на AVL, Red-Black и B-tree с инвертированным текстовым индексом по датасету Stack Overflow.

---

## 1. Структура проекта

```
igpodik_lab5/
├── README.md              
├── DOCUMENTATION.md       
├── REPORT.md              
├── Makefile               
├── main.c                 
├── posting.h, posting.c   
├── preprocess.py
├── app.py                 
├── Dockerfile, docker-compose.yml
│
├── avl/                   
│   ├── avl.h, avl.c
│   └── tests.c
├── rbtree/                
│   ├── rbtree.h, rbtree.c
│   └── tests.c
├── btree/                 
│   ├── btree.h, btree.c
│   └── tests.c
├── index/                 
│   ├── index.h, index.c
│   └── search.h, search.c
│
├── lab3/vector/           
│   ├── generic.h, generic.c
│   └── comparators.h (в lab3/)
│
├── data/                  
│   ├── README.md
│   ├── Questions.csv      
│   ├── processed/docs.jsonl
│   ├── index_avl.txt, index_rb.txt, index_btree.txt
│   └── test/              
│
├── lab4/, graph/          
└── .gitignore
```

### Поток данных

```
Questions.csv
    → preprocess.py → docs.jsonl
    → app index (--type=avl|rb|btree) → index_<type>.txt
    → app search / fuzzysearch → JSON или текст
    → app.py (Streamlit) вызывает ./app search --json
```

### Граф зависимостей модулей (сборка `app`)

```
main.c
  → index/index.c → avl, rbtree, btree, posting
  → index/search.c → index, posting
posting.c → lab3/vector/generic.c
```

---

## 2. Описание файлов

### Корень проекта

#### `Makefile`

**Назначение:** сборка бинарников и автоматические тесты.

| Цель | Результат |
|------|-----------|
| `make` / `make all` | `app` + юнит-тесты деревьев |
| `make app` | только `./app` |
| `make u_tests` | `test_avl`, `test_rb`, `test_btree` и их запуск |
| `make test` | e2e: preprocess → index ×3 → search ×3 |
| `make clean` | удаление `.o`, бинарников, временных индексов |

**Параметры:** `CC=gcc`, `CFLAGS=-Wall -Wextra -std=c11 -O2 -g -I.`  
На Windows для Python в `make test` используется `python`, иначе `python3`.

**Важно:** запускать `make`, а не `./Makefile`.

---

#### `main.c`

**Назначение:** точка входа CLI.

**Режимы (argv[1]):**

| Режим | Описание |
|-------|----------|
| `index` | Читает JSONL, строит индекс, сохраняет на диск |
| `search` | Загружает индекс, точный поиск (AND по токенам) |
| `fuzzysearch` | Нечёткий поиск (Левенштейн) |
| `bench` | Замер времени индексации и пакета запросов |

**Аргументы:**

| Флаг | По умолчанию | Смысл |
|------|--------------|--------|
| `--type=avl\|rb\|btree` | `avl` | Тип дерева |
| `--data=PATH` | `data/processed/docs.jsonl` | Вход для `index` / `bench` |
| `--index=PATH` | `data/index_<type>.txt` | Файл индекса |
| `--json` | выкл. | JSON вместо текста |
| `--dist=N` | `2` | Макс. расстояние Левенштейна (`fuzzysearch`) |
| `--limit=N` | `5000` | Документов в `bench` |
| `--queries=N` | `1000` | Число запросов в `bench` |

**Примеры:**

```bash
./app index --type=rb
./app search --type=avl --json "python list sort"
./app fuzzysearch --type=btree --dist=2 "pythn list"
./app bench --type=avl --limit=50000 --queries=1000
```

---

#### `posting.h` / `posting.c`

**Назначение:** posting list — список вхождений терма в документы.

**Структуры:**

- `PostingEntry`: `doc_id`, `title[MAX_TITLE_LEN]` (256).

**API:**

| Функция | Вход | Выход / поведение |
|---------|------|-------------------|
| `createPostingList()` | — | Пустой `Vector*` с `elem_size = sizeof(PostingEntry)` |
| `appendPosting(list, doc_id, title)` | список, id, заголовок | Добавляет запись; **не дублирует** тот же `doc_id` подряд |
| `clonePostingList(src)` | const Vector* | Глубокая копия списка |

**Освобождение:** `vectorFree(list)` из `lab3/vector`.

**Зависимости:** `lab3/vector/generic.h` (путь от корня: `-I.`).

---

#### `preprocess.py`

**Назначение:** один раз подготовить текст для C-индексатора.

**Вход:** CSV Stack Overflow (`Questions.csv`), колонки `Id`, `Title`, `Body`.

**Выход:** JSONL, одна строка на документ:

```json
{"doc_id": "123", "title": "...", "tokens": ["how", "sort", "list"]}
```

**Аргументы CLI:**

| Аргумент | Обязательный | Описание |
|----------|--------------|----------|
| `--input` | да | Путь к CSV |
| `--output` | да | Путь к `docs.jsonl` |
| `--limit N` | нет | Обрезка числа документов (для экспериментов) |

**Токенизация:** lower case, удаление не-букв/цифр (кроме `_`), слова длиной > 2.

---

#### `app.py`

**Назначение:** веб-интерфейс Streamlit.

**Поведение:** запускает `./app` через `subprocess` с режимом `search` или `fuzzysearch`, флагом `--json`, парсит stdout и показывает топ результатов.

**Зависимости:** собранный бинарник `app`, при необходимости готовые `data/index_<type>.txt`.

**Запуск:** `streamlit run app.py` или `docker compose up web`.

---

#### `Dockerfile` / `docker-compose.yml`

**Dockerfile:** Debian slim → `gcc`, `make`, `python3-venv` → `make app` → venv со Streamlit.

**docker-compose:**

- `web` — порт 8501, volume `./data:/app/data`
- `search` (profile `tools`) — образ для одноразовых команд в контейнере

---

#### `REPORT.md`

Шаблон таблиц для отчёта (время индексации, поиска, память) и команды воспроизведения.

---

### `avl/` — AVL-дерево

#### `avl.h`

**Структуры:**

- `AVLNode`: `key`, `height`, `postings`, `left`, `right`
- `AVLTree`: `root`, `size` (число **уникальных** ключей)

**API:**

| Функция | Вход | Выход |
|---------|------|--------|
| `createAVLTree()` | — | Пустое дерево |
| `freeAVLTree(tree)` | дерево | Освобождает узлы, ключи, posting lists |
| `avlInsert(tree, key, doc_id, title)` | терм, документ | Вставка или append в существующий posting list |
| `avlSearch(tree, key)` | терм | `Vector*` postings или `NULL` |
| `avlTraverse(tree, visit, ctx)` | callback in-order | Обход по возрастанию ключей |

#### `avl.c`

Реализация: BST + повороты при |balance| > 1, высота в узле, `strdup` для новых ключей.

#### `avl/tests.c`

Юнит-тест: вставки, дубликат ключа (2 posting), обход (3 уникальных ключа).

---

### `rbtree/` — Red-Black дерево

#### `rbtree.h`

**Структуры:**

- `RBColor`: `RB_RED`, `RB_BLACK`
- `RBNode`: `key`, `color`, `postings`, `left`, `right`, `parent`
- `RBTree`: `root`, `nil` (sentinel), `size`

**API:** аналогично AVL (`rbInsert`, `rbSearch`, `rbTraverse`, `createRBTree`, `freeRBTree`).

#### `rbtree.c`

Вставка по CLRS: `rb_insert_fixup`, левый/правый поворот, все «листья» — на `nil`.

#### `rbtree/tests.c`

Проверка дубликата ключа и размера posting list.

---

### `btree/` — B-tree

#### `btree.h`

**Константы:** `BTREE_T = 3` (минимальная степень), до `2t-1 = 5` ключей в узле.

**Структуры:**

- `BTreeNode`: массивы `keys[]`, `postings[]`, `children[]`, `n`, `is_leaf`
- `BTree`: `root`, `size`

**API:** `btreeInsert`, `btreeSearch`, `btreeTraverse`, `createBTree`, `freeBTree`.

#### `btree.c`

Поиск с `find_key_index`, split дочернего узла при переполнении, вставка только в лист (или append при совпадении ключа). Обход: ключи и дети в порядке B-tree.

#### `btree/tests.c`

20 вставок `k00`…`k19`, поиск, повторная вставка с тем же ключом.

---

### `index/` — слой индекса

#### `index.h` / `index.c`

**Назначение:** единый интерфейс над тремя деревьями + сериализация + чтение JSONL.

**Типы:**

- `TreeType`: `TREE_AVL`, `TREE_RB`, `TREE_BTREE`
- `Index`: `void* tree` + `type`

**API:**

| Функция | Описание |
|---------|----------|
| `createIndex(type)` | Создаёт нужное дерево |
| `insertTerm(idx, term, doc_id, title)` | Делегирует `*Insert` |
| `lookupTerm(idx, term)` | Делегирует `*Search` |
| `indexDocument(idx, doc_id, title, tokens[], n)` | Вставка всех токенов документа |
| `traverseIndex(idx, visit, ctx)` | Обход для save / fuzzy |
| `saveIndex(idx, path)` | Текстовый формат на диск |
| `loadIndex(path, type)` | Восстановление индекса |
| `freeIndex(idx)` | Освобождение |
| `parseType(s)` / `typeName(t)` | Строка ↔ enum |
| `runIndex(type, data_jsonl, idx_path)` | Полный пайплайн индексации |

**Формат файла индекса (`index_<type>.txt`):**

```
LAB5IDX1
avl
<длина_ключа>
<ключ_raw>
<число_posting>
doc_id<TAB>title
...
END
```

**Парсинг JSONL (упрощённый):** из строки извлекаются `doc_id`, `title`, массив `tokens` без внешней JSON-библиотеки.

---

#### `search.h` / `search.c`

**Назначение:** точный и нечёткий поиск по индексу.

**Структуры:**

- `SearchResult`: `doc_id`, `title`, `score`
- `SearchResults`: `results` (Vector топ-10), `total`, `time_ms`
- `FuzzyCandidate`: `term`, `distance`, `postings`

**API:**

| Функция | Описание |
|---------|----------|
| `intersectPostings(lists[], n)` | AND по `doc_id` (пересечение posting lists) |
| `search(idx, query)` | Токенизация запроса → lookup → intersect → топ-10 |
| `fuzzyFindCandidates(idx, term, max_distance)` | Обход дерева + Левенштейн ≤ max |
| `fuzzySearch(idx, query, max_distance)` | Fuzzy по каждому слову запроса, ранжирование |
| `printResultsText` / `printResultsJSON` | Вывод |
| `freeSearchResults` | Освобождение |

**Точный поиск:**

1. Запрос режется на токены (как в preprocess: lower, alnum/`_`, len > 2).
2. Для каждого токена — `lookupTerm`.
3. Если хотя бы один терм отсутствует — пустой результат.
4. `intersectPostings` — документы, есть во **всех** списках.
5. Топ-10: сортировка по `doc_id`, `score` = число токенов в запросе.

**Fuzzy:**

- Расстояние Левенштейна (DP, ограничение длины строк).
- `score = matched_terms * 10 - avg_distance` (как в README).
- Сортировка по `score` убыв., затем `doc_id`.

---

### `lab3/vector/` — универсальный вектор

#### `generic.h` / `generic.c`

**Назначение:** динамический массив произвольного `elem_size` (из лаб. 3).

**Используется для:** posting lists, списки `SearchResult`, `FuzzyCandidate`, `DocAgg` в fuzzy.

**Ключевые функции:** `createVector`, `appendVectorItem`, `getVectorItem`, `vectorFree`, `clone` через копирование элементов в `posting.c`.

**Не входит в Makefile lab5 напрямую как цель, но компилируется как `lab3/vector/generic.o`.

---

### `data/`

| Путь | Назначение |
|------|------------|
| `Questions.csv` | Исходный Kaggle (локально) |
| `processed/docs.jsonl` | После `preprocess.py` |
| `index_*.txt` | Сохранённые индексы |
| `test/Questions.csv` | 3 строки для `make test` |

---

### Не используются в сборке `app`

- `lab4/` — LRU, hash table (другая лабораторная)
- `graph/` — граф, A* (другая лабораторная)

---

## 3. Сборка и проверка

```bash

make              # app + unit tests
make test         # e2e на data/test/
python preprocess.py --input data/Questions.csv --output data/processed/docs.jsonl --limit 50000
./app index --type=avl
./app search --type=avl "python list"
```
