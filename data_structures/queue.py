"""FIFO Queue implementation for Compliance Review and Transaction Ingestion.

Role in Banking System:
- Regulates incoming flagged transactions into an investigator's triage queue.
- Guarantees fairness: alerts are inspected in strict First-In, First-Out (FIFO) arrival order.
- Prevents starvation of older alerts and handles burst volumes during peak market hours.
- Tradeoff vs SQLite: Very fast O(1) in-memory enqueue/dequeue, but transient. SQLite is used
  as the durable store so unreviewed items survive system restarts.
"""
from typing import Any, Dict, List, Optional
from collections import deque


class FIFOReviewQueue:
    """FIFO (First-In-First-Out) Queue backed by a double-ended queue."""
    def __init__(self, capacity: Optional[int] = None):
        self._items: deque = deque()
        self.capacity: Optional[int] = capacity
        self.total_enqueued: int = 0
        self.total_dequeued: int = 0

    def enqueue(self, item: Any) -> bool:
        """Adds an item to the rear of the queue. O(1) time complexity."""
        if self.capacity is not None and len(self._items) >= self.capacity:
            return False  # Queue overflow
        self._items.append(item)
        self.total_enqueued += 1
        return True

    def dequeue(self) -> Optional[Any]:
        """Removes and returns the item from the front of the queue. O(1) time complexity."""
        if self.is_empty():
            return None
        self.total_dequeued += 1
        return self._items.popleft()

    def peek(self) -> Optional[Any]:
        """Returns the item at the front of the queue without removing it. O(1) time."""
        if self.is_empty():
            return None
        return self._items[0]

    def is_empty(self) -> bool:
        return len(self._items) == 0

    def size(self) -> int:
        return len(self._items)

    def clear(self):
        self._items.clear()

    def to_list(self) -> List[Any]:
        return list(self._items)

    def to_visual_representation(self) -> Dict[str, Any]:
        """Returns a snapshot of the queue formatted for interactive visual rendering."""
        items_list = []
        for i, it in enumerate(self._items):
            items_list.append({
                "position": i,
                "is_front": (i == 0),
                "is_rear": (i == len(self._items) - 1),
                "data": it
            })

        return {
            "size": len(self._items),
            "capacity": self.capacity,
            "total_enqueued": self.total_enqueued,
            "total_dequeued": self.total_dequeued,
            "items": items_list,
            "front": self.peek(),
            "rear": self._items[-1] if self._items else None
        }
