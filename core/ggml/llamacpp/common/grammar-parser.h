// Implements a parser for an extended Backus-Naur form (BNF), producing the
// binary context-free grammar format specified by llama.h. Supports character
// ranges, grouping, and repetition operators. As an example, a grammar for
// arithmetic might look like:
//
// root  ::= expr
// expr  ::= term ([-+*/] term)*
// term  ::= num | "(" space expr ")" space
// num   ::= [0-9]+ space
// space ::= [ \t\n]*

#pragma once
#include "llama.h"
#include <vector>
#include <map>
#include <cstdint>
#include <string>

namespace grammar_parser {
    enum llama_gretype {
        // end of rule definition
        LLAMA_GRETYPE_END            = 0,

        // start of alternate definition for rule
        LLAMA_GRETYPE_ALT            = 1,

        // non-terminal element: reference to rule
        LLAMA_GRETYPE_RULE_REF       = 2,

        // terminal element: character (code point)
        LLAMA_GRETYPE_CHAR           = 3,

        // inverse char(s) ([^a], [^a-b] [^abc])
        LLAMA_GRETYPE_CHAR_NOT       = 4,

        // modifies a preceding LLAMA_GRETYPE_CHAR or LLAMA_GRETYPE_CHAR_ALT to
        // be an inclusive range ([a-z])
        LLAMA_GRETYPE_CHAR_RNG_UPPER = 5,

        // modifies a preceding LLAMA_GRETYPE_CHAR or
        // LLAMA_GRETYPE_CHAR_RNG_UPPER to add an alternate char to match ([ab], [a-zA])
        LLAMA_GRETYPE_CHAR_ALT       = 6,
    };
    typedef struct llama_grammar_element {
        enum llama_gretype type;
        uint32_t           value; // Unicode code point or rule ID
    } llama_grammar_element;
    struct parse_state {
        std::map<std::string, uint32_t>                 symbol_ids;
        std::vector<std::vector<llama_grammar_element>> rules;

        std::vector<const llama_grammar_element *> c_rules();
    };

    parse_state parse(const char * src);
    void print_grammar(FILE * file, const parse_state & state);
}
