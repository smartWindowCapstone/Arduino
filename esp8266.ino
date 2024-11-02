//esp8266 2.7.4 버전
//Firebase ESP8266 Client 4.3.14
//FirebaseJson 3.0.7
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <time.h> 

String state=""; //창문 현재 상태 OPEN CLOSE
String preSky=""; // 날씨 0 맑음  1비 2 눈비 3 눈 4 눈 5 소나기 
String khaiGrade=""; // 미세먼지 농도 1좋음 2보통 3나쁨 4 매우 나쁨
String realTime=""; //"HHMM" 형태
String closeTime=""; //닫는 시간 환기설정 시간 +10분값
boolean ventilation=true; //환기시간에 창문동작이 한번만 작동되도록 함(조건)
String preRealTime=""; //realtime과 비교할때 사용
String settingTime=""; // 설정 시간 변수
String preState=""; //창문 상태 예전 값
boolean doOnce=true; // 창문 상태 변경 한번만 하도록
boolean first= false; //loop안에서 첫 한번 setup에서 설정한 preState와 비교할때 사용

int timezone = 3; 
int dst = 0; 
time_t now;
time_t preNow;
//millis와 비교하여 n초마다 실행
unsigned long previousMillis = 0;
const long interval = 1000; 
unsigned long oneSeconds=0;
unsigned long oneSeconds2=0;
unsigned long weatherSeconds = 0;
unsigned long fiveSeconds=0;

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""  
#define WIFI_SSID "LAPTOP" //와이파이 이름 
#define WIFI_PASSWORD "todha123" //와이파이 비밀번호


void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
//현재시간 받아올 서버+ 한국설정 
configTime(3 * 3600 * 3, 0, "kr.pool.ntp.org", "kr.pool.ntp.org"); 
   Serial.println("\nWaiting for time"); 
   while (!time(nullptr)) {
     Serial.print("."); 
     delay(1000);
   }
   Serial.println(""); 
   time_t now2= time(nullptr);
   preRealTime = String(ctime(&now2)).substring(11,13)+String(ctime(&now2)).substring(14,16);
    Serial.println("realTime "+preRealTime);
  //처음에 값 한번 받아오기 
  preSky=Firebase.getString("preSky");
  khaiGrade=Firebase.getString("khaiGrade");
  settingTime=Firebase.getString("settingTime");
  preState=Firebase.getString("state");
  Serial.println("preSky "+preSky);
  Serial.println("khaiGrade "+khaiGrade);
  Serial.println("settingTime "+settingTime);
    if (Firebase.failed()) {
      Serial.print("setting failed:");
      Serial.println(Firebase.error());
      return;
  }
}

void loop() {
  //우노와 시리얼 통신,C를 받으면 서버에서 state값을 close로 변경
    if(Serial.available()){
    String inChar = Serial.readStringUntil('\n');
    inChar.trim();
    if(doOnce==true){
    if(inChar =="C"){
      updateFirebaseValue("CLOSE");
      doOnce=false;
    }
  }
    }
    else{
    doOnce=true;
  }

  unsigned long currentMillis = millis();

   //현재시간 받아오기
   //서버에서 받아오는 현재시간은 초단위로 업데이트-> 10초마다 예전값과 분 단위로 비교 후 시리얼 통신
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    time_t now2= time(nullptr);
     realTime = String(ctime(&now2)).substring(11,13)+String(ctime(&now2)).substring(14,16);
    if(preRealTime!=realTime){
    Serial.println("realTime "+realTime); 
    preRealTime=realTime;
   }
  }
  
//첫 보드 연결시(업로드 후) state값에 따라 창문 이동 방지 및 이전 값과 변경값이 다를때만 작동(동일 동작 2번 방지) 
   if(first==false){
    state=Firebase.getString("state");
if (preState!=state &&( state=="OPEN" ||state=="CLOSE")){
  first=true;
}
   }

  //1초마다 현재 창문상태 받아오기
  if(first){
  if(currentMillis-oneSeconds>=1000){
    oneSeconds=currentMillis;
  state=Firebase.getString("state");
      Serial.println("state "+state);
  }
  }
    //1초마다 설정 환기시간 받아오기 "HHMM" 형식
  if(currentMillis-oneSeconds2>=1000){
    oneSeconds2=currentMillis;
  settingTime=Firebase.getString("settingTime");
      Serial.println("settingTime "+settingTime);
  }
    now = time(nullptr);
    
//5초마다 기상청 날씨와 미세먼지 값 받아오기
if(currentMillis -fiveSeconds>=5000){
  fiveSeconds = currentMillis;
  preSky=Firebase.getString("preSky");
   khaiGrade=Firebase.getString("khaiGrade");
      Serial.println("preSky "+preSky);
  Serial.println("khaiGrade "+khaiGrade);
       if (Firebase.failed()) {
      Serial.print("setting failed:");
      Serial.println(Firebase.error());  
      return;
         }
}

 //설정 창문 시간이 되면 창문을 열기 및 10분 후 창문 닫기
 if(settingTime==realTime && ventilation){
      updateFirebaseValue("OPEN");
      ventilation=false;
    }else if(settingTime!=realTime){
    ventilation= true;
} else {
      Serial.println("Invalid command");  
    }
  int hour = realTime.substring(0, 2).toInt();
  int minute = realTime.substring(2).toInt();
  minute -= 10;
  if (minute < 0) {
    hour -= 1;
    if (hour < 0) {
      hour = 23;
    }
    minute += 60;
  }
  String hourStr = hour < 10 ? "0" + String(hour) : String(hour);
  String minuteStr = minute < 10 ? "0" + String(minute) : String(minute);
  closeTime = hourStr + minuteStr;
  if(closeTime==settingTime && ventilation){
    updateFirebaseValue("CLOSE");
    ventilation=false;
  }else if(closeTime !=settingTime){
  ventilation=true;   
  }
}

 // Firebase Realtime Database의 /state 값을 업데이트
 void updateFirebaseValue(const String& value) {
  Firebase.setString("state", value);

  // 업데이트 결과 확인
  if (Firebase.failed()) {
    Serial.print("Firebase 업데이트 실패: ");
    Serial.println(Firebase.error());
  } else {
    Serial.println("Firebase 업데이트 성공!");
  }
}
