#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Ticker.h>
#include <FS.h>
#include <Arduino.h>

Ticker timer;
WiFiClient wifiClient;

String URL_test = "http://119.29.29.29:80/d?dn=www.zjut.edu.cn";
String file_name = "/Accounts.txt";
const char *AP_NAME_BASE = "Configure Wifi-";
char MAC[20] = {0};
char sta_wifissid[32] = {0};
char sta_wifipassword[64] = {0};
char loginserver[16] = {0};
char netaccount[32] = {0};
char netpassword[32] = {0};
int Netstatus = -1; //-1 无状态 0 连接中 1 连接失败启动配网模式 2 连接成功但无网络 3 连接成功有网络

// 配网页面代码
const char *page_html = "\
    <!DOCTYPE html>\r\n\
    <html lang='en'>\r\n\
    <head>\r\n\
    <meta charset='UTF-8'>\r\n\
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n\
    <title>Document</title>\r\n\
    </head>\r\n\
    <body>\r\n\
    <form name='input' action='/' method='POST'>\r\n\
            wifi名称: <br>\r\n\
            <input type='text' name='wifissid'><br>\r\n\
            wifi密码:<br>\r\n\
            <input type='text' name='wifipassword'><br>\r\n\
            登录服务器IP:<br>\r\n\
            <input type='text' name='loginserver'><br>\r\n\
            校园网账号:<br>\r\n\
            <input type='text' name='netaccount'><br>\r\n\
            校园网密码:<br>\r\n\
            <input type='text' name='netpassword'><br>\r\n\
            注：留空选项会自动使用最近一次提交的信息 <br>\r\n\
            <input type='submit' value='保存'>\r\n\
        </form>\r\n\
    </body>\r\n\
    </html>\r\n\
    ";

const byte DNS_PORT = 53;        // DNS端口号
IPAddress apIP(192, 168, 10, 1); // esp8266-AP-IP地址
DNSServer dnsServer;             // 创建dnsServer实例
ESP8266WebServer server(80);     // 创建WebServer

void shout_data()
{
    Serial.print("sta_wifissid:");
    Serial.println(sta_wifissid);
    Serial.print("sta_wifipassword:");
    Serial.println(sta_wifipassword);
    Serial.print("loginserver:");
    Serial.println(loginserver);
    Serial.print("netaccount:");
    Serial.println(netaccount);
    Serial.print("netpassword:");
    Serial.println(netpassword);
}

void save_data()
{ // 保存数据
    if (SPIFFS.begin())
    {
        Serial.println("SPIFFS Started");
    }
    else
    {
        Serial.println("SPIFFS Failed to Start");
    }
    SPIFFS.format(); // 格式化SPIFFS
    Serial.println("SPIFFS format finish");
    File dataFile = SPIFFS.open(file_name, "w");
    dataFile.println(sta_wifissid);     // 向文件内写入sta_wifissid
    dataFile.println(sta_wifipassword); // 向文件内写入sta_wifipassword
    dataFile.println(loginserver);      // 向文件内写入loginserver
    dataFile.println(netaccount);       // 向文件内写入netaccount
    dataFile.println(netpassword);      // 向文件内写入netpassword
    dataFile.close();
    Serial.println("Finished Appended data to SPIFFS");
}

void read_data()
{
    char *data[5] = {sta_wifissid, sta_wifipassword, loginserver, netaccount, netpassword};
    char line[64] = {0};
    char now;

    Serial.println("");

    if (SPIFFS.begin())
    {
        Serial.println("SPIFFS Started");
    }
    else
    {
        Serial.println("SPIFFS Failed to Start");
    }

    if (SPIFFS.exists(file_name))
    {
        Serial.print(file_name);
        Serial.println(" FOUND");

        File dataFile = SPIFFS.open(file_name, "r"); // 读取文件内容且保存

        for (int i = 0, j = 0, k = 0; i < dataFile.size(); i++, j++)
        {
            now = (char)dataFile.read();
            if (now == '\n')
            {
                k++;
                j = -1;
            }
            else
            {
                if (now == 0X0D)
                {
                    data[k][j] = 0X00;
                }
                else
                {
                    data[k][j] = now;
                }
            }
        }

        dataFile.close();
        shout_data();
    }
    else
    {
        Serial.print(file_name);
        Serial.println(" NOT FOUND");
        save_data();
    }
}

void handleRoot()
{ // 访问主页回调函数
    server.send(200, "text/html", page_html);
}

void handleRootPost()
{ // Post回调函数
    Serial.println("handleRootPost");
    if (server.hasArg("wifissid"))
    { // 判断是否有账号参数
        if (server.arg("wifissid") != "")
        {
            Serial.print("got wifissid:");
            strcpy(sta_wifissid, server.arg("wifissid").c_str()); // 将账号参数拷贝到sta_ssid中
            Serial.println(sta_wifissid);
        }
    }
    else
    { // 没有参数
        Serial.println("error, not found wifissid");
        server.send(200, "text/html", "<meta charset='UTF-8'>error, not found wifissid"); // 返回错误页面
        return;
    }
    // 密码
    if (server.hasArg("wifipassword"))
    {
        if (server.arg("wifipassword") != "")
        {
            Serial.print("got wifipassword:");
            strcpy(sta_wifipassword, server.arg("wifipassword").c_str());
            Serial.println(sta_wifipassword);
        }
    }
    else
    {
        Serial.println("error, not found wifipassword");
        server.send(200, "text/html", "<meta charset='UTF-8'>error, not found wifipassword");
        return;
    }
    // 登录服务器
    if (server.hasArg("loginserver"))
    {
        if (server.arg("loginserver") != "")
        {
            Serial.print("got loginserver:");
            strcpy(loginserver, server.arg("loginserver").c_str());
            Serial.println(loginserver);
        }
    }
    else
    {
        Serial.println("error, not found loginserver");
        server.send(200, "text/html", "<meta charset='UTF-8'>error, not found loginserver");
        return;
    }
    // 校园网账号
    if (server.hasArg("netaccount"))
    {
        if (server.arg("netaccount") != "")
        {
            Serial.print("got netaccount:");
            strcpy(netaccount, server.arg("netaccount").c_str());
            Serial.println(netaccount);
        }
    }
    else
    {
        Serial.println("error, not found netaccount");
        server.send(200, "text/html", "<meta charset='UTF-8'>error, not found netaccount");
        return;
    }
    // 校园网密码
    if (server.hasArg("netpassword"))
    {
        if (server.arg("netpassword") != "")
        {
            Serial.print("got netpassword:");
            strcpy(netpassword, server.arg("netpassword").c_str());
            Serial.println(netpassword);
        }
    }
    else
    {
        Serial.println("error, not found netpassword");
        server.send(200, "text/html", "<meta charset='UTF-8'>error, not found netpassword");
        return;
    }

    server.send(200, "text/html", "<meta charset='UTF-8'>保存成功"); // 返回保存成功页面

    save_data();

    Serial.println("");
    Serial.println("Now data:");
    shout_data();
    Serial.println("");

    delay(2000);
    // 连接wifi
    connectNewWifi();
}

void initBasic(void)
{ // 初始化基础
    Serial.begin(115200);
    WiFi.hostname("zjutNetAutoLoginDevice"); // 设置ESP8266设备名
    timer.attach(60, teststatus);
}

void initSoftAP(void)
{ // 初始化AP模式
    WiFi.mode(WIFI_AP);
    WiFi.hostname("zjutNetAutoLoginDevice");
    strcpy(MAC, WiFi.macAddress().c_str());
    char *AP_NAME = new char[strlen(AP_NAME_BASE) + strlen(MAC)];
    strcpy(AP_NAME, AP_NAME_BASE);
    strcat(AP_NAME, MAC);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    if (WiFi.softAP(AP_NAME))
    {
        Serial.println("ESP8266 SoftAP is right");
        Serial.println("AP NAME is");
        Serial.println(AP_NAME);
    }
}

void initWebServer(void)
{ // 初始化WebServer
    // server.on("/",handleRoot);
    // 上面那行必须以下面这种格式去写否则无法强制门户
    server.on("/", HTTP_GET, handleRoot);      // 设置主页回调函数
    server.onNotFound(handleRoot);             // 设置无法响应的http请求的回调函数
    server.on("/", HTTP_POST, handleRootPost); // 设置Post请求回调函数
    server.begin();                            // 启动WebServer
    Serial.println("WebServer started!");
}

void initDNS(void)
{ // 初始化DNS服务器
    if (dnsServer.start(DNS_PORT, "*", apIP))
    { // 判断将所有地址映射到esp8266的ip上是否成功
        Serial.println("start dnsserver success.");
    }
    else
        Serial.println("start dnsserver failed.");
}

void connectNewWifi(void)
{
    Netstatus = 0;
    WiFi.mode(WIFI_STA); // 切换为STA模式
    WiFi.hostname("zjutNetAutoLoginDevice");
    WiFi.setAutoConnect(true);                  // 设置自动连接
    WiFi.begin(sta_wifissid, sta_wifipassword); // 连接上一次连接成功的wifi
    Serial.println("");
    Serial.print("Connect to wifi");
    int count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        count++;
        if (count > 20)
        { // 如果10秒内没有连上，就开启Web配网 可适当调整这个时间
            Netstatus = 1;
            Serial.println("");
            initSoftAP();
            initWebServer();
            initDNS();
            break; // 跳出 防止无限初始化
        }
        Serial.print(".");
    }
    Serial.println("");
    if (WiFi.status() == WL_CONNECTED)
    { // 如果连接上 就输出IP信息 防止未连接上break后会误输出
        Netstatus = 2;
        Serial.println("WIFI Connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP()); // 打印esp8266的IP地址
        server.stop();
        login();
    }
}

void login()
{
    Serial.println("login!");
    Netstatus = 3;
}

bool testNet()
{
    String responsePayload = "";

    Serial.println("");

    Serial.println("Testing the Internet");

    // 创建 HTTPClient 对象
    HTTPClient httpClient;

    // 配置请求地址。此处也可以不使用端口号和PATH而单纯的
    httpClient.begin(wifiClient, URL_test.c_str());
    Serial.print("URL: ");
    Serial.println(URL_test);

    // 启动连接并发送HTTP请求
    int httpCode = httpClient.GET();
    Serial.print("Send GET request to URL: ");
    Serial.println(URL_test);

    // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    // 如果服务器不响应OK则将服务器响应状态码通过串口输出
    if (httpCode == HTTP_CODE_OK)
    {
        responsePayload = httpClient.getString();
        Serial.println("Server Response Payload: ");
        Serial.println(responsePayload);
    }
    else
    {
        Serial.print("Server Respose Code:");
        Serial.println(httpCode);
        httpClient.end();
        return false;
    }

    // 关闭ESP8266与服务器连接
    httpClient.end();

    if (responsePayload == "60.191.28.6")
    {
        return true;
    }
    else
    {
        return false;
    }
}

void teststatus()
{
    Serial.print("Netstatus:");
    Serial.println(String(Netstatus));
    if (WiFi.status() == WL_CONNECTED)
    { // 如果连接上 就输出IP信息 防止未连接上break后会误输出
        Netstatus = 2;
        if (testNet())
        {
            Netstatus = 3;
        }
        else
        {
            login();
            delay(1000);
            if (testNet())
            {
                Netstatus = 3;
            }
        }
    }
    else
    {
        connectNewWifi();
    }
}

void setup()
{
    initBasic();
    read_data();
    connectNewWifi();
}

void loop()
{
    server.handleClient();
    dnsServer.processNextRequest();
}
