#ifndef ANDERSEN_H
#define ANDERSEN_H
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <set>

using namespace llvm; // I know this is bad.

typedef enum {
  CK_Simple,                    // a=b
  CK_Base,                      // a=&b
  CK_Complex_RStar,             // a=*b
  CK_Complex_LStar              // *a=b
} ConstraintKind;

class Constraint {
public:
  Value* getLeft() {return lhs;}
  Value* getRight() {return rhs;}
  ConstraintKind getKind() {return kind;}
  Constraint(Value *l, Value *r, ConstraintKind k) : lhs(l), rhs(r), kind(k) {}
  void dump();
private:
  /**
   * assert: lhs and rhs are both AllocaInst
   */
  Value *lhs;
  Value *rhs;
  ConstraintKind kind;
};

void alias_analysis(Function &f);
Value* resolve(Value*v, int &lvl);
void collect_constraints(
                         Function &f,
                         std::vector<Constraint> &cons,
                         std::map<Value*, DILocation*> &dbg
                         );

class AndersenNode {
};

class AndersenEdge {
};

class AndersenGraph {
public:
  AndersenGraph(
                std::vector<Constraint> cons,
                std::map<Value*, DILocation*> dbg)
    : m_cons(cons), m_dbg(dbg) {}
  void Init();
  void Solve();
  void DumpConstraints();
  void Dump();
  void DumpDebug();
private:
  bool addEdge(Value* lhs, Value* rhs);
  void addPts(Value* lhs, Value* rhs);
  bool mergePts(Value* to, Value* from);
  void addComplexRConstraint(Value* lhs, Value* rhs);
  void addComplexLConstraint(Value* lhs, Value* rhs);
  std::vector<Constraint> m_cons;
  std::map<Value*, DILocation*> m_dbg; // debug information for all allocas
  std::set<Value*> m_nodes;
  std::map<Value*, std::set<Value*> > m_pts;
  std::map<Value*, std::set<Value*> > m_out_edge;
  std::map<Value*, std::set<Value*> > m_in_edge;
  std::map<Value*, std::set<Value*> > m_cr_cons;
  std::map<Value*, std::set<Value*> > m_cl_cons;
  
};



#endif /* ANDERSEN_H */
