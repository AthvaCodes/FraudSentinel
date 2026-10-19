"""Data structures package for the Bank Fraud Detection System educational prototype."""
from .linked_list import FlaggedLinkedList, FlagNode
from .graph import TransactionGraph, GraphNode, GraphEdge
from .queue import FIFOReviewQueue
from .stack import AuditStack
from .bst import TransactionBST, BSTNode
from .hash_map import CustomHashMap
from .algorithms import (
    bubble_sort_with_trace,
    merge_sort_with_count,
    linear_search_with_trace,
    binary_search_with_trace
)

__all__ = [
    "FlaggedLinkedList",
    "FlagNode",
    "TransactionGraph",
    "GraphNode",
    "GraphEdge",
    "FIFOReviewQueue",
    "AuditStack",
    "TransactionBST",
    "BSTNode",
    "CustomHashMap",
    "bubble_sort_with_trace",
    "merge_sort_with_count",
    "linear_search_with_trace",
    "binary_search_with_trace",
]
