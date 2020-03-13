#include "parser/quo_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/str_join.h"
#include "gtest/gtest.h"

using namespace std;
using namespace yy;

extern void yy_scan_string(const char*);

Expr* ParseExpr(const string& input) {
  ::parser_testing::ShouldOnlyParseExprForTest = true;
  ::parser_testing::ParsedExprForTest = nullptr;
  yy_scan_string(input.c_str());
  parser p;
  p.set_debug_level(1);
  const int parse_result = p.parse();
  EXPECT_EQ(parse_result, 0) << "Parsing failed for input: " << input;
  // Cannot use ASSERT_* in non-TEST function - see
  // https://stackoverflow.com/a/34699459
  assert(::parser_testing::ParsedExprForTest);
  return ::parser_testing::ParsedExprForTest;
}

TEST(ParserTest, IntLiteralTest) {
  Expr* e = ParseExpr("100");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "literal");
  ASSERT_NE(e->literal, nullptr);
  EXPECT_EQ(e->literal->type->value, "int");
  ASSERT_NE(e->literal->int_value, nullptr);
  EXPECT_EQ(e->literal->int_value->value, 100);
}

TEST(ParserTest, StringLiteralTest) {
  Expr* e = ParseExpr("\"hello world!\"");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "literal");
  ASSERT_NE(e->literal, nullptr);
  EXPECT_EQ(e->literal->type->value, "string");
  ASSERT_NE(e->literal->string_value, nullptr);
  EXPECT_EQ(e->literal->string_value->value, "hello world!");
}

TEST(ParserTest, MemberExprTestWithLiteral) {
  Expr* e = ParseExpr("100.toString");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "member");
  ASSERT_NE(e->member, nullptr);
  ASSERT_NE(e->member->parent_expr, nullptr);
  EXPECT_EQ(e->member->parent_expr->type->value, "literal");
  ASSERT_NE(e->member->parent_expr->literal, nullptr);
  ASSERT_NE(e->member->member_name, nullptr);
  EXPECT_EQ(e->member->member_name->value, "toString");
}

TEST(ParserTest, MemberExprTestWithVar) {
  Expr* e = ParseExpr("x.toString");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "member");
  ASSERT_NE(e->member, nullptr);
  ASSERT_NE(e->member->parent_expr, nullptr);
  EXPECT_EQ(e->member->parent_expr->type->value, "var");
  ASSERT_NE(e->member->parent_expr->var, nullptr);
  ASSERT_NE(e->member->parent_expr->var->name, nullptr);
  EXPECT_EQ(e->member->parent_expr->var->name->value, "x");
  ASSERT_NE(e->member->member_name, nullptr);
  EXPECT_EQ(e->member->member_name->value, "toString");
}

TEST(ParserTest, CallExprTest) {
  Expr* e = ParseExpr("100.toString()");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "call");
  ASSERT_NE(e->call, nullptr);
  ASSERT_NE(e->call->fn_expr, nullptr);
  EXPECT_EQ(e->call->fn_expr->type->value, "member");
  ASSERT_NE(e->call->arg_exprs, nullptr);
  EXPECT_EQ(e->call->arg_exprs->elements.size(), 0);
}

TEST(ParserTest, CallExprTestWithSingleArg) {
  const vector<string> inputs{
      "x.get(42)",
      "x.get(42,)",
  };
  for (const auto& input : inputs) {
    Expr* e = ParseExpr(input);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->type->value, "call");
    ASSERT_NE(e->call, nullptr);
    ASSERT_NE(e->call->fn_expr, nullptr);
    EXPECT_EQ(e->call->fn_expr->type->value, "member");
    ASSERT_NE(e->call->arg_exprs, nullptr);
    EXPECT_EQ(e->call->arg_exprs->elements.size(), 1);
    ASSERT_EQ(e->call->arg_exprs->elements[0]->type_info, &ExprTypeInfo);
    const Expr* arg_expr = static_cast<Expr*>(e->call->arg_exprs->elements[0]);
    EXPECT_EQ(arg_expr->type->value, "literal");
    EXPECT_EQ(arg_expr->literal->type->value, "int");
    ASSERT_NE(arg_expr->literal->int_value, nullptr);
    EXPECT_EQ(arg_expr->literal->int_value->value, 42);
  }
}

TEST(ParserTest, ArithBinaryOpExprTest) {
  Expr* e = ParseExpr("a + b * c - d / e");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "binaryOp");
  ASSERT_NE(e->binaryOp, nullptr);
  ASSERT_NE(e->binaryOp->op, nullptr);
  EXPECT_EQ(e->binaryOp->op->value, "sub");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

