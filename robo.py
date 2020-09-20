import hashlib
import hmac
import json

from http import client
from urllib.parse import urlencode


# Constantes
MB_TAPI_ID = 'b1080fc89f6d69fcfd9354b8df6ccc8a'
MB_TAPI_SECRET = 'b4ec160195e8da08a9f720d2b6b7cbad824e1bb25a7b882640744941d706cc39'
REQUEST_HOST = 'www.mercadobitcoin.net'
REQUEST_PATH = '/tapi/v3/'

# Nonce
# Para obter variação de forma simples
# timestamp pode ser utilizado:
#     import time
#     tapi_nonce = str(int(time.time()))
tapi_nonce = 4

# Parâmetros
params = {
    'tapi_method': 'list_orders',
    'tapi_nonce': tapi_nonce,
    'coin_pair': 'BRLBTC'
}
params = urlencode(params)

# Gerar MAC
params_string = REQUEST_PATH + '?' + params
H = hmac.new(bytes(MB_TAPI_SECRET, encoding='utf8'), digestmod=hashlib.sha512)
H.update(params_string.encode('utf-8'))
tapi_mac = H.hexdigest()

# Gerar cabeçalho da requisição
headers = {
    'Content-Type': 'application/x-www-form-urlencoded',
    'TAPI-ID': MB_TAPI_ID,
    'TAPI-MAC': tapi_mac
}

print ("tapi_mac = {}".format(tapi_mac))
print ("params = {}".format(params))
#print ("headers = {}".format(headers))

# Realizar requisição POST
try:
    conn = client.HTTPSConnection(REQUEST_HOST)
    conn.request("POST", REQUEST_PATH, params, headers)

    # Print response data to console
    response = conn.getresponse()
    response = response.read()

    response_json = json.loads(response)
    print('status: {}'.format(response_json['status_code']))
    print(json.dumps(response_json, indent=4))
finally:
    if conn:
        conn.close()
