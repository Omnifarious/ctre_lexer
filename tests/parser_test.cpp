// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "../parser.hpp"

#include <catch2/catch_all.hpp>

SCENARIO(
   "Parsing various strings yields correct evaluation, infix string, "
   "and postfix string."
) // This relies on the tokenizer tests having worked.
{
    using Tokens::tokenize_input;
    using Parser::parse_statement_list;

    GIVEN("A simple numeric literal")
    {
        std::string input = "42;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("The parse succeeds and evaluates correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 42); // StatementList returns 42
                REQUIRE(result->to_infix_string() == "42;\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    42\n)");
            }
        }
    }

    GIVEN("A simple assignment statement")
    {
        std::string input = "x = 42;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("The parse succeeds and creates assignment")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 0); // StatementList returns 0
                REQUIRE(result->to_infix_string() == "x = 42;\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (setq x 42)\n)");
            }
        }
    }

    GIVEN("Expression with operator precedence: addition and multiplication")
    {
        std::string input = "2 + 3 * 4;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Multiplication binds tighter than addition")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as 2 + (3 * 4) = 14, not (2 + 3) * 4 = 20
                REQUIRE(result->evaluate() == 14);
                REQUIRE(result->to_infix_string() == "(2 + (3 * 4));\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (+ 2 (* 3 4))\n)");
            }
        }
    }

    GIVEN("Expression with parentheses overriding precedence")
    {
        std::string input = "(2 + 3) * 4;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Parentheses override natural precedence")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 20);
                REQUIRE(result->to_infix_string() == "((2 + 3) * 4);\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (* (+ 2 3) 4)\n)");
            }
        }
    }

    GIVEN("Right-associative expression: 8 - 5 - 2")
    {
        std::string input = "8 - 5 - 2;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Expression is right-associative")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as 8 - (5 - 2) due to right-associative grammar
                REQUIRE(result->evaluate() == 5);
                REQUIRE(result->to_infix_string() == "(8 - (5 - 2));\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (- 8 (- 5 2))\n)");
            }
        }
    }

    GIVEN("Left-associative factors: 8 * 5 / 3")
    {
        std::string input = "8 * 5 / 3;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());

            THEN("Expression with factors is left-associative")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as (8 * 5) / 3 due to left-associative grammar
                REQUIRE(result->evaluate() == 13);
                REQUIRE(result->to_infix_string() == "((8 * 5) / 3);\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (/ (* 8 5) 3)\n)");
            }
        }
    }

    GIVEN("Multiple statements, and assignment works correctly")
    {
        std::string input = "x = 10; y = 20; x + y;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("All statements are parsed into a statement list")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 30);
                REQUIRE(result->to_infix_string() == "x = 10;\ny = 20;\n(x + y);\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (setq x 10)\n    (setq y 20)\n    (+ x y)\n)");
            }
        }
    }

    GIVEN("Nested parentheses")
    {
        std::string input = "((2 + 3) * (4 - 1));";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Nested expressions are handled correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 15);
                REQUIRE(result->to_infix_string() == "((2 + 3) * (4 - 1));\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (* (+ 2 3) (- 4 1))\n)");
            }
        }
    }

    GIVEN("Expression with different number formats")
    {
        std::string input = "0x10 + 0755 + 42;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Different number formats are handled in expressions")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 551);
                REQUIRE(result->to_infix_string() == "(16 + (493 + 42));\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    (+ 16 (+ 493 42))\n)");
            }
        }
    }

    GIVEN("Empty input")
    {
        std::string input = "";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Empty input creates empty statement list")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 0);
                REQUIRE(result->to_infix_string() == "");
                REQUIRE(result->to_prefix_string() == "(progn\n)");
            }
        }
    }

    GIVEN("Single identifier")
    {
        std::string input = "variable;";
        auto tokens = tokenize_input(input.begin(), input.end());
        
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_statement_list(tokens.begin(), tokens.end());
            
            THEN("Identifier is parsed as expression")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                REQUIRE(result->evaluate() == 0);
                REQUIRE(result->to_infix_string() == "variable;\n");
                REQUIRE(result->to_prefix_string() == "(progn\n    variable\n)");
            }
        }
    }
}