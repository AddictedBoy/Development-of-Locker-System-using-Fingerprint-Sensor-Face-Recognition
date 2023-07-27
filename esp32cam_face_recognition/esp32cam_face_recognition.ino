#include "esp_camera.h"
#include "fd_forward.h" //works with esp32 board library 1.0.5
#include "fr_forward.h"
#include "fr_flash.h"

#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool initCamera() {

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    return false;
  }

  return true;
}

//variables to keep track
int pictureNumber = 0;
int deleted_id;

mtmn_config_t mtmn_config = {0};
int detections = 0;

//variables to store face id
static face_id_list id_list1 = {0};
static face_id_list id_list2 = {0};

//variables to hold images
static dl_matrix3du_t *image_matrix = NULL;
dl_matrix3du_t *aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
dl_matrix3du_t *resized_matrix = dl_matrix3du_alloc(1, 128, 128, 3);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  Wire.begin(15, 14); //SCL, SDA

  // initialize the LCD
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Face Recognition System!");

  if (!initCamera()) {
    lcd.setCursor(0, 0);
    lcd.print("Failed to initialize camera...");
    return;
  }

  mtmn_config = mtmn_init_config();

  face_id_init(&id_list1, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  face_id_init(&id_list2, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
}

//function to read number
uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

//variables to keep track
uint8_t number;
int cap_num;
int id;
int cap_choose;

void loop() {
  //reset variables to 0 when loop comes here
  String terminalText = "";
  cap_num = 0;
  number = 0;
  id = 0;
  while (Serial.available() > 0) {
    terminalText = Serial.readStringUntil('\n');
    if (terminalText.indexOf("a") != -1) {
      number = 1;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("enrollone");
    }
    if (terminalText.indexOf("c") != -1) {
      number = 2;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("enrolltwo");
    }
    if (terminalText.indexOf("e") != -1) {
      number = 3;
      id = 1;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("searchone");
    }
    if (terminalText.indexOf("g") != -1) {
      number = 4;
      id = 2;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("searchtwo");
    }
    if (terminalText.indexOf("i") != -1) {
      number = 5;
      id = 1;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("searchdeleteone");
    }
    if (terminalText.indexOf("k") != -1) {
      number = 6;
      id = 2;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("searchdeletetwo");
    }
    if (terminalText.indexOf("z") != -1) {
      //number = 7;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("serialcycle");
      Serial.write("yyy");
      lcd.setCursor(0, 0);
      lcd.print("Sending back");
      delay(50);
    }
    switch (number) {
      case 1:
        //enroll one
        lcd.setCursor(0, 0);
        lcd.print("start enro1llone");
        cap_num = capture_and_store(1);
        if (cap_num == 1) {
          Serial.write("bbb");
        }
        break;
      case 2:
        //enroll two
        cap_num = capture_and_store(2);
        if (cap_num == 1) {
          Serial.write("ddd");
        }
        break;
      case 3:
        //search one
        cap_num = capture_and_recognize(1);
        if (cap_num == 1) {
          Serial.write("fff"); //True
        }
        else{
          Serial.write("mmm"); //False
        }
        break;
      case 4:
        //search two
        cap_num = capture_and_recognize(2);
        if (cap_num == 1) {
          Serial.write("hhh"); //True
        }
        else{
          Serial.write("nnn"); //False
        }
        break;
      case 5:
        //search and delete one
        cap_num = capture_and_recognize(1);
        if (cap_num == 1) {
          while ( delete_face(&id_list1) > 0 ) {
            //lcd.setCursor(0, 0);
            //lcd.print("Deleting Face for list 1");
          }
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("list 1 deleted");
          Serial.write("jjj"); //True
        }
        else{
          Serial.write("ooo"); //False
        }
        break;
      case 6:
        //search and delete two
        cap_num = capture_and_recognize(2);
        if (cap_num == 1) {
          while ( delete_face(&id_list2) > 0 ) {
            //lcd.setCursor(0, 0);
            //lcd.print("Deleting Face for list 2");
          }
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("list 2 deleted");
          Serial.write("lll"); //True
        }
        else{
          Serial.write("ppp"); //False
        }
        break;
      case 7:
        Serial.write("ttt");
        lcd.setCursor(0, 0);
        lcd.print("Sending back");
        delay(50);
        break;
      default:
        break;
    }
  }
}

//capture image and store as face id
int capture_and_store(int cap_choose) {
  int cap_store;
  int taken_img = 0;
  if (cap_choose == 1) {
    //face_list1
    while (taken_img < 5) {
      //captures image and store as face_id
      camera_fb_t * frame;
      frame = esp_camera_fb_get();

      dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
      uint32_t res = fmt2rgb888(frame->buf, frame->len, frame->format, image_matrix->item);

      if (!res) {
        lcd.setCursor(0, 0);
        lcd.print("to rgb888 failed");
        dl_matrix3du_free(image_matrix);
      }

      esp_camera_fb_return(frame);

      box_array_t *boxes = face_detect(image_matrix, &mtmn_config);
      if (boxes != NULL) {
        if (align_face(boxes, image_matrix, aligned_face) == ESP_OK) {
          int new_id = enroll_face(&id_list1 , aligned_face);
          lcd.setCursor(0, 0);
          lcd.print(String(new_id) + " id list1");
          taken_img++;
        }
        else {
          lcd.setCursor(0, 0);
          lcd.print("Face Not Aligned");

        }
        dl_lib_free(boxes->score);
        dl_lib_free(boxes->box);
        dl_lib_free(boxes->landmark);
        dl_lib_free(boxes);
      }
      dl_matrix3du_free(image_matrix);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("list1 saved");
    cap_store = 1;
  }
  else if (cap_choose == 2) {
    //face_list2
    while (taken_img < 5) {
      //captures image and store as face_id
      camera_fb_t * frame;
      frame = esp_camera_fb_get();

      dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
      uint32_t res = fmt2rgb888(frame->buf, frame->len, frame->format, image_matrix->item);

      if (!res) {
        lcd.setCursor(0, 0);
        lcd.print("to rgb888 failed");
        dl_matrix3du_free(image_matrix);
      }

      esp_camera_fb_return(frame);

      box_array_t *boxes = face_detect(image_matrix, &mtmn_config);
      if (boxes != NULL) {
        if (align_face(boxes, image_matrix, aligned_face) == ESP_OK) {
          int new_id = enroll_face(&id_list2 , aligned_face);
          lcd.setCursor(0, 0);
          lcd.print(String(new_id) + " id list2");
          taken_img++;
        }
        else {
          lcd.setCursor(0, 0);
          lcd.print("Face Not Aligned");
        }
        dl_lib_free(boxes->score);
        dl_lib_free(boxes->box);
        dl_lib_free(boxes->landmark);
        dl_lib_free(boxes);
      }
      dl_matrix3du_free(image_matrix);

    }
    cap_store = 1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("list2 saved");
  }
  taken_img = 0;
  return cap_store;
}

//capture image and compare image to face id
int capture_and_recognize(int cap_choose) {
  int true_image = 0;
  int reco = 0;
  int cap_box;
  box_array_t *boxes;
  while (reco < 100) {
    //captures image and recognise face
    cap_box = 0;
    boxes = NULL;
    camera_fb_t * frame;
    frame = esp_camera_fb_get();

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
    uint32_t res = fmt2rgb888(frame->buf, frame->len, frame->format, image_matrix->item);

    if (!res) {
      lcd.setCursor(0, 0);
      lcd.print("to rgb888 failed");
      dl_matrix3du_free(image_matrix);
    }

    esp_camera_fb_return(frame);

    boxes = face_detect(image_matrix, &mtmn_config);
    if (boxes != NULL) {
      lcd.setCursor(0, 0);
      lcd.print(String(reco) + " cap_box is good");
      cap_box = 10;
      //break;
    }
    else {
      reco++;
    }
    if (boxes != NULL) {
      if (align_face(boxes, image_matrix, aligned_face) == ESP_OK) {
        if (cap_choose == 1) {
          int matched_id = recognize_face(&id_list1, aligned_face);
          if (matched_id >= 0) {
            lcd.setCursor(0, 0);
            lcd.print(String(matched_id) + " id list1");
            true_image = 1;
            break;
          } else {
            lcd.setCursor(0, 0);
            lcd.print("No Match Found"); 
            lcd.setCursor(0,1);
            lcd.print("list 1");
            matched_id = -1;
            reco++;
          }
        }
        else if (cap_choose == 2) {
          int matched_id = recognize_face(&id_list2, aligned_face);
          if (matched_id >= 0) {
            lcd.setCursor(0, 0);
            lcd.print(String(matched_id) + " id list2");
            true_image = 1;
            break;
          } else {
            lcd.setCursor(0, 0);
            lcd.print("No Match Found"); 
            lcd.setCursor(0,1);
            lcd.print("list 2");
            matched_id = -1;
            reco++;
          }
        }
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("Face Not Aligned");
      }
      dl_lib_free(boxes->score);
      dl_lib_free(boxes->box);
      dl_lib_free(boxes->landmark);
      dl_lib_free(boxes);
    }
    dl_matrix3du_free(image_matrix);
  }
  if (reco == 100) {
    lcd.setCursor(0, 1);
    lcd.print("Incorrect face!");
    true_image = 0;
  }
  if (true_image == 1) {
    lcd.setCursor(0, 1);
    lcd.print("Correct face!");
  }
  reco = 0;
  return true_image;
}
