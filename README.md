# FraudSentinel 🛡️

A bank fraud detection prototype and data structures visualizer built with a native C backend and a Flask web dashboard.

I took a classic C terminal banking project and rebuilt it into a full-stack educational app. All the core data structures, algorithms, and fraud scoring rules run in compiled C99, while Flask, SQLite, and vanilla JS handle the web UI, charts, and interactive visualizers.

---

## What It Does

- **Rule-Based Fraud Detection:** Flags transactions over ₹10,000 (+2 points), detects rapid bursts from high-activity accounts with 5+ outgoing transfers (+2 points), and calculates risk levels (Low, Medium, High). Rules and thresholds can be tuned live from the UI.
- **Money Flow & Laundering Cycles:** Interactive canvas network graph mapping transfers between accounts. Detects circular routing rings (e.g., A → B → C → A) using depth-first search in C.
- **Review Queue & Undo Stack:** Compliance triage desk for flagged transfers. Uses an in-memory LIFO stack in C so investigators can undo previous status overrides.
- **Data Structures Lab:** An interactive playground with visualizers and step-by-step traces for:
  - **Singly Linked List:** In-memory alert log with raw memory pointers (`0xADDR`) and $O(1)$ prepending.
  - **Directed Graph:** Adjacency list with BFS shortest-path queries and cycle detection.
  - **FIFO Queue:** Fair arrival-order transaction triage.
  - **LIFO Stack:** Action audit history and rollbacks.
  - **Binary Search Tree:** Numeric amount indexing, $O(n)$ in-order sorted traversal, and range queries.
  - **Hash Map:** `djb2` rolling hash with separate chaining for $O(1)$ account activity tracking.
  - **Sorting & Searching:** Step-by-step Bubble Sort (with pass/swap counters) vs. Merge Sort and Binary Search.
- **Ledger & Reports:** Search, multi-criteria filtering, column sorting, pagination, customer dossiers, CSV export, and printable audit reports.
- **AI Account Analysis PDF:** Generate a professional AI-written report focused on a customer's account activity, counterparties, and risk indicators.

To enable AI account reports, set `GEMINI_API_KEY` in the server environment. The optional `GEMINI_MODEL` variable selects the model (default: `gemini-3.8-flash`). The report sends account-level transaction records to the Gemini API; keep the key out of source control. PDF generation uses the Python standard library and requires no extra package.

---

## Tech Stack

- **Core Engine & Data Structures:** C99 (`gcc`)
- **Web Backend:** Python 3, Flask, SQLite
- **Front-End:** HTML5, CSS3 (dark fintech theme), Vanilla JavaScript (Canvas & SVG charts, zero external CDNs)

---


---

> *Note: This is an educational prototype built for demonstrating data structures and algorithmic detection concepts, not for actual banking or production financial systems.*
