#ifndef STACK_H
#define STACK_H

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <iterator>
#include <stdexcept>

namespace cxx {
    template <typename K, typename V> class stack {
    private:
        template <typename P>
        using shared_ptr = std::shared_ptr<P>;

        // Comparator to compare keys under shared_ptr<K>
        struct KComp {
            bool operator()(const shared_ptr<K>& lhs,
                            const shared_ptr<K>& rhs) const {
                return *lhs < *rhs;
            }
        };

        /* Data structure:
         * main_stack: List containing stack content in proper order. 
         * stacks_map: Map of stacks, every key is assigned to the stack of 
        elements (in proper order) with that key. Elements in stacks are iterators
        to that element's position in main_stack

         * There is only 1 copy of each key on the heap, we access them by 
        shared_ptr<K>, all values are kept in main_stack
        */
        using main_stack_t = std::list<std::pair<shared_ptr<K>, V>>;
        using key_stack_t = std::stack<typename main_stack_t::iterator>;
        using stacks_map_t = std::map<shared_ptr<K>, key_stack_t, KComp>;

        shared_ptr<main_stack_t> main_stack;
        shared_ptr<stacks_map_t> stacks_map;
        bool shareable; // Whether stack can share data with other stack (copy on write).

        // Makes a copy of Stack before modification (Copy on write). Strong exception guarantee.
        class about_to_modify {
        public:
            explicit about_to_modify(stack& s, bool mark_shareable) : st(s),
                                                                      old_main_stack(s.main_stack), old_stacks_map(s.stacks_map),
                                                                      old_shareable(s.shareable), roll_back(false) {
                if (!s.main_stack.use_count()) {
                    s.main_stack = std::make_shared<main_stack_t>();
                    s.stacks_map = std::make_shared<stacks_map_t>();
                }
                else if (s.main_stack.use_count() > 2) {
                    stack<K, V> new_stack;
                    new_stack.copy(s);
                    s.swap(new_stack);
                }
                s.shareable = mark_shareable;
                roll_back = true;
            }
            ~about_to_modify() noexcept {
                if (roll_back) {
                    st.main_stack = old_main_stack;
                    st.stacks_map = old_stacks_map;
                    st.shareable = old_shareable;
                }
            }
            void drop_roll_back() noexcept {
                roll_back = false;
            }
        private:
            stack& st;
            std::shared_ptr<main_stack_t> old_main_stack;
            std::shared_ptr<stacks_map_t> old_stacks_map;
            bool old_shareable;
            bool roll_back;
        };

        // Pushes new element to main_stack. Strong exception guarantee.
        class main_stack_guard {
        public:
            explicit main_stack_guard(shared_ptr<main_stack_t>& ms,
                                      const shared_ptr<K>& k, const V& v)
                    : main_stack(ms), roll_back(false) {
                main_stack->push_front({k, v});
                roll_back = true;
            }
            ~main_stack_guard() noexcept {
                if (roll_back) {
                    main_stack->pop_front();
                }
            };
            void drop_roll_back() noexcept {
                roll_back = false;
            }
        private:
            shared_ptr<main_stack_t>& main_stack;
            bool roll_back;
        };

        // Inserts new key to stack_map. Strong exception guarantee.
        class stackmap_key_guard {
        public:
            explicit stackmap_key_guard(shared_ptr<stacks_map_t>& sm,
                                        const shared_ptr<K>& k)
                    : stacks_map(sm), roll_back(false) {
                it = stacks_map->insert({k, {}}).first;
                roll_back = true;
            }
            ~stackmap_key_guard() noexcept {
                if (roll_back) {
                    stacks_map->erase(it);
                }
            }
            void drop_roll_back() noexcept {
                roll_back = false;
            }
            typename stacks_map_t ::iterator get_iter() noexcept {
                return it;
            }
        private:
            shared_ptr<stacks_map_t>& stacks_map;
            typename stacks_map_t::iterator it;
            bool roll_back;
        };

        // Pushes new element to stack_map.
        class stackmap_push_guard {
        public:
            explicit stackmap_push_guard(key_stack_t& st,
                                         const main_stack_t::iterator& v)
                    : stack(st), roll_back(false) {
                stack.push(v);
                roll_back = true;
            }
            ~stackmap_push_guard() noexcept {
                if (roll_back) {
                    stack.pop();
                }
            }
            void drop_roll_back() noexcept {
                roll_back = false;
            }
        private:
            key_stack_t & stack;
            bool roll_back;
        };

        void swap(stack& other) noexcept {
            std::swap(stacks_map, other.stacks_map);
            std::swap(main_stack, other.main_stack);
            std::swap(shareable, other.shareable);
        }

        void copy(const stack& other) {
            if (other.main_stack.use_count()) {
                for (auto rit = other.main_stack->crbegin();
                     rit != other.main_stack->crend(); ++rit) {
                    push(*rit->first, rit->second);
                }
            }
        }


    public:
        stack() : main_stack(), stacks_map() , shareable(true) {
            main_stack = std::make_shared<main_stack_t>();
            stacks_map = std::make_shared<stacks_map_t>();
        }

        stack(const stack& other) : main_stack(), stacks_map(), shareable(true) {
            if (other.shareable) {
                main_stack = other.main_stack;
                stacks_map = other.stacks_map;
            }
            else {
                main_stack = std::make_shared<main_stack_t>();
                stacks_map = std::make_shared<stacks_map_t>();
                copy(other);
            }
        }

        stack(stack&& other) noexcept : main_stack(std::move(other.main_stack)),
                stacks_map(std::move(other.stacks_map)),
        shareable(std::move(other.shareable)) {}

        stack& operator=(stack other) noexcept {
            swap(other);
            return *this;
        }

        void push(const K& k, const V& v) {
            about_to_modify make_stack_copy(*this, true);
            shared_ptr<K> temp_key = std::make_shared<K>(k);
            auto key_in_stack = stacks_map->find(temp_key);
            if (key_in_stack != stacks_map->end()) {
                temp_key = key_in_stack->first;
            }

            main_stack_guard push_main_stack(main_stack, temp_key, v);
            auto it = main_stack->begin();

            if (key_in_stack == stacks_map->end()) {
                stackmap_key_guard push_new_key(stacks_map, temp_key);
                stackmap_push_guard push_value(push_new_key.get_iter()->second,
                                               it);
                push_value.drop_roll_back();
                push_new_key.drop_roll_back();
            }
            else {
                stackmap_push_guard push_value(key_in_stack->second, it);
                push_value.drop_roll_back();
            }

            push_main_stack.drop_roll_back();
            make_stack_copy.drop_roll_back();
        }

        void pop() {
            if (!main_stack.use_count() || main_stack->empty()) {
                throw std::invalid_argument("Error: Empty stack");
            }

            about_to_modify make_stack_copy(*this, true);
            auto main_stack_top = main_stack->begin();
            auto k_stack = stacks_map->find(main_stack_top->first);
            k_stack->second.pop();
            if (k_stack->second.size() == 0) {
                stacks_map->erase(k_stack);
            }
            main_stack->pop_front();
            make_stack_copy.drop_roll_back();
        }

        void pop(const K& k) {
            about_to_modify make_stack_copy(*this, true);
            shared_ptr<K> temp_key = std::make_shared<K>(k);
            auto key_in_stack = stacks_map->find(temp_key);

            if (key_in_stack == stacks_map->end() ||
                key_in_stack->second.size() == 0) {
                throw std::invalid_argument(
                        "Error: Element with key k does not exist");
            }

            main_stack->erase(key_in_stack->second.top());
            key_in_stack->second.pop();
            if (key_in_stack->second.size() == 0) {
                stacks_map->erase(key_in_stack);
            }
            make_stack_copy.drop_roll_back();
        }

        std::pair<const K&, const V&> front() const {
            if (!main_stack.use_count() || main_stack->empty()) {
                throw std::invalid_argument("Error: empty stack");
            }
            return std::pair<const K&, const V&>(*(main_stack->front().first),
                                                 main_stack->front().second);
        }

        std::pair<const K&, V&> front() {
            if (!main_stack.use_count() || main_stack->empty()) {
                throw std::invalid_argument("Error: empty stack");
            }
            about_to_modify make_stack_copy(*this, false);
            make_stack_copy.drop_roll_back();
            return std::pair<const K&, V&>(*(main_stack->front().first),
                                           main_stack->front().second);
        }

        V& front(const K& k) {
            about_to_modify make_stack_copy(*this, false);
            auto key = stacks_map->find(std::make_shared<K>(k));
            if (key == stacks_map->end() || key->second.size() == 0) {
                throw std::invalid_argument(
                        "Error: stack does not contain given key");
            }
            make_stack_copy.drop_roll_back();
            return key->second.top()->second;
        }

        const V& front(const K& k) const {
            if (!main_stack.use_count()) {
                throw std::invalid_argument(
                        "Error: stack does not contain given key");
            }
            auto key = stacks_map->find(std::make_shared<K>(k));
            if (key == stacks_map->end() || key->second.size() == 0) {
                throw std::invalid_argument(
                        "Error: stack does not contain given key");
            }
            return key->second.top()->second;
        }

        size_t size() const noexcept {
            if (!main_stack.use_count()) {
                return 0;
            }
            return main_stack->size();
        }

        size_t count(const K& k) const  {
            if (!main_stack.use_count()) {
                return 0;
            }
            auto it = stacks_map->find(std::make_shared<K>(k));
            if (it == stacks_map->end()) {
                return 0;
            }
            return it->second.size();
        };

        void clear() noexcept {
            main_stack.reset();
            stacks_map.reset();
            shareable = true;
        }

        class const_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = K;
            using difference_type = ptrdiff_t;
            using pointer = const value_type*;
            using reference = const value_type&;

            const_iterator() = default;
            const_iterator(const const_iterator& other) = default;
            const_iterator(const_iterator&& other) noexcept = default;

            const_iterator(const stacks_map_t::const_iterator& it)
                    : map_it(it) {};
            const_iterator(stacks_map_t::const_iterator&& it) noexcept
                    : map_it(std::move(it)) {};

            const_iterator& operator=(const const_iterator& other) = default;
            const_iterator& operator=(const_iterator&& other) noexcept = default;

            reference operator*() const noexcept {
                return *map_it->first;
            }

            pointer operator->() const noexcept {
                return map_it->first.get();
            }

            const_iterator& operator++() noexcept {
                ++map_it;
                return *this;
            }

            const_iterator operator++(int) noexcept {
                const_iterator result(*this);
                operator++();
                return result;
            }

            friend bool operator==(const const_iterator& a,
                                   const const_iterator& b) noexcept {
                return a.map_it == b.map_it;
            }

            friend bool operator!=(const const_iterator& a,
                                   const const_iterator& b) noexcept {
                return !(a == b);
            }
        private:
            stacks_map_t::const_iterator map_it;
        };

        const_iterator cbegin() const noexcept {
            if (!stacks_map.use_count()) {
                return const_iterator();
            }
            return const_iterator(stacks_map->cbegin());
        }

        const_iterator cend() const noexcept {
            if (!stacks_map.use_count()) {
                return const_iterator();
            }
            return const_iterator(stacks_map->cend());
        }
    };

}

#endif //STACK_H

