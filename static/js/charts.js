/**
 * Native, Dependency-Free SVG Charting Engine
 */

class NativeSVGChart {
  /**
   * Renders a responsive SVG Donut chart into a target container.
   */
  static renderDonut(containerId, data, options = {}) {
    const container = document.getElementById(containerId);
    if (!container) return;

    const width = options.width || 240;
    const height = options.height || 240;
    const radius = Math.min(width, height) / 2 - 10;
    const innerRadius = radius * 0.62;
    const cx = width / 2;
    const cy = height / 2;

    const total = data.reduce((sum, d) => sum + d.value, 0);
    if (total === 0) {
      container.innerHTML = `<div style="text-align:center;padding:40px;color:#64748b;">No data available</div>`;
      return;
    }

    let startAngle = 0;
    let paths = '';

    data.forEach((slice, idx) => {
      if (slice.value <= 0) return;
      const sliceAngle = (slice.value / total) * 2 * Math.PI;
      const endAngle = startAngle + sliceAngle;

      const x1 = cx + radius * Math.cos(startAngle);
      const y1 = cy + radius * Math.sin(startAngle);
      const x2 = cx + radius * Math.cos(endAngle);
      const y2 = cy + radius * Math.sin(endAngle);

      const ix1 = cx + innerRadius * Math.cos(endAngle);
      const iy1 = cy + innerRadius * Math.sin(endAngle);
      const ix2 = cx + innerRadius * Math.cos(startAngle);
      const iy2 = cy + innerRadius * Math.sin(startAngle);

      const largeArc = sliceAngle > Math.PI ? 1 : 0;

      const pathData = `
        M ${x1} ${y1}
        A ${radius} ${radius} 0 ${largeArc} 1 ${x2} ${y2}
        L ${ix1} ${iy1}
        A ${innerRadius} ${innerRadius} 0 ${largeArc} 0 ${ix2} ${iy2}
        Z
      `;

      paths += `<path d="${pathData}" fill="${slice.color}" opacity="0.9">
        <title>${slice.label}: ${slice.value} (${((slice.value / total) * 100).toFixed(1)}%)</title>
      </path>`;

      startAngle = endAngle;
    });

    let legend = '<div style="display:flex;justify-content:center;gap:14px;margin-top:14px;flex-wrap:wrap;">';
    data.forEach(d => {
      legend += `
        <div style="display:flex;align-items:center;gap:6px;font-size:12px;color:#94a3b8;">
          <span style="width:10px;height:10px;border-radius:2px;background:${d.color};display:inline-block;"></span>
          <span>${d.label}: <strong>${d.value}</strong></span>
        </div>
      `;
    });
    legend += '</div>';

    container.innerHTML = `
      <div style="display:flex;flex-direction:column;align-items:center;">
        <svg width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">
          ${paths}
          <text x="${cx}" y="${cy - 4}" text-anchor="middle" fill="#fff" font-size="20" font-weight="700">${total}</text>
          <text x="${cx}" y="${cy + 16}" text-anchor="middle" fill="#94a3b8" font-size="11">TOTAL</text>
        </svg>
        ${legend}
      </div>
    `;
  }

  /**
   * Renders a responsive horizontal Bar Chart.
   */
  static renderBarChart(containerId, items, options = {}) {
    const container = document.getElementById(containerId);
    if (!container) return;

    if (!items || items.length === 0) {
      container.innerHTML = `<div style="text-align:center;padding:40px;color:#64748b;">No categories</div>`;
      return;
    }

    const maxVal = Math.max(...items.map(d => d.value), 1);
    let html = '<div style="display:flex;flex-direction:column;gap:12px;padding:8px 0;">';

    items.forEach(item => {
      const pct = Math.round((item.value / maxVal) * 100);
      html += `
        <div>
          <div style="display:flex;justify-content:space-between;font-size:12px;margin-bottom:4px;">
            <span style="color:#f8fafc;font-weight:500;">${item.label}</span>
            <span style="color:#94a3b8;">₹${item.value.toLocaleString('en-IN', {minimumFractionDigits: 0})} (${item.count} txs)</span>
          </div>
          <div style="width:100%;height:8px;background:#0f172a;border-radius:999px;overflow:hidden;">
            <div style="width:${pct}%;height:100%;background:${item.color || '#3b82f6'};border-radius:999px;"></div>
          </div>
        </div>
      `;
    });

    html += '</div>';
    container.innerHTML = html;
  }
}
