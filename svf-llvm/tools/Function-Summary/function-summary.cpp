#include "AE/Core/AbstractState.h"
#include "Graphs/SVFG.h"
#include "SVF-LLVM/LLVMUtil.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/CommandLine.h"
#include "Util/Options.h"
#include "WPA/Andersen.h"

using namespace llvm;
using namespace std;
using namespace SVF;

std::set<PTACallGraphNode*> leaf_cgn;
std::set<PTACallGraphNode*> loop_cgn;




// Topological sort on VFG
void traverseOnCallGraph(PTACallGraph * cg) 
{
    // create leaf_cgn set
    long unsigned int leaf_cgnCount = 0;
    do {
        leaf_cgnCount = leaf_cgn.size();

        for(auto it = cg->begin(); it != cg->end(); it++)
        {
            PTACallGraphNode* node = it->second;
            bool outs_all_in_leaf = true;
            for (auto out_it = node->OutEdgeBegin(); out_it != node->OutEdgeEnd(); out_it++)
            {
                auto edge = *out_it;
                if (leaf_cgn.find(edge->getDstNode()) == leaf_cgn.end())
                {
                    outs_all_in_leaf = false;
                    break;
                }
            }
            if (outs_all_in_leaf)
            {
                leaf_cgn.insert(node);
            }
            bool ins_all_in_leaf = true;
            for (auto in_it = node->InEdgeBegin(); in_it != node->InEdgeEnd(); out_it++)
            {
                auto edge = *in_it;
                if (leaf_cgn.find(edge->getSrcNode()) == leaf_cgn.end())
                {
                    ins_all_in_leaf = false;
                    break;
                }
            }
            if (ins_all_in_leaf)
            {
                leaf_cgn.insert(node);
            }
            // TODO: debug
            llvm::errs() << node->getName() << "\n";
            for (auto out_it = node->OutEdgeBegin(); out_it != node->OutEdgeEnd(); out_it++)
            {
                auto edge = *out_it;
                llvm::errs() << "  " << edge->getDstNode()->getName() << "\n";
            }
        }
    } while(leaf_cgnCount != leaf_cgn.size());

    for (auto it = cg->begin(); it != cg->end(); it++)
    {
        PTACallGraphNode* node = it->second;
        if (leaf_cgn.find(node) == leaf_cgn.end())
        {
            loop_cgn.insert(node);
        }
    }
}

// TODO : debug
void printSet(std::set<PTACallGraphNode*> s)
{
    for (auto it = s.begin(); it != s.end(); it++)
    {
        llvm::errs() << (*it)->getName() << "\n";
    }
}


int main(int argc, char ** argv)
{
    std::vector<std::string> moduleNameVec;
    moduleNameVec = OptionBase::parseOptions(
                        argc, argv, "Whole Program Points-to Analysis", "[options] <input-bitcode...>"
                    );

    if (Options::WriteAnder() == "ir_annotator")
    {
        LLVMModuleSet::preProcessBCs(moduleNameVec);
    }

    SVFModule* svfModule = LLVMModuleSet::buildSVFModule(moduleNameVec);

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder(svfModule);
    SVFIR* pag = builder.build();

    /// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Call Graph
    PTACallGraph* callgraph = ander->getCallGraph();

    traverseOnCallGraph(callgraph);

    // TODO: debug
    llvm::errs() << "Leaf CGN\n";
    printSet(leaf_cgn);
    llvm::errs() << "Loop CGN\n";
    printSet(loop_cgn);

    

    return 0;
}