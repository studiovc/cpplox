#pragma once

#include <iterator>
#include <string>

#include "exception.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    /*
    Nystrom's Java code uses a `Scanner` class with a `scan_tokens` method that returns an array of
    tokens. In earlier commits, I mirrored that implementation, but I didn't like any of the choices for
    return type. If I returned an array value, then I'd be making an unnecessary copy.  If I returned a
    mutable array reference, then the class would lose all control over its private data. And if I
    returned a const array reference, then I'd again be making an unnecessary copy for calling code that
    needs a mutable array.

    The new solution below instead uses the iterator pattern. Rather than return a complete list of
    tokens, instead I provide an interface to iterate through the tokens. The call site can then
    populate an array or any other data structure with the begin and end iterators.
    */
    class Token_iterator : public std::iterator<std::forward_iterator_tag, Token> {
        public:
            // Begin
            explicit Token_iterator(const std::string& source);

            // End
            explicit Token_iterator();

            Token_iterator& operator++();
            Token_iterator operator++(int);

            bool operator==(const Token_iterator& rhs) const;
            bool operator!=(const Token_iterator& rhs) const;

            const Token& operator*() const &;
            Token&& operator*() &&;

            const Token* operator->() const;

        private:
            const std::string* source {};

            // Nystrom tracks the substring of a token with two indexes, named `start` and `current`. But in C++,
            // it's considered better style to track positions with iterators. After I changed the type of `start`
            // and `current` to `string::iterator`, it made sense to rename them based on iterator terminology, so
            // I renamed `start` and `current` to `token_begin` and `token_end` respectively.
            std::string::const_iterator token_begin;
            std::string::const_iterator token_end;

            int line {1};
            Token token;

            Token make_token(Token_type);
            Token make_token(Token_type, Literal_multi_type&&);

            // I renamed `match` to `advance_if_match` to communicate the side-effect this function causes
            bool advance_if_match(char expected);

            Token consume_string();
            Token consume_number();
            Token consume_identifier();
            Token consume_token();
    };

    struct Scanner_error : Runtime_error {
        explicit Scanner_error(const std::string& what, int line);
    };
}}
