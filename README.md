# 🛡️ FraudSentinel — Bank Fraud Transaction System
### Complete Educational Prototype Web Application & Data Structures Lab

> **⚠️ EDUCATIONAL PROTOTYPE NOTICE & DISCLAIMER**  
> This software is an **educational prototype** developed for teaching data structures, algorithmic fraud detection, and multi-language system integration (C99 core + Python/Flask + Modern Web). It is **not** a validated banking platform or real-world certified fraud prevention system. Real-world financial fraud detection involves production AML/KYC pipelines, machine learning ensembles, distributed event streams (e.g. Apache Kafka/Flink), and formal compliance verification.

---

## 🌟 Executive Overview

**FraudSentinel** reimagines and expands a foundational C banking terminal project into a production-grade, coherent educational product. 

- **Core Engine & Data Structures:** Written purely in **C99** (`c_src/`), compiled with GCC (`bin/c_engine.exe` and standalone `bin/bank_cli.exe`).
- **Web Layer & Persistence:** Python Flask (`app.py`), bridging directly to the native C binary for all data structures and scoring, while maintaining **SQLite** (`data/bank_fraud.db`) as the durable relational source of truth.
- **Front-End & Visualizations:** Native **HTML5, CSS3, and JavaScript** with an interactive SVG transfer network graph, live memory diagram visualizers, and an interactive **Data Structures Lab**.

---

## 🏛️ System Architecture

```
                    ┌────────────────────────────────────────────────────────┐
                    │               Modern Web Front-End                     │
                    │   HTML5 / CSS3 (Fintech Theme) / Vanilla JavaScript   │
                    │  - Operations Dashboard & SVG Analytics Charts         │
                    │  - Transaction CRUD, Live Filters, Sort & CSV Export   │
                    │  - Transfer Network Graph (Canvas Physics & Cycles)    │
                    │  - Compliance Review Queue with LIFO Action Undo       │
                    │  - Printable Audit & Executive Fraud Reports           │
                    │  - Interactive Data Structures Lab (7 Visualizers)     │
                    └───────────────────────────┬────────────────────────────┘
                                                │ REST / JSON APIs
                                                ▼
                    ┌────────────────────────────────────────────────────────┐
                    │            Python Flask Application Layer              │
                    │                  (Thin API Gateway)                    │
                    │   - Blueprint Route Controllers (`routes/`)           │
                    │   - Input Validation & Parameter Sanitization          │
                    │   - Relational Database Mirroring (`database.py`)      │
                    │   - Python Subprocess C Bridge (`c_bridge.py`)        │
                    └───────────┬────────────────────────────────┬───────────┘
                                │ Sync                           │ IPC / JSON
                                ▼                                ▼
    ┌──────────────────────────────────────┐  ┌──────────────────────────────────────┐
    │     SQLite Relational Database       │  │          Native C99 Engine           │
    │     (Durable Source of Truth)        │  │          (`bin/c_engine.exe`)        │
    │  - `transactions` Table              │  │  - Graph (Adjacency List & Cycles)   │
    │  - `accounts` Table                  │  │  - Linked List (`FlagNode` Prepend)  │
    │  - `audit_log` Table                 │  │  - FIFO Review Queue                 │
    │  - `rules_config` Table              │  │  - LIFO Audit Stack (Undo Support)   │
    │  - Complex SQL Filters & Aggregates  │  │  - Binary Search Tree (Range Query)  │
    └──────────────────────────────────────┘  │  - Hash Map (Separate Chaining)      │
                                              │  - Bubble & Merge Sort, Binary Search│
                                              │  - Rule Scoring & Threshold Tuning   │
                                              └──────────────────────────────────────┘
```

---

## 🧬 C Code Mapping & Evolution

This system directly adopts and enhances the original C console project:

| Original C Concept | Original C Code | Enhanced Implementation in this System |
|---|---|---|
| **Transaction Structure** | `typedef struct { int id; char sender[30]; ... } Transaction;` | Expanded `Transaction` struct with timestamp, category, risk score, risk level, review status, and analyst notes in `c_src/transaction.h`. |
| **Flagged Linked List** | `typedef struct FlagNode { Transaction transaction; ... } FlagNode;` | Complete singly linked list with O(1) prepend, append, search, delete, and pointer memory trace (`0xADDR`) in `c_src/linked_list.c`. |
| **Large Transaction Rule** | `if (t.amount >= LARGE_AMOUNT)` (&ge; ₹10,000) | Preserved & configurable (+2 risk points). Triggers automatic linked list insertion in `c_src/fraud_rules.c`. |
| **High Activity Rule** | `count >= HIGH_ACTIVITY` (&ge; 5 outgoing transfers) | Preserved & configurable (+2 risk points). Rapidly resolved via C Hash Map. |
| **Fraud Risk Scoring** | `calculateFraudScore()` (Score &ge; 4: HIGH, &ge; 2: MEDIUM, else: LOW) | Evaluated on every transfer; dynamic re-scoring with configurable thresholds in `c_src/fraud_rules.c`. |
| **Sorting Algorithm** | `sortTransactions()` (Bubble Sort) | Instrumented with comparison and swap trace steps in `c_src/algorithms.c`; displayed alongside Merge Sort in the CS Lab. |
| **File Persistence** | `saveTransactions()`, `loadTransactions()` | Fully preserved text serialization (`data/transactions.txt`), complemented by durable SQLite mirror (`data/bank_fraud.db`). |
| **Interactive Terminal Menu** | 13-item `main()` menu loop | Preserved in `bin/bank_cli.exe` and `c_engine.exe --cli`, plus expanded options for graph inspection and stack undo! |

---

## 🧪 Meaningful Data Structures in C & Their Real-World Roles

The application implements 7 data structures and algorithms in **native C99**:

### 1. Singly Linked List (`FlagNode`)
- **Role:** In-memory alert stream where new violations are instantly prepended at the head (`flaggedHead = newNode;`).
- **Complexity:** Prepend: $O(1)$, Traverse: $O(n)$, Search: $O(n)$, Space: $O(n)$.
- **Role vs. DB:** Instant zero-allocation prepend for alert streams, but lacks random indexing and range searching.

### 2. Directed Adjacency List Graph (`TransactionGraph`)
- **Role:** Models accounts as vertices and directed money transfers as edges.
- **Algorithms:**
  - **Breadth-First Search (BFS):** Traversal and fewest-hops shortest path finding between suspect and target accounts.
  - **Depth-First Search (DFS):** Reachability exploration.
  - **Cycle Detection:** Identifies circular fund routing characteristic of smurfing rings (e.g. `Mule Account A -> Mule Account B -> Mule Account C -> Mule Account A`).
- **Complexity:** Add Edge: $O(1)$, BFS/DFS: $O(V + E)$, Cycle Detection: $O(V \times E)$.

### 3. FIFO Review Queue (`FIFOReviewQueue`)
- **Role:** Compliance alert triage pipeline.
- **Complexity:** Enqueue: $O(1)$, Dequeue: $O(1)$, Peek: $O(1)$, Space: $O(n)$.
- **Role vs. DB:** Enforces fair First-In, First-Out processing order; guarantees older alerts are not starved during peak transaction hours.

### 4. LIFO Audit Stack (`AuditStack`)
- **Role:** Tracks compliance officer overrides (Investigate, Clear, Block) and powers the one-click **Undo Action** rollback.
- **Complexity:** Push: $O(1)$, Pop: $O(1)$, Peek: $O(1)$, Space: $O(n)$.
- **Role vs. DB:** Zero-overhead in-memory rollback stack; periodic checkpoints are flushed to the SQLite audit log.

### 5. Binary Search Tree (`TransactionBST`)
- **Role:** Hierarchical index of transactions keyed by amount.
- **Features:** Range queries (e.g. "Find all transfers between ₹5,000 and ₹25,000") in $O(h + k)$ time. In-order traversal yields sorted transactions in $O(n)$ time.
- **Complexity:** Insert: $O(\log n)$ avg, Range Query: $O(h + k)$, In-Order: $O(n)$.

### 6. Separate Chaining Hash Map (`HashMap`)
- **Role:** Computes outgoing account frequencies in $O(1)$ average time to evaluate Rule 2 (5+ outgoing transfers).
- **Features:** DJB2 polynomial rolling hash algorithm ($\text{hash} \times 33 + c$) with modulo 16 buckets. Tracks bucket distribution and collisions.
- **Complexity:** Put: $O(1)$ avg, Get: $O(1)$ avg, Worst: $O(n)$ under pathological collisions.

### 7. Sorting & Searching Algorithms
- **Bubble Sort:** Preserved from original C code; instrumented with pass, comparison, and swap step counters.
- **Merge Sort:** Efficient divide-and-conquer $O(n \log n)$ benchmark for normal production use.
- **Binary Search:** $O(\log n)$ halving search on sorted amounts vs. $O(n)$ Linear Search.

---

## 🚀 Quick Start Guide

### Prerequisites
- **Python:** 3.10+
- **C Compiler:** GCC (MinGW or Linux GCC)
- **Pip Packages:** Flask, Pytest

### 1. Build Native C Binaries
From the project root:
```powershell
gcc -Wall -Wextra -std=c99 c_src/transaction.c c_src/fraud_rules.c c_src/linked_list.c c_src/graph.c c_src/queue.c c_src/stack.c c_src/bst.c c_src/hash_map.c c_src/algorithms.c c_src/c_engine.c -o bin/c_engine.exe -lm
gcc -Wall -Wextra -std=c99 c_src/transaction.c c_src/fraud_rules.c c_src/linked_list.c c_src/graph.c c_src/queue.c c_src/stack.c c_src/bst.c c_src/hash_map.c c_src/algorithms.c c_src/c_engine.c -o bin/bank_cli.exe -lm
```

### 2. Launch the Web Application
```powershell
python app.py
```
Open your browser and navigate to:
```
http://127.0.0.1:5000
```

### 3. Run the Standalone Interactive C Console
You can also run the original C console menu directly in your terminal:
```powershell
.\bin\bank_cli.exe
```

### 4. Run Automated Tests
Execute the full test suite covering the C engine, algorithms, data structures, and web workflows:
```powershell
pytest -v tests
```

---

## 🧭 Key User Features & Walkthrough

1. **Fintech Dashboard (`/`):**
   - KPI cards for Total Processed Volume (₹), Flagged Violations, Pending Reviews, and Average Ticket Size.
   - Live native SVG Donut Chart showing Low/Medium/High risk distributions.
   - Quick Transfer Simulator for testing immediate rule evaluations.
   - High-activity account watchlist.

2. **Transaction Ledger (`/transactions`):**
   - Paginated table with live search across accounts, categories, and IDs.
   - Filters for Risk Level (All, Low, Medium, High), Review Status, and Min/Max Amount.
   - Sortable columns (ID, Sender, Receiver, Amount, Risk Score).
   - Instant **CSV Export** button.
   - Modal transaction forensics inspector and deletion with confirmation.

3. **Customer Account Profiles (`/accounts`):**
   - Complete directory of bank accounts with balances and transaction counts.
   - Detailed dossier view (`/accounts/<name>`) displaying inflow/outflow balance cards, counterparty network, and split transaction history.

4. **Transfer Network Visualizer (`/network`):**
   - HTML5 Canvas interactive graph with force-directed physics.
   - Account nodes sized by transaction volume and colored by risk level.
   - **"Highlight Smurfing Cycles"** button that illuminates circular money laundering loops in glowing red!
   - Shortest Path Forensics tool using Breadth-First Search (BFS).

5. **Compliance Review Queue (`/review`):**
   - Status tabs: Pending, Investigating, Blocked, Cleared.
   - One-click investigator triage actions.
   - **"Undo Last Action"** button powered by the C LIFO Audit Stack.

6. **Executive Fraud Reports (`/reports`):**
   - Matches and extends C console reports (`detectLargeTransactions()`, `detectHighActivity()`, `transactionStatistics()`).
   - Clean `@media print` print styling with official audit watermark and **Print Formal Audit PDF** button.

7. **Configurable Risk Model (`/rules`):**
   - Adjust Large Amount threshold (default ₹10,000) and High Activity threshold (default 5).
   - Adjust penalty weights and risk level cutoffs.
   - One-click ledger re-evaluation.

8. **Interactive Data Structures Lab (`/ds-lab`):**
   - 7 interactive tabs showcasing live memory pointers, queue tracks, stack frames, BST range queries, hash buckets, and Bubble vs. Merge sort comparisons!

---

## 📁 Repository Directory Structure

```
bank-fraud-detection-system/
├── app.py                     # Flask application runner & blueprint registry
├── c_bridge.py                # Python-to-C subprocess JSON IPC bridge
├── database.py                # SQLite persistence and synchronization
├── bin/
│   ├── c_engine.exe           # Compiled C backend engine
│   └── bank_cli.exe           # Standalone interactive C console program
├── c_src/                     # Native C99 core engine source code
│   ├── common.h               # Limits, constants, and JSON escaping utilities
│   ├── transaction.h / .c     # Transaction models, seeding, stats
│   ├── fraud_rules.h / .c     # Configurable rule evaluation engine
│   ├── linked_list.h / .c     # Flagged linked list (`FlagNode`)
│   ├── graph.h / .c           # Adjacency list graph, BFS, DFS, cycles
│   ├── queue.h / .c           # FIFO review queue
│   ├── stack.h / .c           # LIFO audit stack (Undo support)
│   ├── bst.h / .c             # Binary search tree (amount indexing)
│   ├── hash_map.h / .c        # Separate chaining hash map
│   ├── algorithms.h / .c      # Bubble sort trace, merge sort, binary search
│   └── c_engine.c             # Master C CLI & JSON endpoint dispatcher
├── data/
│   ├── bank_fraud.db          # SQLite durable relational database
│   ├── transactions.txt       # C file storage ledger
│   └── rules_config.json      # Configurable threshold settings
├── routes/                    # Flask route blueprints
│   ├── dashboard_routes.py    # Executive metrics & charts
│   ├── transaction_routes.py  # CRUD, filtering, sorting, CSV export
│   ├── account_routes.py      # Account directory & customer dossier
│   ├── network_routes.py      # Transfer graph and cycle detection APIs
│   ├── review_routes.py       # Review queue & audit undo
│   ├── report_routes.py       # Printable fraud detection report
│   ├── rules_routes.py        # Configurable risk model settings
│   ├── ds_lab_routes.py       # Data structures lab APIs
│   └── seed_routes.py         # Sample data reset trigger
├── static/
│   ├── css/
│   │   ├── styles.css         # Modern fintech dark slate design system
│   │   └── print.css          # Printable audit report stylesheet
│   └── js/
│       ├── main.js            # Modal handlers & toast notifications
│       ├── charts.js          # Native dependency-free SVG chart engine
│       ├── network_graph.js   # HTML5 canvas interactive network visualizer
│       └── ds_lab.js          # Interactive data structures lab controllers
├── templates/                 # Jinja2 HTML5 responsive templates
│   ├── base.html              # Layout, navigation, prototype badge, modals
│   ├── dashboard.html         # Executive overview
│   ├── transactions/index.html# Ledger with filters & pagination
│   ├── accounts/              # Directory & detail dossiers
│   ├── network/index.html     # Full-screen transfer network graph
│   ├── review/index.html      # Compliance review queue & audit stack
│   ├── reports/index.html     # Executive & printable fraud report
│   ├── rules/index.html       # Rule configuration form
│   └── ds_lab/index.html      # 7-tab CS Data Structures Lab
├── tests/                     # Automated test suite (Pytest)
│   ├── test_c_engine.py       # C binary unit & algorithmic tests
│   └── test_web_routes.py     # Flask route & end-to-end user tests
└── README.md                  # System documentation & architectural guide
```

---

## 🏆 Project Achievements

- **True Multi-Language Architecture:** Business logic and data structures run in pure compiled **C99**, with a responsive, modern **HTML/CSS/JS** front end and **Flask/SQLite** backend.
- **Algorithmic Depth:** Includes working implementations of Graphs with Cycle Detection, Singly Linked Lists, Queues, Stacks, BSTs, Hash Tables, Bubble Sort, Merge Sort, and Binary Search.
- **Fintech Realism:** Features realistic Indian account identities, Rupee (₹) amounts, compliance review workflows, printable audit reports, and circular smurfing detection.
- **Zero Broken Dependencies:** Charts and network visualizers are written in pure SVG and vanilla Canvas—guaranteeing 100% offline functionality without external CDN dependencies.
