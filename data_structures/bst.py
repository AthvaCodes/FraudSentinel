"""Binary Search Tree (BST) for Transaction Amount Indexing and Range Queries.

Role in Banking System:
- Provides hierarchical indexing by numeric values such as transaction amount.
- Enables efficient range queries (e.g., "Find all transactions between ₹10,000 and ₹50,000") in O(h + k).
- Demonstrates how in-order traversal of a BST produces a sorted stream of transactions in O(n) time.
- Tradeoff vs SQLite: SQLite uses B-Trees on disk for database indices. An in-memory BST illustrates
  the core branching concept, but requires self-balancing (AVL / Red-Black) to avoid O(n) degradation
  on pre-sorted inputs.
"""
from typing import Any, Dict, List, Optional


class BSTNode:
    """Node in a Binary Search Tree indexed by transaction amount."""
    def __init__(self, key: float, data: Any):
        self.key: float = float(key)
        self.items: List[Any] = [data] if data is not None else []
        self.left: Optional['BSTNode'] = None
        self.right: Optional['BSTNode'] = None

    def to_dict(self) -> Dict[str, Any]:
        """Hierarchical representation for visual tree rendering."""
        return {
            "key": self.key,
            "count": len(self.items),
            "items": self.items,
            "left": self.left.to_dict() if self.left else None,
            "right": self.right.to_dict() if self.right else None
        }


class TransactionBST:
    """Binary Search Tree indexing transactions by amount."""
    def __init__(self):
        self.root: Optional[BSTNode] = None
        self._size: int = 0

    def insert(self, transaction: Dict[str, Any], key_field: str = "amount") -> BSTNode:
        """Inserts a transaction into the BST. Average O(log n), Worst O(n)."""
        val = float(transaction.get(key_field, 0.0))

        if self.root is None:
            self.root = BSTNode(val, transaction)
            self._size += 1
            return self.root

        current = self.root
        while True:
            if abs(val - current.key) < 1e-6:
                # Key already exists: append to existing node's item bucket
                current.items.append(transaction)
                self._size += 1
                return current
            elif val < current.key:
                if current.left is None:
                    current.left = BSTNode(val, transaction)
                    self._size += 1
                    return current.left
                current = current.left
            else:
                if current.right is None:
                    current.right = BSTNode(val, transaction)
                    self._size += 1
                    return current.right
                current = current.right

    def search(self, key_value: float) -> Optional[List[Any]]:
        """Searches for transactions matching a specific amount. O(h) time."""
        val = float(key_value)
        current = self.root
        while current:
            if abs(val - current.key) < 1e-6:
                return current.items
            elif val < current.key:
                current = current.left
            else:
                current = current.right
        return None

    def find_min(self) -> Optional[float]:
        """Finds minimum transaction amount in the tree."""
        if not self.root:
            return None
        current = self.root
        while current.left:
            current = current.left
        return current.key

    def find_max(self) -> Optional[float]:
        """Finds maximum transaction amount in the tree."""
        if not self.root:
            return None
        current = self.root
        while current.right:
            current = current.right
        return current.key

    def in_order_traversal(self) -> List[Any]:
        """Returns all transactions sorted by amount in ascending order. O(n) time."""
        result: List[Any] = []

        def _traverse(node: Optional[BSTNode]):
            if not node:
                return
            _traverse(node.left)
            result.extend(node.items)
            _traverse(node.right)

        _traverse(self.root)
        return result

    def range_query(self, min_val: float, max_val: float) -> List[Any]:
        """Finds all transactions with min_val <= amount <= max_val. O(h + k) time."""
        result: List[Any] = []

        def _query(node: Optional[BSTNode]):
            if not node:
                return
            if min_val <= node.key:
                _query(node.left)
            if min_val <= node.key <= max_val:
                result.extend(node.items)
            if node.key <= max_val:
                _query(node.right)

        _query(self.root)
        return result

    def delete(self, key_value: float) -> bool:
        """Deletes a node by its key. Returns True if deleted."""
        val = float(key_value)

        def _delete_node(node: Optional[BSTNode], key: float) -> Tuple[Optional[BSTNode], bool]:
            if not node:
                return None, False

            deleted = False
            if key < node.key - 1e-6:
                node.left, deleted = _delete_node(node.left, key)
            elif key > node.key + 1e-6:
                node.right, deleted = _delete_node(node.right, key)
            else:
                deleted = True
                # Case 1: No children
                if not node.left and not node.right:
                    return None, True
                # Case 2: One child
                if not node.left:
                    return node.right, True
                if not node.right:
                    return node.left, True
                # Case 3: Two children - find min in right subtree (in-order successor)
                successor = node.right
                while successor.left:
                    successor = successor.left
                node.key = successor.key
                node.items = successor.items
                node.right, _ = _delete_node(node.right, successor.key)

            return node, deleted

        self.root, was_deleted = _delete_node(self.root, val)
        if was_deleted:
            self._size -= 1
        return was_deleted

    def to_tree_dict(self) -> Optional[Dict[str, Any]]:
        """Returns JSON tree structure for visual hierarchy diagram."""
        if not self.root:
            return None
        return self.root.to_dict()

    def get_height(self) -> int:
        def _h(node: Optional[BSTNode]) -> int:
            if not node:
                return 0
            return 1 + max(_h(node.left), _h(node.right))
        return _h(self.root)

    def size(self) -> int:
        return self._size
