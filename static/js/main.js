/**
 * Global UI Helper & Modal Manager
 */

document.addEventListener('DOMContentLoaded', () => {
  // Auto-dismiss alert boxes after 5 seconds
  const alerts = document.querySelectorAll('.alert');
  alerts.forEach(alert => {
    setTimeout(() => {
      alert.style.transition = 'opacity 0.4s ease, transform 0.4s ease';
      alert.style.opacity = '0';
      alert.style.transform = 'translateY(-10px)';
      setTimeout(() => alert.remove(), 400);
    }, 5000);
  });

  // Modal handlers
  setupModals();
});

function openModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) {
    modal.classList.add('active');
    document.body.style.overflow = 'hidden';
  }
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) {
    modal.classList.remove('active');
    document.body.style.overflow = '';
  }
}

function setupModals() {
  document.querySelectorAll('.modal-backdrop').forEach(modal => {
    modal.addEventListener('click', (e) => {
      if (e.target === modal) {
        modal.classList.remove('active');
        document.body.style.overflow = '';
      }
    });

    const closeBtn = modal.querySelector('.modal-close');
    if (closeBtn) {
      closeBtn.addEventListener('click', () => {
        modal.classList.remove('active');
        document.body.style.overflow = '';
      });
    }
  });
}

function inspectTransaction(txId) {
  fetch(`/transactions/${txId}`)
    .then(res => res.json())
    .then(data => {
      const modal = document.getElementById('txDetailModal');
      if (!modal) return;

      document.getElementById('modalTxId').textContent = data.id;
      document.getElementById('modalTxSender').textContent = data.sender;
      document.getElementById('modalTxReceiver').textContent = data.receiver;
      document.getElementById('modalTxAmount').textContent = `₹${parseFloat(data.amount).toLocaleString('en-IN', {minimumFractionDigits: 2})}`;
      document.getElementById('modalTxTimestamp').textContent = data.timestamp;
      document.getElementById('modalTxCategory').textContent = data.category;
      document.getElementById('modalTxRiskScore').textContent = data.risk_score;
      document.getElementById('modalTxRiskLevel').textContent = data.risk_level;
      document.getElementById('modalTxFlagged').textContent = data.is_flagged ? `YES (${data.flag_reason})` : 'NO';
      document.getElementById('modalTxStatus').textContent = data.review_status;
      document.getElementById('modalTxNotes').textContent = data.analyst_notes || 'No analyst notes recorded.';

      openModal('txDetailModal');
    })
    .catch(err => {
      console.error("Error inspecting transaction:", err);
      alert("Failed to load transaction details.");
    });
}
