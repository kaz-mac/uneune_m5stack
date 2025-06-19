/*
  uneune.ino
  うねうね for M5Stack

  Copyright (c) 2025 Kaz  (https://akibabara.com/blog/)
  Released under the MIT license.
  see https://opensource.org/licenses/MIT

  動作確認機種
  ・M5Stack Core2
  ・M5Stack Core3SE
  ・M5Stack BASIC（タッチ不可）
  ・M5Stack Tab5（ボタン押下不可）
  ・M5Stack M5StickC Plus2（タッチ不可）
*/
#include <M5Unified.h>

// 設定
const int split_width = 160;    // ブロックの幅(px) = canvasFixed.width()
const int split_height = 120;   // ブロックの高さ(px) = canvasFixed.height()
const float une_density = 1.0;  // うねうねの密度（1.0で160x120に約24匹）
float une_speed_default = 0.10; // うねの移動速度
float une_moving = 0.7;   // うねの最大移動範囲(0.0～1.0)
int fps_limit = 30; // FPS制限
bool show_fps = true; // FPSを表示

// 画像データ
#include "image.h"

// グローバル変数
uint8_t selUne = 0; // 選択したうねの番号
int m5_w, m5_h;     // M5Stackの幅と高さ
int m5s_w, m5s_h;   // ブロックのキャンバス canvasFixed の幅と高さ
const int col = (int)(split_width / 24.0 * une_density);   // ブロック内に配置するうねの数(column)
const int row = (int)(split_height / 24.0 * une_density);  // ブロック内に配置するうねの数(raw)
const int une_num = col * row; // ブロック内に存在するうねの数
uint16_t unex[une_num]; // 各うねのx座標
uint16_t uney[une_num]; // 各うねのy座標
float uner[une_num];    // 各うねの移動範囲(%) 各個体にランダム性を持たせる
float une_speed = une_speed_default;   // うねの移動速度
int padding_une;        // うねうねがはみ出る幅(px)  une_moving=100%なら24px
bool touch_enable = true; // タッチパネルの有効/無効
bool btntype_physical = false; // ボタンのタイプ 物理
bool btntype_touch = true; // ボタンのタイプ タッチ
bool lcd_xyswap = false; // LCDのX軸とY軸を入れ替える

//　Canvasの作成
M5Canvas canvasUne;     // うね1匹分のキャンバス
M5Canvas canvasPadding; // うねうねがはみ出る範囲を含むキャンバス
M5Canvas canvasFixed;   // 160x120の固定キャンバス

// うねのクラス
#include "une.h"
bool lcdBusy = false;  // LCD排他制御用のフラグ
Une* unes[une_num];    // Uneインスタンスの配列

// デバッグに便利なマクロ定義 --------
#define sp(x) Serial.println(x)
#define spn(x) Serial.print(x)
#define spp(k,v) Serial.println(String(k)+"="+String(v))
#define spf(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#define array_length(x) (sizeof(x) / sizeof(x[0]))


// デバッグ: 空きメモリ確認
void debug_free_memory(String str) {
  sp("## "+str);
  spf("MALLOC_CAP_INTERNAL: %7d / %7d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  spf("MALLOC_CAP_DMA     : %7d / %7d\n", heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
  spf("MALLOC_CAP_SPIRAM  : %7d / %7d\n\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

// 1匹のうねをCanvasに取り込む
void load_une(M5Canvas *canvas, int no) {
  canvas->setPsram(false);
  canvas->setColorDepth(16);
  canvas->createSprite(imgUnes[no].width, imgUnes[no].height);  // 72x72 px
  canvas->fillScreen(TFT_BLACK);
  canvas->pushImage(0, 0, imgUnes[no].width, imgUnes[no].height, imgUnes[no].data, transparent);
  padding_une = (int)(canvas->width() * une_moving);
  spf("padding_une=%d\n", padding_une);
}

// ---------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------
void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.setTouchButtonHeight(32);  // 仮想ボタンの反応エリアを広げる

  // 機種別の詳細設定
  auto board = M5.getBoard();
  if (M5.Display.width() < M5.Display.height()) {
    lcd_xyswap = true;  // 縦型ディスプレイの機種は縦と横を入れ替える
  }
  if (board == m5::board_t::board_M5Tab5) {
    une_speed_default *= 80.0 /50.0; // M5Tab5は処理が追い付かないので速く見せる
  }

  // ディスプレイのサイズを計算
  m5_w = lcd_xyswap ? M5.Display.height() : M5.Display.width();
  m5_h = lcd_xyswap ? M5.Display.width() : M5.Display.height();
  m5s_w = split_width;
  m5s_h = split_height;
  sp("Start!");
  spf("Display m5_w=%d, m5_h=%d\n", m5_w, m5_h);
  spf("UneUne col=%d, row=%d, total=%d\n", col, row, une_num);
  debug_free_memory("Start");
  delay(1000);

  // ディスプレイの初期化
  M5.Display.setColorDepth(16);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setFont(&fonts::Font4);

  // 1匹のうねをCanvasに取り込む
  load_une(&canvasUne, selUne);

  // Display出力前段階のキャンバス canvasFixed の初期化
  canvasFixed.setPsram(false);
  canvasFixed.setColorDepth(16);
  canvasFixed.createSprite(m5s_w, m5s_h);  // 160x120 px
  canvasFixed.fillScreen(TFT_BLACK);
  spf("canvasFixed.width()=%d, canvasFixed.height()=%d\n", canvasFixed.width(), canvasFixed.height());

  // うねのはみ出しを含むキャンバス canvasPadding の初期化
  canvasPadding.setPsram(false);
  canvasPadding.setColorDepth(16);
  canvasPadding.createSprite(m5s_w + padding_une*2, m5s_h + padding_une*2);  // 208x168 px
  spf("canvasPadding.width()=%d, canvasPadding.height()=%d\n", canvasPadding.width(), canvasPadding.height());
  canvasPadding.fillScreen(TFT_BLACK);
  
  // うねの初期化
  int spacex = (m5s_w - canvasUne.width()) / (col-1);
  int spacey = (m5s_h - canvasUne.height()) / (row-1);
  for (int ux=0; ux<col; ux++) {
    for (int uy=0; uy<row; uy++) {
      int no = uy * col + ux;
      unes[no] = new Une(&canvasPadding, &canvasUne, transparent, 10, &lcdBusy);
      unes[no]->speed = une_speed;  // うねの移動速度
      unex[no] = padding_une/2 + spacex * ux + random(-5, 5);
      uney[no] = padding_une/2 + spacey * uy + random(-5, 5);
      // unes[no]->num = random(10,15);   // 胴体の長さにランダム性を与える
      uner[no] = random(70, 100) / 100.0; // 移動範囲にランダム性を与える（胴体の長さが変わって見える）
    }
  }

  debug_free_memory("Init End");
}

// ---------------------------------------------------------------------
// メイン
// ---------------------------------------------------------------------
void loop() {
  M5.update();
  m5::touch_detail_t  touch = M5.Touch.getDetail();
  float ftarget_x, ftarget_y;

  // キャンバスをクリア
  uint32_t start_tm = millis();
  canvasFixed.fillScreen(TFT_BLACK);
  canvasPadding.fillScreen(TFT_BLACK);
  M5.Display.startWrite();

  // タッチパネルの状態を取得
  bool follow_finger = false;
  if (touch.isPressed()) {
    // ディスプレイ部分がタッチさてたらうねうねを追従させる
    if (touch.y < m5_h) {
      follow_finger = true;
      ftarget_x = (map(touch.x, 0,m5_w, 0,200) - 100) / 100.0;
      ftarget_y = (map(touch.y, 0,m5_h, 0,200) - 100) / 100.0;
      // spf("ft_x=%.2f, ft_y=%.2f\n", ftarget_x, ftarget_y);
    }
  }

  // ボタン操作
  if (M5.BtnA.wasPressed()) {
    selUne++;
    if (selUne >= array_length(imgUnes)) selUne = 0;
    load_une(&canvasUne, selUne);
    spf("Change Une: selUne=%d\n", selUne);
    une_speed = une_speed_default;
  }
  if (M5.BtnB.wasPressed()) {
    une_speed -= (une_speed < 0.10) ? 0.01 : 0.02;
    if (une_speed < 0.01) une_speed = 0.01;
    spf("Speed Down: une_speed=%.2f\n", une_speed);
  }
  if (M5.BtnC.wasPressed()) {
    une_speed += (une_speed < 0.10) ? 0.01 : 0.02;
    spf("Speed UP: une_speed=%.2f\n", une_speed);
  }

  // うねを動かしてキャンバス canvasPadding に描画
  for (int ux=0; ux<col; ux++) {
    for (int uy=0; uy<row; uy++) {
      int no = uy * col + ux;
      // ターゲットに到着したら新しいターゲットを与える
      if (unes[no]->finish) {
        if (follow_finger) {  // 指に追従
          float adj_x = random(-20, 20) / 100.0;
          float adj_y = random(-20, 20) / 100.0;
          float rand_x = ftarget_x * une_moving * uner[no] + adj_x;
          float rand_y = ftarget_y * une_moving * uner[no] + adj_y;
          unes[no]->target(rand_x, rand_y);
          unes[no]->speed = une_speed * 2.0;
          if (no==0) spf("rand_x=%.2f, rand_y=%.2f\n", rand_x, rand_y);
        } else {  // 通常のうねうね
          float rand_x = random(-100, 100) / 100.0 * une_moving * uner[no];
          float rand_y = random(-100, 100) / 100.0 * une_moving * uner[no];
          unes[no]->target(rand_x, rand_y);
          unes[no]->speed = une_speed;
        }
      } else {
        if (follow_finger) {  // 指に追従
          // 即座に新ターゲットに向かわせて速度も上げる
          unes[no]->finish = true;
          unes[no]->speed = une_speed * 2.0;
        }
      }
      unes[no]->update();
      unes[no]->draw(unex[no] + padding_une, uney[no] + padding_une);
    }
  }

  // キャンバス境界の重なる部分の透過処理
  int pz = -padding_une;
  // キャンバスの中央部分をコピーする
  canvasPadding.pushSprite(&canvasFixed, pz, pz);
  // canvasPaddingの左はみ出し部分をcanvasFixedの右に合成する
  canvasPadding.pushSprite(&canvasFixed, pz+canvasFixed.width(), pz, transparent);
  // canvasPaddingの上はみ出し部分をcanvasFixedの下に合成する
  canvasPadding.pushSprite(&canvasFixed, pz, pz+canvasFixed.height(), transparent);

  // LCDに描画
  uint32_t start_tm2 = millis();
  for (int x=0; x<m5_w; x+=canvasFixed.width()) {
    for (int y=0; y<m5_h; y+=canvasFixed.height()) {
      canvasFixed.pushSprite(&M5.Display, x, y);
    }
  }
  M5.Display.endWrite();

  // FPSを表示
  uint32_t elapsed_time = millis() - start_tm;
  float fps = 1000.0 / elapsed_time;
  char fps_str[20];
  if (show_fps) {
    sprintf(fps_str, "%.1f/%d", fps, fps_limit);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextDatum(BL_DATUM);
    M5.Display.drawRightString(fps_str, m5_w-5, m5_h-18, &fonts::Font2);
    M5.Display.setTextDatum(TR_DATUM);
  }
  spf("Time: %d us, Time2: %d us, fps: %s\n", elapsed_time, millis() - start_tm2, fps_str);

  // FPS調整
  int delay_time = 1000 / fps_limit - elapsed_time;
  if (delay_time > 0) {
    delay(delay_time);
  }

}