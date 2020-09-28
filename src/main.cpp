#include <iostream>
#include <curl/curl.h>
#include <ctime>
#include <iomanip>
#include <jansson.h>
#include "hmac.hpp"

#define HTT_GET     0
#define HTT_PUT     1

#define STATUS_ORDER_OPEN       2
#define STATUS_ORDER_CANCELED   3
#define STATUS_ORDER_FILLED     4

using namespace std;

enum Tapi_Method {
    METHOD_TICKER,
    METHOD_LIST_ORDER,
    METHOD_GET_ORDER,
    METHOD_BUY_LIMIT,
    METHOD_SELL_LIMIT,
    METHOD_CANCEL_ORDER,
    METHOD_LIST_TRADES,
    METHOD_LIST_BOOK
};

enum COIN_ID {
    BCH=0, // Bitcoin Cash
    BTC, // Bitcoin
    CHZ, // Chiliz
    ETH, // Ethereum
    LTC, // Litecoin
    PAXG, // PAX Gold
    USDC, // USD Coin
    WBX, // WiBX
    XRP // XRP
};

typedef struct {
    string bch = "BRLBCH"; // Bitcoin Cash
    string btc = "BRLBTC"; // Bitcoin
    string chz = "BRLCHZ"; // Chiliz
    string eth = "BRLETH"; // Ethereum
    string ltc = "BRLLTC"; // Litecoin
    string paxg = "BRLPAXG"; // PAX Gold
    string usdc = "BRLUSDC"; // USD Coin
    string wbx = "BRLWBX"; // WiBX
    string xrp = "BRLXRP"; // XRP
} Coin_Pair_t;

Coin_Pair_t coirPairs;
string currentCoin = coirPairs.btc; // default Bitcoin
string coinStr = "BTC";
COIN_ID curCoinId = BTC;

// Variables
double high, low, last, buy = 0, sell = 0, spread = 0;
double buyPrice, sellPrice;
double gain; // ganho em reais
double spreadTarget; // alvo de ganho
double sellTarget; // preco para ganhar o spreadTarget
string sellTargetStr;
double fee; // comissao em reais
double feeDig; // fee de Venda em moeda digital
double feeDigBuy; // fee de Compra em moeda digital
double minQtd, minQtdBuy, minQtdSell;
uint32_t lastOrderId = 0;
string params;
string buy_limit;
string sell_limit;


// Constantes
const string MB_TAPI_ID = "TAPI-ID: xxxx";
const string MB_TAPI_SECRET = "yyyyy";
const string REQUEST_HOST = "https://www.mercadobitcoin.net";
const string REQUEST_PATH = "/tapi/v3/";

const string BTC_MIN = "0.001";
const string BCH_MIN = "0.001";
const string CHZ_MIN = "500.0";
const string ETH_MIN = "0.001";
const string LTC_MIN = "0.01";
const string PAXG_MIN = "0.005";
const string USDC_MIN = "1.0";
const string WBX_MIN = "1000.0";
const string XRP_MIN = "25";

const double BTC_DECIMAL = 0.01;
const double BCH_DECIMAL = 0.01;
const double CHZ_DECIMAL = 0.0001;
const double ETH_DECIMAL = 0.01;
const double LTC_DECIMAL = 0.01;
const double PAXG_DECIMAL = 0.01;
const double USDC_DECIMAL = 0.0001;
const double WBX_DECIMAL = 0.0001;
const double XRP_DECIMAL = 0.0001;

// Comissao do mercadobitcoin
const double feeTotal = 0.01; // 1% = 0.003(buy) + 0.007(sell);
const double feeLimit = 0.003;
const double feeMarket = 0.007;

template <typename T>
std::string to_string_precision(const T a_value, const int n = 4)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

size_t function_pt(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size*nmemb;
    try
    {
        s->append((char*)contents, newLength);
    }
    catch(std::bad_alloc &e)
    {
        //handle memory problem
        return 0;
    }
    return newLength;
}

string getCoinPair(COIN_ID val)
{
    string coinPair = coirPairs.btc; // default Bitcoin
    switch(val)
    {
        case BCH: // Bitcoin Cash
            coinPair = coirPairs.bch;
            break;
        case BTC: // Bitcoin
            coinPair = coirPairs.btc;
            break;
        case CHZ: // Chiliz
            coinPair = coirPairs.chz;
            break;
        case ETH: // Ethereum
            coinPair = coirPairs.eth;
            break;
        case LTC: // Litecoin
            coinPair = coirPairs.ltc;
            break;
        case PAXG: // PAX Gold
            coinPair = coirPairs.paxg;
            break;
        case USDC: // USD Coin
            coinPair = coirPairs.usdc;
            break;
        case WBX: // WiBX
            coinPair = coirPairs.wbx;
            break;
        case XRP: // XRP
            coinPair = coirPairs.xrp;
            break;
    }
    return coinPair;
}

string getCoinString(COIN_ID val)
{
    string coin = coirPairs.btc; // default Bitcoin
    switch(val)
    {
        case BCH: // Bitcoin Cash
            coin = "BCH";
            break;
        case BTC: // Bitcoin
            coin = "BTC";
            break;
        case CHZ: // Chiliz
            coin = "CHZ";
            break;
        case ETH: // Ethereum
            coin = "ETH";
            break;
        case LTC: // Litecoin
            coin = "LTC";
            break;
        case PAXG: // PAX Gold
            coin = "PAXG";
            break;
        case USDC: // USD Coin
            coin = "USDC";
            break;
        case WBX: // WiBX
            coin = "WBX";
            break;
        case XRP: // XRP
            coin = "XRP";
            break;
    }
    return coin;
}

string getCoinMinOrder(COIN_ID val)
{
    string minVal = BTC_MIN; // default Bitcoin
    switch(val)
    {
        case BCH: // Bitcoin Cash
            minVal = BCH_MIN;
            break;
        case BTC: // Bitcoin
            minVal = BTC_MIN;
            break;
        case CHZ: // Chiliz
            minVal = CHZ_MIN;
            break;
        case ETH: // Ethereum
            minVal = ETH_MIN;
            break;
        case LTC: // Litecoin
            minVal = LTC_MIN;
            break;
        case PAXG: // PAX Gold
            minVal = PAXG_MIN;
            break;
        case USDC: // USD Coin
            minVal = USDC_MIN;
            break;
        case WBX: // WiBX
            minVal = WBX_MIN;
            break;
        case XRP: // XRP
            minVal = XRP_MIN;
            break;
    }
    minQtd = atof(minVal.c_str());
    feeDig = minQtd * feeLimit;
    // min quantity buy
    minQtdBuy = (minQtd * 1.004);
    //minQtdBuy += feeDig;
    feeDigBuy = minQtdBuy * feeLimit;
    //cout << "minQtdBuy = " << minQtdBuy << endl;
    cout << "" << coinStr << " Min buy = " << to_string_precision(minQtdBuy) << endl;
    return minVal;
}

double getCoinDecimal(COIN_ID val)
{
    double coin = BTC_DECIMAL; // default Bitcoin
    switch(val)
    {
        case BCH: // Bitcoin Cash
            coin = BCH_DECIMAL;
            break;
        case BTC: // Bitcoin
            coin = BTC_DECIMAL;
            break;
        case CHZ: // Chiliz
            coin = CHZ_DECIMAL;
            break;
        case ETH: // Ethereum
            coin = ETH_DECIMAL;
            break;
        case LTC: // Litecoin
            coin = LTC_DECIMAL;
            break;
        case PAXG: // PAX Gold
            coin = PAXG_DECIMAL;
            break;
        case USDC: // USD Coin
            coin = USDC_DECIMAL;
            break;
        case WBX: // WiBX
            coin = WBX_DECIMAL;
            break;
        case XRP: // XRP
            coin = XRP_DECIMAL;
            break;
    }
    return coin;
}

string getOrder(uint32_t order_id, string coin)
{
    std::time_t timestamp = std::time(nullptr);
    string param1 = "tapi_method=get_order";
    string param2 = "tapi_nonce=" + to_string(timestamp);
    string param3 = "coin_pair=" + coin;
    string param4 = "order_id=" + to_string(order_id);

    params = param1 + "&" + param2 + "&" + param3 + "&" + param4;

    cout << "params: " << params << endl;

    string message = REQUEST_PATH + "?" + params;

    string output = hmac::get_hmac(MB_TAPI_SECRET, message, hmac::TypeHash::SHA512);

    string result = "TAPI-MAC: " + output;

    return result;
}

string getListOrder(string coin)
{
    std::time_t timestamp = std::time(nullptr);
    string param1 = "tapi_method=list_orders";
    string param2 = "tapi_nonce=" + to_string(timestamp);
    string param3 = "coin_pair=" + coin;

    params = param1 + "&" + param2 + "&" + param3;

    cout << "params: " << params << endl;

    string message = REQUEST_PATH + "?" + params;

    string output = hmac::get_hmac(MB_TAPI_SECRET, message, hmac::TypeHash::SHA512);

    string result = "TAPI-MAC: " + output;

    return result;
}

string getListBook(string coin)
{
    std::time_t timestamp = std::time(nullptr);
    string param1 = "tapi_method=list_orderbook";
    string param2 = "tapi_nonce=" + to_string(timestamp);
    string param3 = "coin_pair=" + coin;
    string param4 = "full=false";

    params = param1 + "&" + param2 + "&" + param3 + "&" + param4;

    cout << "params: " << params << endl;

    string message = REQUEST_PATH + "?" + params;

    string output = hmac::get_hmac(MB_TAPI_SECRET, message, hmac::TypeHash::SHA512);

    string result = "TAPI-MAC: " + output;

    return result;
}

string getListTrades(string coin, int lastHours)
{
    std::time_t t1 = std::time(nullptr);
    std::time_t t2 = t1 - (lastHours*3600); // last N hours

    string result = REQUEST_HOST + "/api/" + coin + "/trades/" + to_string(t2) +
        + "/" + to_string(t1) + "/";

    //cout << "URL= " << result << endl;

    return result;
}

string getBuySellLimit(uint8_t type, string qtd, string price, string coin)
{
    std::time_t timestamp = std::time(nullptr);
    string param1;
    string param2 = "tapi_nonce=" + to_string(timestamp);
    string param3 = "coin_pair=" + coin;
    string param4 = "quantity=" + qtd;
    string param5 = "limit_price=" + price;

    if (type == 0)
        param1 = "tapi_method=place_buy_order";
    else
        param1 = "tapi_method=place_sell_order";

    params = param1 + "&" + param2 + "&" + param3 + "&" + param4 + "&" + param5;

    cout << "params: " << params << endl;

    string message = REQUEST_PATH + "?" + params;

    string output = hmac::get_hmac(MB_TAPI_SECRET, message, hmac::TypeHash::SHA512);

    string result = "TAPI-MAC: " + output;

    return result;
}

string getCancelOrder(uint32_t order_id, string coin)
{
    std::time_t timestamp = std::time(nullptr);
    string param1 = "tapi_method=cancel_order";
    string param2 = "tapi_nonce=" + to_string(timestamp);
    string param3 = "coin_pair=" + coin;
    string param4 = "order_id=" + to_string(order_id);

    params = param1 + "&" + param2 + "&" + param3 + "&" + param4;

    cout << "params: " << params << endl;

    string message = REQUEST_PATH + "?" + params;

    string output = hmac::get_hmac(MB_TAPI_SECRET, message, hmac::TypeHash::SHA512);

    string result = "TAPI-MAC: " + output;

    return result;
}

void parseGetOrderResp(const char *resp)
{
    json_t *root, *obj;
    json_error_t error;

    root = json_loads(resp, 0, &error );
    if (root) {
        json_t *respJson = json_object_get(root, "response_data");
        obj = json_object_get(respJson, "order");

        const char *strText;
        json_t *obj_txt;

        // order_id
        obj_txt = json_object_get(obj, "order_id");
        lastOrderId = json_integer_value(obj_txt);
        printf("order_id = %u\n", lastOrderId);
        // has_fills
        obj_txt = json_object_get(obj, "has_fills");
        bool fills = json_is_true(obj_txt);
        printf("has_fills = %d\n", fills);
        // quantity
        obj_txt = json_object_get(obj, "quantity");
        strText = json_string_value(obj_txt);
        printf("quantity = %s\n", strText);
        // limit_price
        obj_txt = json_object_get(obj, "limit_price");
        strText = json_string_value(obj_txt);
        printf("limit_price = %s\n", strText);
        // executed_quantity
        obj_txt = json_object_get(obj, "executed_quantity");
        strText = json_string_value(obj_txt);
        printf("executed_quantity = %s\n", strText);
        // executed_price_avg
        obj_txt = json_object_get(obj, "executed_price_avg");
        strText = json_string_value(obj_txt);
        printf("executed_price_avg = %s\n", strText);
        // fee
        obj_txt = json_object_get(obj, "fee");
        strText = json_string_value(obj_txt);
        printf("fee = %s\n\n", strText);

        json_decref(root);
    }
}

void parseListOrderResp(const char *resp, size_t total)
{
    json_t *root, *obj, *obj2, *obj3;
    json_error_t error;
    int status = 0;
    size_t j = 0;

    root = json_loads(resp, 0, &error );
    if (root) {
        json_t *respJson = json_object_get(root, "response_data");
        obj = json_object_get(respJson, "orders");

        if (json_array_size(obj) < 1) {
            printf("json_array_size %zu\n", json_array_size(obj));
            json_decref(root);
            return;
        }

        for(size_t i = 0; j < total; i++) {
            const char *strText;
            json_t *obj_item, *obj_txt;

            obj_item = json_array_get(obj, i);
            // status
            obj_txt = json_object_get(obj_item, "status");
            status = json_integer_value(obj_txt);
            if (status == STATUS_ORDER_CANCELED)
                continue;
            j++;
            printf("status %zu = %d\n", i, status);

            // order_id
            obj_txt = json_object_get(obj_item, "order_id");
            lastOrderId = json_integer_value(obj_txt);
            printf("order_id %zu = %u\n", i, lastOrderId);
            // quantity
            obj_txt = json_object_get(obj_item, "quantity");
            strText = json_string_value(obj_txt);
            printf("quantity %zu = %s\n", i, strText);
            // limit_price
            obj_txt = json_object_get(obj_item, "limit_price");
            strText = json_string_value(obj_txt);
            printf("limit_price %zu = %s\n", i, strText);
            // executed_quantity
            obj_txt = json_object_get(obj_item, "executed_quantity");
            strText = json_string_value(obj_txt);
            printf("executed_quantity %zu = %s\n", i, strText);
            // executed_price_avg
            obj_txt = json_object_get(obj_item, "executed_price_avg");
            strText = json_string_value(obj_txt);
            printf("executed_price_avg %zu = %s\n", i, strText);
            // fee
            obj_txt = json_object_get(obj_item, "fee");
            strText = json_string_value(obj_txt);
            printf("fee %zu = %s\n", i, strText);
            // fee rate
            obj2 = json_object_get(obj_item, "operations");
            obj3 = json_array_get(obj2, 0);
            obj_txt = json_object_get(obj3, "fee_rate");
            strText = json_string_value(obj_txt);
            printf("fee rate %zu = %s\n\n", i, strText);
        }
        json_decref(root);
    }
}

void parseTickerResp(const char *resp)
{
    json_t *root, *obj;
    json_error_t error;

    root = json_loads(resp, 0, &error );
    if (root) {

        obj = json_object_get(root, "ticker");
        //
        high = atof(json_string_value(json_object_get(obj, "high")));
        printf("high = %.4f ", high);
        //
        low = atof(json_string_value(json_object_get(obj, "low")));
        printf("low = %.4f ", low);
        //
        last = atof(json_string_value(json_object_get(obj, "last")));
        printf("last = %.4f\n", last);
        //
        sell = atof(json_string_value(json_object_get(obj, "sell")));
        sell_limit = to_string_precision(sell - getCoinDecimal(curCoinId));
        sellPrice = atof(sell_limit.c_str());
        cout << "sell = " << to_string_precision(sell) << ", sell_limit = " << sell_limit << endl;
        //
        buy = atof(json_string_value(json_object_get(obj, "buy")));
        buy_limit = to_string_precision(buy + getCoinDecimal(curCoinId));
        buyPrice = atof(buy_limit.c_str());
        cout << "buy  = " << to_string_precision(buy) << ", buy_limit  = " << buy_limit << endl;
        //
        //cout << "minQtdBuy = " << minQtdBuy << ", feeDigBuy = " << feeDigBuy << endl;
        // spread atual
        spread = sellPrice - buyPrice;
        fee = (feeDigBuy * buyPrice) + (feeDig * sellPrice);
        gain = sellPrice * (minQtd-feeDig) - buyPrice * (minQtdBuy+feeDigBuy);

        printf("spreadCur    = %.4f, gain_spread = R$ %.4f, fee = R$ %.4f\n", spread, gain, fee);
        // spread alvo
        sellTarget = buyPrice + spreadTarget;
        sellTargetStr = to_string_precision(sellTarget);
        //spread = sellTarget - buyPrice;
        fee = (feeDig * sellTarget) + (feeDigBuy * buyPrice);
        gain = sellTarget * (minQtd-feeDig) - buyPrice * (minQtdBuy+feeDigBuy);
        printf("spreadTarget = %.4f, gain_Target = R$ %.4f, fee = R$ %.4f\n", spreadTarget, gain, fee);
        //cout << "sellTarget = " << sellTargetStr << endl;
        json_decref(root);
    }
}

void parseListTrades(const char *resp)
{
    json_t *root, *obj;
    json_error_t error;
    double lastHighSell = 0, lastLowBuy = 1000000000, price = 0;
    string type;

    root = json_loads(resp, 0, &error );
    if (root) {
        for(size_t i = 0; i < json_array_size(root); i++) {
            obj = json_array_get(root, i);
            //
            price = json_real_value(json_object_get(obj, "price"));
            //cout << "price" << i << ": " << to_string_precision(price) << endl;
            type = json_string_value(json_object_get(obj, "type"));
            //cout << "type" << i << ": " << type << endl;
            if (type == "sell") {
                if (price < lastLowBuy)
                    lastLowBuy = price;
            } else if (type == "buy") {
                if (price > lastHighSell)
                    lastHighSell = price;
            }
        }
        cout << "lastHighSell = " << to_string_precision(lastHighSell) << endl;
        cout << "lastLowBuy = " << to_string_precision(lastLowBuy) << endl;
        json_decref(root);
    }
}

void send_request(Tapi_Method reqID)
{
    string tapi_mac;
    string url;
    int method, total = 0;

    switch(reqID)
    {
        case METHOD_TICKER:
            method = HTT_GET;
            url = "https://www.mercadobitcoin.net/api/" + coinStr + "/ticker/";
            break;

        case METHOD_LIST_ORDER:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            tapi_mac = getListOrder(currentCoin);
            break;

        case METHOD_GET_ORDER:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            tapi_mac = getOrder(lastOrderId, currentCoin);
            break;

        case METHOD_BUY_LIMIT:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            tapi_mac = getBuySellLimit(0, to_string_precision(minQtdBuy), buy_limit, currentCoin);
            break;

        case METHOD_SELL_LIMIT:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            if (spreadTarget != 0)
                tapi_mac = getBuySellLimit(1, getCoinMinOrder(curCoinId), sellTargetStr, currentCoin);
            else
                tapi_mac = getBuySellLimit(1, getCoinMinOrder(curCoinId), sell_limit, currentCoin);
            break;

        case METHOD_CANCEL_ORDER:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            tapi_mac = getCancelOrder(lastOrderId, currentCoin);
            break;

        case METHOD_LIST_TRADES:
            method = HTT_GET;
            url = getListTrades(coinStr, 4);
            break;

        case METHOD_LIST_BOOK:
            method = HTT_PUT;
            url = REQUEST_HOST + REQUEST_PATH;
            tapi_mac = getListBook(currentCoin);
            break;

        default:
            cout << "unknow method" << endl;
            break;
    }

    CURL *curl;
    CURLcode res;
    string response;

    /* In windows, this will init the winsock stuff */ 
    curl_global_init(CURL_GLOBAL_ALL);

    //cout << "url: " << url << endl;

    /* get a curl handle */ 
    curl = curl_easy_init();
    struct curl_slist *list = NULL;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        if (method == HTT_PUT) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
            list = curl_slist_append(list, MB_TAPI_ID.c_str());
            list = curl_slist_append(list, tapi_mac.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, function_pt);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_slist_free_all(list);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    //cout << "raw: " << response << endl;

    switch(reqID)
    {
        case METHOD_TICKER:
            parseTickerResp(response.c_str());
            break;

        case METHOD_LIST_ORDER:
            cout << "Total orders: ";
            cin >> total;
            parseListOrderResp(response.c_str(), total);
            break;

        case METHOD_GET_ORDER:
            parseGetOrderResp(response.c_str());
            break;

        case METHOD_BUY_LIMIT:
        case METHOD_SELL_LIMIT:
        case METHOD_CANCEL_ORDER:
            parseGetOrderResp(response.c_str());
            break;

        case METHOD_LIST_TRADES:
            parseListTrades(response.c_str());
            break;

        case METHOD_LIST_BOOK:
            cout << "raw: " << response << endl;
            break;

        default:
            cout << "unknow method parse" << endl;
            break;
    }
}

int main(int argc, char *argv[])
{
    int id;

    cout << "0=BCH, 1=BTC, 2=CHZ, 3=ETH, 4=LTC, 5=PAXG, 6=USDC, 7=WBX, 8=XRP" << endl;
    cout << "Enter coin to operation: ";
    cin >> id;
    curCoinId = static_cast<COIN_ID>(id);
    currentCoin = getCoinPair(curCoinId);
    coinStr = getCoinString(curCoinId);
    getCoinMinOrder(curCoinId);
    spreadTarget = 0;
    cout << "Enter spreadTarget: ";
    cin >> spreadTarget;

    while(1) {
        cout << "0=Ticker, 1=ListOrder, 2=GetOrder, 3=Buy, 4=Sell," \
            "5=CancelOrder, 6=List Trades, 7=List OrderBook" << endl;
        cout << "method: ";
        cin >> id;
        send_request(static_cast<Tapi_Method>(id));
    }

    return 0;
}
