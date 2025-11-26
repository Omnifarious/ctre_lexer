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
    using Parser::parse_top;
    ::std::vector<::std::uintmax_t> results;
    auto save_result = [&results](::std::uintmax_t result) {
        results.push_back(result);
    };

    GIVEN("A simple numeric literal")
    {
        std::string input = "42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse succeeds and evaluates correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 42); // StatementList returns 42
                CHECK(result->to_infix_string() == "42;\n");
                CHECK(result->to_prefix_string() == "(progn\n    42\n)");
            }
        }
    }

    GIVEN("A simple assignment statement missing a variable declaration")
    {
        std::string input = "x = 42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse fails because x is not declared.")
            {
                // TODO This following is wrong, and needs fixing in the code
                CHECK(result != nullptr);
                REQUIRE(remainder != tokens.end());
            }
        }
    }

    GIVEN("A simple variable declaration and assignment statement")
    {
        std::string input = "var x = 0; x = 42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The parse fails because x is not declared.")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 0); // StatementList returns 0
                CHECK(result->to_infix_string() == "var x = 0;\nx = 42;\n");
                CHECK(result->to_prefix_string() == "(progn\n"
                    "    (setq-new x 0)\n    (setq x 42)\n)");
            }
        }
    }

    GIVEN("Expression with operator precedence: addition and multiplication")
    {
        std::string input = "2 + 3 * 4;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Multiplication binds tighter than addition")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as 2 + (3 * 4) = 14, not (2 + 3) * 4 = 20
                CHECK(result->evaluate() == 14);
                CHECK(result->to_infix_string() == "(2 + (3 * 4));\n");
                CHECK(result->to_prefix_string() == "(progn\n    (+ 2 (* 3 4))\n)");
            }
        }
    }

    GIVEN("Expression with parentheses overriding precedence")
    {
        std::string input = "(2 + 3) * 4;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Parentheses override natural precedence")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 20);
                CHECK(result->to_infix_string() == "((2 + 3) * 4);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (* (+ 2 3) 4)\n)");
            }
        }
    }

    GIVEN("Left-associative expression: 8 - 5 - 2")
    {
        std::string input = "8 - 5 - 2;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Expression is left-associative")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as (8 - 5) - 2 due to left-associative grammar
                CHECK(result->evaluate() == 1);
                CHECK(result->to_infix_string() == "((8 - 5) - 2);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (- (- 8 5) 2)\n)");
            }
        }
    }

    GIVEN("Left-associative factors: 8 * 5 / 3")
    {
        std::string input = "8 * 5 / 3;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Expression with factors is left-associative")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Should parse as (8 * 5) / 3 due to left-associative grammar
                CHECK(result->evaluate() == 13);
                CHECK(result->to_infix_string() == "((8 * 5) / 3);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (/ (* 8 5) 3)\n)");
            }
        }
    }

    GIVEN("Multiple statements, and assignment works correctly")
    {
        std::string input = "var x = 0; var y = 0; x = 10; y = 20; x + y;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [ast_top, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("All statements are parsed into a statement list")
            {
                using Catch::Matchers::RangeEquals;
                REQUIRE(ast_top != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *ast_top);
                CHECK(eval.current_result_ == 30);
                CHECK_THAT(results, RangeEquals({0U, 0U, 0U, 0U, 30U}));
                CHECK(ast_top->to_infix_string() == "var x = 0;\nvar y = 0;\nx = 10;\ny = 20;\n(x + y);\n");
                CHECK(ast_top->to_prefix_string() == "(progn\n    (setq-new x 0)\n    (setq-new y 0)\n    (setq x 10)\n    (setq y 20)\n    (+ x y)\n)");
            }
        }
    }

    GIVEN("Nested parentheses")
    {
        std::string input = "((2 + 3) * (4 - 1));";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Nested expressions are handled correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 15);
                CHECK(result->to_infix_string() == "((2 + 3) * (4 - 1));\n");
                CHECK(result->to_prefix_string() == "(progn\n    (* (+ 2 3) (- 4 1))\n)");
            }
        }
    }

    GIVEN("Expression with different number formats")
    {
        std::string input = "0x10 + 0755 + 42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Different number formats are handled in expressions")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 551);
                CHECK(result->to_infix_string() == "((16 + 493) + 42);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (+ (+ 16 493) 42)\n)");
            }
        }
    }

    GIVEN("Expression with boolean operators.")
    {
        std::string input = "var five = 5; var one = 1; var six = 6;"
                            "five - one * five && one;"
                            "five * six - six && one;"
                            "0 && 1;"
                            "1 && 0;"
                            "0 && 0;"
                            "0 || 1 && 5;"
                            "1 && 5 || 0;"
                            "0 || 6 && 0;"
                            "0 && 5 || 1 && 0;"
                            "5 || 0 && 0;"
                            "1 || 6;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [ast_top, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Boolean operators are handled correctly")
            {
                using Catch::Matchers::RangeEquals;
                REQUIRE(ast_top != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *ast_top);
                CHECK(eval.current_result_ == 1);
                CHECK_THAT(results, RangeEquals({
                    0U, // five = 5
                    0U, // one = 1
                    0U, // six = 6
                    0U, // five - one * five && one
                    1U, // five * six - six && one
                    0U, // 0 && 1
                    0U, // 1 && 0
                    0U, // 0 && 0
                    1U, // 0 || 1 && 5
                    1U, // 1 && 5 || 0
                    0U, // 0 || 6 && 0
                    0U, // 0 && 5 || 1 && 0
                    0U, // 5 || 0 && 0
                    1U  // 1 || 6
                }));
                CHECK(ast_top->to_infix_string() == "var five = 5;\nvar one = 1;\nvar six = 6;\n((five - (one * five)) && one);\n(((five * six) - six) && one);\n(0 && 1);\n(1 && 0);\n(0 && 0);\n((0 || 1) && 5);\n((1 && 5) || 0);\n((0 || 6) && 0);\n(((0 && 5) || 1) && 0);\n((5 || 0) && 0);\n(1 || 6);\n");
                CHECK(ast_top->to_prefix_string() == "(progn\n    (setq-new five 5)\n    (setq-new one 1)\n    (setq-new six 6)\n    (&& (- five (* one five)) one)\n    (&& (- (* five six) six) one)\n    (&& 0 1)\n    (&& 1 0)\n    (&& 0 0)\n    (&& (|| 0 1) 5)\n    (|| (&& 1 5) 0)\n    (&& (|| 0 6) 0)\n    (&& (|| (&& 0 5) 1) 0)\n    (&& (|| 5 0) 0)\n    (|| 1 6)\n)");
            }
        }
    }

    GIVEN("Empty input")
    {
        std::string input = "";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Empty input creates empty statement list")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 0);
                CHECK(result->to_infix_string() == "");
                CHECK(result->to_prefix_string() == "(progn\n)");
            }
        }
    }

    GIVEN("Single identifier")
    {
        std::string input = "variable;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Parsing fails because the identifier wasn't declared.")
            {
                // TODO This following is wrong, and needs fixing in the code
                CHECK(result != nullptr);
                REQUIRE(remainder != tokens.end());
            }
        }
    }

    GIVEN("Declaration, then single identifier")
    {
        std::string input = "var variable = 0; variable;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Identifier is parsed as expression")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 0);
                CHECK(result->to_infix_string() == "var variable = 0;\nvariable;\n");
                CHECK(result->to_prefix_string() == "(progn\n    (setq-new variable 0)\n    variable\n)");
            }
        }
    }

    GIVEN("Simple if statement without else")
    {
        std::string input = "if (1) 42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The if statement is parsed correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 42);
                CHECK(result->to_infix_string() == "if (1) {\n42;\n}\n");
                CHECK(result->to_prefix_string() == "(progn\n    (if (1) (progn\n42)\n)\n\n)");
            }
        }
    }

    GIVEN("If statement with else")
    {
        std::string input = "if (0) 42; else 24;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The if-else statement is parsed correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 24);
                CHECK(result->to_infix_string() == "if (0) {\n42;\n} else {\n24;\n}\n");
                CHECK(result->to_prefix_string() == "(progn\n    (if (0) (progn\n42)\n    (progn\n24))\n\n)");
            }
        }
    }

    GIVEN("If statement with true expression condition")
    {
        std::string input = "var x = 5; if (x - 4) x + 1; else x - 1;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The if statement evaluates condition correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                // x = 5, then if (5 > 3) which is if(2) -> truthy, so x + 1 = 6
                CHECK(eval.current_result_ == 6);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({0U, 6U}));
            }
        }
    }

    GIVEN("If statement with false expression condition")
    {
        std::string input = "var x = 2; if (x - 2) x + 10; else x * 2;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("The else branch is executed")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                // x = 2, then if (2 > 3) which is if(-1) -> falsy, so x * 2 = 4
                CHECK(eval.current_result_ == 4);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({0U, 4U}));
            }
        }
    }

    GIVEN(::std::string("   Given: Nested if statements"))
    {
        std::string input = "if (1) if (1) 42; else 0; else 24;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Nested if statements work correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                CHECK(result->evaluate() == 42);
            }
        }
    }

    GIVEN("If statement without else, false condition")
    {
        std::string input = "if (0) 42;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("False condition with no else branch does nothing")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // The if statement itself doesn't return a value when condition is false and no else
                CHECK(result->evaluate() == 0);
            }
        }
    }

    GIVEN("If statement with assignment in branches")
    {
        std::string input = "var x = 0; if (1) x = 10; else x = 20; x;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Assignment in if branch works")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                REQUIRE(eval.current_result_ == 10);
                using Catch::Matchers::RangeEquals;
                REQUIRE_THAT(results, RangeEquals({0U, 0U, 10U}));
            }
        }
    }

    GIVEN("If statement with complex boolean expressions")
    {
        std::string input = "var a = 1; var b = 0; if (a && b || a) 100; else 200;";
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Complex boolean condition is evaluated correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                // a = 1, b = 0, condition: (1 && 0) || 1 = 0 || 1 = 1 (true)
                CHECK(eval.current_result_ == 100);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({0U, 0U, 100U}));
            }
        }
    }

    GIVEN("Chained relational operators to test associativity")
    {
        ::std::string input =
            "1 < 2 < 3;"          // Should be (1 < 2) < 3 = 1 < 3 = 1
            "3 > 2 > 1;"          // Should be (3 > 2) > 1 = 1 > 1 = 0
            "1 <= 2 <= 2;"        // Should be (1 <= 2) <= 2 = 1 <= 2 = 1
            "5 >= 3 >= 1;"        // Should be (5 >= 3) >= 1 = 1 >= 1 = 1
            "2 = 2 = 1;"          // Should be (2 = 2) = 1 = 1 = 1 = 1
            "3 != 2 != 1;";       // Should be (3 != 2) != 1 = 1 != 1 = 0
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Chained relational operators are left-associative")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                CHECK(eval.current_result_ == 0);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({1U, 0U, 1U, 1U, 1U, 0U}));
                CHECK(result->to_infix_string() == "((1 < 2) < 3);\n((3 > 2) > 1);\n((1 <= 2) <= 2);\n((5 >= 3) >= 1);\n((2 = 2) = 1);\n((3 != 2) != 1);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (< (< 1 2) 3)\n    (> (> 3 2) 1)\n    (<= (<= 1 2) 2)\n    (>= (>= 5 3) 1)\n    (= (= 2 2) 1)\n    (!= (!= 3 2) 1)\n)");
            }
        }
    }

    GIVEN("Complex precedence mixing arithmetic, relational, and boolean operators")
    {
        ::std::string input =
            "2 + 3 * 4 > 10 && 1;"      // ((2 + (3 * 4)) > 10) && 1 = (14 > 10) && 1 = 1 && 1 = 1
            "5 - 2 <= 3 || 0;"          // ((5 - 2) <= 3) || 0 = (3 <= 3) || 0 = 1 || 0 = 1
            "6 / 2 = 3 && 4 * 2 != 7;"  // ((6 / 2) = 3) && ((4 * 2) != 7) = (3 = 3) && (8 != 7) = 1 && 1 = 1
            "1 + 2 > 2 * 1 && 3 - 1 < 4 || 0;"; // (((1 + 2) > (2 * 1)) && ((3 - 1) < 4)) || 0 = ((3 >= 2) && (2 < 4)) || 0 = (1 && 1) || 0 = 1 || 0 = 1
        auto tokens = tokenize_input(input.begin(), input.end());

        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Operator precedence is correctly handled across all operator types")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                CHECK(eval.current_result_ == 1);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({1U, 1U, 1U, 1U}));
                CHECK(result->to_infix_string() == "(((2 + (3 * 4)) > 10) && 1);\n(((5 - 2) <= 3) || 0);\n(((6 / 2) = 3) && ((4 * 2) != 7));\n((((1 + 2) > (2 * 1)) && ((3 - 1) < 4)) || 0);\n");
                CHECK(result->to_prefix_string() == "(progn\n    (&& (> (+ 2 (* 3 4)) 10) 1)\n    (|| (<= (- 5 2) 3) 0)\n    (&& (= (/ 6 2) 3) (!= (* 4 2) 7))\n    (|| (&& (> (+ 1 2) (* 2 1)) (< (- 3 1) 4)) 0)\n)");
            }
        }
    }

    GIVEN("A series of statements using relational operators")
    {
        ::std::string input =
            "5 < 2;"
            "2 < 2;"
            "2 < 5;"
            "5 <= 2;"
            "2 <= 2;"
            "2 <= 5;"
            "5 > 2;"
            "2 > 2;"
            "2 > 5;"
            "5 >= 2;"
            "2 >= 2;"
            "2 >= 5;"
            "5 = 2;"
            "2 = 2;"
            "5 != 2;"
            "2 != 2;";
        auto tokens = tokenize_input(input.begin(), input.end());
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Relational operations are evaluated correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                CHECK(eval.current_result_ == 0);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({
                    0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0
                }));
            }
        }
    }

    GIVEN("Some expressions mixing relational operators with other operators")
    {
        ::std::string input =
            "5 > 2 * 3;"
            "3 * 2 > 5;"
            "1 != 2 && 1 = 1;"
            "1 != 0 || 1 = 1;";
        auto tokens = tokenize_input(input.begin(), input.end());
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

            THEN("Relational operations are evaluated correctly")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                ::std::visit(eval, *result);
                CHECK(eval.current_result_ == 1);
                using Catch::Matchers::RangeEquals;
                CHECK_THAT(results, RangeEquals({
                    0, 1, 1, 1
                }));
            }
        }
    }

    GIVEN("Some input that has erroneously failed in the past.")
    {
        AND_GIVEN("A simple if/else that has previously failed.")
        {
            ::std::string input =
                "var a = 0;\n"
                "var b = 0;\n"
                "if (a < b) {\n"
                "5;\n"
                "} else {\n"
                "6;\n"
                "}\n";
            auto tokens = tokenize_input(input.begin(), input.end());
            REQUIRE(tokens.size() == 25);

            WHEN("It is parsed.")
            {
                auto [result, remainder] = parse_top(tokens.begin(), tokens.end());

                THEN("It should parse and have the appropriate output.")
                {
                    REQUIRE(result != nullptr);
                    REQUIRE(remainder == tokens.end());
                    Parser::SimpleEvaluator eval(save_result);
                    ::std::visit(eval, *result);
                    CHECK(eval.current_result_ == 6);
                }
            }
        }
    }

    GIVEN("A variable declaration with no initializer.") {
        ::std::string input = "var x;";
        auto tokens = tokenize_input(input.begin(), input.end());
        REQUIRE(tokens.size() == 3);

        WHEN("It is parsed.") {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            THEN("It shouldn't parse correctly.") {
                // TODO This is wrong, it should be the opposite, fix later
                CHECK(result != nullptr);
                REQUIRE(remainder != tokens.end());
            }
        }
    }

    GIVEN("A global variable and the variable assigned in a local scope") {
        ::std::string input = "var x = 10; { x = 20; } x;";
        auto tokens = tokenize_input(input.begin(), input.end());
        REQUIRE(tokens.size() == 13);

        WHEN("It is parsed and run.") {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            REQUIRE(result != nullptr);
            REQUIRE(remainder == tokens.end());
            THEN("Its value should change.") {
                Parser::SimpleEvaluator eval;
                ::std::visit(eval, *result);
                CHECK(eval.current_result_ == 20);
            }
        }
    }

    GIVEN("A global variable shadowed by a local variable.") {
        ::std::string input = "var x = 10; { var x = 20; x; } x;";
        auto tokens = tokenize_input(input.begin(), input.end());
        REQUIRE(tokens.size() == 16);

        WHEN("It is parsed and run.") {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            REQUIRE(result != nullptr);
            //CHECK(result->to_prefix_string() == "bad value\n");
            REQUIRE(remainder == tokens.end());
            THEN("The local variable should be used.") {
                using Catch::Matchers::RangeEquals;
                Parser::SimpleEvaluator eval{save_result};
                ::std::visit(eval, *result);
                CHECK_THAT(results, RangeEquals({
                    0U,  // var x = 10
                    0U,  // var x = 20
                    20U, // x
                    20U, // braced statement
                    10U  // final x
                }));
            }
        }
    }

    GIVEN("A loop with a variable declaration.") {
        ::std::string input =
            "var count = 3;\n"
            "while (count > 0) {\n"
            "    var x = 10 - count;\n"
            "    count = count - 1;\n"
            "    x;\n"
            "}\n";
        auto tokens = tokenize_input(input.begin(), input.end());
        REQUIRE(tokens.size() == 28);
        WHEN("It is parsed and run.") {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            REQUIRE(result != nullptr);
            REQUIRE(remainder == tokens.end());
            THEN("The sequence of values produced by statements should be correct.") {
                using Catch::Matchers::RangeEquals;
                Parser::SimpleEvaluator eval{save_result};
                ::std::visit(eval, *result);
                CHECK_THAT(results, RangeEquals({
                    0U, // var count = 3
                    0U, 0U, 7U,  // body of while loop
                    0U, 0U, 8U,  // body of while loop
                    0U, 0U, 9U,  // body of while loop
                    0U // while loop result
                }));
            }
        }
    }
}
