from flask import Blueprint, render_template, request, jsonify, abort, current_app, flash, redirect, url_for, send_file
from io import BytesIO
import json
import os
from datetime import datetime
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen
import database
import c_bridge

accounts_bp = Blueprint('accounts', __name__)


def _account_analysis(account, outgoing, incoming, counterparties):
    """Return AI-authored narrative grounded in computed account-level facts."""
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        raise RuntimeError("AI reports need GEMINI_API_KEY set in the server environment.")

    total = len(outgoing) + len(incoming)
    volume = float(account['total_sent']) + float(account['total_received'])
    flagged = sum(bool(tx['is_flagged']) for tx in outgoing + incoming)
    high_risk = sum(tx['risk_level'] == 'HIGH' for tx in outgoing + incoming)
    counterpart_rows = sorted(
        ({"account": name, **values} for name, values in counterparties.items()),
        key=lambda row: row['sent_to'] + row['recv_from'], reverse=True
    )[:12]
    facts = {
        "account_name": account['account_name'],
        "account_risk_level_from_existing_rules": account['risk_level'],
        "transaction_count": total,
        "outgoing_count": len(outgoing), "incoming_count": len(incoming),
        "total_outgoing_inr": round(float(account['total_sent']), 2),
        "total_incoming_inr": round(float(account['total_received']), 2),
        "total_flow_inr": round(volume, 2),
        "average_transaction_inr": round(volume / total, 2) if total else 0,
        "flagged_transaction_count": flagged,
        "high_risk_transaction_count": high_risk,
        "records_included_cap": "Up to 100 latest incoming and 100 latest outgoing records are provided to the AI.",
        "top_counterparties": counterpart_rows,
        "transactions": [
            {"id": tx['id'], "direction": direction, "counterparty": tx['receiver'] if direction == 'OUTGOING' else tx['sender'],
             "amount_inr": round(float(tx['amount']), 2), "category": tx['category'],
             "timestamp": tx['timestamp'], "rule_risk_level": tx['risk_level'],
             "rule_flagged": bool(tx['is_flagged']), "review_status": tx['review_status']}
            for direction, rows in (("OUTGOING", outgoing), ("INCOMING", incoming))
            for tx in rows[:100]
        ]
    }
    schema = {
        "type": "object", "additionalProperties": False,
        "properties": {
            "executive_summary": {"type": "string"},
            "observations": {"type": "array", "items": {"type": "string"}},
            "risk_assessment": {"type": "string"},
            "recommended_review_steps": {"type": "array", "items": {"type": "string"}},
            "data_limitations": {"type": "string"}
        },
        "required": ["executive_summary", "observations", "risk_assessment", "recommended_review_steps", "data_limitations"]
    }
    instructions = (
            "You write concise, professional account-level fraud analysis for human compliance review. "
            "Use only the supplied data. Do not infer identity, intent, criminality, or facts absent from it. "
            "Call out patterns as indicators requiring review, distinguish observations from conclusions, "
            "and state that this is not a definitive fraud determination. Return specific, actionable review steps."
    )
    model = os.environ.get("GEMINI_MODEL", "gemini-3.8-flash")
    payload = {
        "system_instruction": {"parts": [{"text": instructions}]},
        "contents": [{"parts": [{"text": f"Analyze this account data:\n{json.dumps(facts, ensure_ascii=False)}"}]}],
        "generationConfig": {
            "responseMimeType": "application/json",
            "responseSchema": schema,
            "temperature": 0.2
        }
    }
    req = Request(
        f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json", "x-goog-api-key": api_key},
        method="POST"
    )
    try:
        with urlopen(req, timeout=30) as response:
            result = json.loads(response.read().decode("utf-8"))
    except HTTPError as exc:
        raise RuntimeError(f"Gemini API returned HTTP {exc.code}. Check your API key, model, and quota.") from exc
    except (URLError, TimeoutError) as exc:
        raise RuntimeError("Could not reach the Gemini API. Check your internet connection and try again.") from exc

    parts = result.get("candidates", [{}])[0].get("content", {}).get("parts", [])
    response_text = "".join(part.get("text", "") for part in parts)
    if not response_text:
        raise RuntimeError("Gemini returned an empty report. Please try again.")
    return json.loads(response_text), facts


def _build_account_pdf(account, analysis, facts):
    page_width, page_height = 595, 842  # A4 points
    pages = []
    commands = []
    cursor = [0]

    def pdf_text(value):
        value = str(value or '').encode('ascii', 'replace').decode('ascii')
        return value.replace('\\', '\\\\').replace('(', '\\(').replace(')', '\\)')

    def text(x, baseline, value, size=9, color=(0.08, 0.14, 0.23), bold=False):
        font = 'F2' if bold else 'F1'
        commands.append(f"BT /{font} {size} Tf {color[0]} {color[1]} {color[2]} rg {x} {baseline} Td ({pdf_text(value)}) Tj ET")

    def rect(x, bottom, width, height, fill):
        commands.append(f"{fill[0]} {fill[1]} {fill[2]} rg {x} {bottom} {width} {height} re f")

    def line(x1, y1, x2, y2, stroke=(0.84, 0.88, 0.92)):
        commands.append(f"{stroke[0]} {stroke[1]} {stroke[2]} RG 0.6 w {x1} {y1} m {x2} {y2} l S")

    def start_page():
        nonlocal commands
        if commands:
            pages.append('\n'.join(commands))
        commands = []
        rect(0, page_height-92, page_width, 92, (0.06, 0.14, 0.25))
        text(46, page_height-43, 'ACCOUNT FRAUD ANALYSIS', 19, (1, 1, 1), True)
        text(46, page_height-65, 'Customer-level activity review | FraudSentinel', 9, (0.78, 0.86, 0.93))
        cursor[0] = page_height - 119

    def ensure_space(height):
        if cursor[0] - height < 55:
            start_page()

    def wrap(value, max_chars=100):
        words = str(value or '').split()
        lines = []
        current = ''
        for word in words:
            if current and len(current) + len(word) + 1 > max_chars:
                lines.append(current)
                current = word
            else:
                current = f'{current} {word}'.strip()
        if current:
            lines.append(current)
        return lines or ['']

    def section(title):
        ensure_space(34)
        y_delta = 17
        cursor[0] -= y_delta
        text(46, cursor[0], title.upper(), 10, (0.09, 0.40, 0.52), True)
        line(46, cursor[0]-6, page_width-46, cursor[0]-6)
        cursor[0] -= 19

    def paragraph(value, bullet=False, small=False):
        lines = wrap(value, 97 if not small else 112)
        for index, row in enumerate(lines):
            ensure_space(14)
            prefix = '- ' if bullet and index == 0 else ('  ' if bullet else '')
            text(48, cursor[0], prefix + row, 8 if small else 9, (0.32, 0.39, 0.47) if small else (0.10, 0.16, 0.24))
            cursor[0] -= 13 if not small else 11
        cursor[0] -= 3

    start_page()
    text(46, cursor[0], f"Account: {account['account_name']}", 11, (0.08, 0.14, 0.23), True)
    cursor[0] -= 17
    text(46, cursor[0], f"Generated {datetime.now().astimezone().strftime('%d %b %Y, %H:%M %Z')}", 8, (0.32, 0.39, 0.47))
    cursor[0] -= 26

    card_gap, card_width = 9, 118
    metrics = [
        ('RULE RISK', account['risk_level']),
        ('TRANSACTIONS', str(facts['transaction_count'])),
        ('FLAGGED', str(facts['flagged_transaction_count'])),
        ('TOTAL FLOW (INR)', f"{facts['total_flow_inr']:,.2f}")
    ]
    for index, (label, value) in enumerate(metrics):
        x = 46 + index * (card_width + card_gap)
        rect(x, cursor[0]-39, card_width, 48, (0.93, 0.96, 0.98))
        text(x+8, cursor[0]-8, label, 6.5, (0.32, 0.39, 0.47), True)
        text(x+8, cursor[0]-27, value, 10 if len(value) < 14 else 8, (0.06, 0.14, 0.25), True)
    cursor[0] -= 60

    section('Account Activity Profile')
    paragraph(f"The account has {facts['outgoing_count']} outgoing and {facts['incoming_count']} incoming transfers. Total outflow is INR {facts['total_outgoing_inr']:,.2f}; total inflow is INR {facts['total_incoming_inr']:,.2f}. Existing rules flagged {facts['flagged_transaction_count']} transactions, including {facts['high_risk_transaction_count']} rated HIGH.")
    section('AI Executive Analysis')
    paragraph(analysis['executive_summary'])
    for title, key in [('Key Observations', 'observations'), ('Risk Assessment', 'risk_assessment'), ('Recommended Review Steps', 'recommended_review_steps')]:
        section(title)
        items = analysis[key] if isinstance(analysis[key], list) else [analysis[key]]
        for item in items:
            paragraph(item, bullet=(key != 'risk_assessment'))
        if not items:
            paragraph('No material observations from the available records.')

    section('Largest Counterparty Relationships')
    text(48, cursor[0], 'COUNTERPARTY', 7, (0.32, 0.39, 0.47), True)
    text(284, cursor[0], 'SENT (INR)', 7, (0.32, 0.39, 0.47), True)
    text(385, cursor[0], 'RECEIVED (INR)', 7, (0.32, 0.39, 0.47), True)
    text(520, cursor[0], 'COUNT', 7, (0.32, 0.39, 0.47), True)
    cursor[0] -= 13
    for cp in facts['top_counterparties'][:8] or [{'account': 'No counterparties recorded', 'sent_to': 0, 'recv_from': 0, 'count': 0}]:
        ensure_space(15)
        text(48, cursor[0], str(cp['account'])[:35], 7.5)
        text(284, cursor[0], f"{cp['sent_to']:,.2f}", 7.5)
        text(385, cursor[0], f"{cp['recv_from']:,.2f}", 7.5)
        text(520, cursor[0], str(cp['count']), 7.5)
        line(46, cursor[0]-4, page_width-46, cursor[0]-4)
        cursor[0] -= 15
    section('Data Limitations & Method')
    paragraph(analysis['data_limitations'])
    paragraph('This AI-generated narrative is an analytical aid, not a confirmed fraud finding. The rule-based rating is computed independently. Review source records and apply human judgment before taking action.', small=True)

    pages.append('\n'.join(commands))
    total_pages = len(pages)
    for number, content in enumerate(pages, 1):
        content += f"\n0.84 0.88 0.92 RG 0.6 w 46 39 m {page_width-46} 39 l S\nBT /F1 7 Tf 0.32 0.39 0.47 rg 46 25 Td (CONFIDENTIAL - INTERNAL COMPLIANCE REVIEW) Tj ET\nBT /F1 7 Tf 0.32 0.39 0.47 rg {page_width-78} 25 Td (Page {number} of {total_pages}) Tj ET"
        pages[number-1] = content

    objects = [None,
        b'<< /Type /Catalog /Pages 2 0 R >>',
        None,
        b'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',
        b'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>'
    ]
    page_refs = []
    for content in pages:
        page_id = len(objects)
        stream_id = page_id + 1
        page_refs.append(f'{page_id} 0 R')
        stream = content.encode('ascii', 'replace')
        objects.append(f'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {page_width} {page_height}] /Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> /Contents {stream_id} 0 R >>'.encode('ascii'))
        objects.append(f'<< /Length {len(stream)} >>\nstream\n'.encode('ascii') + stream + b'\nendstream')
    objects[2] = f'<< /Type /Pages /Kids [{" ".join(page_refs)}] /Count {len(page_refs)} >>'.encode('ascii')

    output = BytesIO()
    output.write(b'%PDF-1.4\n%\xe2\xe3\xcf\xd3\n')
    offsets = [0]
    for object_id, body in enumerate(objects[1:], 1):
        offsets.append(output.tell())
        output.write(f'{object_id} 0 obj\n'.encode('ascii'))
        output.write(body)
        output.write(b'\nendobj\n')
    xref_offset = output.tell()
    output.write(f'xref\n0 {len(objects)}\n'.encode('ascii'))
    output.write(b'0000000000 65535 f \n')
    for offset in offsets[1:]:
        output.write(f'{offset:010d} 00000 n \n'.encode('ascii'))
    output.write(f'trailer\n<< /Size {len(objects)} /Root 1 0 R >>\nstartxref\n{xref_offset}\n%%EOF'.encode('ascii'))
    output.seek(0)
    return output

@accounts_bp.route('/accounts')
def list_accounts():
    search = request.args.get('search', '').strip()
    risk_filter = request.args.get('risk', '').strip().upper()

    conn = database.get_db_connection()
    cursor = conn.cursor()

    conditions = []
    params = []
    if search:
        conditions.append("account_name LIKE ?")
        params.append(f"%{search}%")
    if risk_filter and risk_filter != "ALL":
        conditions.append("risk_level = ?")
        params.append(risk_filter)

    where = " WHERE " + " AND ".join(conditions) if conditions else ""
    query = f"""
    SELECT * FROM accounts
    {where}
    ORDER BY total_sent + total_received DESC, transaction_count DESC
    """
    cursor.execute(query, params)
    accounts = [dict(r) for r in cursor.fetchall()]
    conn.close()

    return render_template(
        'accounts/index.html',
        accounts=accounts,
        search=search,
        risk_filter=risk_filter
    )

@accounts_bp.route('/accounts/<account_name>')
def account_detail(account_name: str):
    conn = database.get_db_connection()
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM accounts WHERE account_name = ?", (account_name,))
    account = cursor.fetchone()

    if not account:
        conn.close()
        abort(404, description="Account not found")

    account = dict(account)

    # Get outgoing transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE sender = ? ORDER BY id DESC
    """, (account_name,))
    outgoing = [dict(r) for r in cursor.fetchall()]

    # Get incoming transactions
    cursor.execute("""
    SELECT * FROM transactions WHERE receiver = ? ORDER BY id DESC
    """, (account_name,))
    incoming = [dict(r) for r in cursor.fetchall()]

    # Counterparties
    counterparties = {}
    for tx in outgoing:
        target = tx['receiver']
        if target not in counterparties:
            counterparties[target] = {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0}
        counterparties[target]['sent_to'] += tx['amount']
        counterparties[target]['count'] += 1

    for tx in incoming:
        src = tx['sender']
        if src not in counterparties:
            counterparties[src] = {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0}
        counterparties[src]['recv_from'] += tx['amount']
        counterparties[src]['count'] += 1

    conn.close()

    return render_template(
        'accounts/detail.html',
        account=account,
        outgoing=outgoing,
        incoming=incoming,
        counterparties=counterparties
    )


@accounts_bp.route('/accounts/<account_name>/fraud-report.pdf')
def account_fraud_report(account_name: str):
    conn = database.get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM accounts WHERE account_name = ?", (account_name,))
    row = cursor.fetchone()
    if not row:
        conn.close()
        abort(404, description="Account not found")
    account = dict(row)
    cursor.execute("SELECT * FROM transactions WHERE sender = ? ORDER BY id DESC", (account_name,))
    outgoing = [dict(item) for item in cursor.fetchall()]
    cursor.execute("SELECT * FROM transactions WHERE receiver = ? ORDER BY id DESC", (account_name,))
    incoming = [dict(item) for item in cursor.fetchall()]
    conn.close()

    counterparties = {}
    for tx in outgoing:
        cp = counterparties.setdefault(tx['receiver'], {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0})
        cp['sent_to'] += tx['amount']; cp['count'] += 1
    for tx in incoming:
        cp = counterparties.setdefault(tx['sender'], {'sent_to': 0.0, 'recv_from': 0.0, 'count': 0})
        cp['recv_from'] += tx['amount']; cp['count'] += 1

    try:
        analysis, facts = _account_analysis(account, outgoing, incoming, counterparties)
        pdf = _build_account_pdf(account, analysis, facts)
    except Exception as exc:
        current_app.logger.exception("Failed generating AI account report")
        flash(str(exc) if isinstance(exc, RuntimeError) else "Could not generate the AI report. Check the API configuration and try again.", "danger")
        return redirect(url_for('accounts.account_detail', account_name=account_name))

    return send_file(pdf, mimetype='application/pdf', as_attachment=True,
                     download_name=f"fraud-analysis-{account_name.lower().replace(' ', '-')}.pdf")
