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
  EXPECT_EQ(e->literal->type->value, "int");
  EXPECT_EQ(e->literal->int_value->value, 100);
}

TEST(ParserTest, StringLiteralTest) {
  Expr* e = ParseExpr("\"hello world!\"");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "literal");
  EXPECT_EQ(e->literal->type->value, "string");
  EXPECT_EQ(e->literal->string_value->value, "hello world!");
}

TEST(ParserTest, MemberExprTestWithLiteral) {
  Expr* e = ParseExpr("100.toString");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "member");
  EXPECT_EQ(e->member->parent_expr->type->value, "literal");
  EXPECT_EQ(e->member->member_name->value, "toString");
}

TEST(ParserTest, MemberExprTestWithVar) {
  Expr* e = ParseExpr("x.toString");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "member");
  EXPECT_EQ(e->member->parent_expr->type->value, "var");
  EXPECT_EQ(e->member->parent_expr->var->name->value, "x");
  EXPECT_EQ(e->member->member_name->value, "toString");
}

TEST(ParserTest, CallExprTest) {
  Expr* e = ParseExpr("100.toString()");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "call");
  EXPECT_EQ(e->call->fn_expr->type->value, "member");
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
    EXPECT_EQ(e->call->fn_expr->type->value, "member");
    EXPECT_EQ(e->call->arg_exprs->elements.size(), 1);
    ASSERT_EQ(e->call->arg_exprs->elements[0]->type_info, &ExprTypeInfo);
    const Expr* arg_expr = static_cast<Expr*>(e->call->arg_exprs->elements[0]);
    EXPECT_EQ(arg_expr->type->value, "literal");
    EXPECT_EQ(arg_expr->literal->type->value, "int");
    EXPECT_EQ(arg_expr->literal->int_value->value, 42);
  }
}

TEST(ParserTest, ArithBinaryOpExprTest) {
  Expr* e = ParseExpr("a + b * c - d % e / f");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->op->value, "sub");
  EXPECT_EQ(e->binaryOp->left_expr->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->left_expr->binaryOp->op->value, "add");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->op->value, "mul");
  EXPECT_EQ(e->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->right_expr->binaryOp->op->value, "div");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->left_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->left_expr->binaryOp->op->value, "mod");
}

TEST(ParserTest, ArithBinaryOpExprWithParensTest) {
  Expr* e = ParseExpr("(a + b * (c - d) % e) / f");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->op->value, "div");
  EXPECT_EQ(e->binaryOp->left_expr->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->left_expr->binaryOp->op->value, "add");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->op->value, "mod");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->left_expr->type
          ->value,
      "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->left_expr
          ->binaryOp->op->value,
      "mul");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->left_expr
          ->binaryOp->right_expr->type->value,
      "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->left_expr
          ->binaryOp->right_expr->binaryOp->op->value,
      "sub");
}

TEST(ParserTest, CompAndBoolBinaryOpExprTest) {
  Expr* e = ParseExpr("a + b > c && d == 3 || e && f <= g + h");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->op->value, "or");
  EXPECT_EQ(e->binaryOp->left_expr->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->left_expr->binaryOp->op->value, "and");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->left_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->left_expr->binaryOp->op->value, "gt");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->left_expr->binaryOp->left_expr->type
          ->value,
      "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->left_expr->binaryOp->left_expr->binaryOp
          ->op->value,
      "add");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->left_expr->binaryOp->right_expr->binaryOp->op->value, "eq");
  EXPECT_EQ(e->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(e->binaryOp->right_expr->binaryOp->op->value, "and");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->right_expr->type->value, "binaryOp");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->right_expr->binaryOp->op->value, "le");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->right_expr->binaryOp->right_expr->type
          ->value,
      "binaryOp");
  EXPECT_EQ(
      e->binaryOp->right_expr->binaryOp->right_expr->binaryOp->right_expr
          ->binaryOp->op->value,
      "add");
}

TEST(ParserTest, AssignExprTest) {
  Expr* e = ParseExpr("f(a).c = d || e");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "assign");
  EXPECT_EQ(e->assign->dest_expr->type->value, "member");
  EXPECT_EQ(e->assign->dest_expr->member->parent_expr->type->value, "call");
  EXPECT_EQ(e->assign->value_expr->type->value, "binaryOp");
  EXPECT_EQ(e->assign->value_expr->binaryOp->op->value, "or");
}

TEST(ParserTest, AssignExprAssociativityTest) {
  Expr* e = ParseExpr("a = f(b).c = d");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->type->value, "assign");
  EXPECT_EQ(e->assign->dest_expr->type->value, "var");
  EXPECT_EQ(e->assign->value_expr->type->value, "assign");
  EXPECT_EQ(e->assign->value_expr->assign->dest_expr->type->value, "member");
  EXPECT_EQ(e->assign->value_expr->assign->value_expr->type->value, "var");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

