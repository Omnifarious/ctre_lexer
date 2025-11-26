// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "../tokens.hpp"

#include <catch2/catch_all.hpp>

SCENARIO("Fixed string generates expected list of tokens.")
{
    using Tokens::tokenize_input;
    GIVEN("A simple mathematical expression string")
    {
        std::string input = "x = 42 + 3 * y;";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("The correct tokens are generated")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Identifier{"x", "x"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"42", 42},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"3", 3},
                    t::Operator{"*", t::Operator::Multiply},
                    t::Identifier{"y", "y"},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }
    
    GIVEN("A string with different number formats")
    {
        std::string input = "0x1A + 0755 + 123";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("Different number formats are recognized with correct values")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::UnsignedInteger{"0x1A", 0x1A},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"0755", 0755},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"123", 123}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }
    
    GIVEN("A string with parentheses and semicolon")
    {
        std::string input = "(a - b);";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("Parentheses and semicolon are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"a", "a"},
                    t::Operator{"-", t::Operator::Minus},
                    t::Identifier{"b", "b"},
                    t::Paren{")", t::Paren::Close},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }
    
    GIVEN("A string with invalid tokens")
    {
        std::string input = "x = 42 @ y;";  // @ is not a valid token
        
        WHEN("The string is tokenized")
        {
            THEN("A parse_error exception is thrown")
            {
                using Tokens::tokenize_error;
                REQUIRE_THROWS_AS(tokenize_input(input.begin(), input.end()), tokenize_error);
            }
            
            AND_THEN("The exception message contains the unexpected token")
            {
                REQUIRE_THROWS_WITH(
                    tokenize_input(input.begin(), input.end()),
                    Catch::Matchers::ContainsSubstring("Unknown token: @")
                );
            }
        }
    }

    GIVEN("A string with boolean operators")
    {
        std::string input = "x = true && false || true;";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Identifier{"x", "x"},
            t::Operator{"=", t::Operator::Equal},
            t::Identifier{"true", "true"},
            t::Operator{"&&", Tokens::Operator::BoolAnd},
            t::Identifier{"false", "false"},
            Tokens::Operator{"||", Tokens::Operator::BoolOr},
            t::Identifier{"true", "true"},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The correct tokens are generated")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with the 'var' keyword for declaration and initialization")
    {
        std::string input = "var x = 3;";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Keyword{"var", t::Keyword::Var},
            t::Identifier{"x", "x"},
            t::Operator{"=", t::Operator::Equal},
            t::UnsignedInteger{"3", 3},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The 'var' declaration is correctly tokenized")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("Multiple 'var' declarations including one without initializer")
    {
        std::string input = "var a; var b = 2;";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Keyword{"var", t::Keyword::Var},
            t::Identifier{"a", "a"},
            t::Semicolon{},
            t::Keyword{"var", t::Keyword::Var},
            t::Identifier{"b", "b"},
            t::Operator{"=", t::Operator::Equal},
            t::UnsignedInteger{"2", 2},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Both 'var' statements are correctly tokenized")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("Identifiers like 'vars' should not be mistaken for the 'var' keyword")
    {
        std::string input = "vars = 5;";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Identifier{"vars", "vars"},
            t::Operator{"=", t::Operator::Equal},
            t::UnsignedInteger{"5", 5},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("'vars' is tokenized as an identifier, not a 'var' keyword")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with curly brackets for code blocks")
    {
        std::string input = "{ x = 42; }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The curly brackets are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::CurlyBracket{"{", t::CurlyBracket::Open},
                    t::Identifier{"x", "x"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"42", 42},
                    t::Semicolon{},
                    t::CurlyBracket{"}", t::CurlyBracket::Close}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with if-else keywords")
    {
        std::string input = "if (x) y = 1; else y = 0;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The keywords are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Keyword{"if", t::Keyword::If},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"x", "x"},
                    t::Paren{")", t::Paren::Close},
                    t::Identifier{"y", "y"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},
                    t::Keyword{"else", t::Keyword::Else},
                    t::Identifier{"y", "y"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"0", 0},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A simple function declaration using the 'def' keyword and comma punctuator")
    {
        // This also validates that ',' is recognized as a punctuator token
        std::string input = "def sum(a, b);";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Keyword{"def", t::Keyword::Def},
            t::Identifier{"sum", "sum"},
            t::Paren{"(", t::Paren::Open},
            t::Identifier{"a", "a"},
            t::Punctuator{",", t::Punctuator::Comma},
            t::Identifier{"b", "b"},
            t::Paren{")", t::Paren::Close},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The 'def' keyword and comma punctuator are correctly tokenized")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("Identifiers like 'define' should not be mistaken for the 'def' keyword")
    {
        std::string input = "define x;";
        namespace t = Tokens;
        const t::toklist_t expected_tokens{
            t::Identifier{"define", "define"},
            t::Identifier{"x", "x"},
            t::Semicolon{}
        };

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("'define' is tokenized as an identifier, not the 'def' keyword")
            {
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with keywords that should not be confused with identifiers")
    {
        std::string input = "ifx = 1; elsif = 2; if_var = 3;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Keywords with additional characters are treated as identifiers")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Identifier{"ifx", "ifx"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},
                    t::Identifier{"elsif", "elsif"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"2", 2},
                    t::Semicolon{},
                    t::Identifier{"if_var", "if_var"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"3", 3},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with nested curly brackets")
    {
        std::string input = "{ { x = 1; } }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Nested curly brackets are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::CurlyBracket{"{", t::CurlyBracket::Open},
                    t::CurlyBracket{"{", t::CurlyBracket::Open},
                    t::Identifier{"x", "x"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},
                    t::CurlyBracket{"}", t::CurlyBracket::Close},
                    t::CurlyBracket{"}", t::CurlyBracket::Close}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A complete if-else statement with blocks")
    {
        std::string input = "if (x + 0) { result = 1; } else { result = 0; }";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("All tokens including keywords and brackets are correctly identified")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Keyword{"if", t::Keyword::If},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"x", "x"},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"0", 0},
                    t::Paren{")", t::Paren::Close},
                    t::CurlyBracket{"{", t::CurlyBracket::Open},
                    t::Identifier{"result", "result"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},
                    t::CurlyBracket{"}", t::CurlyBracket::Close},
                    t::Keyword{"else", t::Keyword::Else},
                    t::CurlyBracket{"{", t::CurlyBracket::Open},
                    t::Identifier{"result", "result"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"0", 0},
                    t::Semicolon{},
                    t::CurlyBracket{"}", t::CurlyBracket::Close}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("An empty string")
    {
        std::string input = "";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("No tokens are generated")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{};
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with only whitespace")
    {
        std::string input = "   \t  \n  ";
        
        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            
            THEN("No tokens are generated")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{};
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with relational operators")
    {
        std::string input = "x < y; a > b; c <= d; e >= f;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("The relational operators are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Identifier{"x", "x"},
                    t::Operator{"<", t::Operator::Less},
                    t::Identifier{"y", "y"},
                    t::Semicolon{},
                    t::Identifier{"a", "a"},
                    t::Operator{">", t::Operator::Greater},
                    t::Identifier{"b", "b"},
                    t::Semicolon{},
                    t::Identifier{"c", "c"},
                    t::Operator{"<=", t::Operator::LessEqual},
                    t::Identifier{"d", "d"},
                    t::Semicolon{},
                    t::Identifier{"e", "e"},
                    t::Operator{">=", t::Operator::GreaterEqual},
                    t::Identifier{"f", "f"},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with complex relational expressions")
    {
        std::string input =
            "if (x <= 10 && y >= 5 || z > 0) result = x < y; else result = x != y;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Complex relational expressions are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Keyword{"if", t::Keyword::If},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"x", "x"},
                    t::Operator{"<=", t::Operator::LessEqual},
                    t::UnsignedInteger{"10", 10},
                    t::Operator{"&&", t::Operator::BoolAnd},
                    t::Identifier{"y", "y"},
                    t::Operator{">=", t::Operator::GreaterEqual},
                    t::UnsignedInteger{"5", 5},
                    t::Operator{"||", t::Operator::BoolOr},
                    t::Identifier{"z", "z"},
                    t::Operator{">", t::Operator::Greater},
                    t::UnsignedInteger{"0", 0},
                    t::Paren{")", t::Paren::Close},
                    t::Identifier{"result", "result"},
                    t::Operator{"=", t::Operator::Equal},
                    t::Identifier{"x", "x"},
                    t::Operator{"<", t::Operator::Less},
                    t::Identifier{"y", "y"},
                    t::Semicolon{},
                    t::Keyword{"else", t::Keyword::Else},
                    t::Identifier{"result", "result"},
                    t::Operator{"=", t::Operator::Equal},
                    t::Identifier{"x", "x"},
                    t::Operator{"!=", t::Operator::NotEqual},
                    t::Identifier{"y", "y"},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string testing edge cases with relational operators")
    {
        std::string input = "a<b; c>d; e<=f; g>=h;";  // No spaces around operators

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Relational operators without spaces are correctly tokenized")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Identifier{"a", "a"},
                    t::Operator{"<", t::Operator::Less},
                    t::Identifier{"b", "b"},
                    t::Semicolon{},

                    t::Identifier{"c", "c"},
                    t::Operator{">", t::Operator::Greater},
                    t::Identifier{"d", "d"},
                    t::Semicolon{},

                    t::Identifier{"e", "e"},
                    t::Operator{"<=", t::Operator::LessEqual},
                    t::Identifier{"f", "f"},
                    t::Semicolon{},

                    t::Identifier{"g", "g"},
                    t::Operator{">=", t::Operator::GreaterEqual},
                    t::Identifier{"h", "h"},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A string with relational operators and numbers")
    {
        std::string input = "42 < 100; 0x10 >= 15; 0755 <= 500;";

        WHEN("The string is tokenized")
        {
            auto tokens = tokenize_input(input.begin(), input.end());

            THEN("Relational operators with different number formats work correctly")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::UnsignedInteger{"42", 42},
                    t::Operator{"<", t::Operator::Less},
                    t::UnsignedInteger{"100", 100},
                    t::Semicolon{},
                    t::UnsignedInteger{"0x10", 0x10},
                    t::Operator{">=", t::Operator::GreaterEqual},
                    t::UnsignedInteger{"15", 15},
                    t::Semicolon{},
                    t::UnsignedInteger{"0755", 0755},
                    t::Operator{"<=", t::Operator::LessEqual},
                    t::UnsignedInteger{"500", 500},
                    t::Semicolon{}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }

    GIVEN("A some uses of the while keyword.")
    {
        std::string input = "while (whilex < 10) {"
                            "    whilex = whilex + 1;"
                            "    while5 = 3;"
                            "    if ( whilex > 5 ) {"
                            "        while( whilex < 10 ) whilex = whilex + 1;"
                            "    }"
                            "}";
        WHEN("The string is tokenized.")
        {
            auto tokens = tokenize_input(input.begin(), input.end());
            THEN("The while keyword is correctly tokenized.")
            {
                namespace t = Tokens;
                const t::toklist_t expected_tokens{
                    t::Keyword{"while", t::Keyword::While},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"<", t::Operator::Less},
                    t::UnsignedInteger{"10", 10},
                    t::Paren{")", t::Paren::Close},
                    t::CurlyBracket{"{", t::CurlyBracket::Open},

                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"=", t::Operator::Equal},
                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},

                    t::Identifier{"while5", "while5"},
                    t::Operator{"=", t::Operator::Equal},
                    t::UnsignedInteger{"3", 3},
                    t::Semicolon{},

                    t::Keyword{"if", t::Keyword::If},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"whilex", "whilex"},
                    t::Operator{">", t::Operator::Greater},
                    t::UnsignedInteger{"5", 5},
                    t::Paren{")", t::Paren::Close},
                    t::CurlyBracket{"{", t::CurlyBracket::Open},

                    t::Keyword{"while", t::Keyword::While},
                    t::Paren{"(", t::Paren::Open},
                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"<", t::Operator::Less},
                    t::UnsignedInteger{"10", 10},
                    t::Paren{")", t::Paren::Close},

                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"=", t::Operator::Equal},
                    t::Identifier{"whilex", "whilex"},
                    t::Operator{"+", t::Operator::Plus},
                    t::UnsignedInteger{"1", 1},
                    t::Semicolon{},

                    t::CurlyBracket{"}", t::CurlyBracket::Close},
                    t::CurlyBracket{"}", t::CurlyBracket::Close}
                };
                REQUIRE_THAT(tokens, Catch::Matchers::Equals(expected_tokens));
            }
        }
    }
}
