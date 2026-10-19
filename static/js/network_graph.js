/**
 * Interactive HTML5 Canvas Transfer Network Graph Visualizer
 */

class TransferNetworkVisualizer {
  constructor(canvasId, options = {}) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;
    this.ctx = this.canvas.getContext('2d');

    this.nodes = [];
    this.edges = [];
    this.cycles = [];
    this.highlightCycles = false;

    this.selectedNode = null;
    this.hoveredNode = null;

    // Viewport transform (zoom / pan)
    this.scale = 1.0;
    this.panX = 0;
    this.panY = 0;

    this.isDragging = false;
    this.isPanning = false;
    this.dragNode = null;
    this.lastMouseX = 0;
    this.lastMouseY = 0;

    this.initCanvasSize();
    this.bindEvents();
    this.loadData();
  }

  initCanvasSize() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    this.canvas.width = rect.width || 800;
    this.canvas.height = rect.height || 540;
    this.panX = this.canvas.width / 2;
    this.panY = this.canvas.height / 2;
  }

  bindEvents() {
    window.addEventListener('resize', () => this.initCanvasSize());

    this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
    this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
    window.addEventListener('mouseup', () => this.onMouseUp());
    this.canvas.addEventListener('wheel', (e) => this.onWheel(e));

    const cycleBtn = document.getElementById('btnToggleCycles');
    if (cycleBtn) {
      cycleBtn.addEventListener('click', () => {
        this.highlightCycles = !this.highlightCycles;
        cycleBtn.classList.toggle('active', this.highlightCycles);
        this.render();
      });
    }

    const resetBtn = document.getElementById('btnResetZoom');
    if (resetBtn) {
      resetBtn.addEventListener('click', () => {
        this.scale = 1.0;
        this.panX = this.canvas.width / 2;
        this.panY = this.canvas.height / 2;
        this.render();
      });
    }
  }

  loadData() {
    Promise.all([
      fetch('/api/network/data').then(r => r.json()),
      fetch('/api/network/cycles').then(r => r.json())
    ]).then(([graphData, cycleData]) => {
      this.cycles = Array.isArray(cycleData) ? cycleData : [];
      this.setupGraph(graphData);
    }).catch(err => console.error("Error loading network graph data:", err));
  }

  setupGraph(data) {
    if (!data || !data.nodes) return;

    const n = data.nodes.length;
    const radius = Math.min(this.canvas.width, this.canvas.height) * 0.35;

    // Distribute nodes in a circle initially
    this.nodes = data.nodes.map((node, i) => {
      const angle = (i / n) * 2 * Math.PI;
      const totalVol = (node.total_sent || 0) + (node.total_received || 0);
      const r = Math.max(16, Math.min(32, 16 + Math.log10(Math.max(1, totalVol)) * 3));

      let color = '#10b981'; // LOW risk
      if (node.risk_score >= 4 || node.is_flagged) {
        color = '#f43f5e'; // HIGH risk
      } else if (node.risk_score >= 2 || node.in_degree + node.out_degree >= 5) {
        color = '#f59e0b'; // MEDIUM risk
      }

      return {
        id: node.id,
        label: node.label || node.id,
        total_sent: node.total_sent,
        total_received: node.total_received,
        risk_score: node.risk_score,
        is_flagged: node.is_flagged,
        radius: r,
        color: color,
        x: Math.cos(angle) * radius,
        y: Math.sin(angle) * radius,
        vx: 0,
        vy: 0
      };
    });

    this.edges = data.edges || [];
    this.startSimulation();
  }

  startSimulation() {
    let iterations = 100;
    const runStep = () => {
      this.stepPhysics();
      this.render();
      iterations--;
      if (iterations > 0) {
        requestAnimationFrame(runStep);
      }
    };
    runStep();
  }

  stepPhysics() {
    const kRepel = 3000;
    const kSpring = 0.05;
    const restLen = 140;

    // Node repulsion (Coulomb force)
    for (let i = 0; i < this.nodes.length; i++) {
      for (let j = i + 1; j < this.nodes.length; j++) {
        const n1 = this.nodes[i];
        const n2 = this.nodes[j];
        let dx = n2.x - n1.x;
        let dy = n2.y - n1.y;
        let dist = Math.sqrt(dx * dx + dy * dy) || 1;
        let force = kRepel / (dist * dist);
        let fx = (dx / dist) * force;
        let fy = (dy / dist) * force;

        n1.vx -= fx;
        n1.vy -= fy;
        n2.vx += fx;
        n2.vy += fy;
      }
    }

    // Edge attraction (Hooke's spring)
    const nodeMap = new Map(this.nodes.map(n => [n.id, n]));
    for (const e of this.edges) {
      const src = nodeMap.get(e.source);
      const tgt = nodeMap.get(e.target);
      if (src && tgt) {
        let dx = tgt.x - src.x;
        let dy = tgt.y - src.y;
        let dist = Math.sqrt(dx * dx + dy * dy) || 1;
        let force = (dist - restLen) * kSpring;
        let fx = (dx / dist) * force;
        let fy = (dy / dist) * force;

        src.vx += fx;
        src.vy += fy;
        tgt.vx -= fx;
        tgt.vy -= fy;
      }
    }

    // Apply damping and integrate positions
    for (const n of this.nodes) {
      if (n === this.dragNode) continue;
      n.vx *= 0.75;
      n.vy *= 0.75;
      n.x += n.vx;
      n.y += n.vy;
    }
  }

  render() {
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    ctx.save();
    ctx.translate(this.panX, this.panY);
    ctx.scale(this.scale, this.scale);

    const nodeMap = new Map(this.nodes.map(n => [n.id, n]));

    // Draw edges
    for (const edge of this.edges) {
      const src = nodeMap.get(edge.source);
      const tgt = nodeMap.get(edge.target);
      if (!src || !tgt) continue;

      let isCycleEdge = false;
      if (this.highlightCycles && this.cycles.length > 0) {
        for (const cycle of this.cycles) {
          const path = cycle.path || [];
          for (let p = 0; p < path.length - 1; p++) {
            if (path[p] === edge.source && path[p + 1] === edge.target) {
              isCycleEdge = true;
              break;
            }
          }
        }
      }

      ctx.beginPath();
      ctx.moveTo(src.x, src.y);
      ctx.lineTo(tgt.x, tgt.y);

      if (isCycleEdge) {
        ctx.strokeStyle = '#f43f5e';
        ctx.lineWidth = 3.5;
        ctx.shadowColor = '#f43f5e';
        ctx.shadowBlur = 10;
      } else {
        ctx.strokeStyle = 'rgba(148, 163, 184, 0.25)';
        ctx.lineWidth = Math.max(1, Math.min(3, Math.log10(edge.amount + 1)));
        ctx.shadowBlur = 0;
      }
      ctx.stroke();

      // Arrow head
      this.drawArrow(ctx, src.x, src.y, tgt.x, tgt.y, tgt.radius, isCycleEdge ? '#f43f5e' : 'rgba(148, 163, 184, 0.4)');
    }

    // Draw nodes
    for (const node of this.nodes) {
      let isCycleNode = false;
      if (this.highlightCycles && this.cycles.length > 0) {
        for (const cycle of this.cycles) {
          if ((cycle.path || []).includes(node.id)) {
            isCycleNode = true;
            break;
          }
        }
      }

      ctx.beginPath();
      ctx.arc(node.x, node.y, node.radius, 0, 2 * Math.PI);
      ctx.fillStyle = isCycleNode ? '#f43f5e' : node.color;
      if (isCycleNode) {
        ctx.shadowColor = '#f43f5e';
        ctx.shadowBlur = 14;
      } else {
        ctx.shadowBlur = 0;
      }
      ctx.fill();

      ctx.lineWidth = (node === this.selectedNode || node === this.hoveredNode) ? 3 : 1.5;
      ctx.strokeStyle = '#fff';
      ctx.stroke();

      // Label
      ctx.font = '11px sans-serif';
      ctx.fillStyle = '#f8fafc';
      ctx.textAlign = 'center';
      ctx.fillText(node.label, node.x, node.y + node.radius + 14);
    }

    ctx.restore();
  }

  drawArrow(ctx, x1, y1, x2, y2, targetRadius, color) {
    const angle = Math.atan2(y2 - y1, x2 - x1);
    const arrowLen = 9;
    const ax = x2 - Math.cos(angle) * (targetRadius + 2);
    const ay = y2 - Math.sin(angle) * (targetRadius + 2);

    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.moveTo(ax, ay);
    ctx.lineTo(ax - arrowLen * Math.cos(angle - Math.PI / 6), ay - arrowLen * Math.sin(angle - Math.PI / 6));
    ctx.lineTo(ax - arrowLen * Math.cos(angle + Math.PI / 6), ay - arrowLen * Math.sin(angle + Math.PI / 6));
    ctx.closePath();
    ctx.fill();
  }

  getNodeAt(clientX, clientY) {
    const rect = this.canvas.getBoundingClientRect();
    const mx = (clientX - rect.left - this.panX) / this.scale;
    const my = (clientY - rect.top - this.panY) / this.scale;

    for (let i = this.nodes.length - 1; i >= 0; i--) {
      const n = this.nodes[i];
      const dist = Math.hypot(n.x - mx, n.y - my);
      if (dist <= n.radius + 4) {
        return n;
      }
    }
    return null;
  }

  onMouseDown(e) {
    const node = this.getNodeAt(e.clientX, e.clientY);
    if (node) {
      this.isDragging = true;
      this.dragNode = node;
      this.selectedNode = node;
      this.displayDossier(node);
    } else {
      this.isPanning = true;
    }
    this.lastMouseX = e.clientX;
    this.lastMouseY = e.clientY;
  }

  onMouseMove(e) {
    if (this.isDragging && this.dragNode) {
      const dx = (e.clientX - this.lastMouseX) / this.scale;
      const dy = (e.clientY - this.lastMouseY) / this.scale;
      this.dragNode.x += dx;
      this.dragNode.y += dy;
      this.lastMouseX = e.clientX;
      this.lastMouseY = e.clientY;
      this.render();
    } else if (this.isPanning) {
      this.panX += e.clientX - this.lastMouseX;
      this.panY += e.clientY - this.lastMouseY;
      this.lastMouseX = e.clientX;
      this.lastMouseY = e.clientY;
      this.render();
    } else {
      const node = this.getNodeAt(e.clientX, e.clientY);
      if (node !== this.hoveredNode) {
        this.hoveredNode = node;
        this.canvas.style.cursor = node ? 'pointer' : 'default';
        this.render();
      }
    }
  }

  onMouseUp() {
    this.isDragging = false;
    this.isPanning = false;
    this.dragNode = null;
  }

  onWheel(e) {
    e.preventDefault();
    const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
    this.scale = Math.max(0.3, Math.min(3.0, this.scale * zoomFactor));
    this.render();
  }

  displayDossier(node) {
    const card = document.getElementById('accountDossierCard');
    if (!card) return;

    card.style.display = 'block';
    document.getElementById('dossierName').textContent = node.label;
    document.getElementById('dossierRisk').textContent = node.risk_score >= 4 ? 'HIGH' : (node.risk_score >= 2 ? 'MEDIUM' : 'LOW');
    document.getElementById('dossierSent').textContent = `₹${(node.total_sent || 0).toLocaleString('en-IN', {minimumFractionDigits: 2})}`;
    document.getElementById('dossierReceived').textContent = `₹${(node.total_received || 0).toLocaleString('en-IN', {minimumFractionDigits: 2})}`;
    document.getElementById('dossierLink').href = `/accounts/${encodeURIComponent(node.id)}`;
  }
}

document.addEventListener('DOMContentLoaded', () => {
  if (document.getElementById('networkCanvas')) {
    window.networkVisualizer = new TransferNetworkVisualizer('networkCanvas');
  }
});
