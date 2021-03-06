#include "vm.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>

#include "compiler.hpp"
#include "debug.hpp"

using std::cout;
using std::move;
using std::nullptr_t;
using std::string;
using std::uint16_t;

using boost::apply_visitor;
using boost::get;
using boost::static_visitor;

using namespace motts::lox;

namespace {
    // Only false and nil are falsey, everything else is truthy
    struct Is_truthy_visitor : static_visitor<bool> {
        auto operator()(bool value) const {
            return value;
        }

        auto operator()(nullptr_t) const {
            return false;
        }

        template<typename T>
            auto operator()(const T&) const {
                return true;
            }
    };

    struct Is_equal_visitor : static_visitor<bool> {
        // Different types automatically compare false
        template<typename T, typename U>
            auto operator()(const T&, const U&) const {
                return false;
            }

        // Same types use normal equality test
        template<typename T>
            auto operator()(const T& lhs, const T& rhs) const {
                return lhs == rhs;
            }
    };

    struct Plus_visitor : static_visitor<Value> {
        auto operator()(const string& lhs, const string& rhs) const {
            return Value{lhs + rhs};
        }

        auto operator()(double lhs, double rhs) const {
            return Value{lhs + rhs};
        }

        // All other type combinations can't be '+'-ed together
        template<typename T, typename U>
            Value operator()(const T&, const U&) const {
                throw VM_error{"Operands must be two numbers or two strings."};
            }
    };
}

namespace motts { namespace lox {
    void VM::interpret(const string& source) {
      const auto chunk = compile(source);

      chunk_ = &chunk;
      ip_ = chunk.code.cbegin();

      run();
    }

    void VM::run() {
        for (;;) {
            cout << "          ";
            for (const auto value : stack_) {
                cout << "[ ";
                print_value(value);
                cout << " ]";
            }
            cout << "\n";
            disassemble_instruction(*chunk_, ip_ - chunk_->code.cbegin());

            const auto instruction = static_cast<Op_code>(*ip_++);
            switch (instruction) {
                case Op_code::constant: {
                    const auto constant_offset = *ip_++;
                    const auto constant = chunk_->constants.at(constant_offset);
                    stack_.push_back(constant);

                    break;
                }

                case Op_code::nil: {
                    stack_.push_back(Value{nullptr});
                    break;
                }

                case Op_code::true_: {
                    stack_.push_back(Value{true});
                    break;
                }

                case Op_code::false_: {
                    stack_.push_back(Value{false});
                    break;
                }

                case Op_code::pop: {
                    stack_.pop_back();
                    break;
                }

                case Op_code::get_local: {
                    stack_.push_back(stack_.at(*ip_++));
                    break;
                }

                case Op_code::set_local: {
                    stack_.at(*ip_++) = stack_.back();
                    break;
                }

                case Op_code::get_global: {
                    const auto name = get<string>(chunk_->constants.at(*ip_++).variant);
                    const auto found_value = globals_.find(name);
                    if (found_value == globals_.end()) {
                        throw VM_error{"Undefined variable '" + name + "'"};
                    }
                    stack_.push_back(found_value->second);

                    break;
                }

                case Op_code::set_global: {
                    const auto name = get<string>(chunk_->constants.at(*ip_++).variant);
                    const auto found_value = globals_.find(name);
                    if (found_value == globals_.end()) {
                        throw VM_error{"Undefined variable '" + name + "'"};
                    }
                    found_value->second = stack_.back();

                    break;
                }

                case Op_code::define_global: {
                    const auto name = get<string>(chunk_->constants.at(*ip_++).variant);
                    globals_[name] = stack_.back();
                    stack_.pop_back();

                    break;
                }

                case Op_code::equal: {
                    const auto right_value = move(stack_.back());
                    stack_.pop_back();

                    const auto left_value = move(stack_.back());
                    stack_.pop_back();

                    stack_.push_back(Value{
                        apply_visitor(Is_equal_visitor{}, left_value.variant, right_value.variant)
                    });

                    break;
                }

                case Op_code::greater: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value > right_value});

                    break;
                }

                case Op_code::less: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value < right_value});

                    break;
                }

                case Op_code::add: {
                    const auto right_value = move(stack_.back());
                    stack_.pop_back();

                    const auto left_value = move(stack_.back());
                    stack_.pop_back();

                    stack_.push_back(apply_visitor(Plus_visitor{}, left_value.variant, right_value.variant));

                    break;
                }

                case Op_code::subtract: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value - right_value});

                    break;
                }

                case Op_code::multiply: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value * right_value});

                    break;
                }

                case Op_code::divide: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value / right_value});

                    break;
                }

                case Op_code::not_: {
                    const auto value = apply_visitor(Is_truthy_visitor{}, stack_.back().variant);
                    stack_.pop_back();
                    stack_.push_back(Value{!value});

                    break;
                }

                case Op_code::negate: {
                    // TODO Convert error: "Operand must be a number."
                    const auto value = get<double>(stack_.back().variant);
                    stack_.pop_back();
                    stack_.push_back(Value{-value});

                    break;
                }

                case Op_code::print: {
                    print_value(stack_.back());
                    cout << "\n";
                    stack_.pop_back();

                    break;
                }

                case Op_code::jump: {
                    // DANGER! Reinterpret cast: The two bytes following a jump_if_false instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*ip_);
                    ip_ += 2;

                    ip_ += jump_length;

                    break;
                }

                case Op_code::jump_if_false: {
                    // DANGER! Reinterpret cast: The two bytes following a jump_if_false instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*ip_);
                    ip_ += 2;

                    if (!apply_visitor(Is_truthy_visitor{}, stack_.back().variant)) {
                        ip_ += jump_length;
                    }

                    break;
                }

                case Op_code::loop: {
                    // DANGER! Reinterpret cast: The two bytes following a loop instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*ip_);
                    ip_ -= 1;

                    ip_ -= jump_length;

                    break;
                }

                case Op_code::return_: {
                    return;
                }
            }
        }
    }
}}
