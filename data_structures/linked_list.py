"""Singly Linked List implementation for Flagged Transactions.

Directly evolves the C project's:
    typedef struct FlagNode {
        Transaction transaction;
        char reason[100];
        struct FlagNode *next;
    } FlagNode;
    FlagNode *flaggedHead = NULL;
"""
from typing import Optional, Dict, Any, List


class FlagNode:
    """Represents a single node in the flagged transactions linked list."""
    def __init__(self, transaction: Dict[str, Any], reason: str):
        self.transaction = transaction
        self.reason = reason
        self.next: Optional['FlagNode'] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "transaction": self.transaction,
            "reason": self.reason
        }


class FlaggedLinkedList:
    """Singly Linked List storing flagged transactions with O(1) prepend.
    
    Role in Banking System:
    - Originally used in the C system as an in-memory alert stream where new
      violations are instantly pushed to the head of the list (`flaggedHead`).
    - Best suited for streaming alerts where prepending is O(1) without array reallocations.
    - Tradeoff vs SQLite: Volatile in-memory storage, linear O(n) search time, no indexed queries.
    """
    def __init__(self):
        self.head: Optional[FlagNode] = None
        self._size: int = 0

    def prepend(self, transaction: Dict[str, Any], reason: str) -> FlagNode:
        """Adds a new flagged transaction to the head of the list. O(1) time complexity.
        Matches the original C logic:
            newNode->next = flaggedHead;
            flaggedHead = newNode;
        """
        new_node = FlagNode(transaction, reason)
        new_node.next = self.head
        self.head = new_node
        self._size += 1
        return new_node

    def append(self, transaction: Dict[str, Any], reason: str) -> FlagNode:
        """Adds a new flagged transaction to the end of the list. O(n) time complexity."""
        new_node = FlagNode(transaction, reason)
        if not self.head:
            self.head = new_node
        else:
            current = self.head
            while current.next:
                current = current.next
            current.next = new_node
        self._size += 1
        return new_node

    def delete_by_id(self, transaction_id: int) -> bool:
        """Deletes a node by its transaction ID. Returns True if deleted, False otherwise."""
        current = self.head
        prev = None

        while current:
            tx_id = current.transaction.get("id") if isinstance(current.transaction, dict) else getattr(current.transaction, "id", None)
            if tx_id == transaction_id:
                if prev is None:
                    self.head = current.next
                else:
                    prev.next = current.next
                self._size -= 1
                return True
            prev = current
            current = current.next

        return False

    def search_by_id(self, transaction_id: int) -> Optional[FlagNode]:
        """Searches for a flagged transaction by ID. O(n) time complexity."""
        current = self.head
        while current:
            tx_id = current.transaction.get("id") if isinstance(current.transaction, dict) else getattr(current.transaction, "id", None)
            if tx_id == transaction_id:
                return current
            current = current.next
        return None

    def clear(self):
        """Clears all nodes. In C this required explicit loop with free()."""
        self.head = None
        self._size = 0

    def __len__(self) -> int:
        return self._size

    def to_list(self) -> List[Dict[str, Any]]:
        """Converts linked list into a standard Python list of dicts."""
        result = []
        current = self.head
        while current:
            result.append({
                "transaction": current.transaction,
                "reason": current.reason
            })
            current = current.next
        return result

    def to_visual_representation(self) -> List[Dict[str, Any]]:
        """Returns node information structured for step-by-step visual rendering."""
        nodes = []
        current = self.head
        idx = 0
        while current:
            tx = current.transaction
            tx_id = tx.get("id") if isinstance(tx, dict) else getattr(tx, "id", "?")
            sender = tx.get("sender") if isinstance(tx, dict) else getattr(tx, "sender", "?")
            receiver = tx.get("receiver") if isinstance(tx, dict) else getattr(tx, "receiver", "?")
            amount = tx.get("amount") if isinstance(tx, dict) else getattr(tx, "amount", 0.0)

            nodes.append({
                "index": idx,
                "id": tx_id,
                "sender": sender,
                "receiver": receiver,
                "amount": float(amount),
                "reason": current.reason,
                "hasNext": current.next is not None,
                "memoryAddress": f"0x{id(current):x}"[-6:].upper()
            })
            current = current.next
            idx += 1
        return nodes
