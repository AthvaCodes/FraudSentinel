"""Automated tests for Flask web routes and end-to-end user workflows."""
import pytest
import os
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, BASE_DIR)
from app import create_app
import database


@pytest.fixture
def client():
    app = create_app()
    app.config['TESTING'] = True
    with app.test_client() as client:
        yield client


def test_dashboard_endpoint(client):
    res = client.get('/')
    assert res.status_code == 200
    assert b"FraudSentinel" in res.data
    assert b"TOTAL PROCESSED VOLUME" in res.data
    assert b"Educational Prototype" in res.data


def test_transactions_list_and_filter(client):
    res = client.get('/transactions')
    assert res.status_code == 200
    assert b"Bank Transactions Ledger" in res.data

    # Filter test: only HIGH risk
    res_high = client.get('/transactions?risk=HIGH')
    assert res_high.status_code == 200

    # Search test
    res_search = client.get('/transactions?search=Sharma')
    assert res_search.status_code == 200
    assert b"Sharma" in res_search.data


def test_create_transaction_validation(client):
    # Test identical sender and receiver rejection
    res = client.post('/transactions/new', data={
        'sender': 'SameAccount',
        'receiver': 'SameAccount',
        'amount': '500'
    }, follow_redirects=True)
    assert b"cannot be the same account" in res.data

    # Test negative amount rejection
    res_neg = client.post('/transactions/new', data={
        'sender': 'AccountA',
        'receiver': 'AccountB',
        'amount': '-100'
    }, follow_redirects=True)
    assert b"greater than zero" in res_neg.data

    # Test successful addition
    res_ok = client.post('/transactions/new', data={
        'sender': 'ValidSender',
        'receiver': 'ValidReceiver',
        'amount': '12500',
        'category': 'Transfer',
        'notes': 'Test transfer'
    }, follow_redirects=True)
    assert res_ok.status_code == 200
    assert b"processed" in res_ok.data or b"completed" in res_ok.data


def test_accounts_directory_and_detail(client):
    res = client.get('/accounts')
    assert res.status_code == 200
    assert b"Bank Account Directory" in res.data

    # Detail page for Aarav Sharma
    res_detail = client.get('/accounts/Aarav%20Sharma')
    assert res_detail.status_code == 200
    assert b"Aarav Sharma" in res_detail.data
    assert b"CURRENT LEDGER BALANCE" in res_detail.data


def test_transfer_network_and_apis(client):
    res = client.get('/network')
    assert res.status_code == 200
    assert b"Interactive Transfer Network Graph" in res.data

    # JSON API data
    api_res = client.get('/api/network/data')
    assert api_res.status_code == 200
    data = api_res.get_json()
    assert "nodes" in data
    assert "edges" in data

    # Cycles API
    cycles_res = client.get('/api/network/cycles')
    assert cycles_res.status_code == 200
    cycles = cycles_res.get_json()
    assert isinstance(cycles, list)


def test_compliance_review_queue(client):
    res = client.get('/review')
    assert res.status_code == 200
    assert b"Compliance Investigator Review Queue" in res.data


def test_reports_and_csv_export(client):
    # HTML Report
    res_rep = client.get('/reports')
    assert res_rep.status_code == 200
    assert b"Comprehensive Fraud Detection Report" in res_rep.data

    # CSV Export
    res_csv = client.get('/transactions/export/csv')
    assert res_csv.status_code == 200
    assert res_csv.headers.get("Content-Type") == "text/csv; charset=utf-8"
    assert b"Transaction ID,Sender,Receiver,Amount (INR)" in res_csv.data


def test_data_structures_lab_endpoints(client):
    res_lab = client.get('/ds-lab')
    assert res_lab.status_code == 200
    assert b"Interactive Data Structures Lab" in res_lab.data

    # API endpoints
    assert client.get('/api/ds/linked-list').status_code == 200
    assert client.get('/api/ds/graph').status_code == 200
    assert client.get('/api/ds/queue').status_code == 200
    assert client.get('/api/ds/stack').status_code == 200
    assert client.get('/api/ds/bst').status_code == 200
    assert client.get('/api/ds/hash-map').status_code == 200
    assert client.get('/api/ds/sorting/bubble').status_code == 200
    assert client.get('/api/ds/searching/linear?target=1500').status_code == 200
    assert client.get('/api/ds/searching/binary?target=1500').status_code == 200
