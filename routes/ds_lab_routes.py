from flask import Blueprint, render_template, request, jsonify
import c_bridge

ds_lab_bp = Blueprint('ds_lab', __name__)

@ds_lab_bp.route('/ds-lab')
def ds_lab_view():
    return render_template('ds_lab/index.html')

@ds_lab_bp.route('/api/ds/linked-list')
def api_ds_linked_list():
    res = c_bridge.run_c_engine('linked_list')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/graph')
def api_ds_graph():
    res = c_bridge.run_c_engine('graph')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/graph/bfs')
def api_ds_graph_bfs():
    start = request.args.get('start', 'Aarav Sharma')
    res = c_bridge.run_c_engine('graph_bfs', start)
    return jsonify(res)

@ds_lab_bp.route('/api/ds/graph/dfs')
def api_ds_graph_dfs():
    start = request.args.get('start', 'Aarav Sharma')
    res = c_bridge.run_c_engine('graph_dfs', start)
    return jsonify(res)

@ds_lab_bp.route('/api/ds/graph/path')
def api_ds_graph_path():
    start = request.args.get('start', '').strip()
    end = request.args.get('end', '').strip()
    if not start or not end:
        return jsonify({"status": "error", "message": "Start and end accounts required"}), 400
    res = c_bridge.run_c_engine('graph_path', start, end)
    return jsonify(res)

@ds_lab_bp.route('/api/ds/graph/cycles')
def api_ds_graph_cycles():
    res = c_bridge.run_c_engine('graph_cycles')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/queue')
def api_ds_queue():
    res = c_bridge.run_c_engine('queue')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/stack')
def api_ds_stack():
    res = c_bridge.run_c_engine('stack')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/bst')
def api_ds_bst():
    res = c_bridge.run_c_engine('bst')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/bst/range')
def api_ds_bst_range():
    min_val = request.args.get('min', '5000')
    max_val = request.args.get('max', '25000')
    res = c_bridge.run_c_engine('bst_range', min_val, max_val)
    return jsonify(res)

@ds_lab_bp.route('/api/ds/bst/in-order')
def api_ds_bst_in_order():
    res = c_bridge.run_c_engine('bst_in_order')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/hash-map')
def api_ds_hash_map():
    res = c_bridge.run_c_engine('hash_map')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/sorting/bubble')
def api_ds_sorting_bubble():
    res = c_bridge.run_c_engine('bubble_sort')
    return jsonify(res)

@ds_lab_bp.route('/api/ds/searching/linear')
def api_ds_searching_linear():
    target = request.args.get('target', '10000.0')
    res = c_bridge.run_c_engine('linear_search', target)
    return jsonify(res)

@ds_lab_bp.route('/api/ds/searching/binary')
def api_ds_searching_binary():
    target = request.args.get('target', '10000.0')
    res = c_bridge.run_c_engine('binary_search', target)
    return jsonify(res)
