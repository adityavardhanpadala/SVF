//===- ICFG.h ----------------------------------------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2018>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * ICFG.h
 *
 *  Created on: 11 Sep. 2018
 *      Author: Yulei
 */

#ifndef INCLUDE_UTIL_ICFG_H_
#define INCLUDE_UTIL_ICFG_H_

#include "Graphs/ICFGNode.h"
#include "Graphs/ICFGEdge.h"
#include "Util/WorkList.h"
#include "MemoryModel/SVFLoop.h"

namespace SVF
{

class CallGraph;

/*!
 * Interprocedural Control-Flow Graph (ICFG)
 */
typedef GenericGraph<ICFGNode,ICFGEdge> GenericICFGTy;
class ICFG : public GenericICFGTy
{
    friend class ICFGBuilder;
    friend class SVFIRWriter;
    friend class SVFIRReader;
    friend class ICFGSimplification;

public:

    typedef OrderedMap<NodeID, ICFGNode *> ICFGNodeIDToNodeMapTy;
    typedef ICFGEdge::ICFGEdgeSetTy ICFGEdgeSetTy;
    typedef ICFGNodeIDToNodeMapTy::iterator iterator;
    typedef ICFGNodeIDToNodeMapTy::const_iterator const_iterator;

    typedef Map<const SVFFunction*, FunEntryICFGNode *> FunToFunEntryNodeMapTy;
    typedef Map<const SVFFunction*, FunExitICFGNode *> FunToFunExitNodeMapTy;
    typedef Map<const SVFInstruction*, CallICFGNode *> CSToCallNodeMapTy;
    typedef Map<const SVFInstruction*, RetICFGNode *> CSToRetNodeMapTy;
    typedef Map<const SVFInstruction*, IntraICFGNode *> InstToBlockNodeMapTy;
    typedef std::vector<const SVFLoop *> SVFLoopVec;
    typedef Map<const ICFGNode *, SVFLoopVec> ICFGNodeToSVFLoopVec;

    NodeID totalICFGNode;

private:
    FunToFunEntryNodeMapTy FunToFunEntryNodeMap; ///< map a function to its FunExitICFGNode
    FunToFunExitNodeMapTy FunToFunExitNodeMap; ///< map a function to its FunEntryICFGNode
    CSToCallNodeMapTy CSToCallNodeMap; ///< map a callsite to its CallICFGNode
    CSToRetNodeMapTy CSToRetNodeMap; ///< map a callsite to its RetICFGNode
    InstToBlockNodeMapTy InstToBlockNodeMap; ///< map a basic block to its ICFGNode
    GlobalICFGNode* globalBlockNode; ///< unique basic block for all globals
    ICFGNodeToSVFLoopVec icfgNodeToSVFLoopVec; ///< map ICFG node to the SVF loops where it resides

    Map<const ICFGNode*, std::vector<const ICFGNode*>> _subNodes; ///<map a node(1st node of basicblock) to its subnodes
    Map<const ICFGNode*, const ICFGNode*> _repNode; ///<map a subnode to its representative node(1st node of basicblock)


public:
    /// Constructor
    ICFG();

    /// Destructor
    ~ICFG() override;

    /// Get a ICFG node
    inline ICFGNode* getICFGNode(NodeID id) const
    {
        return getGNode(id);
    }

    /// Whether has the ICFGNode
    inline bool hasICFGNode(NodeID id) const
    {
        return hasGNode(id);
    }

    /// Whether we has a SVFG edge
    //@{
    ICFGEdge* hasIntraICFGEdge(ICFGNode* src, ICFGNode* dst, ICFGEdge::ICFGEdgeK kind);
    ICFGEdge* hasInterICFGEdge(ICFGNode* src, ICFGNode* dst, ICFGEdge::ICFGEdgeK kind);
    ICFGEdge* hasThreadICFGEdge(ICFGNode* src, ICFGNode* dst, ICFGEdge::ICFGEdgeK kind);
    //@}

    /// Get a SVFG edge according to src and dst
    ICFGEdge* getICFGEdge(const ICFGNode* src, const ICFGNode* dst, ICFGEdge::ICFGEdgeK kind);

    /// Dump graph into dot file
    void dump(const std::string& file, bool simple = false);

    /// View graph from the debugger
    void view();

    /// update ICFG for indirect calls
    void updateCallGraph(CallGraph* callgraph);

    /// Whether node is in a loop
    inline bool isInLoop(const ICFGNode *node)
    {
        auto it = icfgNodeToSVFLoopVec.find(node);
        return it != icfgNodeToSVFLoopVec.end();
    }

    /// Whether node is in a loop
    inline bool isInLoop(const SVFInstruction* inst)
    {
        return isInLoop(getICFGNode(inst));
    }

    /// Insert (node, loop) to icfgNodeToSVFLoopVec
    inline void addNodeToSVFLoop(const ICFGNode *node, const SVFLoop* loop)
    {
        icfgNodeToSVFLoopVec[node].push_back(loop);
    }

    /// Get loops where a node resides
    inline SVFLoopVec& getSVFLoops(const ICFGNode *node)
    {
        auto it = icfgNodeToSVFLoopVec.find(node);
        assert(it != icfgNodeToSVFLoopVec.end() && "node not in loop");
        return it->second;
    }

    inline const ICFGNodeToSVFLoopVec& getIcfgNodeToSVFLoopVec() const
    {
        return icfgNodeToSVFLoopVec;
    }

protected:
    /// Add intraprocedural and interprocedural control-flow edges.
    //@{
    ICFGEdge* addIntraEdge(ICFGNode* srcNode, ICFGNode* dstNode);
    ICFGEdge* addConditionalIntraEdge(ICFGNode* srcNode, ICFGNode* dstNode, const SVFValue* condition, s32_t branchCondVal);
    ICFGEdge* addCallEdge(ICFGNode* srcNode, ICFGNode* dstNode);
    ICFGEdge* addRetEdge(ICFGNode* srcNode, ICFGNode* dstNode);
    //@}
    /// Remove a ICFG edge
    inline void removeICFGEdge(ICFGEdge* edge)
    {
        edge->getDstNode()->removeIncomingEdge(edge);
        edge->getSrcNode()->removeOutgoingEdge(edge);
        delete edge;
    }

    /// Remove a ICFGNode
    inline void removeICFGNode(ICFGNode* node)
    {
        removeGNode(node);
    }

    /// sanitize Intra edges, verify that both nodes belong to the same function.
    inline void checkIntraEdgeParents(const ICFGNode *srcNode, const ICFGNode *dstNode)
    {
        const SVFFunction* srcfun = srcNode->getFun();
        const SVFFunction* dstfun = dstNode->getFun();
        if(srcfun != nullptr && dstfun != nullptr)
        {
            assert((srcfun == dstfun) && "src and dst nodes of an intra edge should in the same function!" );
        }
    }

    /// Add a ICFG node
    virtual inline void addICFGNode(ICFGNode* node)
    {
        addGNode(node->getId(),node);
        _repNode[node] = node;
        _subNodes[node].push_back(node);
    }

public:
    /// Get a basic block ICFGNode
    /// TODO:: need to fix the assertions
    //@{
    ICFGNode* getICFGNode(const SVFInstruction* inst);

    /// Whether has the ICFGNode
    bool hasICFGNode(const SVFInstruction* inst);

    CallICFGNode* getCallICFGNode(const SVFInstruction* inst);

    CallICFGNode* addCallICFGNode(const SVFInstruction* inst);

    RetICFGNode* getRetICFGNode(const SVFInstruction* inst);

    RetICFGNode* addRetICFGNode(const SVFInstruction* inst);

    IntraICFGNode* getIntraICFGNode(const SVFInstruction* inst);

    IntraICFGNode* addIntraICFGNode(const SVFInstruction* inst);

    FunEntryICFGNode* getFunEntryICFGNode(const SVFFunction*  fun);

    FunExitICFGNode* getFunExitICFGNode(const SVFFunction*  fun);

    inline GlobalICFGNode* getGlobalICFGNode() const
    {
        return globalBlockNode;
    }
    inline void addGlobalICFGNode()
    {
        globalBlockNode = new GlobalICFGNode(totalICFGNode++);
        addICFGNode(globalBlockNode);
    }

    const std::vector<const ICFGNode*>& getSubNodes(const ICFGNode* node) const
    {
        return _subNodes.at(node);
    }

    const ICFGNode* getRepNode(const ICFGNode* node) const
    {
        return _repNode.at(node);
    }


    void updateSubAndRep(const ICFGNode* rep, const ICFGNode* sub)
    {
        addSubNode(rep, sub);
        updateRepNode(rep, sub);
    }
    //@}

private:
    /// when ICFG is simplified, SubNode would merge repNode, then update the map
    void addSubNode(const ICFGNode* rep, const ICFGNode* sub)
    {
        std::vector<const ICFGNode*>& subNodes = _subNodes[sub];
        if(std::find(subNodes.begin(), subNodes.end(), rep) == subNodes.end())
        {
            subNodes.push_back(rep);
        }
    }

    /// when ICFG is simplified, some node would be removed, this map records the removed node to its rep node
    void updateRepNode(const ICFGNode* rep, const ICFGNode* sub)
    {
        _repNode[rep] = sub;
    }

    /// Add ICFG edge, only used by addIntraEdge, addCallEdge, addRetEdge etc.
    inline bool addICFGEdge(ICFGEdge* edge)
    {
        bool added1 = edge->getDstNode()->addIncomingEdge(edge);
        bool added2 = edge->getSrcNode()->addOutgoingEdge(edge);
        bool all_added = added1 && added2;
        assert(all_added && "ICFGEdge not added?");
        return all_added;
    }

    /// Get/Add IntraBlock ICFGNode
    inline IntraICFGNode* getIntraBlock(const SVFInstruction* inst)
    {
        InstToBlockNodeMapTy::const_iterator it = InstToBlockNodeMap.find(inst);
        if (it == InstToBlockNodeMap.end())
            return nullptr;
        return it->second;
    }
    inline IntraICFGNode* addIntraBlock(const SVFInstruction* inst)
    {
        IntraICFGNode* sNode = new IntraICFGNode(totalICFGNode++,inst);
        addICFGNode(sNode);
        InstToBlockNodeMap[inst] = sNode;
        return sNode;
    }

    /// Get/Add a function entry node
    inline FunEntryICFGNode* getFunEntryBlock(const SVFFunction* fun)
    {
        FunToFunEntryNodeMapTy::const_iterator it = FunToFunEntryNodeMap.find(fun);
        if (it == FunToFunEntryNodeMap.end())
            return nullptr;
        return it->second;
    }
    inline FunEntryICFGNode* addFunEntryBlock(const SVFFunction* fun)
    {
        FunEntryICFGNode* sNode = new FunEntryICFGNode(totalICFGNode++,fun);
        addICFGNode(sNode);
        FunToFunEntryNodeMap[fun] = sNode;
        return sNode;
    }

    /// Get/Add a function exit node
    inline FunExitICFGNode* getFunExitBlock(const SVFFunction* fun)
    {
        FunToFunExitNodeMapTy::const_iterator it = FunToFunExitNodeMap.find(fun);
        if (it == FunToFunExitNodeMap.end())
            return nullptr;
        return it->second;
    }
    inline FunExitICFGNode* addFunExitBlock(const SVFFunction* fun)
    {
        FunExitICFGNode* sNode = new FunExitICFGNode(totalICFGNode++, fun);
        addICFGNode(sNode);
        FunToFunExitNodeMap[fun] = sNode;
        return sNode;
    }

    /// Get/Add a call node
    inline CallICFGNode* addCallBlock(const SVFInstruction* cs)
    {
        CallICFGNode* sNode = new CallICFGNode(totalICFGNode++, cs);
        addICFGNode(sNode);
        CSToCallNodeMap[cs] = sNode;
        return sNode;
    }
    inline CallICFGNode* getCallBlock(const SVFInstruction* cs)
    {
        CSToCallNodeMapTy::const_iterator it = CSToCallNodeMap.find(cs);
        if (it == CSToCallNodeMap.end())
            return nullptr;
        return it->second;
    }

    /// Get/Add a return node
    inline RetICFGNode* getRetBlock(const SVFInstruction* cs)
    {
        CSToRetNodeMapTy::const_iterator it = CSToRetNodeMap.find(cs);
        if (it == CSToRetNodeMap.end())
            return nullptr;
        return it->second;
    }
    inline RetICFGNode* addRetBlock(const SVFInstruction* cs)
    {
        CallICFGNode* callBlockNode = getCallICFGNode(cs);
        RetICFGNode* sNode = new RetICFGNode(totalICFGNode++, cs, callBlockNode);
        callBlockNode->setRetICFGNode(sNode);
        addICFGNode(sNode);
        CSToRetNodeMap[cs] = sNode;
        return sNode;
    }

};

} // End namespace SVF

namespace SVF
{
/* !
 * GenericGraphTraits specializations for generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */
template<> struct GenericGraphTraits<SVF::ICFGNode*> : public GenericGraphTraits<SVF::GenericNode<SVF::ICFGNode,SVF::ICFGEdge>*  >
{
};

/// Inverse GenericGraphTraits specializations for call graph node, it is used for inverse traversal.
template<>
struct GenericGraphTraits<Inverse<SVF::ICFGNode *> > : public GenericGraphTraits<Inverse<SVF::GenericNode<SVF::ICFGNode,SVF::ICFGEdge>* > >
{
};

template<> struct GenericGraphTraits<SVF::ICFG*> : public GenericGraphTraits<SVF::GenericGraph<SVF::ICFGNode,SVF::ICFGEdge>* >
{
    typedef SVF::ICFGNode *NodeRef;
};

} // End namespace llvm

#endif /* INCLUDE_UTIL_ICFG_H_ */
