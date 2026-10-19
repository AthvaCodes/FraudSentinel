from flask import Blueprint, render_template, request, jsonify
import c_bridge

network_bp = Blueprint('network', __name__)

@network_bp.route('/network')
def network_view():
    return render_template('network/index.html')

@network_bp.route('/api/network/data')
def api_network_data():
    data = c_bridge.run_c_engine('graph')
    return jsonify(data)

@network_bp.route('/api/network/cycles')
def api_network_cycles():
    cycles = c_bridge.run_c_engine('graph_cycles')
    return jsonify(cycles)

@network_bp.route('/api/network/path')
def api_network_path():
    start = request.args.get('start', '').strip()
    end = request.args.get('end', '').strip()
    if not start or not end:
        return jsonify({"status": "error", "message": "Start and end accounts required"}), 400
    res = c_bridge.run_c_engine('graph_path', start, end)
    return jsonify(res)

@network_bp.route('/api/network/bfs')
def api_network_bfs():
    start = request.args.get('start', '').strip()
    if not start:
        return jsonify([])
    res = c_bridge.run_c_engine('graph_bfs', start)
    return jsonify(res)

@network_bp.route('/api/network/dfs')
def api_network_dfs():
    start = request.args.get('start', '').strip()
    if not start:
        return jsonify([])
    res = c_bridge.run_c_engine('graph_dfs', start)
    return jsonify(res)
