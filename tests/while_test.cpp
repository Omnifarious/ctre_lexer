// Copyright 2025 by Eric Hopper
// See project LICENSE file for details

#include "../parser.hpp"
#include <catch2/catch_all.hpp>

SCENARIO("While loops and evaluator callback behavior")
{
    using Tokens::tokenize_input;
    using Parser::parse_top;

    std::vector<std::uintmax_t> results;
    auto save_result = [&results](std::uintmax_t r){ results.push_back(r); };

    GIVEN("A simple while loop that decrements to zero")
    {
        std::string input = "var x = 3; var y = 0; while (x) { x = x - 1; y = y + 1; } y;";
        auto tokens = tokenize_input(input.begin(), input.end());
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            THEN("The while loop executes until the condition becomes false")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                // Final expression evaluates to x, which should be 0 after the loop
                CHECK(result->evaluate() == 3);
                // Infix stringization of the whole sequence
                CHECK(result->to_infix_string() ==
                      "var x = 3;\n"
                      "var y = 0;\n"
                      "while (x) {\n"
                      "x = (x - 1);\n"
                      "y = (y + 1);\n"
                      ";\n"
                      "}\n"
                      ";\n"
                      "y;\n");
                // Prefix stringization of the whole sequence
                CHECK(result->to_prefix_string() ==
                      "(progn\n"
                      "    (setq-new x 3)\n"
                      "    (setq-new y 0)\n"
                      "    (while (x) (progn\n"
                      "(progn\n"
                      "    (setq x (- x 1))\n"
                      "    (setq y (+ y 1))\n"
                      ")))\n"
                      "\n"
                      "    y\n"
                      ")");
            }
        }
    }

    GIVEN("A while loop with initially false condition")
    {
        std::string input = "var x = 0; while (x) x = 1; x;";
        auto tokens = tokenize_input(input.begin(), input.end());
        WHEN("The tokens are parsed")
        {
            auto [result, remainder] = parse_top(tokens.begin(), tokens.end());
            THEN("The loop body is not executed and x remains 0")
            {
                REQUIRE(result != nullptr);
                REQUIRE(remainder == tokens.end());
                Parser::SimpleEvaluator eval(save_result);
                std::visit(eval, *result);
                CHECK(eval.current_result_ == 0);
                using Catch::Matchers::RangeEquals;
                // Statement results: assignment -> 0, while (not entered) -> 0, final x -> 0
                CHECK_THAT(results, RangeEquals({0U, 0U, 0U}));
            }
        }
    }
}
