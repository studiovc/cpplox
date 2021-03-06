#pragma once

#include <string>
#include "expression_visitor.hpp"

namespace motts { namespace lox {
    class Ast_printer : public Expr_visitor {
        public:
            void visit(const gcpp::deferred_ptr<const Binary_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Grouping_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Literal_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Unary_expr>&) override;

            const std::string& result() const &;
            std::string&& result() &&;

        private:
            std::string result_;
    };
}}
