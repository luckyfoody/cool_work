class TreeNode{
 data:String;
 left:TreeNode;
 right:TreeNode;
init(d:String):TreeNode{
 {data<-d;
left<-new TreeNode;
right<-new TreeNode;
self;
}
};

set_left(l:TreeNode):
TreeNode{
left<-l};

set_right(r:TreeNode):
TreeNode{
right<-r};

get_data():String {data};
get_left():TreeNode{left};
get_right():TreeNode{right};
};

class BinaryTree{
root:TreeNode;
init(r:TreeNode):BinaryTree{
{
root<-r;
self;
}
};
in_order():Object{
let io:IO<-new IO in{
  io.out_string("ZXBL:");
  if not (isvoid root) then
    in_order_traversal(root,io)
  else
    io.out_string("Tree is empty\n")
fi;
io.out_string("\n");
}
};

in_order_traversal(node:TreeNode,io:IO):Object{
 if not (isvoid node) then {
  in_order_traversal(node.get_left(),io);
  io.out_string(node.get_data());
  io.out_string("");
  in_order_traversal(node.get_right(),io);
}else
  true
fi
};
};
class Main{
main():Object{
let
n1:TreeNode<-(new TreeNode).init("A"),
n2:TreeNode<-(new TreeNode).init("B"),
n3:TreeNode<-(new TreeNode).init("C"),
n4:TreeNode<-(new TreeNode).init("D"),
n5:TreeNode<-(new TreeNode).init("E"),
tree:BinaryTree
in{
n1.set_left(n2);
n1.set_right(n3);
n2.set_left(n4);
n2.set_right(n5);
tree<-(new BinaryTree).init(n1);
tree.in_order();
}
};
};








