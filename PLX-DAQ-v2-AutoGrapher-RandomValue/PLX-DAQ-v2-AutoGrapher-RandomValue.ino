#include <LWiFi.h>//引入WiFi連線函式庫標題檔
#include "MCS.h"//引入MCS雲端連線函式庫標題檔
#include "Wire.h"//引入序列埠通訊函式庫標題檔
#include <Ultrasonic.h>//引入超音波函式庫標題檔
#include <OneWire.h>//引入水溫感測器函式庫標題檔
#include <DallasTemperature.h>//引入水溫感測器函式庫標題檔
#include <Servo.h>//引入伺服馬達函式庫標題檔

#define SCOUNT  10//採樣數
#define sec    5//水位抽水進水時間5秒
#define badsec    17//沒達標準抽水進水時間1分鐘

Servo __myservo9;
OneWire oneWire(5);//初始化腳位連接水溫感測器
DallasTemperature sensors(&oneWire);
Ultrasonic ultrasonic(3, 4);//宣告超音波接腳
MCSLiteDevice mcs("BJh4vkWhn", "ea440c22ea440c229eaf93f69eaf93f6d70c09b4ea440c22ed9190259eaf93f6", "192.168.62.111", 3000);
//取得MCS雲端ID與Key與IP位址和埠做連線
MCSDisplayFloat PHFloat("PHFloat");
MCSDisplayFloat VoltageFloat("VoltageFloat");
MCSDisplayFloat TempFloat("TempFloat");
MCSDisplayFloat TdsValueFloat("TdsValueFloat");
String lineToken = "CxxrvUsETJXJFnW8VHbimX81U9WTabXaFnMgQei6DoP";

int angle=0, shake=0;
int Delay = 15;//開始延遲
float averageVoltage = 0, tdsValue = 0,dist=0;//電壓值與TDS
float pHArray[SCOUNT];   //Store the average value of the sensor feedback
int pHArrayIndex = 0;
char _lwifi_ssid[] = "xixa3333";//WiFi帳號
char _lwifi_pass[] = "asdfghjkl";//WiFi帳號
float voltage, PH, temp;//濁度、PH、水溫
int level = 0, Level_Flag = 0, PumpCount = 0, PumpCount2 = 0, LimeFlag = 0, ChangeFlag = 0;//水位等級、旗標、抽水馬達計時器

void sendLineMsg(String myMsg) {//LINE連接與傳遞訊息
    static TLSClient line_client;//創建LINE的區域變量
    myMsg.replace("%", "%25");//轉換字符
    myMsg.replace("&", "%26");
    myMsg.replace("§", "&");
    myMsg.replace("\\n", "\n");
    if (line_client.connect("notify-api.line.me", 443)) {//判斷LINE是否連接成功
        line_client.println("POST /api/notify HTTP/1.1");//傳遞給LINE資料
        line_client.println("Connection: close");
        line_client.println("Host: notify-api.line.me");
        line_client.println("Authorization: Bearer " + lineToken);
        line_client.println("Content-Type: application/x-www-form-urlencoded");
        line_client.println("Content-Length: " + String(myMsg.length()));
        line_client.println();
        line_client.println(myMsg);
        line_client.println();
        line_client.stop();//停止傳遞
    }
    else Serial.println("Line Notify failed");
}

void camera(String msg, int pack, int id){
  sendLineMsg(String("message=\n") + msg + "§stickerPackageId=" + pack + "§stickerId=" + id);
    //傳遞訊息及貼圖至LINE
    digitalWrite(11, HIGH);
    delay(100);
    digitalWrite(11, LOW);
}

void LED(int Y, int R) {//LED亮燈與MCS顯示副程式
    digitalWrite(7, Y);
    digitalWrite(10, R);
    Level_Flag = level;
}

void linemsg(String msg, int pack, int id, int sec2) {//傳LINE副程式
    camera(msg,pack,id);
    PumpCount2 = sec2;
    digitalWrite(13, HIGH);//傳遞高電位給海水抽水馬達
}

void linemsg2(String msg, int pack, int id, int sec2) {//第二傳LINE副程式
    camera(msg,pack,id);
    PumpCount = sec2;
    digitalWrite(12, HIGH);//傳遞高電位給魚塭抽水馬達
}

int RotationAngle(int Angle){
  __myservo9.write(Angle);
  Serial.println(Angle);
  delay(100);
}

float getMedianNum(float bArray[], int iFilterLen) //取中位數
{
      float bTemp=0;
      for (int i = 0; i < iFilterLen; i++)bTemp = bTemp+bArray[i];
      bTemp=bTemp/iFilterLen;
      return bTemp;
}

void setup()
{
    Serial.begin(9600);//序列埠初始化
    Serial.println("Wifi連線中");//傳遞訊息至序列埠並換行
    while (WiFi.begin(_lwifi_ssid, _lwifi_pass) != WL_CONNECTED) { delay(1000); }
    //判斷WiFi是否連線成功
    Serial.println("Wifi連線完成");
    Serial.println("MCS連線中");
    while (!mcs.connected()) { mcs.connect(); }//判斷MCS是否連線成功
    Serial.println("MCS連線完成");
    mcs.addChannel(PHFloat);
    mcs.addChannel(VoltageFloat);
    mcs.addChannel(TempFloat);
    mcs.addChannel(TdsValueFloat);
    sensors.begin();
    pinMode(A0, INPUT);//初始化腳位連接濁度感測器
    pinMode(A1, INPUT);//初始化腳位連接PH感測器
    pinMode(7, OUTPUT);//初始化腳位連接黃燈
    pinMode(10, OUTPUT);//初始化腳位連接紅燈
    pinMode(11, OUTPUT);//初始化腳位連接ESP32-CAM
    pinMode(12, OUTPUT);//初始化腳位連接魚塭抽水馬達
    pinMode(13, OUTPUT);//初始化腳位連接海水抽水馬達
    pinMode(A2, INPUT);//初始化腳位連接TDS感測器
    __myservo9.attach(9, 500, 2500);//初始化腳位連接伺服馬達
    Serial.println("CLEARDATA");
    Serial.println("LABEL,Date,Time,Millis,PH,voltage,temp,TDS");
    int rndseed = Serial.readStringUntil(10).toInt();
    Serial.println( (String) "Got random value '" + rndseed + "' from Excel" );
    randomSeed(rndseed);
}

void loop()
{
    while (!mcs.connected()) {
        mcs.connect();//與MCS同步
        if (mcs.connected()) { Serial.println("MCS Reconnected."); }
        //判斷MCS是否連線成功
    }
    mcs.process(100);
    
    if(Delay==9)for(int i=180;i>=0;i-=10) RotationAngle(i);
    RotationAngle(0);
    voltage = (float)analogRead(A0) / 1300;//讀取並計算濁度感測器的值
    sensors.requestTemperatures();//計算水溫值
    temp = sensors.getTempCByIndex(0);//讀取水溫值
    
    averageVoltage = analogRead(A2) * 5.0 / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temp - 25.0);
    float compensationVolatge = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * pow(compensationVolatge, 3) - 255.86 * pow(compensationVolatge, 2) + 857.39 * compensationVolatge) * 0.5;
    
    PH=6.86+(2180-(float)analogRead(A1))*0.00782;
    Serial.println(String() + "PH值：" + PH);
    pHArray[pHArrayIndex++] =PH;
    pHArrayIndex=pHArrayIndex%SCOUNT;
    PH = getMedianNum(pHArray,SCOUNT);
    dist = ultrasonic.read();//超音波距離
    Serial.println(String() + "距離：" + dist);
    Serial.println(String() + "PH值2：" + PH);
    Serial.println(String() + "濁度值：" + voltage);
    Serial.println(String() + "水溫值：" + temp);
    Serial.println(String() + "導電率：" + tdsValue);
    Serial.println( (String) "DATA,DATE,TIME," + millis() + "," + PH+"," +voltage+"," +temp+"," +tdsValue);
    PHFloat.set(PH);
    VoltageFloat.set(voltage);
    TempFloat.set(temp);
    TdsValueFloat.set(tdsValue);
    
    if (PumpCount != 0) {
        PumpCount -= 1;//魚溫抽水馬達計時器倒數
        if (PumpCount == 0) {
            digitalWrite(12, LOW);//傳遞低電位給魚溫抽水馬達
            if (ChangeFlag-- == 1) {
                digitalWrite(13, HIGH);
                PumpCount2 = badsec;
            }
        }
    }

    if (PumpCount2 != 0) {
        PumpCount2 -= 1;//海水抽水馬達計時器倒數
        if (PumpCount2 == 0) {
            digitalWrite(13, LOW);//傳遞低電位給海水抽水馬達
            if (ChangeFlag-- == 1) {
                digitalWrite(12, HIGH);
                PumpCount = badsec;
            }
        }
    }
    
    if (dist>9)level = 1;//判斷第一個偵測器無感應到，低水位
    else if (dist<8)level = 2;//判斷第二個偵測器有感應到，高水位
    else level = 0;//都無感應到則等級為0
    Serial.println(String() + "dist：" + dist);
    Serial.println(String() + "level：" + level);
    if (level == 1)LED(1, 0);
    else if (level == 2)LED(0, 1);
    else LED(0, 0);

    if (PumpCount == 0 && PumpCount2 == 0 && Delay <= 0) {
        if (level == 1)linemsg("低水位", 6362, 11087937, sec);
        else if (level == 2)linemsg2("高水位", 1070, 17856, sec);
        else if (PH > 2 && PH < 5) {
          digitalWrite(11, HIGH);
          delay(100);
          digitalWrite(11, LOW);
            while ((shake += 1) < 5) {
                while ((angle += 10) <= 180) RotationAngle(angle);
                while ((angle -= 10) >= 150) RotationAngle(angle);
            }
            angle += 10;
            while ((angle -= 10) >= 0) RotationAngle(angle);
            shake = 0;
            Delay = 10;
        }
        else if (voltage > 1.7) {
            ChangeFlag = 1;
            linemsg2("水質太混濁", 6362, 11087937, badsec);
        }
        else if (temp >= 30 || temp <= 16) {
            ChangeFlag = 1;
            linemsg("水溫不適", 6362, 11087937, badsec);
        }
        else if (tdsValue >= 7000) {
            ChangeFlag = 1;
            linemsg2("導電率太高", 6362, 11087937, badsec);
        }
        else if(Delay==0||Level_Flag != level)camera("水質已回復正常",6362,11087937);
    }
    Delay--;
    delay(200);
}
