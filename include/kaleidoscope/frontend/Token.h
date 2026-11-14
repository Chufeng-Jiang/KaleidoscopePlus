#pragma once

namespace kaleidoscope {

enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,

    // control
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_for = -9,
    tok_in = -10,

    // operator
    tok_binary = -11,
    tok_unary = -12,

    // var
    tok_var = -13,
    tok_global = -14,
    tok_invalid = -15,
    tok_integer = -16,
    tok_true = -17,
    tok_false = -18,
    tok_type = -19,
    tok_acc = -20
};

} // namespace kaleidoscope
