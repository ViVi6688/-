#include <ESP8266WiFi.h>//A0 MAX9814 D2 WS2812控制口
#include <FastLED.h>

#define NOISE 40               //噪音底线
#define LED_PIN D6             // WS2812数据引脚
#define MIC A0                 // MAX9 814数据引脚
#define NUM_LEDS 30            // WS2812数量
#define LED_TYPE WS2812B       // WS2812型号

CRGB leds[NUM_LEDS];

#define server_ip "bemfa.com"   //巴法云服务器地址默认即可
#define server_port "8344"      //服务器端口，tcp创客云端口8344

#define wifi_name  "www"                          //WIFI名称，区分大小写，不要写错
#define wifi_password   "88888888"                //WIFI密码
String UID = "c7602c3a1381826031bc1a5496967e8c";  //用户私钥，可在控制台获取,修改为自己的UID
String TOPIC = "WSB2812";                         //主题名字，可在控制台新建

#define MAX_PACKETSIZE 512      //最大字节数
#define KEEPALIVEATIME 60*1000  //设置心跳值60s

WiFiClient TCPclient;           //tcp客户端相关初始化，默认即可
String TcpClient_Buff = "";     //初始化字符串，用于接收服务器发来的数据
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;      //心跳
unsigned long preTCPStartTick = 0;   //连接
bool preTCPConnected = false;

//相关函数初始化
//连接WIFI
void doWiFiTick();//此函数主要用于确保设备在Wi-Fi环境下能够连接到互联网并与服务器进行通信。在执行主程序之前，通常会先执行此函数来确保设备能够正常连接到互联网。
void startSTA();  //启动ESP8266的STA模式，以连接到指定的WiFi网络。

//TCP初始化连接

/*检查WiFi连接是否正常，若不正常则退出函数。如果连接正常，则检查TCP连接是否断开，若断开则尝试重连。
  重连前会记录当前时间，并在等待一定时间后再次尝试连接。如果TCP连接正常，则会接收从TCP服务器发送过来的数据，并存入缓冲区中。
  如果缓冲区中有数据，并且超过了一定时间没有新的数据，则会清空缓冲区，并根据接收到的消息内容进行相应的操作。*/
void doTCPClientTick();

//该函数的作用是用于连接TCP服务器，发送订阅指令并设置TCP通信不延迟。
void startTCPClient();

//将指定的数据字符串发送到TCP服务器
void sendtoTCPServer(String p);

//控制函数，具体函数内容见下方
void red();
void orange();
void yellow();
void green();
void blue();
void indigo();
void purple();

int zhouqi=0.1;//持续时间
int liangdu=255;//亮度
int MODE=0;//模式
int bianhua=10;//变化周期

/*
  *发送数据到TCP服务器
 */
void sendtoTCPServer(String p){
  if (!TCPclient.connected())                //检查 TCP 客户端是否已连接到服务器 
  {
    Serial.println("Client is not readly");  //未连接，则输出提示信息
    return;
  }
  TCPclient.print(p);                        //如果已连接将数据发送到TCP服务器
  preHeartTick = millis();                   //记录当前时间为心跳计时的开始时间，用于计算心跳间隔时间
}


/*
  *初始化和服务器建立连接
*/
void startTCPClient(){
  if(TCPclient.connect(server_ip, atoi(server_port))){  //尝试连接到TCP服务器
    Serial.print("\nConnected to server:");             //连接成功，输出提示信息
    Serial.printf("%s:%d\r\n",server_ip,atoi(server_port));
    
    String tcpTemp="";  //初始化字符串
    tcpTemp = "cmd=1&uid="+UID+"&topic="+TOPIC+"\r\n";  //构建订阅指令
    sendtoTCPServer(tcpTemp);                           //发送订阅指令到服务器端
    tcpTemp="";//清空
    preTCPConnected = true;                             //设置TCP连接标志为true
    TCPclient.setNoDelay(true);                         //设置TCP通信不延迟
  }
  else{
    Serial.print("Failed connected to server:");        //若连接失败，则输出失败信息
    Serial.println(server_ip);
    TCPclient.stop();                                   //停止TCP连接
    preTCPConnected = false;                            //设置TCP连接标志为false
  }
  preTCPStartTick = millis();                           //获取当前时间，用于计算重连时间
}


/*
  *检查数据，发送心跳
*/
void doTCPClientTick(){
 //检查是否断开，断开后重连
   if(WiFi.status() != WL_CONNECTED) return; //判断WiFi连接是否正常，若不正常则退出函数
  if (!TCPclient.connected()) {              //断开重连
  if(preTCPConnected == true){               //若之前已经连接成功
    preTCPConnected = false;                 //TCP连接标志置为false
    preTCPStartTick = millis();              //记录当前时间，用于计算重连时间
    Serial.println(); 
    Serial.println("TCP Client disconnected.");
    TCPclient.stop();
  }
  else if(millis() - preTCPStartTick > 1*1000) //若已经等待了1秒，重连
    startTCPClient();
  }  
  //如果连接正常，则检查TCP连接是否断开，若断开则尝试重连。重连前会记录当前时间，并在等待一定时间后再次尝试连接。
  else
  {
    if (TCPclient.available()) {  //收数据
      char c =TCPclient.read();   //读取一个字节的数据
      TcpClient_Buff +=c;         //将数据存入TcpClient_Buff字符串中
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();
      
      if(TcpClient_BuffIndex>=MAX_PACKETSIZE - 1){
        TcpClient_BuffIndex = MAX_PACKETSIZE-2;
        TcpClient_preTick = TcpClient_preTick - 200;
      }
  //如果TCP连接正常，则会接收从TCP服务器发送过来的数据，并存入缓冲区中。
    }
    if(millis() - preHeartTick >= KEEPALIVEATIME){//保持心跳
      preHeartTick = millis();  //记录当前时间
      Serial.println("--Keep alive:");
      sendtoTCPServer("ping\r\n"); //发送心跳，指令需\r\n结尾，详见接入文档介绍
    }
  }
  if((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick>=200))  //若TcpClient_Buff中有数据，且已经超过200ms未接收到新的数据
  {
    TCPclient.flush(); //清空TCP缓存区
    Serial.print("Rev string: ");
    TcpClient_Buff.trim(); //去掉首位空格
    Serial.println(TcpClient_Buff); //打印接收到的消息
    String getTopic = "";
    String getMsg = "";
    //如果缓冲区中有数据，并且超过了一定时间没有新的数据，则会清空缓冲区，并根据接收到的消息内容进行相应的操作。
    if(TcpClient_Buff.length() > 15){//注意TcpClient_Buff只是个字符串，在上面开头做了初始化 String TcpClient_Buff = "";
          //此时会收到推送的指令，指令大概为 cmd=2&uid=xxx&topic=light002&msg=off
          int topicIndex = TcpClient_Buff.indexOf("&topic=")+7; //c语言字符串查找，查找&topic=位置，并移动7位
          int msgIndex = TcpClient_Buff.indexOf("&msg=");//c语言字符串查找，查找&msg=位置
          getTopic = TcpClient_Buff.substring(topicIndex,msgIndex);//c语言字符串截取，截取到topic
          getMsg = TcpClient_Buff.substring(msgIndex+5);//c语言字符串截取，截取到消息
          Serial.print("topic:------");
          Serial.println(getTopic); //打印截取到的主题值
          Serial.print("msg:--------");
          Serial.println(getMsg);   //打印截取到的消息值
   }
   if(getMsg  == "on"){      
     Serial.println("连接");
   }else if(getMsg == "off"){ 
     Serial.println("断开");
    }
   else if(getMsg == "auto"){ 
    MODE=0;
    }
   else if(getMsg == "White"){ 
    MODE=1;
    }
   else if(getMsg == "yansered"){ 
    while(!TCPclient.available())
     red();
     MODE=2;
    }
   else if(getMsg == "yanseorange"){ 
    while(!TCPclient.available())
     orange();
     MODE=3;
    }
   else if(getMsg == "yanseyellow"){ 
    while(!TCPclient.available())
     yellow();
     MODE=4;
    }
   else if(getMsg == "yansegreen"){ 
    while(!TCPclient.available())
     green();
     MODE=5;
    }
   else if(getMsg == "yanseblue"){ 
    while(!TCPclient.available())
     blue();
     MODE=6;
    }
   else if(getMsg == "yanseindigo"){ 
    while(!TCPclient.available())
      indigo();
      MODE=7;
    }
   else if(getMsg == "yansepurple"){ 
    while(!TCPclient.available())
      purple();
      MODE=8;
    }
   else if(getMsg == "liangdu1"){ 
      liangdu-=30;
        if(liangdu<0){
          liangdu=0;
          fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0)); // 设置所有LED灯为黑色
          FastLED.show(); // 更新LED灯
          }
          Serial.print("当前亮度值：");
          Serial.println(liangdu);
    }
   else if(getMsg == "liangdu2"){ 
      if(liangdu<225){
        liangdu=liangdu+30;
        }else{
          liangdu=255;
          }
          Serial.print("当前亮度值：");
          Serial.println(liangdu);
    }
   else if(getMsg == "xiaoguo1"){ 
    while(!TCPclient.available())
     caihong();
    }
   else if(getMsg == "xiaoguo2"){ 
    while(!TCPclient.available())
      suiji();
    }

   else if(getMsg == "zhouqi1"){ 
      zhouqi-=3;
      if(zhouqi<0){
          zhouqi=0.1;
    }
          Serial.print("当前持续时间值：");
          Serial.println(zhouqi);
   }
   else if(getMsg == "zhouqi2"){ 
      zhouqi+=3;
       Serial.print("当前持续时间值：");
       Serial.println(zhouqi);
    }
   else if(getMsg == "bianhua1"){ 
      bianhua-=3;
      if(bianhua<0){
          bianhua=0.1;
    }
          Serial.print("当前变化周期值：");
          Serial.println(bianhua);
   }
   else if(getMsg == "bianhua2"){ 
      bianhua+=3;
       Serial.print("当前变化周期值：");
       Serial.println(bianhua);
    }


   TcpClient_Buff="";
   TcpClient_BuffIndex = 0;
  }
}
/*
  *初始化wifi连接
*/
void startSTA(){
  WiFi.disconnect();  // 断开WIFI连接
  WiFi.mode(WIFI_STA);  // 设置为WIFI_STA模式
  WiFi.begin(wifi_name, wifi_password);  //连接指定WIFI网络，传入参数为网络名和密码
}


void doWiFiTick(){
  static bool startSTAFlag = false;       // 静态变量，用于记录是否启动了STA模式
  static bool taskStarted = false;        // 静态变量，用于记录任务是否已经开始执行
  static uint32_t lastWiFiCheckTick = 0;  // 静态变量，用于记录上次检查WIFI连接状态的时间戳

  //如果还没有启动STA模式,将启动STA模式的标志设为true, 调用startSTA函数启动STA模式
  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
  }

  //检查Wi-Fi连接状态。如果未连接且距离上次检查时间超过1秒，则更新上次检查时间。
  if ( WiFi.status() != WL_CONNECTED ) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  }
  
  /*如果Wi-Fi已连接，且任务尚未启动，则启动任务。任务的具体操作
   是打印本地IP地址，并调用startTCPClient函数建立TCP连接。*/
  else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");  
      Serial.println(WiFi.localIP());  // 打印本地IP地址
      startTCPClient();                //调用startTCPClient函数建立TCP连接
    }
  }
}



// 初始化，相当于main 函数
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS);  // 初始化WS2812
  FastLED.setBrightness(50);       // 设置亮度      
  Serial.begin(115200);            // 初始化串口
  pinMode(MIC,INPUT);              // 将MIC引脚设为输入模式
  Serial.println("Beginning...");  // 打印字符串
}

  
//循环
void loop() {
  doWiFiTick();
  doTCPClientTick();
  if(MODE==0)
  {
    int level = analogRead(MIC);  // 读取声音水平

  // 将声音水平映射到LED亮度值和颜色变化速度
  int brightness = map(level, 0, 1023, 0, 255);//将声音水平映射到LED亮度值
  int speed = map(level, 0, 1023, 1, 50);// 将声音水平映射到颜色变化速度

  // 根据声音水平和颜色变化速度生成颜色序列
  uint8_t hue = 0;                       //定义颜色起始值为0
  for (int i = 0; i <NUM_LEDS ; i++) {
    if(TCPclient.available())break;
    leds[i] = CHSV(hue, 255, brightness);//设置颜色为hue，亮度为brightness，饱和度为255
    hue += speed;
  }

  // 更新LED显示
  FastLED.show();

  // 延时一段时间，持续时间与声音水平成反比
  int duration = map(level, 0, 1023, 100, 10);//将声音水平映射到持续时间
  delay(duration);

  // 打印声音水平和持续时间到串口监视器
  /*Serial.print("Level: ");
  Serial.print(level);
  Serial.print(", Duration: ");
  Serial.println(duration);*/
  
    }
  else if(MODE==1)
  {
    for(int i=0; i<NUM_LEDS; i++) { 
      leds[i] = CRGB::Black;
      FastLED.show();    
      if(TCPclient.available())break;
    }}
}


void red() { //红色（255,0,0）
  for (int light1 = 0; light1 < liangdu; light1 += 10) {  //循环变量light1从0开始，每次增加10，直到达到亮度值liangdu
    FastLED.setBrightness(light1); //设置当前灯带亮度
    delay(zhouqi); //延时一段时间
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0)); // 设置所有LED灯为红色
    FastLED.show(); // 更新LED灯
    if (TCPclient.available()) break;  // 如果接收到TCP客户端发送数据，则跳出循环
  } 
  
  for (int light2 = liangdu; light2 > 0; light2 -= 10) { //循环变量light2从亮度值liangdu开始，每次减少10，直到达到0
    FastLED.setBrightness(light2); // 设置亮度值
    delay(zhouqi); //延时一段时间
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0)); // 设置所有LED灯为红色
    FastLED.show();
    if (TCPclient.available()) break;
  }  
}

void orange() { //橘色（255,165,0）
  for (int light1 = 0; light1 < liangdu; light1 += 10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(255, 165, 0));
    FastLED.show();
    if (TCPclient.available()) break;
  }
  for (int light2 = liangdu; light2 > 0; light2 -= 10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(255, 165, 0));
    FastLED.show();
    if (TCPclient.available()) break;
  }
}

void yellow() { //绿色（255,255,0）
  for (int light1 = 0; light1 < liangdu; light1 += 10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(255, 255, 0)); // 设置所有LED为黄色
    FastLED.show(); // 更新LED显示
    if (TCPclient.available()) break;
  }
  for (int light2 = liangdu; light2 > 0; light2 -= 10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(255, 255, 0)); // 设置所有LED为黄色
    FastLED.show(); // 更新LED显示
    if (TCPclient.available()) break;
  }
}

void green() { //绿色（0,255,0）
  for(int light1=0; light1<liangdu; light1+=10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(0, 255, 0));
    FastLED.show();
    if(TCPclient.available()) break;
  }
  for(int light2=liangdu; light2>0; light2-=10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(0, 255, 0));
    FastLED.show();
    if(TCPclient.available()) break;
  }
}

void blue() { //蓝色（0,0,255）
  for(int light1=0; light1<liangdu; light1+=10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 255));
    FastLED.show();
    if(TCPclient.available()) break;
  }
  for(int light2=liangdu; light2>0; light2-=10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 255));
    FastLED.show();
    if(TCPclient.available()) break;
  }
}

void indigo() { //靛色（75,0,130）
  for(int light1=0; light1<liangdu; light1+=10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(75, 0, 130));
    FastLED.show();
    if(TCPclient.available()) break;
  }
  for(int light2=liangdu; light2>0; light2-=10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(75, 0, 130));
    FastLED.show();
    if(TCPclient.available()) break;
  }
}

void purple() { //紫色（128,0,128）
  for(int light1=0; light1<liangdu; light1+=10) {
    FastLED.setBrightness(light1);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(128, 0, 128));
    FastLED.show();
    if(TCPclient.available()) break;
  }
  for(int light2=liangdu; light2>0; light2-=10) {
    FastLED.setBrightness(light2);
    delay(zhouqi);
    fill_solid(leds, NUM_LEDS, CRGB(128, 0, 128));
    FastLED.show();
    if(TCPclient.available()) break;
  }
}

void caihong(){
 static uint8_t hue = 0; // 色相变量，初始值为0
  for(int i = 0; i < NUM_LEDS; i++) { // 遍历所有灯珠
    leds[i] = CHSV(hue + ((NUM_LEDS - i) * 255 / NUM_LEDS), 255, 255); // 设置每个灯珠的颜色
  }
  FastLED.show(); // 更新LED灯
  FastLED.delay(bianhua); // 延时一段时间
  hue++; // 色相增加
}

void suiji(){   // 随机色闪烁
  for (int i = 0; i < NUM_LEDS; i++) {   // 对每一个灯珠进行闪烁处理
    if (random(10) == 0) {  // 以10%的概率进行闪烁
      leds[i] =  CHSV(random(255), 255, 255); // 随机色调的颜色
    } else {
      leds[i] = CRGB::Black; // 灯珠熄灭
    }
  }
  FastLED.show(); // 更新LED灯
  delay(bianhua); // 延时一段时间
 }
