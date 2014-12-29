/*
 *
 *
 *
 */

#ifndef __TREENODE_HPP__
#define __TREENODE_HPP__

#include <string>

using namespace std;

class TreeNode {

public:
    TreeNode(const string&, int count = 0); 
    ~TreeNode();
    
    TreeNode* left_node() const;
    TreeNode* right_node() const;

private:
    string     word;
    int        count;
    TreeNode*  left;
    TreeNode*  right;
};

inline TreeNode::TreeNode(const string& str, int n)
    : word(str), count(n)
{
}

inline TreeNode::~TreeNode()
{
}

inline TreeNode* TreeNode::left_node() const
{
    return left;
}

inline TreeNode* TreeNode::right_node() const
{
    return right;
}


#endif
