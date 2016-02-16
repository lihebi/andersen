#include "Andersen.hpp"
#include <set>
using namespace llvm;

/**
 * This is the LLVM Pass to realize Andersen style points-to analysis.
 * This Pass is a function pass.
 * Which means we only handle intra-procedure points-to.
 * 
 * The algorithm:
 * 
 * Init graph and points to sets using base and simple constrints
 * Let W = {v | pts(v) \ne \empty}
 * While W not empty
 *   v <- select from W
 *   for each a \in pts(v) do
 *     for each constraint p \subseteq *v
 *       add edge a->p
 *
 *     
 */

/**
 * 1. collect constraints
 * 2. init points-to set
 * 2. build graph
 * 3. transitive closure
 */

void Constraint::dump() {
  switch (kind) {
  case CK_Simple:
    errs() << lhs->getName() << " = " << rhs->getName() << "\n"; break;
  case CK_Base:
    errs() << lhs->getName() << " = &" << rhs->getName() << "\n"; break;
  case CK_Complex_RStar:
    errs() << lhs->getName() << " = *" << rhs->getName() << "\n"; break;
  case CK_Complex_LStar:
    errs() << "*" << lhs->getName() << " = " << rhs->getName() << "\n"; break;
  default: break;
  }
  // Type *ty;
  // ty = dyn_cast<AllocaInst>(lhs)->getAllocatedType();
  // if (ty->isPointerTy()) {
  //   errs() << "pointer: ";
  // }
  // errs() << *ty << "\n";
  // ty = dyn_cast<AllocaInst>(rhs)->getAllocatedType();
  // if (ty->isPointerTy()) {
  //   errs() << "pointer:";
  // }
  // errs() << *ty << "\n";
  // errs() << *lhs << "\n";
  // errs() << *rhs << "\n";
}

/**
 * Init graph with base and simple constraints
 */
void AndersenGraph::Init() {
  // get all pointer nodes
  Value *alloca;
  for (Constraint &c : m_cons) {
    alloca = c.getLeft();
    if (dyn_cast<AllocaInst>(alloca)->getType()->isPointerTy()) {
      m_nodes.insert(alloca);
    }
  }
  // init all points-to set for these nodes, by base a=&b
  // create edge by simple a=b
  for (Constraint c : m_cons) {
    switch (c.getKind()) {
    case CK_Simple: {
      // a=b
      addEdge(c.getRight(), c.getLeft());
      break;
    }
    case CK_Base: {
      // a=&b
      addPts(c.getLeft(), c.getRight());
      break;
    }
      /**
       * the following construct the constraints with their pointer.
       */
    case CK_Complex_RStar: {
      // a=*b
      // only add for b
      addComplexRConstraint(c.getLeft(), c.getRight());
      break;
    }
    case CK_Complex_LStar: {
      // *a=b
      // only add for a
      addComplexLConstraint(c.getLeft(), c.getRight());
      break;
    }
    default: break;
    }
  }
}

/**
 * lhs=*rhs
 * Add for rhs
 */
void AndersenGraph::addComplexRConstraint(Value* lhs, Value* rhs) {
  if (m_cr_cons.find(rhs) == m_cr_cons.end()) m_cr_cons[rhs] = std::set<Value*>();
  m_cr_cons[rhs].insert(lhs);
}
/**
 * *lhs=rhs
 */
void AndersenGraph::addComplexLConstraint(Value* lhs, Value* rhs) {
  if (m_cl_cons.find(lhs) == m_cl_cons.end()) m_cl_cons[lhs] = std::set<Value*>();
  m_cl_cons[lhs].insert(rhs);
}

/**
 * Add rhs into pts set of lhs. lhs=&rhs
*/
void AndersenGraph::addPts(Value* lhs, Value* rhs) {
  if (m_pts.find(lhs) == m_pts.end()) m_pts[lhs] = std::set<Value*>();
  m_pts[lhs].insert(rhs);
}

/**
 * Add an edge from lhs to rhs.
 * Record in both m_in_edge and m_out_edge.
 * @return true if edge is new
 */
bool AndersenGraph::addEdge(Value* lhs, Value* rhs) {
  if (m_out_edge.count(lhs) == 0) m_out_edge[lhs] = std::set<Value*>();
  if (m_in_edge.count(rhs) == 0) m_in_edge[rhs] = std::set<Value*>();
  if (m_out_edge[lhs].count(rhs) == 1) return false;
  m_out_edge[lhs].insert(rhs);
  m_in_edge[rhs].insert(lhs);
  return true;
}

void AndersenGraph::Solve() {
  std::set<Value*> worklist;
  for (Value *vp : m_nodes) {
    if (m_pts.find(vp) != m_pts.end()) worklist.insert(vp);
  }
  while (!worklist.empty()) {
    Value *v = *worklist.begin();
    worklist.erase(v);
    // step 1
    // for every items in points-to set of v
    for (Value *a : m_pts[v]) {
      if (m_cr_cons.find(v) != m_cr_cons.end()) {
        for (Value *p : m_cr_cons[v]) {
          // add edge a->p, and add a to worklist if the edge is new
          if (addEdge(a, p)) worklist.insert(a);
        }
      }
      if (m_cl_cons.count(v) == 1) {
        for (Value *q : m_cl_cons[v]) {
          // add edge q->a, and add q to worklist if the edge is new.
          if (addEdge(q, a)) worklist.insert(q);
        }
      }
    }
    // step 2
    // for every v->q
    for (Value *q : m_out_edge[v]) {
      // merge pts[v] into pts[q], and add q to worklist if pts[q] changed.
      if (mergePts(q, v)) worklist.insert(q);
    }
  }
}

/**
 * pts[to] = pts[to] U pts[from].
 * @return true if pts[to] changed.
 */
bool AndersenGraph::mergePts(Value* to, Value* from) {
  // errs() << "merge:" << to->getName() << " from " << from->getName() << "\n";
  size_t size = m_pts[to].size();
  
  m_pts[to].insert(m_pts[from].begin(), m_pts[from].end());
  if (size != m_pts[to].size()) return true;
  else return false;
}
void AndersenGraph::DumpConstraints() {
  errs() << "Constraints:\n";
  for (Constraint &c : m_cons) {
    errs() << "\t";
    c.dump();
  }
}

void AndersenGraph::Dump() {
  /**
   * (filename, line, var, (pts1, pts2, ...))
   * e.g. (a.c, 100, v, (a, b, c))
   */
  for (Value* v : m_nodes) {
    StringRef filename;
    unsigned line;
    if (m_dbg.count(v) == 0) {
      // continue;
      filename="NA";
      line=0;
    } else {
      // file name
      filename = m_dbg[v]->getFilename();
      // line number
      line = m_dbg[v]->getLine();
    }
    if (m_pts[v].size() > 0) {
      FILE *fp = fopen("output.txt", "a");
      fprintf(fp, "%s:%u %s ===> {", filename.str().c_str(), line, v->getName().str().c_str());
      for (Value *a : m_pts[v]) {
        fprintf(fp, "%s, ", a->getName().str().c_str());
      }
      fprintf(fp, "}\n");
      fclose(fp);
    }
  }
}
void AndersenGraph::DumpDebug() {
  errs() << "pts:\n";

  /**
   * (filename, line, var, (pts1, pts2, ...))
   * e.g. (a.c, 100, v, (a, b, c))
   */
  for (Value* v : m_nodes) {
    StringRef filename;
    unsigned line;
    if (m_dbg.count(v) == 0) {
      // continue;
      filename="NA";
      line=0;
    } else {
      // file name
      filename = m_dbg[v]->getFilename();
      // line number
      line = m_dbg[v]->getLine();
    }
    errs() << "\t(" << filename << ":" << line << " " << v->getName() << ":";
    for (Value *a : m_pts[v]) {
      errs() << a->getName() << ", ";
      // getchar();
    }
    errs() << ")\n";
  }
  // errs() << "======= Edges ======\n";
  errs() << "edges:\n";
  for (Value *v : m_nodes) {
    for (Value *p : m_out_edge[v]) {
      errs() << "\t" << v->getName() << " -> " << p->getName() << "\n";
    }
  }
}


class AndersenPass : public FunctionPass {
public:
  static char ID;
  AndersenPass() : FunctionPass(ID) {}
  bool runOnFunction(Function &f) override {
    // errs() << "-------- function: " << f.getName() << "\n";
    // alias_analysis(f);
    // return false;
    std::vector<Constraint> cons;
    std::map<Value*, DILocation*> dbg;
    collect_constraints(f, cons, dbg);
    AndersenGraph graph(cons, dbg);
    // graph.DumpConstraints();
    graph.Init();
    graph.Solve();
    graph.Dump();
    return false;
  }
protected:
private:
};

char AndersenPass::ID = 0;

static RegisterPass<AndersenPass> X("andersen", "Andersen Pass", false, false);

/*******************************
 ** Helper functions
 *******************************/


/**
 * Follow v to the alloca object.
 * record level accordingly.
 */
Value* resolve(Value*v, int &lvl) {
  // errs() << *v << "\n";
  Instruction *i = dyn_cast<Instruction>(v);
  // assert(i && "Not instruction?");
  if (!i) {
    // immediate address, ignore
    // errs() << "ignored: " << *v << "\n";
    return nullptr;
  }
  if (i->isCast()) {
    // CastInst *c = dyn_cast<CastInst>(i);
    // no way to get operand of a CastInst!
    // this will cause trouble for int **c=b, which is a "bitcastinst"
    return nullptr;
  }
  switch (i->getOpcode()) {
  case Instruction::Alloca: {
    return i;
  }
  case Instruction::Load: {
    LoadInst *l = dyn_cast<LoadInst>(i);
    lvl++;
    return resolve(l->getPointerOperand(), lvl);
  }
  default: {
    // errs() << *i << "\n";
    // errs() << i->getOpcodeName() << "\n";
    // assert(false);
    // HEBI ASSERT
    return nullptr;
  }
  }
  // if (AllocaInst *v = dyn_cast<AllocaInst>(value)) {
  // } else if (LoadInst *v = dyn_cast<LoadInst>(value)) {
  //   Value *tmp = value;
  //   // resolve until alloca
  //   while (!isa<AllocaInst>(tmp)) {
  //     if (LoadInst *l = dyn_cast<LoadInst>(tmp)) {
  //       errs() << *tmp << "\n";
  //       tmp = l->getPointerOperand();
  //     } else if ()
  //       }
  //   errs() << v->getName() << "\n";
  // }
}

void alias_analysis(Function &f) {
  std::vector<Value*> allocs;
  for (inst_iterator itr = inst_begin(f), ite = inst_end(f);
       itr != ite; ++itr) {
    auto inst = itr.getInstructionIterator();
    if (isa<AllocaInst>(inst)) {
      AllocaInst *alloc = dyn_cast<AllocaInst>(inst);
      if (alloc->getType()->isPointerTy()) {
        allocs.push_back(alloc);
      }
    }
  }
  // output allocs
  errs() << "pointers:";
  for (Value *v : allocs) {
    errs() << v->getName() << " ";
  }
  errs() << "\n";
    
  // call alias interface
  AliasAnalysis aa;
  for (size_t i=0;i<allocs.size();i++) {
    for (size_t j=i+1;j<allocs.size();j++) {
      AliasResult r = aa.alias(allocs[i], allocs[j]);
      // NoAlias, MayAlias, PartialAlias, MustAlias
      if (r == PartialAlias || r == MustAlias) {
        errs() << allocs[i]->getName() << " <---> " << allocs[j]->getName() << "\n";
      }
    }
  }

}

void collect_constraints(
                         Function &f, std::vector<Constraint> &cons,
                         std::map<Value*, DILocation*> &dbg
                         ) {
  for (inst_iterator itr = inst_begin(f), ite = inst_end(f);
       itr != ite; ++itr) {
    
    auto inst = itr.getInstructionIterator();
    // errs() <<"\t\t"<< *inst << "\n";
    // construct alloca debug loc
    if (isa<DbgDeclareInst>(inst)) {
      if (DILocation *Loc = inst->getDebugLoc()) {
        // unsigned Line = Loc->getLine();
        // StringRef File = Loc->getFilename();
        // StringRef Dir = Loc->getDirectory();
        Value *var = dyn_cast<DbgDeclareInst>(inst)->getAddress();
        // errs() << var->getName() << " " << File << ":" << Line << "\n";
        dbg[var] = Loc;
      }
    }

    // we only care about store, that is assignment
    if (StoreInst *in = dyn_cast<StoreInst>(inst)) {
      // errs() << "store!" << *in << "\n";
      Value *value = in->getValueOperand();
      Value *pointer = in->getPointerOperand();
      // errs() << *in << "\n";
      int value_lvl = 0;
      int pointer_lvl = 0;
      // errs() << "value: " << *value << "\n";
      // errs() << "pointer: " << *pointer << "\n";
      value = resolve(value, value_lvl);
      pointer = resolve(pointer, pointer_lvl);
      if (!value || !pointer) {
        continue;
      }
      // errs() << value->getName() << " " << value_lvl<<"\n";
      // errs() << pointer->getName() << " " << pointer_lvl << "\n";
      
      // errs() << pointer->getName() << "="<<value->getName()<<"\n";
      // errs() << value_lvl << pointer_lvl << "\n";

      ConstraintKind kind;
      if (value_lvl == 1 && pointer_lvl == 1) {
        kind = CK_Complex_LStar; // *a=b
      } else if (value_lvl == 0 && pointer_lvl == 0) {
        kind = CK_Base; // a = &b;
      } else if (value_lvl == 1 && pointer_lvl == 0) {
        kind = CK_Simple; // p = q;
      } else if (value_lvl == 2 && pointer_lvl == 0) {
        kind = CK_Complex_RStar;
      } else {
        continue;
        // assert(false && "other situation?");
      }
      // char *s=0;
      if (value->getName().empty()) continue;
      // errs() << "constructing constraint\n";
      Constraint c {pointer, value, kind};
      // c.dump();
      cons.push_back(c);
    } else {
      // TODO other statements
      // errs() << *inst << "\n";
      // errs() << inst->getOpcodeName() << "\n";
    }
  }
  // for (Function::iterator it=f.begin(),end=f.end(); it!=end; it++) {
  //   for (BasicBlock::iterator jt=it->begin(), jend=it->end(); jt!=jend; jt++) {

  //   }
  // }
}
