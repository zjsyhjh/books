- 红黑树定义：

  - 节点是红色或者黑色
  - 根是黑色
  - 所有叶子都是黑色（叶子是NIL节点）
  - 每个红色节点必须有两个黑色的子节点
  - 从任一节点到每个叶子的所有简单路径都包含相同数目的黑色节点

- 以上的约束保证了红黑树的关键特性：从根到叶子的最长的可能路径不多于最短的可能路径的两倍长

- 红黑树的另一种等价定义是：

  - 该树是二叉树
  - 红链接都是左链接
  - 没有任何一个节点同时和两条红链接相连
  - 该树是完美黑色平衡的：即任意空链接到根节点的路径上的黑链接数量相同

- 红黑树和2-3树对应：如果将一颗红黑树的红链接画平，那么所有的空链接到根节点的距离都将是相同的

- 如何使用左旋、右旋以及颜色转换来保证插入操作后红黑树和2-3树之间的对应关系

  - 在沿着插入点到根节点的路径向上移动时在所经过的每个节点中顺序完成以下操作，我们就能完成插入操作

    - 如果右子节点是红色的而左子节点为黑色，那就进行左旋

    - 如果左子节点是红色的并且它的左子节点也是红色的，那就进行右旋

    - 如果左右子节点都是红色的，进行颜色转换

    - ```java
      if (isRed(h.right) && !isRed(h.left)) h = rotateLeft(h);
      if (isRed(h.left) && isRed(h.left.left)) h = rotateRigth(h);
      if (isRead(h.left) && isRed(h.right)) flipColors(h);
      ```



- 删除最小键
  - 注意到从树底的3- 节点中删除键是很简单的，但2- 节点则不然，从2- 节点中删除一个键会留下一个空节点，一般我们会将它替换为一个空链接，但这样会破话树的平衡性。所以我们需要这样做：为了保证不会删除一个2- 节点，我们沿着左链接向下变换，确保当前节点不是2- 节点（可能是3- 节点，也可能是临时的4- 节点）。首先根节点有两种情况。如果根节点是2- 节点并且它的两个子节点都是2- 节点，我们可以直接把这三个节点变成4- 节点；否则我们需要保证根节点的左子节点不是2- 节点，如有必要可以从它的右侧的兄弟节点“借”一个键来，在沿着左链接向下的过程中，保证以下情况成立：
    - 如果当前节点的左子节点不是2- 节点，完成
    - 如果当前节点的左子节点是2- 节点而它的亲兄弟节点不是2- 节点，将左子节点的兄弟节点中的一个键移动到左子节点中
    - 如果当前节点的左子节点和它的亲兄弟节点都是2- 节点，将左子节点、父节点中的最小键以及左子节点最近的兄弟节点合并为4- 节点，使父节点为由3- 节点变为2-节点或者由4-节点变为3-节点