# Enhanced exception-safe stack
A C++ programming course project regarding exception safety and memory management with smart pointers.

## Problem statement
The goal of this project is to implement a container template behaving like a stack where each element consists of a (key, value) pair. The container must provide a strong exception guarantee and realize the copy-on-write semantics.

The template is to be parametrized by the key and value types, named `K` and `V` accordingly. The key type `K` has value semantics (provides a default constructor, copy constructor, move constructor, and assignment operators). A linear order is defined on `K` and objects of that type can be freely compared. Type `V` only guarantees a copy constructor.

The class `stack` must be visible in a `cxx` namespace and have the following declaration:
```c++
namespace cxx {
  template <typename K, typename V> class stack;
}
```

The class has to offer all of the operations described below. Time complexities apply only to cases when no copy is created. Time complexity of copying is `O(n log n)`, where `n` is the number of elements on the stack. All operations must provide a strong exception guarantee, the move constructor and the desctructor must not throw.

- Constructors. Time complexity `O(1)`.
```c++
  stack();
  stack(stack const &);
  stack(stack &&);
```
- Assignment operator, Time complexity `O(1)` plus the time of destroying the overriden object.
```c++
  stack & operator=(stack);
```
- Push method. Time complexity `O(log n)`.
```c++
  void push(K const &, V const &);
```
- Pop method. If the stack is empty, `std::invalid_argument` should be thrown. Time complexity `O(log n)`.
```c++
  void pop();
```
- Pop method with a key parameter. Pops the closest element with the given key. If the stack is empty, `std::invalid_argument` should be thrown. Time complexity `O(log n)`.
```c++
  void pop(K const &);
```
- Front methods returning a pair of references to the key and value on the top of the stack. In the non-const version the pair should allow for modifying the value, but not the key. A modifying operation on the stack might invalidate the returned references. If the stack is empty, `std::invalid_argument` should be thrown. Time complexity `O(1)`.
```c++
  std::pair<K const &, V &> front();
  std::pair<K const &, V const &> front() const;
```
- Front methods with a key parameter. Returns the closest value with the given key. Details as above. Time complexity `O(log n)`.
```c++
  V & front(K const &);
  V const & front(K const &) const;
```
- Size method. Time complexity `O(1)`.
```c++
  size_t size() const;
```
- Count method returning the number of elements with the given key. Time complexity `O(log n)`.
```c++
  size_t count(K const &) const;
```
- Clear method removing all elements from the stack. Time complexity `O(n)`.
```c++
  void clear();
```
- Iterator `const_iterator`, `cbegin`, `cend` methods on the stack, and assignment operators, comparisons (`==`, `!=`), incrementation (both prefix and postfix), dereferencing (`*`, `->`) allowing for looping through the key set in their increasing order. The iterator must satisfy the `std::forward_iterator` concept. All operations in `O(log n)`. Looping through all the keys in `O(n)`. The iterator should behave like a `const_iterator` from the standard library.

The methods should be declared `const` and `noexcept` wherever possible and reasonable.

An object of type `stack` should only keep one copy of the inserted keys and values. Storing copies of equal keys is forbidden.

An example use can be viewed in `stack_example.cc`.

## Formal requirements
The solution will be compiled on the `students` machine with the following command:
```bash
g++ -Wall -Wextra -O2 -std=c++20 *.cc
```
Credit to my friend at college for translating this readme to English [Nikodem Gapski](https://github.com/NikodemGapski)