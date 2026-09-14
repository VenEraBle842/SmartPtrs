## Умные указатели и динамические последовательности

Целью работы является реализация собственных механизмов управления динамической памятью в рамках стандарта C++20 без использования готовых умных указателей из стандартной библиотеки (`std::unique_ptr`, `std::shared_ptr`).

Реализованы:
1. Шаблонный класс **`UniquePtr<T>`** с семантикой монопольного владения;
2. Шаблонный класс **`SharedPtr<T>`** с семантикой разделяемого владения на базе счетчика ссылок;
3. Поддержка специализаций для массивов (`UniquePtr<T[]>` и `SharedPtr<T[]>`) с корректным освобождением через `delete[]`;
4. Поддержка безопасного полиморфного приведения типов (upcasting) на этапе компиляции с использованием концептов C++20;
5. Шаблонный контейнер **`SmartArraySequence<T>`**, реализующий абстрактный интерфейс `Sequence<T>` и управляющий внутренним динамическим буфером исключительно через умные указатели;
6. Модульные тесты функциональности и бенчмарки накладных расходов памяти и времени работы.

---

## Архитектурные особенности и детали реализации

### 1. `UniquePtr<T>`
- **Монопольное владение:** конструктор копирования и копирующий оператор присваивания явно удалены (`= delete`).
- **Семантика перемещения:** поддерживается перемещающий конструктор и оператор присваивания с защитой от самоприсваивания (`this != &other`).
- **Специализация для массивов:** используются метафункции `std::remove_extent_t<T>` и `std::is_array_v<T>`. Для скалярных типов вызывается `delete ptr`, для массивов — `delete[] ptr`. Перегружен `operator[]`.
- **Поддержка полиморфизма:** перемещающий конструктор и оператор присваивания от другого типа параметризованы ограничением:
  ```cpp
  requires (!is_array && std::derived_from<U, ElementType>)
  ```
  Это позволяет безопасно передавать `UniquePtr<Derived>` в `UniquePtr<Base>`, запрещая нисходящие преобразования (downcasting) и приведение несовместимых типов на этапе компиляции.
- **Интерфейс:** методы `Release()`, `Reset()`, `Get()`, операторы разыменования `*`, `->`, явное приведение к `bool`.

### 2. `SharedPtr<T>`
- **Разделяемое владение:** управление временем жизни объекта через динамически выделенный счетчик ссылок (`int* ref_cnt`).
- **Корректность копирования и перемещения:**
  - конструктор копирования и копирующий оператор присваивания корректно инкрементируют счетчик ссылок, обрабатывают самоприсваивание и присваивание указателей с одинаковым управляющим блоком (`ref_cnt == other.ref_cnt`).
  - перемещающие операции обнуляют исходный объект без изменения счетчика.
- **Полиморфизм подтипов:** конструкторы и операторы присваивания для производных типов используют аналогичные ограничения через концепт `std::derived_from`.
- **Интерфейс:** `Reset()`, `Get()`, `UseCnt()`, операторы `*`, `->`, `[]` (для массивов), явный `operator bool`.

### 3. `SmartArraySequence<T>`
- Реализует абстрактный интерфейс `Sequence<T>`.
- Внутренний массив управляется через `UniquePtr<T[]> buffer`, что исключает утечки памяти при перевыделении буфера и при возникновении исключений (гарантия RAII).
- **Динамическое расширение буфера:** при заполнении емкость удваивается (`capacity * 2`).
- **Безопасность для move-only типов:** если хранимый тип не поддерживает копирование (например, `UniquePtr`), попытка вызвать копирующий конструктор или копирующий оператор присваивания контейнера пресекается через `if constexpr (std::is_copy_assignable_v<T>)` с выбросом исключения `std::runtime_error`.
- **Проверка границ:** вызовы `Get()`, `InsertAt()`, `RemoveAt()` валидируют индексы с выбросом `std::out_of_range`.

---

## Системные требования и зависимости

- **Стандарт C++:** C++20 (используются концепты, `requires`-clauses, `std::derived_from`).
- **Компилятор:** GCC >= 11, Clang >= 13 или MSVC (Visual Studio 2022+).
- **Система сборки:** CMake >= 3.20.
- **Сторонние зависимости:**
  - [GoogleTest](https://github.com/google/googletest) (версия 1.14.0, автоматически загружается через CMake `FetchContent`). Дополнительная ручная установка не требуется.

---

## Сборка и запуск

### 1. Клонирование репозитория
```bash
git clone https://github.com/VenEraBle842/SmartPtrs.git
cd SmartPtrs
```

### 2. Конфигурация проекта (генератор сборки: Ninja)
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```
> Флаг `Release` важен для бенчмарков. В `Debug` компилятор отключит оптимизации и исказит время.

### 3. Компиляция
```bash
cmake --build build
```

### 4. Запуск демонстрационного приложения
Программа демонстрирует полиморфное использование умных указателей в контейнере `SmartArraySequence`:
```bash
./build/smart_ptrs_demo
```
*(На Windows: `.\build\Release\smart_ptrs_demo.exe` или `.\build\smart_ptrs_demo.exe`)*

### 5. Запуск тестов
- Запустить **все тесты** разом с выводом таблиц в консоль:
  ```bash
  ./build/smart_ptrs_tests
  ```
- Запустить **только функциональные тесты**:
  ```bash
  ./build/smart_ptrs_tests --gtest_filter="*Test.*"
  ```
- Запустить **только бенчмарки**:
  ```bash
  ./build/smart_ptrs_tests --gtest_filter="BenchmarkTest.*"
  ```
- Или через стандартный runner `ctest`:
  ```bash
  ctest --test-dir build --output-on-failure
  ```

#### Функциональные тесты (`tests/FunctionalTests.cpp`)
- Проверка базовых операций жизненного цикла `UniquePtr` и `SharedPtr` (создание, сброс, разыменование, перемещение).
- Проверка предотвращения утечек памяти через вспомогательный счетчик живых объектов (`Tracker`).
- Проверка работы со специализированными массивами (`delete[]`).
- Проверка краевых случаев (самоприсваивание по ссылке, самосброс через `Reset(Get())`).
- Статические проверки компиляции (`static_assert`):
  - Разрешение приведения `Circle` &rarr; `Shape`.
  - Запрет на этапе компиляции несовместимых приведений (`UnrelatedClass` &rarr; `Shape`).
  - Запрет нисходящего приведения (`Shape` &rarr; `Circle`).
- Проверка операций контейнера `SmartArraySequence` (`Append`, `Prepend`, `InsertAt`, `RemoveAt`) и очистки памяти буфера через `Tracker`.
- Проверка совместной работы `SmartArraySequence` с полиморфными указателями (`SharedPtr<Animal>`) и валидация счетчиков ссылок.

#### Анализ производительности и памяти (`tests/BenchmarkTests.cpp`)
Для объективной оценки накладных расходов в тестах переопределены глобальные операторы `new`/`delete`, что позволяет измерять реальное количество динамических аллокаций и байт на куче через структуру `MemoryTracker`.

Бенчмарки сравнивают следующие подходы:
- Raw Pointer (ручное управление)
- Собственный `UniquePtr`
- `std::unique_ptr`
- Собственный `SharedPtr`
- `std::shared_ptr`
- `std::make_shared`

Результаты форматируются и выводятся в виде наглядных консольных таблиц с метриками размера стека, числа аллокаций, памяти на куче и времени выполнения.

### Проверка на утечки памяти (Sanitizers / Valgrind)

Сборка с AddressSanitizer и UndefinedBehaviorSanitizer (GCC / Clang):
```bash
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan
./build-asan/smart_ptrs_tests
```

Запуск под Valgrind (Linux):
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/smart_ptrs_tests
```

---

## Структура репозитория

```text
SmartPtrs/
├── CMakeLists.txt              # Конфигурация сборки CMake (FetchContent GTest, флаги компиляции)
├── main.cpp                    # Точка входа демонстрационного сценария (полиморфизм фигур)
├── src/
│   ├── SharedPtr.hpp           # Шаблонный класс SharedPtr с поддержкой массивов и подтипов
│   ├── UniquePtr.hpp           # Шаблонный класс UniquePtr с поддержкой массивов и подтипов
│   └── SmartArraySequence.hpp  # Интерфейс Sequence и динамический массив на базе UniquePtr
└── tests/
    ├── FunctionalTests.cpp     # GoogleTest модульные тесты функциональности и static_assert
    └── BenchmarkTests.cpp      # Сравнительные бенчмарки времени и использования кучи
```

---

## Результат работы демонстрационной программы

Вывод приложения `smart_ptrs_demo`:

```text
SmartArraySequence container with polymorphic pointers
Init SmartArraySequence<SharedPtr<Shape>> canvas

Appending different shapes (Circle and Rectangle):
[+] Shape ctor: Canvas circle 1
 [+] Circle ctor (r = 2.5)
[+] Shape ctor: Canvas rect 1
 [+] Rectangle ctor (5.00x2.00)
[+] Shape ctor: Canvas circle 2
 [+] Circle ctor (r = 3.0)

Shapes on the canvas: 3

Iter over the canvas and draw all the shapes:
[0] Drawing circle Canvas circle 1 with radius 2.5 (Area: 19.63)
[1] Drawing rectangle Canvas rect 1 with dimensions 5.00x2.00 (Area: 10.00)
[2] Drawing circle Canvas circle 2 with radius 3.0 (Area: 28.27)

Removing the middle shape (rectangle):
 [-] Rectangle dtor
[-] Shape dtor: Canvas rect 1

Shapes remaining: 2

[0] Drawing circle Canvas circle 1 with radius 2.5 (Area: 19.63)
[1] Drawing circle Canvas circle 2 with radius 3.0 (Area: 28.27)

Scope exit (all remaining shapes will now be removed)...
 [-] Circle dtor [-] Shape dtor: Canvas circle 1
 [-] Circle dtor [-] Shape dtor: Canvas circle 2

The scenario has been succesfully completed!
```
