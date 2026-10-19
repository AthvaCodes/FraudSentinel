"""LIFO Stack implementation for Audit Trails and Investigator Action Rollbacks.

Role in Banking System:
- Tracks compliance officer actions (e.g., status overrides, rule changes, approval decisions).
- Implements immediate undo / rollback capabilities: the most recent action is at the TOP of the stack.
- Essential for multi-step audit tracing where investigators can step backwards through decisions.
- Tradeoff vs SQLite: LIFO stack allows immediate zero-overhead in-memory undo/redo, but must
  periodically flush committed audit checkpoints to SQLite ledger tables.
"""
from typing import Any, Dict, List, Optional


class AuditStack:
    """LIFO (Last-In-First-Out) Stack for analyst actions and rollback operations."""
    def __init__(self, capacity: Optional[int] = 100):
        self._items: List[Any] = []
        self.capacity: Optional[int] = capacity
        self.total_pushed: int = 0
        self.total_popped: int = 0

    def push(self, action: Any) -> bool:
        """Pushes an action onto the top of the stack. O(1) time complexity."""
        if self.capacity is not None and len(self._items) >= self.capacity:
            # Drop oldest or reject; here we reject to enforce stack bound
            return False
        self._items.append(action)
        self.total_pushed += 1
        return True

    def pop(self) -> Optional[Any]:
        """Pops and returns the action from the top of the stack. O(1) time complexity."""
        if self.is_empty():
            return None
        self.total_popped += 1
        return self._items.pop()

    def peek(self) -> Optional[Any]:
        """Inspects the top of the stack without removing it. O(1) time complexity."""
        if self.is_empty():
            return None
        return self._items[-1]

    def is_empty(self) -> bool:
        return len(self._items) == 0

    def size(self) -> int:
        return len(self._items)

    def clear(self):
        self._items.clear()

    def to_list(self) -> List[Any]:
        """Returns items from top (most recent) to bottom (oldest)."""
        return list(reversed(self._items))

    def to_visual_representation(self) -> Dict[str, Any]:
        """Returns visual snapshot of the stack from top to bottom."""
        items_visual = []
        n = len(self._items)
        for i in range(n - 1, -1, -1):
            items_visual.append({
                "index_from_top": n - 1 - i,
                "is_top": (i == n - 1),
                "data": self._items[i]
            })

        return {
            "size": len(self._items),
            "capacity": self.capacity,
            "total_pushed": self.total_pushed,
            "total_popped": self.total_popped,
            "top": self.peek(),
            "items": items_visual
        }
