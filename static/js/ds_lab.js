/**
 * Data Structures Lab Interactive Visualizers & Step-by-Step Algorithms
 */

document.addEventListener('DOMContentLoaded', () => {
  setupDSTabs();
  loadLinkedListVisualizer();
  loadQueueVisualizer();
  loadStackVisualizer();
  loadBSTVisualizer();
  loadHashMapVisualizer();
  setupSortingAndSearchLab();
});

function setupDSTabs() {
  const tabs = document.querySelectorAll('.ds-tab-btn');
  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.ds-content-panel').forEach(p => p.classList.remove('active'));

      tab.classList.add('active');
      const targetPanel = document.getElementById(tab.dataset.target);
      if (targetPanel) {
        targetPanel.classList.add('active');
      }
    });
  });
}

/* ==========================================================================
   1. Linked List Visualizer (FlagNode in C)
   ========================================================================== */

function loadLinkedListVisualizer() {
  const container = document.getElementById('llVisualContainer');
  if (!container) return;

  fetch('/api/ds/linked-list')
    .then(r => r.json())
    .then(data => {
      document.getElementById('llTotalNodes').textContent = data.total_nodes || 0;
      document.getElementById('llHeadAddress').textContent = data.head_address || 'NULL';

      const nodes = data.nodes || [];
      if (nodes.length === 0) {
        container.innerHTML = '<div style="color:#64748b;padding:20px;">Flagged linked list is empty (flaggedHead == NULL).</div>';
        return;
      }

      let html = '<div class="memory-node-list">';
      nodes.forEach((n, idx) => {
        html += `
          <div class="mem-node">
            <div class="mem-header">
              <span>Node #${idx}</span>
              <span>${n.memory_address}</span>
            </div>
            <div class="mem-body">
              <div style="font-weight:700;color:#fff;margin-bottom:2px;">Tx #${n.transaction_id}</div>
              <div style="color:#94a3b8;font-size:11px;">${n.sender} → ${n.receiver}</div>
              <div style="color:#38bdf8;font-weight:600;margin-top:4px;">₹${n.amount.toLocaleString('en-IN', {minimumFractionDigits: 2})}</div>
              <div style="color:#f43f5e;font-size:10px;margin-top:4px;">${n.reason}</div>
            </div>
            <div class="mem-pointer">
              <span>next*</span>
              <span>${n.next_address ? n.next_address : 'NULL'}</span>
            </div>
          </div>
        `;
        if (n.next_address) {
          html += '<div class="mem-arrow">➔</div>';
        } else {
          html += '<div class="mem-arrow" style="color:#64748b;">➔ [NULL]</div>';
        }
      });
      html += '</div>';
      container.innerHTML = html;
    })
    .catch(err => console.error("Error loading linked list:", err));
}

/* ==========================================================================
   2. FIFO Review Queue Visualizer
   ========================================================================== */

function loadQueueVisualizer() {
  const container = document.getElementById('queueVisualContainer');
  if (!container) return;

  fetch('/api/ds/queue')
    .then(r => r.json())
    .then(data => {
      document.getElementById('queueSize').textContent = data.size || 0;
      document.getElementById('queueEnqueued').textContent = data.total_enqueued || 0;
      document.getElementById('queueDequeued').textContent = data.total_dequeued || 0;

      const items = data.items || [];
      if (items.length === 0) {
        container.innerHTML = '<div style="color:#64748b;padding:20px;">Review Queue is currently empty.</div>';
        return;
      }

      let html = '<div style="display:flex;gap:12px;align-items:center;overflow-x:auto;padding:16px 0;">';
      html += '<div style="font-size:11px;font-weight:700;color:#10b981;border:1px dashed #10b981;padding:6px;border-radius:4px;">FRONT (Next to Review)</div>';

      items.forEach((it, idx) => {
        const tx = it.transaction || {};
        html += `
          <div style="background:#1e293b;border:1px solid #334155;border-radius:8px;padding:10px 14px;min-width:160px;">
            <div style="font-size:11px;color:#94a3b8;display:flex;justify-content:space-between;">
              <span>Pos ${idx}</span>
              <span class="badge-risk badge-risk-${(tx.risk_level || 'LOW').toLowerCase()}">${tx.risk_level || 'LOW'}</span>
            </div>
            <div style="font-weight:700;color:#fff;margin:4px 0;">Tx #${tx.id}</div>
            <div style="font-size:11px;color:#cbd5e1;">₹${(tx.amount || 0).toLocaleString('en-IN', {minimumFractionDigits: 0})}</div>
          </div>
        `;
        if (idx < items.length - 1) html += '<span style="color:#64748b;">➔</span>';
      });

      html += '<div style="font-size:11px;font-weight:700;color:#3b82f6;border:1px dashed #3b82f6;padding:6px;border-radius:4px;">REAR (Latest Arrival)</div>';
      html += '</div>';
      container.innerHTML = html;
    })
    .catch(err => console.error("Error loading queue:", err));
}

/* ==========================================================================
   3. LIFO Audit Stack Visualizer
   ========================================================================== */

function loadStackVisualizer() {
  const container = document.getElementById('stackVisualContainer');
  if (!container) return;

  fetch('/api/ds/stack')
    .then(r => r.json())
    .then(data => {
      document.getElementById('stackSize').textContent = data.size || 0;
      document.getElementById('stackPushed').textContent = data.total_pushed || 0;
      document.getElementById('stackPopped').textContent = data.total_popped || 0;

      const items = data.items || [];
      if (items.length === 0) {
        container.innerHTML = '<div style="color:#64748b;padding:20px;">Audit stack is empty. No recent manual actions to undo.</div>';
        return;
      }

      let html = '<div style="display:flex;flex-direction:column;gap:10px;max-width:480px;">';
      items.forEach((it) => {
        html += `
          <div style="background:#1e293b;border:1px solid ${it.is_top ? '#3b82f6' : '#334155'};border-radius:8px;padding:12px 16px;box-shadow:var(--shadow-sm);position:relative;">
            ${it.is_top ? '<span style="position:absolute;right:12px;top:10px;background:#3b82f6;color:#fff;font-size:10px;font-weight:700;padding:2px 6px;border-radius:4px;">TOP OF STACK</span>' : ''}
            <div style="font-weight:700;color:#fff;font-size:13px;">${it.action_type}: Tx #${it.transaction_id}</div>
            <div style="font-size:12px;color:#94a3b8;margin-top:2px;">
              Status: <span style="color:#f43f5e;">${it.old_status}</span> ➔ <span style="color:#10b981;">${it.new_status}</span>
            </div>
            <div style="font-size:11px;color:#64748b;margin-top:4px;">${it.timestamp} ${it.details ? '| ' + it.details : ''}</div>
          </div>
        `;
      });
      html += '</div>';
      container.innerHTML = html;
    })
    .catch(err => console.error("Error loading stack:", err));
}

/* ==========================================================================
   4. Binary Search Tree (BST) Visualizer
   ========================================================================== */

function loadBSTVisualizer() {
  const container = document.getElementById('bstVisualContainer');
  if (!container) return;

  fetch('/api/ds/bst')
    .then(r => r.json())
    .then(data => {
      document.getElementById('bstNodesCount').textContent = data.total_nodes || 0;
      document.getElementById('bstMinAmount').textContent = data.min_amount ? `₹${data.min_amount.toLocaleString('en-IN')}` : '₹0';
      document.getElementById('bstMaxAmount').textContent = data.max_amount ? `₹${data.max_amount.toLocaleString('en-IN')}` : '₹0';

      if (!data.root) {
        container.innerHTML = '<div style="color:#64748b;padding:20px;">BST is empty.</div>';
        return;
      }

      container.innerHTML = `<div style="overflow-x:auto;padding:16px 0;">${renderBSTNodeHTML(data.root)}</div>`;
    })
    .catch(err => console.error("Error loading BST:", err));
}

function renderBSTNodeHTML(node) {
  if (!node) return '<span style="color:#475569;font-size:11px;">[empty]</span>';

  let html = `
    <div style="display:inline-flex;flex-direction:column;align-items:center;margin:6px 12px;">
      <div style="background:#0f172a;border:2px solid #3b82f6;border-radius:8px;padding:8px 12px;text-align:center;box-shadow:var(--shadow-sm);min-width:100px;">
        <div style="font-weight:700;color:#38bdf8;font-size:13px;">₹${node.amount.toLocaleString('en-IN')}</div>
        <div style="font-size:10px;color:#94a3b8;">${node.count} transaction${node.count > 1 ? 's' : ''}</div>
      </div>
  `;

  if (node.left || node.right) {
    html += `
      <div style="display:flex;gap:16px;margin-top:12px;position:relative;">
        <div style="text-align:center;">
          <div style="font-size:10px;color:#64748b;margin-bottom:2px;">L (&lt;)</div>
          ${renderBSTNodeHTML(node.left)}
        </div>
        <div style="text-align:center;">
          <div style="font-size:10px;color:#64748b;margin-bottom:2px;">R (&gt;)</div>
          ${renderBSTNodeHTML(node.right)}
        </div>
      </div>
    `;
  }

  html += '</div>';
  return html;
}

function executeBSTRangeQuery() {
  const minVal = document.getElementById('bstRangeMin').value || 5000;
  const maxVal = document.getElementById('bstRangeMax').value || 25000;
  const resultDiv = document.getElementById('bstRangeResults');

  resultDiv.innerHTML = '<div style="color:#94a3b8;">Querying BST in O(h + k) time...</div>';

  fetch(`/api/ds/bst/range?min=${minVal}&max=${maxVal}`)
    .then(r => r.json())
    .then(data => {
      const txs = Array.isArray(data) ? data : [];
      if (txs.length === 0) {
        resultDiv.innerHTML = `<div style="color:#94a3b8;">No transactions found between ₹${minVal} and ₹${maxVal}.</div>`;
        return;
      }
      let html = `<div style="font-weight:600;color:#fff;margin-bottom:8px;">Found ${txs.length} transactions in range [₹${minVal} - ₹${maxVal}]:</div>`;
      html += '<div style="display:flex;gap:10px;flex-wrap:wrap;">';
      txs.forEach(t => {
        html += `
          <div style="background:#0f172a;border:1px solid #334155;border-radius:6px;padding:6px 10px;font-size:12px;">
            <strong>Tx #${t.id}</strong>: ${t.sender} ➔ ${t.receiver} (<strong>₹${t.amount.toLocaleString('en-IN')}</strong>)
          </div>
        `;
      });
      html += '</div>';
      resultDiv.innerHTML = html;
    })
    .catch(err => {
      console.error(err);
      resultDiv.innerHTML = '<div style="color:#f43f5e;">Error executing range query.</div>';
    });
}

/* ==========================================================================
   5. Hash Map Visualizer (Separate Chaining)
   ========================================================================== */

function loadHashMapVisualizer() {
  const container = document.getElementById('hashMapVisualContainer');
  if (!container) return;

  fetch('/api/ds/hash-map')
    .then(r => r.json())
    .then(data => {
      document.getElementById('hmTotalBuckets').textContent = data.num_buckets || 16;
      document.getElementById('hmTotalItems').textContent = data.total_items || 0;
      document.getElementById('hmCollisions').textContent = data.collision_count || 0;
      document.getElementById('hmLoadFactor').textContent = data.load_factor || 0;

      const buckets = data.buckets || [];
      let html = '<div style="display:flex;flex-direction:column;gap:8px;">';

      buckets.forEach(b => {
        const hasChain = b.chain && b.chain.length > 0;
        html += `
          <div style="display:flex;align-items:center;gap:12px;background:#0f172a;border:1px solid #1e293b;border-radius:6px;padding:8px 12px;">
            <div style="width:70px;font-family:var(--font-mono);font-size:12px;font-weight:700;color:#38bdf8;">
              [Bucket ${b.bucket_index}]
            </div>
            <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;flex:1;">
        `;

        if (!hasChain) {
          html += '<span style="color:#475569;font-size:11px;">(empty)</span>';
        } else {
          b.chain.forEach((node, idx) => {
            html += `
              <div style="background:#1e293b;border:1px solid #334155;border-radius:6px;padding:4px 8px;font-size:11px;">
                <span style="font-weight:700;color:#fff;">${node.key}</span>
                <span style="color:#94a3b8;margin-left:4px;">(${node.tx_count} txs | ₹${node.total_amount.toLocaleString('en-IN', {minimumFractionDigits: 0})})</span>
              </div>
            `;
            if (idx < b.chain.length - 1) {
              html += '<span style="color:#f59e0b;font-weight:bold;">➔</span>';
            }
          });
        }

        html += `</div></div>`;
      });

      html += '</div>';
      container.innerHTML = html;
    })
    .catch(err => console.error("Error loading hash map:", err));
}

/* ==========================================================================
   6. Sorting & Searching Algorithm Lab
   ========================================================================== */

function setupSortingAndSearchLab() {
  const btnRunBubble = document.getElementById('btnRunBubbleSort');
  if (btnRunBubble) {
    btnRunBubble.addEventListener('click', () => runBubbleSortDemo());
  }

  const btnRunLinear = document.getElementById('btnRunLinearSearch');
  if (btnRunLinear) {
    btnRunLinear.addEventListener('click', () => runSearchDemo('linear'));
  }

  const btnRunBinary = document.getElementById('btnRunBinarySearch');
  if (btnRunBinary) {
    btnRunBinary.addEventListener('click', () => runSearchDemo('binary'));
  }
}

function runBubbleSortDemo() {
  const outputDiv = document.getElementById('sortTraceOutput');
  outputDiv.innerHTML = '<div style="color:#94a3b8;">Running C Bubble Sort Trace...</div>';

  fetch('/api/ds/sorting/bubble')
    .then(r => r.json())
    .then(data => {
      document.getElementById('bubbleComparisons').textContent = data.total_comparisons || 0;
      document.getElementById('bubbleSwaps').textContent = data.total_swaps || 0;

      const steps = data.steps || [];
      let html = `<div style="margin-bottom:12px;font-weight:600;color:#38bdf8;">Executed Bubble Sort in C: ${data.total_comparisons} comparisons, ${data.total_swaps} swaps.</div>`;
      html += '<div style="max-height:260px;overflow-y:auto;background:#0f172a;border:1px solid #1e293b;border-radius:6px;padding:12px;font-family:var(--font-mono);font-size:12px;line-height:1.6;">';

      steps.forEach(s => {
        const swapBadge = s.swapped ? '<span style="color:#f43f5e;font-weight:bold;">[SWAP]</span>' : '<span style="color:#64748b;">[NO SWAP]</span>';
        html += `<div>Pass ${s.pass}, Step ${s.step}: Comparing ₹${s.values[0]} & ₹${s.values[1]} ➔ ${swapBadge}</div>`;
      });

      html += '</div>';
      outputDiv.innerHTML = html;
    })
    .catch(err => console.error(err));
}

function runSearchDemo(type) {
  const target = document.getElementById('searchTargetAmount').value || '10000.0';
  const outputDiv = document.getElementById('searchTraceOutput');
  outputDiv.innerHTML = `<div style="color:#94a3b8;">Executing ${type} search in C for ₹${target}...</div>`;

  const endpoint = type === 'linear' ? `/api/ds/searching/linear?target=${target}` : `/api/ds/searching/binary?target=${target}`;

  fetch(endpoint)
    .then(r => r.json())
    .then(data => {
      let html = `
        <div style="font-weight:600;margin-bottom:8px;color:${data.found ? '#10b981' : '#f43f5e'};">
          ${data.algorithm} (${data.time_complexity}): ${data.found ? 'MATCH FOUND' : 'NOT FOUND'} in ${data.total_comparisons} steps!
        </div>
      `;

      html += '<div style="background:#0f172a;border:1px solid #1e293b;border-radius:6px;padding:12px;font-family:var(--font-mono);font-size:12px;">';
      (data.steps || []).forEach(s => {
        if (type === 'linear') {
          html += `<div>Step ${s.step}: Checked Index ${s.index} (₹${s.current_amount}) ➔ ${s.match ? 'MATCH!' : 'Mismatch'}</div>`;
        } else {
          html += `<div>Step ${s.step}: Low=${s.low}, Mid=${s.mid} (₹${s.mid_amount}), High=${s.high} ➔ ${s.match ? 'MATCH!' : 'Dividing'}</div>`;
        }
      });
      html += '</div>';
      outputDiv.innerHTML = html;
    })
    .catch(err => console.error(err));
}
