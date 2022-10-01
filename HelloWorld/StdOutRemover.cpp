// cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCT_Clang_INSTALL_DIR=/root/anaconda3/envs/llvm/lib/cmake/llvm ../HelloWorld
#include <iostream>

#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/AST/Expr.h>

using namespace clang;
using namespace ast_matchers;

//-----------------------------------------------------------------------------
// ASTMatcher callback
//-----------------------------------------------------------------------------
class StdOutMatcher : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  StdOutMatcher(clang::Rewriter &LACRewriter) : LACRewriter(LACRewriter) {}
  void
  run(const clang::ast_matchers::MatchFinder::MatchResult &result) override {
    std::cerr << "MATCHED!\n";
    auto call = result.Nodes.getNodeAs<CXXOperatorCallExpr>("root");
    call->dump();
    return;
  }
  // Callback that's executed at the end of the translation unit
  void onEndOfTranslationUnit() override {
    LACRewriter.getEditBuffer(LACRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

private:
  clang::Rewriter LACRewriter;
  // llvm::SmallSet<clang::FullSourceLoc, 8> EditedLocations;
};

//-----------------------------------------------------------------------------
// ASTConsumer
//-----------------------------------------------------------------------------
class StdOutASTConsumer : public clang::ASTConsumer {
public:
  StdOutASTConsumer(clang::Rewriter &R) : StdOutHandler(R) {
    // m  cxxOperatorCallExpr(hasOperatorName("<<"), hasDescendant( declRefExpr(to(varDecl(hasName("cout"))))))
    //auto m = cxxOperatorCallExpr(hasOperatorName("<<"));
    StatementMatcher m = cxxOperatorCallExpr(
        hasOperatorName("<<"),
        hasDescendant(declRefExpr(to(varDecl(hasName("cout")))))).bind("root");
    Finder.addMatcher(m, &StdOutHandler);
  }
  void HandleTranslationUnit(clang::ASTContext &Ctx) override {
    Finder.matchAST(Ctx);
  }

private:
  clang::ast_matchers::MatchFinder Finder;
  StdOutMatcher StdOutHandler;
};

//-----------------------------------------------------------------------------
// FrontendAction
//-----------------------------------------------------------------------------
class StdOutRemoverPluginAction : public clang::PluginASTAction {
public:
  // Our plugin can alter behavior based on the command line options
  bool ParseArgs(const clang::CompilerInstance &,
                 const std::vector<std::string> &) override {
    return true;
  }

  // Returns our ASTConsumer per translation unit.
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI,
                    llvm::StringRef file) override {
    RewriterForLAC.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<StdOutASTConsumer>(RewriterForLAC);
  }

private:
  clang::Rewriter RewriterForLAC;
};

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static clang::FrontendPluginRegistry::Add<StdOutRemoverPluginAction>
    X(/*Name=*/"STDOR",
      /*Desc=*/"Stdout Remover");
