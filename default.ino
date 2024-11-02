#include <SoftwareSerial.h>
#include <TM1637Display.h>
#include <Stepper.h>
const int moveStep = 64;
Stepper stepper(moveStep, 8, 10, 9, 11);
String beforeState="";
// 빗물 감지센서 핀 번호
const int rainPin = A0;
 // 빗물 감지 한계값
const int maxRainSensorValue = 500; 
String state = "";// "OPENSTOP","CLOSESTOP", "OPEN", "CLOSE" 
// "true", "false"
bool rainCheck = true; // "true", "false"
//  String beforeState = "CLOSE"; // "OPEN", "CLOSE"
//esp8266 시리얼 핀
#define BT_RXD 2
#define BT_TXD 3
//tm1637 시리얼 핀
#define CLK 4
#define DIO 5
//esp8266
String preSky="";
String bePreSky ="";
String beKhaiGrade="";
String khaiGrade="";
String realTime="";
String settingTime="";
String beforeSettingTime="";
String closeTime="";
SoftwareSerial ESP_wifi(BT_RXD, BT_TXD); 
//tm1637
TM1637Display display(CLK, DIO);

void setup()
{
  Serial.begin(9600);
  ESP_wifi.begin(9600);
  ESP_wifi.setTimeout(5000);
  delay(1000);
  // 디스플레이 밝기 설정 (0 ~ 7)
    display.setBrightness(7);
    // 모터 속도
      stepper.setSpeed(220); 
      // 시작 시간 초기화
  // 빗물 감지 센서 핀 입력 설정
  pinMode(rainPin, INPUT);  
}

void loop(){
  //esp8266에서 값받아오기 "변수이름 값" 형태 값부분만 받아서 변수에 다시저장
 if(ESP_wifi.available()){
        String inChar =ESP_wifi.readStringUntil('\n');
  inChar.trim();
    if(inChar.substring(6,-1)=="OPEN" &&beforeState!="OPEN" ){
      state="OPEN";
    }else if(inChar.substring(6,-1)=="CLOSE" &&beforeState!="CLOSE" ){
      state="CLOSE";
    }
 
  if(inChar.startsWith("preSky")==1 && bePreSky!=inChar.substring(7,-1)){
    preSky=inChar.substring(7,-1);
    bePreSky=preSky;
  }
  if(inChar.startsWith("khaiGrade")==1 && beKhaiGrade!=inChar.substring(10,-1)){
    khaiGrade=inChar.substring(10,-1);
    beKhaiGrade=khaiGrade;
  }

  if(inChar.startsWith("settingTime")==1 &&inChar.substring(12,-1)!=beforeSettingTime){
  settingTime=inChar.substring(12,-1);
  }
  
  }
  
  //0맑음 1비 2눈/비 3 눈 4 눈 5 소나기
if((preSky=="1"||preSky=="2"|| preSky=="3"||preSky=="4"||preSky=="5")&& beforeState=="OPEN"){
  ESP_wifi.println("C");
  bePreSky=preSky;
  preSky="-1";
}

// 미세먼지 1좋음 2보통 3 나쁨, 4 매우나쁨
//창문이 열린 상태에서 미세먼지가 나쁨또는 매우나쁨이면 esp8266에 C전송-> esp8266에선 C를 받으면 서버에 창문상태 변수를 닫음으로 변경
if((khaiGrade=="3" || khaiGrade=="4")&& beforeState=="OPEN"){
   ESP_wifi.println("C");
   beKhaiGrade=khaiGrade;
   khaiGrade="-1";
}
//세그먼트에 시간 표시 
if(settingTime!=""){
  int displayTime = settingTime.toInt();
    // 콜론(:) 표시 0b01000000
  display.showNumberDecEx(displayTime, 0b01000000, true);
}

    int rainValue = analogRead(rainPin);  // 빗물 감지 센서 값 읽기
    
    // 빗물 감지를 한다면
  // check 가 true 일때 창문이 열려있을 때
  if (rainValue < maxRainSensorValue&& state=="OPENSTOP" &&rainCheck == true) { 
    ESP_wifi.println("C");
    Serial.println("rain OPEN");
    // check를 false로 바꾼다
            rainCheck = false; 
            // 빗물 미감지시
    }else if(state!="OPENSTOP"){
      // check를 true로 바꾼다 
    rainCheck = true; 
    }
    
// state가 open 이면
  if (state == "OPEN") { 
      stepper.step(-2765);
      beforeState="OPEN";
      //빗물 감지 센서때 "STOP"사용시 OPEN CLOSE 구별 안됨 그래서 "OPENSTOP"
      state="OPENSTOP";
  } else if (state == "CLOSE") {
stepper.step(2765);
beforeState="CLOSE";
state="STOP";
  }
}
